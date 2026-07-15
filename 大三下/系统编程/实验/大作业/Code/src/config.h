/*
 * config.h
 *
 * 服务器/客户端共用的配置结构体与加载接口。
 * 所有路径、FIFO 名都从配置文件派生，避免在代码里硬编码。
 */

#ifndef CHAT_CONFIG_H
#define CHAT_CONFIG_H

typedef struct {
    /* 配置文件原始字段 */
    char server_name[64];
    char short_name[16];
    char version[16];
    char fifo_dir[256];          /* 公共 FIFO 目录 ~/Server/fifo */
    char client_fifo_dir[256];   /* 客户端私有 FIFO 目录 ~/Client/fifo */
    char log_dir[256];           /* 日志根目录 ~/log/chat-logs */
    char fifo_prefix[16];        /* FIFO 命名前缀 lwj */
    int  poolsize;               /* 线程池大小，默认 100 */

    /* 派生字段，chat_config_load 自动填充 */
    char full_name[96];          /* "chatserver_lwj_1.4" */
    char server_fifo_dir[256];   /* = fifo_dir，服务器侧别名 */
    char fifo_register[320];     /* <fifo_dir>/lwj_reg_fifo */
    char fifo_login[320];        /* <fifo_dir>/lwj_login_fifo */
    char fifo_message[320];      /* <fifo_dir>/lwj_msg_fifo */
    char fifo_logout[320];       /* <fifo_dir>/lwj_logout_fifo */
    char log_dir_server[320];    /* <log_dir>/server */
    char server_log_path[384];   /* <log_dir>/server/server.log */
    char threads_log_path[384];  /* <log_dir>/server/threads.log */
} ChatConfig;

/* 成功返回 0；失败返回 -1 并向 stderr 打印原因。 */
int chat_config_load(const char *path, ChatConfig *out);

#endif
