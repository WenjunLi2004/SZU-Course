/*
 * chatserver.c
 *
 * 多用户聊天系统服务器。
 *
 * 架构：
 *   - 守护进程化（fork/setsid/chdir/umask/重定向标准流），日志写 ~/log/chat-logs。
 *   - 用户表在 POSIX 共享内存（src/user_store.[ch]），进程间互斥锁保护。
 *   - 固定大小线程池（POOLSIZE，src/thread_pool.[ch]），空闲线程 LIFO 栈管理。
 *   - 主线程用 select 多路复用 4 个公共 FIFO（注册/登录/发消息/登出），读到完整请求后
 *     打包成 job 派发给空闲工作线程；业务逻辑在工作线程里跑，日志线程安全。
 *   - 业务：注册/登录（含在线名单广播）、登出广播、一对一发送（成功计数+重要朋友 *）、
 *     离线消息暂存与回推、机器人管理（__botmgr__）。
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "chat_common.h"
#include "src/config.h"
#include "src/user_store.h"
#include "src/thread_pool.h"

#define SERVER_OWNER "liwenjun2023150001"

/* 4 路公共 FIFO 的统一索引。 */
enum { FIFO_REGISTER = 0, FIFO_LOGIN, FIFO_MESSAGE, FIFO_LOGOUT, FIFO_COUNT };

/* 派发给工作线程的 job：一份完整请求 + 类型。 */
enum { JOB_REGISTER = 0, JOB_LOGIN, JOB_LOGOUT, JOB_CHAT };
typedef struct {
    int kind;
    union {
        ChatAuthRequest   auth;
        ChatLogoutRequest logout;
        ChatSendRequest   chat;
    } u;
} ServerJob;

static ChatConfig      g_cfg;
static UserStoreHandle g_store;
static ThreadPool     *g_pool = NULL;
static int        read_fds[FIFO_COUNT] = {-1, -1, -1, -1};
static int        hold_fds[FIFO_COUNT] = {-1, -1, -1, -1};
static const char *fifo_paths[FIFO_COUNT];

static int g_log_fd = -1;      /* server.log */
static int g_threads_fd = -1;  /* threads.log */
/* 工作线程并发写日志，用一把锁串行化整行写出，避免交错。 */
static pthread_mutex_t g_log_lock = PTHREAD_MUTEX_INITIALIZER;

static volatile sig_atomic_t g_shutdown_requested = 0;
static volatile sig_atomic_t g_shutdown_signal = 0;

/* ------------------------------------------------------------------ */
/* 日志                                                                */
/* ------------------------------------------------------------------ */

/* 把 t 格式化为 "YYYY-MM-DD HH:MM:SS"。 */
static void fmt_time(long t, char *buf, size_t sz)
{
    time_t tt = (time_t)t;
    struct tm tmv;
    localtime_r(&tt, &tmv);
    strftime(buf, sz, "%Y-%m-%d %H:%M:%S", &tmv);
}

/* 向 fd 写一整行日志（带时间戳 + pid + 可选 level），整行在锁内一次 write。 */
static void vlog_to(int fd, const char *level, const char *fmt, va_list ap)
{
    char   ts[32], line[1100];
    int    hdr, body;
    size_t len;
    int    out = (fd != -1) ? fd : STDERR_FILENO;

    fmt_time((long)time(NULL), ts, sizeof(ts));
    if (level)
        hdr = snprintf(line, sizeof(line), "%s [pid=%d] [%s] ", ts, (int)getpid(), level);
    else
        hdr = snprintf(line, sizeof(line), "%s [pid=%d] ", ts, (int)getpid());
    if (hdr < 0 || (size_t)hdr >= sizeof(line)) return;

    body = vsnprintf(line + hdr, sizeof(line) - (size_t)hdr, fmt, ap);
    if (body < 0) return;
    len = (size_t)hdr + (size_t)body;
    if (len >= sizeof(line) - 1) len = sizeof(line) - 2;
    line[len++] = '\n';

    pthread_mutex_lock(&g_log_lock);
    {
        size_t off = 0;
        while (off < len)
        {
            ssize_t w = write(out, line + off, len - off);
            if (w <= 0) { if (w == -1 && errno == EINTR) continue; break; }
            off += (size_t)w;
        }
    }
    pthread_mutex_unlock(&g_log_lock);
}

static void log_message(const char *level, const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    vlog_to(g_log_fd, level, fmt, ap);
    va_end(ap);
}

