/*
 * chatclient.c
 *
 * 多用户聊天系统客户端。
 * 客户端负责创建自己的私有 FIFO，通过服务器的公共 FIFO 发送注册、
 * 登录和聊天请求，同时通过 select() 同时监听键盘输入和服务器回复。
 */

#include <ctype.h>
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

/* 当前客户端的身份信息和私有 FIFO 路径。 */
static char username[CHAT_NAME_LEN];
static char password[CHAT_PASSWORD_LEN];
static char myfifo[CHAT_FIFO_PATH_LEN];

/* fifo_fd 用于读取服务器回复，hold_fd 用于保持本客户端 FIFO 的写端打开。 */
static int fifo_fd = -1;
static int hold_fd = -1;

/* 安全复制字符串，保证目标数组以 '\0' 结尾。 */
static void copy_string(char *dst, size_t dst_size, const char *src)
{
    snprintf(dst, dst_size, "%s", src == NULL ? "" : src);
}

/* 退出前关闭文件描述符并删除当前客户端的私有 FIFO。 */
static void cleanup(int status)
{
    if (fifo_fd != -1)
        close(fifo_fd);
    if (hold_fd != -1)
        close(hold_fd);
    if (myfifo[0] != '\0')
        unlink(myfifo);
    exit(status);
}

/* 收到 Ctrl+C、kill 等信号时统一走 cleanup，避免残留 FIFO 文件。 */
static void handle_signal(int sig)
{
    (void)sig;
    cleanup(EXIT_SUCCESS);
}

/* 如果目录不存在则创建；如果路径已存在但不是目录，则认为实验环境异常。 */
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

/* 创建实验要求的目录树，客户端和服务器都依赖这些路径。 */
static int ensure_tree(void)
{
    return ensure_dir(CHAT_USER_HOME) == 0 &&
           ensure_dir(CHAT_APP_DIR) == 0 &&
           ensure_dir(CHAT_DATA_DIR) == 0 &&
           ensure_dir(CHAT_SERVER_FIFO_DIR) == 0 &&
           ensure_dir(CHAT_CLIENT_FIFO_DIR) == 0;
}

/*
 * 检查用户名是否合法。
 * 用户名会直接作为私有 FIFO 文件名，因此不允许出现 '/'、空格等字符。
 */
static int valid_username(const char *name)
{
    size_t i;

    if (name[0] == '\0')
        return 0;

    for (i = 0; name[i] != '\0'; i++)
    {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_' && name[i] != '-')
            return 0;
    }

    return 1;
}

/*
 * 创建并打开客户端私有 FIFO。
 * 路径格式为 data/client_fifo/<username>。
 * 读端使用非阻塞方式，便于 select() 监听；额外打开写端 hold_fd，
 * 是为了避免本进程在没有服务器写入时读到 EOF。
 */
static int prepare_client_fifo(void)
{
    struct stat st;

    snprintf(myfifo, sizeof(myfifo), "%s/%s", CHAT_CLIENT_FIFO_DIR, username);
    if (stat(myfifo, &st) == 0)
    {
        if (!S_ISFIFO(st.st_mode))
        {
            fprintf(stderr, "%s exists but is not a FIFO\n", myfifo);
            return -1;
        }
    }
    else if (mkfifo(myfifo, 0600) == -1)
    {
        perror(myfifo);
        return -1;
    }

    fifo_fd = open(myfifo, O_RDONLY | O_NONBLOCK);
    if (fifo_fd == -1)
    {
        perror(myfifo);
        return -1;
    }

    hold_fd = open(myfifo, O_WRONLY | O_NONBLOCK);
    if (hold_fd == -1)
    {
        perror(myfifo);
        return -1;
    }

    return 0;
}

/*
 * 向指定服务器公共 FIFO 写入一个完整请求结构体。
 * server_fifo 可以是注册 FIFO、登录 FIFO 或聊天 FIFO。
 */
static int write_request(const char *server_fifo, const void *request, size_t size)
{
    int fd;
    ssize_t written;

    fd = open(server_fifo, O_WRONLY | O_NONBLOCK);
    if (fd == -1)
    {
        perror(server_fifo);
        return -1;
    }

    written = write(fd, request, size);
    close(fd);

    if (written != (ssize_t)size)
    {
        perror("write");
        return -1;
    }

    return 0;
}

/*
 * 构造注册或登录请求。
 * 注册和登录使用同一个 ChatAuthRequest，只是写入的公共 FIFO 不同。
 */
static int send_auth_request(const char *server_fifo)
{
    ChatAuthRequest req;

    memset(&req, 0, sizeof(req));
    copy_string(req.username, sizeof(req.username), username);
    copy_string(req.password, sizeof(req.password), password);
    copy_string(req.fifo, sizeof(req.fifo), myfifo);

    return write_request(server_fifo, &req, sizeof(req));
}

/* 构造聊天请求，并写入服务器的消息 FIFO。 */
static int send_chat_request(const char *to, const char *text)
{
    ChatSendRequest req;

    memset(&req, 0, sizeof(req));
    copy_string(req.from, sizeof(req.from), username);
    copy_string(req.to, sizeof(req.to), to);
    copy_string(req.text, sizeof(req.text), text);

    return write_request(CHAT_FIFO_MESSAGE, &req, sizeof(req));
}

