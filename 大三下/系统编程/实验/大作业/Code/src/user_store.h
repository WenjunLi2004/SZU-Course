/*
 * user_store.h
 *
 * 服务器内部的共享内存用户表
 * 用户表放在 POSIX 共享内存对象里，由放在共享区内、属性为
 * PTHREAD_PROCESS_SHARED 的 pthread 互斥锁保护。服务器是
 * 多线程（线程池）单进程，工作线程并发访问这张表，全部走这把锁。
 *
 * 这里是“服务器内部存储”，不是 client<->server 的线协议（线协议在 chat_common.h）。
 *
 * 表中维护：is_bot/login_time/logout_time、成功发送计数矩阵、
 * 离线消息缓冲、在线名单快照、机器人增删等，支撑在线广播/离线回推/重要朋友/机器人管理。
 */

#ifndef CHAT_USER_STORE_H
#define CHAT_USER_STORE_H

#include <pthread.h>
#include <stddef.h>

#include "chat_common.h"

#define CHAT_USER_STORE_MAGIC      0x43485553u   /* "CHUS" */
#define CHAT_USER_STORE_VERSION    2u            /* 共享内存结构版本号 */
#define CHAT_MAX_OFFLINE_MESSAGES  256
#define CHAT_IMPORTANT_FRIEND_MIN  5             /* 成功发送 > 5 次即为重要朋友 */

/* 单条用户记录。 */
typedef struct {
    char username[CHAT_NAME_LEN];
    char password[CHAT_PASSWORD_LEN];
    char fifo[CHAT_FIFO_PATH_LEN];   /* 机器人无客户端 FIFO，留空 */
    int  online;
    int  is_bot;
    long login_time;                 /* 登录时间，用于日志/展示 */
    long logout_time;                /* 登出时间，用于日志/展示 */
} ChatUserRecord;

/* 一条暂存的离线消息。 */
typedef struct {
    int  used;
    char from[CHAT_NAME_LEN];
    char to[CHAT_NAME_LEN];
    char text[CHAT_TEXT_LEN];
    long timestamp;                  /* 原始发送时间 */
} ChatOfflineMessage;

/* 共享内存对象整体布局：互斥锁在最前，保护其后所有可变字段。 */
typedef struct {
    pthread_mutex_t mutex;
    unsigned int    magic;
    unsigned int    version;
    int             user_count;
    unsigned int    rand_state;      /* 机器人随机名/密码用的 LCG 状态 */
    unsigned long   bot_seq;         /* 机器人序号，保证用户名唯一 */
    ChatUserRecord  users[CHAT_MAX_USERS];
    int             send_count[CHAT_MAX_USERS][CHAT_MAX_USERS];
    ChatOfflineMessage offline_messages[CHAT_MAX_OFFLINE_MESSAGES];
} ChatUserStore;

/* 进程内句柄。 */
typedef struct {
    char           name[64];         /* "/chatroom_lwj_users" */
    ChatUserStore *store;
    size_t         size;
    int            process_shared;   /* 1 = 互斥锁成功设为 PROCESS_SHARED */
} UserStoreHandle;

typedef enum {
    USER_STORE_OK = 0,
    USER_STORE_ERR_EXISTS,
    USER_STORE_ERR_FULL,
    USER_STORE_ERR_NOT_FOUND,
    USER_STORE_ERR_BAD_PASSWORD,
    USER_STORE_ERR_FIFO_MISMATCH
} UserStoreStatus;

/* prepare_send 的一致快照结果。 */
typedef struct {
    int  sender_online;
    int  target_exists;
    int  target_online;
    int  target_is_bot;
    char from_fifo[CHAT_FIFO_PATH_LEN];
    char to_fifo[CHAT_FIFO_PATH_LEN];
    int  send_count;                 /* 当前 send_count[from][to]（自增前） */
} ChatSendInfo;

/* ---- 生命周期 ---- */
int  user_store_init(UserStoreHandle *h, const char *short_name);
void user_store_destroy(UserStoreHandle *h);

/* ---- 注册/登录/登出（均内部加锁）---- */
UserStoreStatus user_store_register(UserStoreHandle *h, const char *username,
                                    const char *password, const char *fifo, int is_bot);
UserStoreStatus user_store_login(UserStoreHandle *h, const char *username,
                                 const char *password, const char *fifo, long now);
UserStoreStatus user_store_logout(UserStoreHandle *h, const char *username,
                                  const char *fifo, long now);

/* ---- 在线名单 ---- */
/* 为 viewer 构造在线名单串 "u1,u2*,u3"（*=对 viewer 而言的重要朋友）。 */
void user_store_build_online_list(UserStoreHandle *h, const char *viewer,
                                  char *buf, size_t bufsz, int *online_count);
/* 为每个“在线且有 FIFO 的真实客户端”构造个性化 ONLINE_LIST 包 + 其 FIFO。
 * 返回填充数量；timestamp 由调用方在锁外补。 */
int  user_store_build_online_broadcast(UserStoreHandle *h, ChatPacket *packets,
                                       char fifos[][CHAT_FIFO_PATH_LEN], int max);
/* 快照所有“在线且有 FIFO 的真实客户端”的 FIFO（可排除一个用户名）。 */
int  user_store_snapshot_receiver_fifos(UserStoreHandle *h,
                                        char fifos[][CHAT_FIFO_PATH_LEN], int max,
                                        const char *exclude);

/* ---- 发送/计数/离线 ---- */
void user_store_prepare_send(UserStoreHandle *h, const char *from, const char *to,
                             ChatSendInfo *out);
/* 成功投递后自增 send_count[from][to]，返回新值；任一方不存在返回 -1。 */
int  user_store_increment_send(UserStoreHandle *h, const char *from, const char *to);
/* 在线投递失败时把目标置为离线：仅当 username 与 fifo 都匹配、且当前仍 online 时生效。
 * 返回 1 表示已置离线，0 表示未匹配。 */
int  user_store_set_offline(UserStoreHandle *h, const char *username, const char *fifo);
/* 暂存一条离线消息（仅对真实离线用户调用）。 */
UserStoreStatus user_store_store_offline(UserStoreHandle *h, const char *from,
                                         const char *to, const char *text, long timestamp);
/* 取出并清空发给 user 的所有离线消息（最多 max 条），返回条数。
 * 在一次加锁内完成"复制 + 清槽"；调用方负责把取出的消息逐条回推给客户端。 */
int  user_store_pop_offline(UserStoreHandle *h, const char *user,
                            ChatOfflineMessage *out, int max);
/* 查 username 的 fifo，找到返回 1 并复制到 buf。 */
int  user_store_lookup_fifo(UserStoreHandle *h, const char *username,
                            char *buf, size_t bufsz);
/* 机器人管理鉴权：仅当 username 存在、在线、非机器人且 fifo 非空时返回 1 并复制 fifo。 */
int  user_store_lookup_online_client_fifo(UserStoreHandle *h, const char *username,
                                          char *buf, size_t bufsz);

/* ---- 机器人 ---- */
/* 创建一个随机用户名/密码的在线机器人（is_bot=1, online=1），名字写入 out_name。 */
UserStoreStatus user_store_add_bot(UserStoreHandle *h, char *out_name, size_t out_sz, long now);
/* 随机选最多 x 个在线机器人置为离线，名字写入 names，返回实际数量。 */
int  user_store_pick_online_bots(UserStoreHandle *h, int x,
                                 char names[][CHAT_NAME_LEN], int max, long now);

#endif