static void threads_log(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    vlog_to(g_threads_fd, NULL, fmt, ap);
    va_end(ap);
}

/* ------------------------------------------------------------------ */
/* 目录 / FIFO / 权限                                                  */
/* ------------------------------------------------------------------ */

static void copy_string(char *dst, size_t dst_size, const char *src)
{
    snprintf(dst, dst_size, "%s", src == NULL ? "" : src);
}

/* mkdir -p：逐级创建。成功或已存在返回 0。 */
static int mkdir_p(const char *path, mode_t mode)
{
    char tmp[512];
    size_t len = strlen(path);
    size_t i;
    if (len == 0 || len >= sizeof(tmp)) { errno = ENAMETOOLONG; return -1; }
    memcpy(tmp, path, len + 1);
    for (i = 1; i < len; i++)
    {
        if (tmp[i] != '/') continue;
        tmp[i] = '\0';
        if (mkdir(tmp, mode) == -1 && errno != EEXIST) return -1;
        tmp[i] = '/';
    }
    if (mkdir(tmp, mode) == -1 && errno != EEXIST) return -1;
    return 0;
}

/* 服务器以 sudo 启动时把目录/FIFO 所有权交还给普通用户，便于客户端读写。 */
static void chown_back_to_owner(const char *path)
{
    struct passwd *pw = getpwnam(SERVER_OWNER);
    if (!pw) return;
    if (chown(path, pw->pw_uid, pw->pw_gid) == -1 && errno != EPERM)
        log_message("WARN", "chown %s failed: %s", path, strerror(errno));
}

/* 创建日志目录树并打开 server.log / threads.log，均强制 0600（保持 root 所有）。 */
static int open_logs(const ChatConfig *cfg)
{
    if (mkdir_p(cfg->log_dir_server, 0700) == -1)
    {
        log_message("ERROR", "mkdir -p %s failed: %s", cfg->log_dir_server, strerror(errno));
        return -1;
    }

    g_log_fd = open(cfg->server_log_path, O_CREAT | O_APPEND | O_WRONLY, 0600);
    if (g_log_fd == -1)
    {
        log_message("ERROR", "open %s failed: %s", cfg->server_log_path, strerror(errno));
        return -1;
    }
    if (fchmod(g_log_fd, 0600) == -1 && errno != EINVAL)
    {
        log_message("ERROR", "fchmod %s failed: %s", cfg->server_log_path, strerror(errno));
        return -1;
    }

    g_threads_fd = open(cfg->threads_log_path, O_CREAT | O_APPEND | O_WRONLY, 0600);
    if (g_threads_fd == -1)
    {
        log_message("ERROR", "open %s failed: %s", cfg->threads_log_path, strerror(errno));
        return -1;
    }
    if (fchmod(g_threads_fd, 0600) == -1 && errno != EINVAL)
    {
        log_message("ERROR", "fchmod %s failed: %s", cfg->threads_log_path, strerror(errno));
        return -1;
    }
    return 0;
}

/* 创建公共 FIFO 目录（root 创建后 chown 回普通用户）。 */
static int ensure_fifo_dir(const ChatConfig *cfg)
{
    if (mkdir_p(cfg->fifo_dir, 0777) == -1)
    {
        log_message("ERROR", "mkdir -p %s failed: %s", cfg->fifo_dir, strerror(errno));
        return -1;
    }
    chown_back_to_owner(cfg->fifo_dir);
    return 0;
}

static int ensure_fifo(const char *path)
{
    struct stat st;
    if (stat(path, &st) == 0)
    {
        if (S_ISFIFO(st.st_mode)) return 0;
        log_message("ERROR", "%s exists but is not a FIFO", path);
        return -1;
    }
    if (mkfifo(path, 0666) == -1)
    {
        log_message("ERROR", "mkfifo %s failed: %s", path, strerror(errno));
        return -1;
    }
    return 0;
}

static int open_server_fifo(const char *path, int index)
{
    if (ensure_fifo(path) == -1) return -1;
    chown_back_to_owner(path);

    /* 读端非阻塞，完全交给主循环的 select 驱动，不在 read 上阻塞。 */
    read_fds[index] = open(path, O_RDONLY | O_NONBLOCK);
    if (read_fds[index] == -1)
    {
        log_message("ERROR", "open(rd) %s failed: %s", path, strerror(errno));
        return -1;
    }
    /* 自己再占一个永不写入的写端：否则当最后一个客户端关闭写端时，读端会被 select
     * 持续报可读（读到 EOF），导致没有客户端时主循环空转。留住它即可避免。 */
    hold_fds[index] = open(path, O_WRONLY | O_NONBLOCK);
    if (hold_fds[index] == -1)
    {
        log_message("ERROR", "open(wr) %s failed: %s", path, strerror(errno));
        return -1;
    }
    return 0;
}