/* 打印客户端可用命令。 */
static void print_help(void)
{
    printf("commands:\n");
    printf("  /register             register current username and password\n");
    printf("  /login                login current username and password\n");
    printf("  /send <user> <text>   send text to another online user\n");
    printf("  /quit                 exit client\n");
}

/* 重新打印交互提示符。 */
static void print_prompt(void)
{
    printf("chat> ");
    fflush(stdout);
}

/* 根据服务器返回的数据包类型，选择不同的显示格式。 */
static void print_packet(const ChatPacket *packet)
{
    if (packet->type == CHAT_PACKET_MESSAGE)
    {
        printf("\n[%s] %s\n", packet->from, packet->message);
        return;
    }

    if (packet->type == CHAT_PACKET_REPLY)
    {
        printf("\n[server] %s: %s\n", packet->ok ? "OK" : "ERROR", packet->message);
        return;
    }

    printf("\n[server] unknown packet type %d\n", packet->type);
}

/*
 * 读取当前私有 FIFO 中所有已经到达的数据包。
 * 因为 fifo_fd 是非阻塞的，读到 EAGAIN/EWOULDBLOCK 说明暂时没有更多数据。
 */
static void drain_packets(void)
{
    ChatPacket packet;
    ssize_t nread;

    while (1)
    {
        nread = read(fifo_fd, &packet, sizeof(packet));
        if (nread == sizeof(packet))
        {
            print_packet(&packet);
            continue;
        }

        if (nread == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            if (errno == EINTR)
                continue;
            perror("read");
            cleanup(EXIT_FAILURE);
        }

        if (nread == 0)
            return;

        fprintf(stderr, "\nIgnoring incomplete packet: %zd bytes\n", nread);
    }
}

/*
 * 解析用户在客户端输入的一行命令。
 * 返回 1 表示需要退出客户端，返回 0 表示继续运行。
 */
static int handle_line(char *line)
{
    char *p;
    char *to;
    char *text;

    line[strcspn(line, "\n")] = '\0';

    if (line[0] == '\0')
        return 0;

    if (strcmp(line, "/help") == 0)
    {
        print_help();
        return 0;
    }

    if (strcmp(line, "/register") == 0)
    {
        send_auth_request(CHAT_FIFO_REGISTER);
        return 0;
    }

    if (strcmp(line, "/login") == 0)
    {
        send_auth_request(CHAT_FIFO_LOGIN);
        return 0;
    }

    if (strcmp(line, "/quit") == 0)
        return 1;

    if (strncmp(line, "/send ", 6) == 0)
    {
        p = line + 6;
        /* 跳过 /send 后面的空白，先取目标用户名。 */
        while (isspace((unsigned char)*p))
            p++;

        to = p;
        while (*p != '\0' && !isspace((unsigned char)*p))
            p++;

        if (*p == '\0')
        {
            printf("Usage: /send <user> <text>\n");
            return 0;
        }

        *p++ = '\0';
        /* 剩余非空内容都作为消息正文，允许正文中继续包含空格。 */
        while (isspace((unsigned char)*p))
            p++;

        text = p;
        if (to[0] == '\0' || text[0] == '\0')
        {
            printf("Usage: /send <user> <text>\n");
            return 0;
        }

        send_chat_request(to, text);
        return 0;
    }

    printf("Unknown command. Type /help for command list.\n");
    return 0;
}

int main(int argc, char *argv[])
{
    fd_set rfds;
    char line[512];
    int maxfd;

    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s username password\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!valid_username(argv[1]))
    {
        fprintf(stderr, "Username may contain only letters, digits, '_' and '-'\n");
        return EXIT_FAILURE;
    }

    copy_string(username, sizeof(username), argv[1]);
    copy_string(password, sizeof(password), argv[2]);

    /* 注册退出信号处理函数，确保异常退出时也清理私有 FIFO。 */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGHUP, handle_signal);

    if (!ensure_tree() || prepare_client_fifo() == -1)
        cleanup(EXIT_FAILURE);

    printf("client fifo: %s\n", myfifo);
    print_help();
    print_prompt();

    while (1)
    {
        /*
         * 同时监听两个输入源：
         * 1. STDIN_FILENO: 用户输入命令；
         * 2. fifo_fd: 服务器写回的回复或聊天消息。
         */
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(fifo_fd, &rfds);
        maxfd = fifo_fd > STDIN_FILENO ? fifo_fd : STDIN_FILENO;

        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) == -1)
        {
            if (errno == EINTR)
                continue;
            perror("select");
            cleanup(EXIT_FAILURE);
        }

        if (FD_ISSET(fifo_fd, &rfds))
        {
            /* 服务器有新数据，读完当前积压的所有 ChatPacket。 */
            drain_packets();
            print_prompt();
        }

        if (FD_ISSET(STDIN_FILENO, &rfds))
        {
            /* 用户有新输入，解析命令并发送相应请求。 */
            if (fgets(line, sizeof(line), stdin) == NULL)
                break;
            if (handle_line(line))
                break;
            print_prompt();
        }
    }

    cleanup(EXIT_SUCCESS);
}
