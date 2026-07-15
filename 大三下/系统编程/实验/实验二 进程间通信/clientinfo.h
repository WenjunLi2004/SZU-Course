/* clientinfo.h */

#ifndef CLIENTINFO_H /* 头文件保护开始 */
#define CLIENTINFO_H /* 头文件保护宏 */

/* 应用数据与FIFO端点的基础路径。 */
#define CALC_USER_HOME "/home/szu/liwenjun2023150001"     /* 用户主目录 */
#define CALC_APP_DIR CALC_USER_HOME "/chatapplication"    /* 应用根目录 */
#define CALC_DATA_DIR CALC_APP_DIR "/data"                /* 数据目录 */
#define CALC_SERVER_FIFO_DIR CALC_DATA_DIR "/server_fifo" /* 服务器FIFO目录 */
#define CALC_CLIENT_FIFO_DIR CALC_DATA_DIR "/client_fifo" /* 客户端FIFO目录 */

/* FIFO名称：服务器公共FIFO与每个客户端FIFO模板。 */
#define FIFO_NAME CALC_SERVER_FIFO_DIR "/chat_server_fifo"              /* 服务器公共FIFO路径 */
#define CLIENT_FIFO_TEMPLATE CALC_CLIENT_FIFO_DIR "/chat_client%d_fifo" /* 客户端FIFO模板 */

/* FIFO路径和服务器回复的缓冲区大小。 */
#define CLIENT_FIFO_NAME_LEN 256 /* 客户端FIFO路径缓冲区长度 */
#define SERVER_REPLY_LEN 128     /* 服务器回复缓冲区长度 */

/*
 * 通过服务器FIFO发送的客户端请求包。
 * myfifo: 客户端回复FIFO的绝对路径。
 * leftarg/rightarg: 计算的左右操作数（整数）。
 * op: 运算符字符（'+', '-', '*', '/'）。
 */
typedef struct
{
    char myfifo[CLIENT_FIFO_NAME_LEN]; /* 客户端回复FIFO路径 */
    int leftarg;                       /* 左操作数 */
    int rightarg;                      /* 右操作数 */
    char op;                           /* 运算符 */
} CLIENTINFO, *CLIENTINFOPTR;

#endif /* CLIENTINFO_H */