static void cleanup(int status)
{
    int i;
    if (g_pool)
    {
        thread_pool_destroy(g_pool);   /* 通知并 join 所有工作线程 */
        g_pool = NULL;
        log_message("INFO", "thread pool shut down (all workers joined)");
    }
    for (i = 0; i < FIFO_COUNT; i++)
    {
        if (read_fds[i] != -1) close(read_fds[i]);
        if (hold_fds[i] != -1) close(hold_fds[i]);
        if (fifo_paths[i]) unlink(fifo_paths[i]);
    }
    user_store_destroy(&g_store);
    log_message("INFO", "server stopped");
    exit(status);
}

/* 信号处理器里只置 sig_atomic_t 标志，真正的 join/清理留给主循环做——
 * 避免在异步信号上下文里调用非 async-signal-safe 的函数。 */
static void handle_signal(int sig)
{
    g_shutdown_signal = sig;
    g_shutdown_requested = 1;
}

static void shutdown_if_requested(void)
{
    if (!g_shutdown_requested) return;
    log_message("INFO", "received signal %d, shutting down", (int)g_shutdown_signal);
    cleanup(EXIT_SUCCESS);
}

/* ------------------------------------------------------------------ */
/* 数据包构造与发送                                                    */
/* ------------------------------------------------------------------ */

static void make_packet(ChatPacket *p, int type, int ok, const char *from, const char *message)
{
    memset(p, 0, sizeof(*p));
    p->type = type;
    p->ok = ok;
    copy_string(p->from, sizeof(p->from), from);
    copy_string(p->message, sizeof(p->message), message);
}

/* 非阻塞打开并写：目标客户端 FIFO 没有读者（进程已退）时立即失败而不是阻塞工作线程，
 * 调用方据此把对方当作已掉线处理。 */
static int send_packet(const char *fifo, const ChatPacket *packet)
{
    int fd;
    ssize_t written;
    if (!fifo || !fifo[0]) return -1;
    fd = open(fifo, O_WRONLY | O_NONBLOCK);
    if (fd == -1) return -1;
    written = write(fd, packet, sizeof(*packet));
    close(fd);
    return written == (ssize_t)sizeof(*packet) ? 0 : -1;
}

static void reply_to_fifo(const char *fifo, int ok, const char *message)
{
    ChatPacket p;
    make_packet(&p, CHAT_PACKET_REPLY, ok, "server", message);
    p.timestamp = (long)time(NULL);
    if (send_packet(fifo, &p) == -1)
        log_message("WARN", "failed to reply through %s", fifo);
}

/* 向所有在线真实客户端广播一条 SYSTEM 文案（可排除一人）。 */
static void broadcast_system(const char *text, const char *exclude)
{
    char (*fifos)[CHAT_FIFO_PATH_LEN] = malloc((size_t)CHAT_MAX_USERS * CHAT_FIFO_PATH_LEN);
    int n, i;
    ChatPacket p;
    if (!fifos) return;
    n = user_store_snapshot_receiver_fifos(&g_store, fifos, CHAT_MAX_USERS, exclude);
    make_packet(&p, CHAT_PACKET_SYSTEM, 1, "server", text);
    p.timestamp = (long)time(NULL);
    for (i = 0; i < n; i++) send_packet(fifos[i], &p);
    free(fifos);
}

/* 向所有在线真实客户端广播个性化在线名单（重要朋友带 *）。 */
static void broadcast_online_list(void)
{
    ChatPacket *pk = malloc(sizeof(ChatPacket) * (size_t)CHAT_MAX_USERS);
    char (*fifos)[CHAT_FIFO_PATH_LEN] = malloc((size_t)CHAT_MAX_USERS * CHAT_FIFO_PATH_LEN);
    int n, i;
    long now = (long)time(NULL);
    if (!pk || !fifos) { free(pk); free(fifos); return; }
    n = user_store_build_online_broadcast(&g_store, pk, fifos, CHAT_MAX_USERS);
    for (i = 0; i < n; i++) { pk[i].timestamp = now; send_packet(fifos[i], &pk[i]); }
    free(pk); free(fifos);
}

