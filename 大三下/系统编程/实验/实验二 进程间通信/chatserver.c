/*
 * chatserver.c
 *
 * 多用户聊天系统服务器。
 * 服务器创建 3 个公共 FIFO，分别接收注册、登录和聊天请求；
 * 通过 select() 同时监听这些 FIFO，并维护用户表完成身份校验和消息转发。
 */

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "chat_common.h"

typedef struct
{
    char username[CHAT_NAME_LEN];
    char password[CHAT_PASSWORD_LEN];
    char fifo[CHAT_FIFO_PATH_LEN];
    int online;
} User;

/* 用户表：保存已注册用户的账号、密码、私有 FIFO 和在线状态。 */
static User users[CHAT_MAX_USERS];
static int user_count = 0;

/*
 * read_fds 保存 3 个公共 FIFO 的读端，用于 select() 监听。
 * hold_fds 保存同一组 FIFO 的写端，用来保持 FIFO 两端都打开，
 * 避免没有客户端写入时服务器读端反复得到 EOF。
 */
static int read_fds[3] = {-1, -1, -1};
static int hold_fds[3] = {-1, -1, -1};

/* 安全复制字符串，避免固定长度字段溢出。 */
static void copy_string(char *dst, size_t dst_size, const char *src)
{
    snprintf(dst, dst_size, "%s", src == NULL ? "" : src);
}

