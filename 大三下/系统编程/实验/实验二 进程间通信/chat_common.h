/*
 * chat_common.h
 *
 * 多用户聊天系统的公共协议头文件。
 * 服务器和客户端都包含这个文件，保证双方使用相同的 FIFO 路径、
 * 字段长度和通信结构体格式。如果两端结构体定义不一致，read/write
 * 传递二进制数据时就会出现字段错位或读取长度不匹配的问题。
 */

#ifndef CHAT_COMMON_H
#define CHAT_COMMON_H

/* 实验要求的主目录和数据目录。 */
#define CHAT_USER_HOME "/home/szu/liwenjun2023150001"
#define CHAT_APP_DIR CHAT_USER_HOME "/chatapplication"
#define CHAT_DATA_DIR CHAT_APP_DIR "/data"
#define CHAT_SERVER_FIFO_DIR CHAT_DATA_DIR "/server_fifo"
#define CHAT_CLIENT_FIFO_DIR CHAT_DATA_DIR "/client_fifo"

/*
 * 服务器端的 3 个公共 FIFO。
 * FIFO_1: 接收注册请求。
 * FIFO_2: 接收登录请求。
 * FIFO_3: 接收聊天消息发送请求。
 */
#define CHAT_FIFO_REGISTER CHAT_SERVER_FIFO_DIR "/FIFO_1"
#define CHAT_FIFO_LOGIN CHAT_SERVER_FIFO_DIR "/FIFO_2"
#define CHAT_FIFO_MESSAGE CHAT_SERVER_FIFO_DIR "/FIFO_3"

/* 用户数量和各字段长度限制，避免固定数组越界。 */
#define CHAT_MAX_USERS 64
#define CHAT_NAME_LEN 32
#define CHAT_PASSWORD_LEN 32
#define CHAT_FIFO_PATH_LEN 256
#define CHAT_TEXT_LEN 256

/*
 * 注册和登录请求结构体。
 * username/password 用于身份信息，fifo 用于告诉服务器：
 * “请把本次操作的回复写回这个客户端私有 FIFO”。
 */
typedef struct
{
    char username[CHAT_NAME_LEN];
    char password[CHAT_PASSWORD_LEN];
    char fifo[CHAT_FIFO_PATH_LEN];
} ChatAuthRequest;

/*
 * 客户端发送聊天消息时使用的请求结构体。
 * from 是发送方用户名，to 是接收方用户名，text 是消息正文。
 */
typedef struct
{
    char from[CHAT_NAME_LEN];
    char to[CHAT_NAME_LEN];
    char text[CHAT_TEXT_LEN];
} ChatSendRequest;

/*
 * 服务器写回客户端的数据包类型。
 * REPLY 表示服务器对某个操作的确认或错误提示；
 * MESSAGE 表示另一个用户发来的聊天消息。
 */
typedef enum
{
    CHAT_PACKET_REPLY = 1,
    CHAT_PACKET_MESSAGE = 2
} ChatPacketType;

/*
 * 服务器向客户端发送的统一数据包。
 * type 决定客户端如何解释该包；
 * ok 只对 CHAT_PACKET_REPLY 有意义，1 表示成功，0 表示失败；
 * from 表示消息来源，服务器回复时为 "server"；
 * message 保存提示内容或聊天正文。
 */
typedef struct
{
    int type;
    int ok;
    char from[CHAT_NAME_LEN];
    char message[CHAT_TEXT_LEN];
} ChatPacket;

#endif