/* ------------------------------------------------------------------ */
/* 业务处理（工作线程上下文）                                          */
/* ------------------------------------------------------------------ */

static void do_register(const ChatAuthRequest *req)
{
    UserStoreStatus st;
    char tbuf[32];
    fmt_time((long)time(NULL), tbuf, sizeof(tbuf));

    if (req->username[0] == '\0' || req->password[0] == '\0')
    {
        reply_to_fifo(req->fifo, 0, "username and password must not be empty");
        return;
    }
    st = user_store_register(&g_store, req->username, req->password, req->fifo, 0);
    switch (st)
    {
    case USER_STORE_OK:
        log_message("INFO", "(%s, register, %s)", req->username, tbuf);
        reply_to_fifo(req->fifo, 1, "register ok");
        break;
    case USER_STORE_ERR_EXISTS:
        log_message("WARN", "duplicate register rejected: %s", req->username);
        reply_to_fifo(req->fifo, 0, "username already exists");
        break;
    case USER_STORE_ERR_FULL:
        reply_to_fifo(req->fifo, 0, "server user table is full");
        break;
    default:
        reply_to_fifo(req->fifo, 0, "register failed");
        break;
    }
}

/* 登录成功后把暂存的离线消息按原始时间回推给该用户。 */
static void push_offline_messages(const char *user, const char *user_fifo)
{
    ChatOfflineMessage *msgs =
        malloc(sizeof(ChatOfflineMessage) * (size_t)CHAT_MAX_OFFLINE_MESSAGES);
    int n, i, pushed = 0;
    if (!msgs) return;

    /* 取出即清空：一次拿到发给该用户的全部离线消息，再逐条回推。 */
    n = user_store_pop_offline(&g_store, user, msgs, CHAT_MAX_OFFLINE_MESSAGES);
    for (i = 0; i < n; i++)
    {
        ChatPacket p;
        char tbuf[32];
        make_packet(&p, CHAT_PACKET_OFFLINE_PUSH, 1, msgs[i].from, msgs[i].text);
        p.timestamp = msgs[i].timestamp;             /* 保留原始发送时间 */
        fmt_time(msgs[i].timestamp, tbuf, sizeof(tbuf));
        send_packet(user_fifo, &p);
        user_store_increment_send(&g_store, msgs[i].from, user);
        log_message("INFO", "(%s, %s, %s, sent) [offline push]",
                    msgs[i].from, user, tbuf);
        pushed++;
    }
    if (pushed > 0)
        log_message("INFO", "pushed %d offline message(s) to %s", pushed, user);
    free(msgs);
}

static void do_login(const ChatAuthRequest *req)
{
    UserStoreStatus st;
    char tbuf[32];
    long now = (long)time(NULL);
    fmt_time(now, tbuf, sizeof(tbuf));

    st = user_store_login(&g_store, req->username, req->password,
                          req->fifo, now);
    if (st == USER_STORE_OK)
    {
        char list[CHAT_TEXT_LEN];
        char msg[CHAT_TEXT_LEN];
        int  count = 0;
        ChatPacket p;

        user_store_build_online_list(&g_store, req->username,
                                     list, sizeof(list), &count);
        snprintf(msg, sizeof(msg), "login ok; online %d: %s", count, list);
        make_packet(&p, CHAT_PACKET_REPLY, 1, "server", msg);
        p.online_count = count;
        p.timestamp = now;
        send_packet(req->fifo, &p);

        log_message("INFO", "(%s, login, %s) online=%d",
                    req->username, tbuf, count);

        /* 回推离线消息，再把上线广播给其他人，最后刷新所有人的在线名单。 */
        push_offline_messages(req->username, req->fifo);
        {
            char sys[128];
            snprintf(sys, sizeof(sys), "%s is now online", req->username);
            broadcast_system(sys, req->username);
        }
        broadcast_online_list();
        return;
    }
    if (st == USER_STORE_ERR_NOT_FOUND)
        reply_to_fifo(req->fifo, 0, "username does not exist");
    else if (st == USER_STORE_ERR_BAD_PASSWORD)
        reply_to_fifo(req->fifo, 0, "password is incorrect");
    else
        reply_to_fifo(req->fifo, 0, "login failed");
    log_message("INFO", "login failed for %s", req->username);
}