/* 如果目录不存在则创建；如果路径已存在但不是目录，则返回错误。 */
static int ensure_dir(const char *path)
{
    struct stat st;

    if (stat(path, &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
            return 0;
        fprintf(stderr, "%s exists but is not a directory\n", path);
        return -1;
    }

    if (mkdir(path, 0777) == -1)
    {
        perror(path);
        return -1;
    }

    return 0;
}

/* 创建实验要求的聊天系统目录树。 */
static int ensure_tree(void)
{
    return ensure_dir(CHAT_USER_HOME) == 0 &&
           ensure_dir(CHAT_APP_DIR) == 0 &&
           ensure_dir(CHAT_DATA_DIR) == 0 &&
           ensure_dir(CHAT_SERVER_FIFO_DIR) == 0 &&
           ensure_dir(CHAT_CLIENT_FIFO_DIR) == 0;
}

/* 确保指定路径存在且类型为 FIFO，不存在时创建命名管道。 */
static int ensure_fifo(const char *path)
{
    struct stat st;

    if (stat(path, &st) == 0)
    {
        if (S_ISFIFO(st.st_mode))
            return 0;
        fprintf(stderr, "%s exists but is not a FIFO\n", path);
        return -1;
    }

    if (mkfifo(path, 0666) == -1)
    {
        perror(path);
        return -1;
    }

    return 0;
}

/*
 * 打开一个服务器公共 FIFO。
 * index=0 对应注册 FIFO，index=1 对应登录 FIFO，index=2 对应聊天 FIFO。
 */
static int open_server_fifo(const char *path, int index)
{
    if (ensure_fifo(path) == -1)
        return -1;

    /* 读端用于接收客户端请求，非阻塞模式便于 drain_* 函数循环读取。 */
    read_fds[index] = open(path, O_RDONLY | O_NONBLOCK);
    if (read_fds[index] == -1)
    {
        perror(path);
        return -1;
    }

    /* 写端由服务器自己持有，防止公共 FIFO 在无客户端时出现 EOF 状态。 */
    hold_fds[index] = open(path, O_WRONLY | O_NONBLOCK);
    if (hold_fds[index] == -1)
    {
        perror(path);
        return -1;
    }

    return 0;
}

/* 关闭所有 FIFO，并删除服务器创建的 3 个公共 FIFO。 */
static void cleanup(int status)
{
    int i;

    for (i = 0; i < 3; i++)
    {
        if (read_fds[i] != -1)
            close(read_fds[i]);
        if (hold_fds[i] != -1)
            close(hold_fds[i]);
    }

    unlink(CHAT_FIFO_REGISTER);
    unlink(CHAT_FIFO_LOGIN);
    unlink(CHAT_FIFO_MESSAGE);
    exit(status);
}

/* 收到退出信号时清理公共 FIFO，避免下次运行时残留旧文件。 */
static void handle_signal(int sig)
{
    (void)sig;
    cleanup(EXIT_SUCCESS);
}

/* 在用户表中按用户名查找用户，找到返回下标，找不到返回 -1。 */
static int find_user(const char *username)
{
    int i;

    for (i = 0; i < user_count; i++)
    {
        if (strcmp(users[i].username, username) == 0)
            return i;
    }

    return -1;
}

/* 向某个客户端私有 FIFO 写入一个 ChatPacket。 */
static int send_packet(const char *fifo, const ChatPacket *packet)
{
    int fd;
    ssize_t written;

    fd = open(fifo, O_WRONLY | O_NONBLOCK);
    if (fd == -1)
        return -1;

    written = write(fd, packet, sizeof(*packet));
    close(fd);

    return written == sizeof(*packet) ? 0 : -1;
}

/* 统一构造服务器发送给客户端的数据包。 */
static void make_packet(ChatPacket *packet, int type, int ok,
                        const char *from, const char *message)
{
    memset(packet, 0, sizeof(*packet));
    packet->type = type;
    packet->ok = ok;
    copy_string(packet->from, sizeof(packet->from), from);
    copy_string(packet->message, sizeof(packet->message), message);
}

/* 发送服务器回复包，ok=1 表示成功，ok=0 表示错误。 */
static void reply_to_fifo(const char *fifo, int ok, const char *message)
{
    ChatPacket packet;

    make_packet(&packet, CHAT_PACKET_REPLY, ok, "server", message);
    if (send_packet(fifo, &packet) == -1)
        fprintf(stderr, "Failed to reply through %s\n", fifo);
}

/*
 * 处理注册请求。
 * 注册成功时把用户加入 users 数组，但 online 先置为 0；
 * 用户必须再执行 /login 后才被认为在线。
 */
static void handle_register(const ChatAuthRequest *req)
{
    if (req->username[0] == '\0' || req->password[0] == '\0')
    {
        reply_to_fifo(req->fifo, 0, "username and password must not be empty");
        return;
    }

    /* 用户名是唯一键，重复注册会被拒绝。 */
    if (find_user(req->username) != -1)
    {
        reply_to_fifo(req->fifo, 0, "username already exists");
        return;
    }

    if (user_count >= CHAT_MAX_USERS)
    {
        reply_to_fifo(req->fifo, 0, "server user table is full");
        return;
    }

    /* 保存注册信息以及当前客户端 FIFO，便于向客户端返回注册结果。 */
    copy_string(users[user_count].username, sizeof(users[user_count].username), req->username);
    copy_string(users[user_count].password, sizeof(users[user_count].password), req->password);
    copy_string(users[user_count].fifo, sizeof(users[user_count].fifo), req->fifo);
    users[user_count].online = 0;
    user_count++;

    printf("registered user: %s\n", req->username);
    reply_to_fifo(req->fifo, 1, "register ok");
}

/*
 * 处理登录请求。
 * 登录成功后刷新用户的私有 FIFO 路径，并把 online 设置为 1。
 */
static void handle_login(const ChatAuthRequest *req)
{
    int index = find_user(req->username);

    if (index == -1)
    {
        reply_to_fifo(req->fifo, 0, "username does not exist");
        return;
    }

    /* 使用注册时保存的密码进行校验。 */
    if (strcmp(users[index].password, req->password) != 0)
    {
        reply_to_fifo(req->fifo, 0, "password is incorrect");
        return;
    }

    /*
     * 客户端重新启动后私有 FIFO 可能重新创建，
     * 因此登录成功时更新为本次请求携带的 fifo。
     */
    copy_string(users[index].fifo, sizeof(users[index].fifo), req->fifo);
    users[index].online = 1;

    printf("login user: %s, fifo: %s\n", req->username, req->fifo);
    reply_to_fifo(req->fifo, 1, "login ok");
}

/*
 * 处理聊天消息请求。
 * 服务器先校验发送方和接收方状态，再把消息写入接收方私有 FIFO。
 */
static void handle_chat(const ChatSendRequest *req)
{
    int from_index = find_user(req->from);
    int to_index = find_user(req->to);
    ChatPacket packet;
    char message[CHAT_TEXT_LEN];

    if (from_index == -1 || !users[from_index].online)
        return;

    /* 目标用户不存在时，向发送方返回错误。 */
    if (to_index == -1)
    {
        reply_to_fifo(users[from_index].fifo, 0, "target user does not exist");
        return;
    }

    /* 目标用户已注册但未登录，不能接收消息。 */
    if (!users[to_index].online)
    {
        reply_to_fifo(users[from_index].fifo, 0, "target user is not online");
        return;
    }

    /* 构造聊天消息包，并写入接收方的私有 FIFO。 */
    make_packet(&packet, CHAT_PACKET_MESSAGE, 1, req->from, req->text);
    if (send_packet(users[to_index].fifo, &packet) == -1)
    {
        /* 如果接收方 FIFO 已不可用，则把该用户标记为离线。 */
        users[to_index].online = 0;
        reply_to_fifo(users[from_index].fifo, 0, "target fifo is not available");
        return;
    }

    /* 消息送达接收方 FIFO 后，再向发送方回确认包。 */
    snprintf(message, sizeof(message), "message sent to %s", req->to);
    reply_to_fifo(users[from_index].fifo, 1, message);
    printf("%s -> %s: %s\n", req->from, req->to, req->text);
}

/*
 * 读取注册或登录 FIFO 中当前积压的所有认证请求。
 * is_register=1 表示注册请求，is_register=0 表示登录请求。
 */
static void drain_auth_fifo(int fd, int is_register)
{
    ChatAuthRequest req;
    ssize_t nread;

    while (1)
    {
        nread = read(fd, &req, sizeof(req));
        if (nread == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            if (errno == EINTR)
                continue;
            perror("read");
            return;
        }

        if (nread == 0)
            return;

        /* FIFO 中应当每次写入一个完整结构体，不完整数据直接忽略。 */
        if (nread != sizeof(req))
        {
            fprintf(stderr, "Ignoring incomplete auth request: %zd bytes\n", nread);
            continue;
        }

        if (is_register)
            handle_register(&req);
        else
            handle_login(&req);
    }
}

/* 读取聊天 FIFO 中当前积压的所有聊天请求。 */
static void drain_chat_fifo(int fd)
{
    ChatSendRequest req;
    ssize_t nread;

    while (1)
    {
        nread = read(fd, &req, sizeof(req));
        if (nread == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            if (errno == EINTR)
                continue;
            perror("read");
            return;
        }

        if (nread == 0)
            return;

        /* 聊天请求也必须是一个完整的 ChatSendRequest。 */
        if (nread != sizeof(req))
        {
            fprintf(stderr, "Ignoring incomplete chat request: %zd bytes\n", nread);
            continue;
        }

        handle_chat(&req);
    }
}

int main(void)
{
    fd_set rfds;
    int maxfd;

    /* 注册信号处理函数，使 Ctrl+C 或 kill 终止时仍能清理 FIFO。 */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGHUP, handle_signal);

    if (!ensure_tree())
        exit(EXIT_FAILURE);

    if (open_server_fifo(CHAT_FIFO_REGISTER, 0) == -1 ||
        open_server_fifo(CHAT_FIFO_LOGIN, 1) == -1 ||
        open_server_fifo(CHAT_FIFO_MESSAGE, 2) == -1)
        cleanup(EXIT_FAILURE);

    printf("chatserver is ready\n");
    printf("register fifo: %s\n", CHAT_FIFO_REGISTER);
    printf("login fifo:    %s\n", CHAT_FIFO_LOGIN);
    printf("message fifo:  %s\n", CHAT_FIFO_MESSAGE);

    while (1)
    {
        /*
         * select() 同时监听 3 个公共 FIFO：
         * read_fds[0] -> 注册请求；
         * read_fds[1] -> 登录请求；
         * read_fds[2] -> 聊天请求。
         */
        FD_ZERO(&rfds);
        FD_SET(read_fds[0], &rfds);
        FD_SET(read_fds[1], &rfds);
        FD_SET(read_fds[2], &rfds);

        maxfd = read_fds[0];
        if (read_fds[1] > maxfd)
            maxfd = read_fds[1];
        if (read_fds[2] > maxfd)
            maxfd = read_fds[2];

        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) == -1)
        {
            if (errno == EINTR)
                continue;
            perror("select");
            cleanup(EXIT_FAILURE);
        }

        /* 哪个 FIFO 可读，就调用对应的 drain 函数处理该类请求。 */
        if (FD_ISSET(read_fds[0], &rfds))
            drain_auth_fifo(read_fds[0], 1);
        if (FD_ISSET(read_fds[1], &rfds))
            drain_auth_fifo(read_fds[1], 0);
        if (FD_ISSET(read_fds[2], &rfds))
            drain_chat_fifo(read_fds[2]);
    }
}
