/*
 * chat_common.h
 *
 * 多用户聊天系统的公共线协议头文件
 * 服务器和客户端都包含这个文件，保证双方字段长度与结构体格式一致
 * client <-> server 之间的“线协议”
 */

#ifndef CHAT_COMMON_H
#define CHAT_COMMON_H

/* 用户数量和各字段长度限制，避免固定数组越界。 */
#define CHAT_MAX_USERS 64
#define CHAT_NAME_LEN 32
#define CHAT_PASSWORD_LEN 32
#define CHAT_FIFO_PATH_LEN 256
/* 正文/在线名单串长度。在线名单可能较长（多个用户名），故给到 512；
 * sizeof(ChatPacket) 仍远小于 PIPE_BUF(4096)，保证单包 write 在 FIFO 上原子。 */
#define CHAT_TEXT_LEN 512

/* 机器人管理请求的保留目标名：客户端把 /bot add|del 当作发给 __botmgr__ 的消息。
 * 复用 MSG_FIFO，不另设公共 FIFO（公共 FIFO 一共只有 4 个）。 */
#define CHAT_BOTMGR_TARGET "__botmgr__"

/*
 * 注册和登录请求结构体。
 * 注册写入 REG_FIFO，登录写入 LOGIN_FIFO，结构体复用——
 * op 信息隐式地由“写入的 FIFO 是哪一个”表达。
 */
typedef struct
{
    char username[CHAT_NAME_LEN];
    char password[CHAT_PASSWORD_LEN];
    char fifo[CHAT_FIFO_PATH_LEN];
} ChatAuthRequest;

/* 客户端发送聊天消息时使用的请求结构体；机器人管理也复用它（to=__botmgr__）。 */
typedef struct
{
    char from[CHAT_NAME_LEN];
    char to[CHAT_NAME_LEN];
    char text[CHAT_TEXT_LEN];
} ChatSendRequest;

/*
 * 登出请求。仅需身份标识 + 客户端 FIFO，用 FIFO 做弱身份校验：
 * 只有当请求中的 fifo 与用户表中记录的一致时，服务器才接受登出。
 */
typedef struct
{
    char username[CHAT_NAME_LEN];
    char fifo[CHAT_FIFO_PATH_LEN];
} ChatLogoutRequest;

typedef enum
{
    CHAT_PACKET_REPLY        = 1, /* 对某请求的应答 */
    CHAT_PACKET_MESSAGE      = 2, /* 别人发来的实时消息（含机器人回复） */
    CHAT_PACKET_ONLINE_LIST  = 3, /* 在线人数 + 名单广播 */
    CHAT_PACKET_OFFLINE_PUSH = 4, /* 重新登录后回推的离线消息（带原始时间） */
    CHAT_PACKET_SYSTEM       = 5  /* 系统事件：登入/登出/机器人增减广播 */
} ChatPacketType;

/*
 * 服务器向客户端发送的统一数据包。
 *   - REPLY:        from="server", message=文案, online_count=登录回执时的在线数
 *   - MESSAGE:      from=发送方（>5 次往来时尾部带 '*'）, timestamp=收到时间, send_count=累计成功次数
 *   - ONLINE_LIST:  from="server", message="u1,u2*,u3", online_count=在线总数
 *   - OFFLINE_PUSH: from=原始发送方, message=原始正文, timestamp=原始发送时间
 *   - SYSTEM:       from="server", message=事件文案
 */
typedef struct
{
    int  type;                    /* ChatPacketType */
    int  ok;                      /* REPLY: 1=成功 0=失败；其他类型固定 1 */
    char from[CHAT_NAME_LEN];     /* 来源；服务器回包用 "server" */
    char message[CHAT_TEXT_LEN];  /* 文本载荷 / 在线名单 */
    long timestamp;               /* 服务器侧 time(NULL)；OFFLINE_PUSH 为原始发送时间 */
    int  send_count;              /* from->to 的累计成功次数（含本次） */
    int  online_count;            /* 在线总数（ONLINE_LIST / 登录回执） */
} ChatPacket;

#endif