static void do_logout(const ChatLogoutRequest *req)
{
    UserStoreStatus st;
    char tbuf[32];
    long now = (long)time(NULL);
    fmt_time(now, tbuf, sizeof(tbuf));

    st = user_store_logout(&g_store, req->username, req->fifo, now);
    if (st != USER_STORE_OK) return;   /* 不存在 / fifo 不匹配：静默丢弃 */

    reply_to_fifo(req->fifo, 1, "logout ok");
    log_message("INFO", "(%s, logout, %s)", req->username, tbuf);
    {
        char sys[128];
        snprintf(sys, sizeof(sys), "%s has logged out", req->username);
        broadcast_system(sys, NULL);   /* 该用户已离线，不在接收集合中 */
    }
    broadcast_online_list();
}

/* 处理发给机器人的消息：在线机器人立即回固定话术；离线/不存在则丢弃不暂存。 */
static void do_chat_to_bot(const char *from, const char *from_fifo,
                           const char *botname, const ChatSendInfo *info)
{
    char tbuf[32];
    fmt_time((long)time(NULL), tbuf, sizeof(tbuf));

    if (!info->target_online)
    {
        reply_to_fifo(from_fifo, 0, "bot is offline; message discarded (no offline queue for bots)");
        log_message("INFO", "(%s, %s, %s, discarded) [bot offline]", from, botname, tbuf);
        return;
    }
    /* 记录 sender->bot，并立即代机器人回复 sender。 */
    user_store_increment_send(&g_store, from, botname);
    log_message("INFO", "(%s, %s, %s, sent) [to bot]", from, botname, tbuf);
    {
        ChatPacket p;
        char reply[CHAT_TEXT_LEN];
        snprintf(reply, sizeof(reply), "幸会，%s，很高兴认识您", from);
        make_packet(&p, CHAT_PACKET_MESSAGE, 1, botname, reply);
        p.timestamp = (long)time(NULL);
        send_packet(from_fifo, &p);
    }
    user_store_increment_send(&g_store, botname, from);
    log_message("INFO", "(%s, %s, %s, sent) [bot reply]", botname, from, tbuf);
    reply_to_fifo(from_fifo, 1, "bot replied");
}

static void do_chat_to_human(const ChatSendRequest *req, const ChatSendInfo *info)
{
    char tbuf[32];
    long now = (long)time(NULL);
    fmt_time(now, tbuf, sizeof(tbuf));

    if (info->target_online)
    {
        ChatPacket msg;
        make_packet(&msg, CHAT_PACKET_MESSAGE, 1, req->from, req->text);
        msg.timestamp = now;
        if (send_packet(info->to_fifo, &msg) == 0)
        {
            int c = user_store_increment_send(&g_store, req->from, req->to);
            int important = (c > CHAT_IMPORTANT_FRIEND_MIN);
            char ack[CHAT_TEXT_LEN];
            ChatPacket rp;
            log_message("INFO", "(%s, %s, %s, sent)", req->from, req->to, tbuf);
            snprintf(ack, sizeof(ack), "message sent to %s%s",
                     req->to, important ? "*" : "");
            make_packet(&rp, CHAT_PACKET_REPLY, 1, "server", ack);
            rp.send_count = c;
            rp.timestamp = now;
            send_packet(info->from_fifo, &rp);
            return;
        }
        /* 在线但写 FIFO 失败：认定目标已掉线，置其离线后落入下面的离线暂存分支。 */
        user_store_set_offline(&g_store, req->to, info->to_fifo);
    }

    /* 目标离线（或刚被判定掉线）：暂存离线消息，等其重新登录再回推。 */
    if (user_store_store_offline(&g_store, req->from, req->to,
                                 req->text, now) == USER_STORE_OK)
    {
        log_message("INFO", "(%s, %s, %s, pending)", req->from, req->to, tbuf);
        reply_to_fifo(info->from_fifo, 0,
                      "target offline; message stored as pending");
    }
    else
    {
        log_message("WARN", "offline queue full; dropping (%s -> %s)", req->from, req->to);
        reply_to_fifo(info->from_fifo, 0, "target offline and offline queue full");
    }
}

/* 解析 "add <x>" / "del <x>"，返回 1 并填 op/count；否则 0。 */
static int parse_bot_cmd(const char *text, char *op, int *count)
{
    int x = 0;
    if (sscanf(text, "%7s %d", op, &x) >= 1)
    {
        if (x < 0) x = 0;
        *count = x;
        return 1;
    }
    return 0;
}

static void do_bot_manager(const ChatSendRequest *req)
{
    char from_fifo[CHAT_FIFO_PATH_LEN];
    char op[8] = {0};
    int  x = 0;
    char tbuf[32];
    long now = (long)time(NULL);
    fmt_time(now, tbuf, sizeof(tbuf));

    /* 机器人管理是“在线客户端用户”的动作：要求发起者存在、在线、非机器人且有私有 FIFO。
     * 仅注册过却未登录的用户（用户表里仍留着 fifo）不允许增删机器人。 */
    if (!user_store_lookup_online_client_fifo(&g_store, req->from, from_fifo, sizeof(from_fifo)))
    {
        /* 未登录：能找到该用户注册时留下的 FIFO 就回一句明确错误，否则静默丢弃。 */
        char fallback[CHAT_FIFO_PATH_LEN];
        if (user_store_lookup_fifo(&g_store, req->from, fallback, sizeof(fallback)) && fallback[0])
            reply_to_fifo(fallback, 0, "bot manager requires login");
        log_message("WARN", "bot manager rejected (not logged in): %s", req->from);
        return;
    }

    if (!parse_bot_cmd(req->text, op, &x) || (strcmp(op, "add") != 0 && strcmp(op, "del") != 0))
    {
        reply_to_fifo(from_fifo, 0, "usage: /bot add <x> | /bot del <x>");
        return;
    }

    if (strcmp(op, "add") == 0)
    {
        char summary[CHAT_TEXT_LEN];
        size_t off = 0;
        int i, made = 0;
        summary[0] = '\0';
        for (i = 0; i < x; i++)
        {
            char name[CHAT_NAME_LEN];
            if (user_store_add_bot(&g_store, name, sizeof(name), now) != USER_STORE_OK) break;
            log_message("INFO", "(%s, register, %s) [bot]", name, tbuf);
            log_message("INFO", "(%s, login, %s) [bot]", name, tbuf);
            int n = snprintf(summary + off, sizeof(summary) - off, "%s%s", off ? "," : "", name);
            if (n > 0 && (size_t)n < sizeof(summary) - off) off += (size_t)n;
            made++;
        }
        broadcast_online_list();
        {
            char ack[CHAT_TEXT_LEN];
            snprintf(ack, sizeof(ack), "added %d bot(s): %s", made, summary);
            reply_to_fifo(from_fifo, 1, ack);
        }
        log_message("INFO", "bot manager: %s requested add %d, created %d", req->from, x, made);
    }
    else /* del */
    {
        char names[CHAT_MAX_USERS][CHAT_NAME_LEN];
        char summary[CHAT_TEXT_LEN];
        size_t off = 0;
        int n, i;
        summary[0] = '\0';
        n = user_store_pick_online_bots(&g_store, x, names, CHAT_MAX_USERS, now);
        for (i = 0; i < n; i++)
        {
            int k;
            log_message("INFO", "(%s, logout, %s) [bot]", names[i], tbuf);
            k = snprintf(summary + off, sizeof(summary) - off, "%s%s", off ? "," : "", names[i]);
            if (k > 0 && (size_t)k < sizeof(summary) - off) off += (size_t)k;
            {
                char sys[128];
                snprintf(sys, sizeof(sys), "robot %s has logged out", names[i]);
                broadcast_system(sys, NULL);
            }
        }
        broadcast_online_list();
        {
            char ack[CHAT_TEXT_LEN];
            snprintf(ack, sizeof(ack), "removed %d bot(s): %s", n, summary);
            reply_to_fifo(from_fifo, 1, ack);
        }
        log_message("INFO", "bot manager: %s requested del %d, removed %d", req->from, x, n);
    }
}

static void do_chat(const ChatSendRequest *req)
{
    ChatSendInfo info;

    if (strcmp(req->to, CHAT_BOTMGR_TARGET) == 0) { do_bot_manager(req); return; }

    user_store_prepare_send(&g_store, req->from, req->to, &info);
    if (!info.sender_online) return;                 /* 发送方未登录：丢弃 */
    if (!info.target_exists)
    {
        reply_to_fifo(info.from_fifo, 0, "target user does not exist");
        return;
    }
    if (info.target_is_bot) { do_chat_to_bot(req->from, info.from_fifo, req->to, &info); return; }
    do_chat_to_human(req, &info);
}

/* ------------------------------------------------------------------ */
/* 线程池回调                                                          */
/* ------------------------------------------------------------------ */

static const char *job_type_name(int kind)
{
    switch (kind)
    {
    case JOB_REGISTER: return "register";
    case JOB_LOGIN:    return "login";
    case JOB_LOGOUT:   return "logout";
    case JOB_CHAT:     return "chat";
    default:           return "?";
    }
}

static const char *job_requester(const ServerJob *j)
{
    switch (j->kind)
    {
    case JOB_REGISTER:
    case JOB_LOGIN:    return j->u.auth.username;
    case JOB_LOGOUT:   return j->u.logout.username;
    case JOB_CHAT:     return j->u.chat.from;
    default:           return "?";
    }
}

/* 工作线程入口：threads.log 记 dispatch(busy)/recycle(idle)，中间跑业务。 */
static void handle_job(void *job, int worker_index, unsigned long tid)
{
    ServerJob *j = (ServerJob *)job;

    threads_log("dispatch worker=#%d tid=%lu type=%s user=%s state=busy",
                worker_index, tid, job_type_name(j->kind), job_requester(j));

    switch (j->kind)
    {
    case JOB_REGISTER: do_register(&j->u.auth);  break;
    case JOB_LOGIN:    do_login(&j->u.auth);     break;
    case JOB_LOGOUT:   do_logout(&j->u.logout);  break;
    case JOB_CHAT:     do_chat(&j->u.chat);      break;
    default: break;
    }

    threads_log("recycle  worker=#%d tid=%lu type=%s state=idle",
                worker_index, tid, job_type_name(j->kind));
}

static void free_job(void *job) { free(job); }

/* ------------------------------------------------------------------ */
/* 主线程：select 读请求并派发                                          */
/* ------------------------------------------------------------------ */

/* 无空闲线程时尽量回 server busy（仅在请求自带 reply fifo 时）。 */
static void reply_busy(const ServerJob *j)
{
    if (j->kind == JOB_REGISTER || j->kind == JOB_LOGIN)
        reply_to_fifo(j->u.auth.fifo, 0, "server busy, please retry");
    else if (j->kind == JOB_LOGOUT)
        reply_to_fifo(j->u.logout.fifo, 0, "server busy, please retry");
}

static void drain_fifo(int fd, int kind)
{
    for (;;)
    {
        ServerJob job;
        ssize_t  nread;
        size_t   reqsz;
        void    *dst;

        job.kind = kind;
        switch (kind)
        {
        case JOB_CHAT:   reqsz = sizeof(ChatSendRequest);   dst = &job.u.chat;   break;
        case JOB_LOGOUT: reqsz = sizeof(ChatLogoutRequest); dst = &job.u.logout; break;
        default:         reqsz = sizeof(ChatAuthRequest);   dst = &job.u.auth;   break;
        }

        nread = read(fd, dst, reqsz);
        if (nread == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            if (errno == EINTR) continue;
            log_message("ERROR", "read %s fifo failed: %s", job_type_name(kind), strerror(errno));
            return;
        }
        if (nread == 0) return;
        if ((size_t)nread != reqsz)
        {
            log_message("WARN", "ignoring incomplete %s request: %zd bytes", job_type_name(kind), nread);
            continue;
        }

        log_message("INFO", "request arrival: type=%s user=%s", job_type_name(kind), job_requester(&job));

        {
            ServerJob *jp = malloc(sizeof(*jp));
            if (!jp) { log_message("ERROR", "out of memory packaging job"); continue; }
            *jp = job;
            if (thread_pool_dispatch(g_pool, jp) != 0)
            {
                log_message("WARN", "no idle worker (busy), dropping %s from %s",
                            job_type_name(kind), job_requester(&job));
                reply_busy(&job);
                free(jp);
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* 启动                                                                */
/* ------------------------------------------------------------------ */

static void redirect_std_to_devnull(void)
{
    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) return;
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    if (fd > STDERR_FILENO) close(fd);
}

static int daemonize_step1_fork(void)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        log_message("ERROR", "fork failed: %s", strerror(errno));
        return -1;
    }
    if (pid > 0) _exit(EXIT_SUCCESS);
    if (setsid() == -1)
    {
        log_message("ERROR", "setsid failed: %s", strerror(errno));
        return -1;
    }
    return 0;
}

static void log_startup_info(const ChatConfig *cfg)
{
    char tbuf[32];
    fmt_time((long)time(NULL), tbuf, sizeof(tbuf));
    log_message("INFO", "server starting at %s", tbuf);
    log_message("INFO", "config: %s", cfg->full_name);
    log_message("INFO", "fifo_dir        = %s", cfg->fifo_dir);
    log_message("INFO", "client_fifo_dir = %s", cfg->client_fifo_dir);
    log_message("INFO", "log_dir         = %s", cfg->log_dir);
    log_message("INFO", "REG_FIFO    -> %s", cfg->fifo_register);
    log_message("INFO", "LOGIN_FIFO  -> %s", cfg->fifo_login);
    log_message("INFO", "MSG_FIFO    -> %s", cfg->fifo_message);
    log_message("INFO", "LOGOUT_FIFO -> %s", cfg->fifo_logout);
    log_message("INFO", "POOLSIZE = %d", cfg->poolsize);
    log_message("INFO", "sizeof(ChatPacket)=%zu sizeof(ChatSendRequest)=%zu",
                sizeof(ChatPacket), sizeof(ChatSendRequest));
}

int main(int argc, char *argv[])
{
    fd_set rfds;
    int maxfd, i;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <config-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (chat_config_load(argv[1], &g_cfg) != 0)
        return EXIT_FAILURE;

    if (daemonize_step1_fork() != 0)
        return EXIT_FAILURE;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    if (chdir("/") == -1) return EXIT_FAILURE;
    umask(0);

    if (open_logs(&g_cfg) != 0) return EXIT_FAILURE;
    redirect_std_to_devnull();

    log_startup_info(&g_cfg);

    /* 共享内存用户表。 */
    if (user_store_init(&g_store, g_cfg.short_name) != 0)
    {
        log_message("ERROR", "user store init failed: %s", strerror(errno));
        cleanup(EXIT_FAILURE);
    }
    log_message("INFO", "shm user store ready: name=%s shm_size=%zu",
                g_store.name, g_store.size);
    if (!g_store.process_shared)
    {
        log_message("ERROR", "process-shared mutex required but unavailable; aborting");
        cleanup(EXIT_FAILURE);
    }
    log_message("INFO", "user store mutex initialized as process-shared");

    /* 线程池。 */
    g_pool = thread_pool_create(g_cfg.poolsize, handle_job, free_job);
    if (!g_pool)
    {
        log_message("ERROR", "thread pool creation failed (poolsize=%d)", g_cfg.poolsize);
        cleanup(EXIT_FAILURE);
    }
    log_message("INFO", "thread pool created: %d threads", thread_pool_size(g_pool));

    /* 公共 FIFO。 */
    if (ensure_fifo_dir(&g_cfg) == -1) cleanup(EXIT_FAILURE);
    fifo_paths[FIFO_REGISTER] = g_cfg.fifo_register;
    fifo_paths[FIFO_LOGIN]    = g_cfg.fifo_login;
    fifo_paths[FIFO_MESSAGE]  = g_cfg.fifo_message;
    fifo_paths[FIFO_LOGOUT]   = g_cfg.fifo_logout;
    for (i = 0; i < FIFO_COUNT; i++)
        if (open_server_fifo(fifo_paths[i], i) == -1) cleanup(EXIT_FAILURE);

    log_message("INFO", "%s ready (select main loop + %d-thread pool)",
                g_cfg.full_name, thread_pool_size(g_pool));

    while (1)
    {
        /* 每轮都重建：select 会就地改写 rfds（只保留就绪的位），不重建下一轮集合就不完整。 */
        FD_ZERO(&rfds);
        maxfd = -1;
        for (i = 0; i < FIFO_COUNT; i++)
        {
            FD_SET(read_fds[i], &rfds);
            if (read_fds[i] > maxfd) maxfd = read_fds[i];
        }

        if (select(maxfd + 1, &rfds, NULL, NULL, NULL) == -1)
        {
            if (errno == EINTR) { shutdown_if_requested(); continue; }
            log_message("ERROR", "select failed: %s", strerror(errno));
            cleanup(EXIT_FAILURE);
        }
        shutdown_if_requested();

        if (FD_ISSET(read_fds[FIFO_REGISTER], &rfds))
            drain_fifo(read_fds[FIFO_REGISTER], JOB_REGISTER);
        if (FD_ISSET(read_fds[FIFO_LOGIN], &rfds))
            drain_fifo(read_fds[FIFO_LOGIN], JOB_LOGIN);
        if (FD_ISSET(read_fds[FIFO_MESSAGE], &rfds))
            drain_fifo(read_fds[FIFO_MESSAGE], JOB_CHAT);
        if (FD_ISSET(read_fds[FIFO_LOGOUT], &rfds))
            drain_fifo(read_fds[FIFO_LOGOUT], JOB_LOGOUT);
    }
}
