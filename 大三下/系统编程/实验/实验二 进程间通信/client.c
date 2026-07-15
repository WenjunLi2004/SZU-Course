/* client.c */

#include <errno.h>      /* errno与错误码 */
#include <fcntl.h>      /* open标志 */
#include <signal.h>     /* signal */
#include <stdio.h>      /* printf/perror */
#include <stdlib.h>     /* exit/atoi */
#include <string.h>     /* strlen/strcpy/memset */
#include <sys/stat.h>   /* mkfifo/mkdir */
#include <sys/types.h>  /* 基本类型 */
#include <unistd.h>     /* access/read/write/usleep */
#include "clientinfo.h" /* CLIENTINFO与路径宏 */

static char mypipename[CLIENT_FIFO_NAME_LEN]; /* 客户端FIFO路径缓存 */
static int my_fifo = -1;                      /* 客户端FIFO文件描述符 */

/*
 * 释放资源并退出。
 * status: 返回给操作系统的退出码。
 */
static void cleanup(int status)
{
    if (my_fifo != -1)         /* 描述符有效则关闭 */
        close(my_fifo);        /* 关闭客户端FIFO */
    if (mypipename[0] != '\0') /* FIFO路径已生成 */
        unlink(mypipename);    /* 删除客户端FIFO文件 */
    exit(status);              /* 退出进程 */
}

/*
 * 信号处理函数，用于安全清理。
 * sig: 接收到的信号编号（未使用）。
 */
static void handler(int sig)
{
    (void)sig;             /* 避免未使用参数警告 */
    cleanup(EXIT_FAILURE); /* 异常退出时清理 */
}

/*
 * 若目录不存在则创建。
 * path: 要创建的目录绝对路径。
 * 返回值: 成功0，失败-1。
 */
static int ensure_dir(const char *path)
{
    // mkdir 成功返回 0，失败返回 -1，并设置 errno
    // EEXIST 是错误码，表示要创建的目录已经存在
    // 两个条件同时成立才说明目录不存在且创建失败
    if (mkdir(path, 0777) == -1 && errno != EEXIST) /* 不存在则创建 */
    {
        perror(path); /* 输出失败原因 */
        return -1;    /* 创建失败 */
    }
    return 0; /* 创建成功或已存在 */
}

/*
 * 确保FIFO存放所需的目录都存在。
 * 返回值: 成功0，失败-1。
 */
static int prepare_dirs(void)
{
    if (ensure_dir(CALC_USER_HOME) == -1 ||       /* 用户主目录 */
        ensure_dir(CALC_APP_DIR) == -1 ||         /* 应用目录 */
        ensure_dir(CALC_DATA_DIR) == -1 ||        /* 数据目录 */
        ensure_dir(CALC_SERVER_FIFO_DIR) == -1 || /* 服务器FIFO目录 */
        ensure_dir(CALC_CLIENT_FIFO_DIR) == -1)   /* 客户端FIFO目录 */
        return -1;                                /* 任意失败则返回 */

    return 0; /* 目录准备成功 */
}

/*
 * 解析命令行参数并填充CLIENTINFO请求。
 * argc/argv: 命令行参数；argv[1] 左操作数，argv[2] 运算符，argv[3] 右操作数。
 * info: 输出的请求结构体。
 * 返回值: 成功0，校验失败-1。
 */
static int parse_args(int argc, char *argv[], CLIENTINFOPTR info)
{
    if (argc != 4) /* 参数数量校验 */
    {
        fprintf(stderr, "Usage: %s op1 operation op2\n", argv[0]); /* 用法提示 */
        return -1;                                                 /* 参数数量错误 */
    }

    if (strlen(argv[2]) != 1 || strchr("+-*/", argv[2][0]) == NULL) /* 运算符校验 */
    {
        fprintf(stderr, "Operation must be one of: + - '*' /\n"); /* 运算符提示 */
        return -1;                                                /* 运算符非法 */
    }

    info->leftarg = atoi(argv[1]);  /* 左操作数 */
    info->op = argv[2][0];          /* 运算符 */
    info->rightarg = atoi(argv[3]); /* 右操作数 */
    return 0;                       /* 解析成功 */
}

int main(int argc, char *argv[])
{
    int server_fd;                   /* 服务器FIFO描述符 */
    CLIENTINFO info;               /* 请求数据结构体 */
    char buffer[SERVER_REPLY_LEN]; /* 回复缓冲区 */
    ssize_t nread;                 /* 读取字节数 */

    signal(SIGINT, handler);  /* Ctrl+C */
    signal(SIGTERM, handler); /* 终止信号 */
    signal(SIGHUP, handler);  /* 终端挂起 */

    /* 在使用FIFO前先校验输入并创建所需目录。 */
    if (parse_args(argc, argv, &info) == -1 || prepare_dirs() == -1) /* 解析或目录准备失败 */
        exit(EXIT_FAILURE);                                          /* 解析或目录准备失败 */

    /* 服务器FIFO必须已存在（服务器应已启动）。 */
    if (access(FIFO_NAME, F_OK) == -1) /* 检查服务器FIFO是否存在 */
    {
        fprintf(stderr, "Could not find server FIFO %s. Start calcserver first.\n",
                FIFO_NAME); /* 提示先启动服务器 */
        exit(EXIT_FAILURE); /* 服务器未就绪 */
    }

    /* 根据PID生成唯一的客户端FIFO路径。 */
    snprintf(mypipename, sizeof(mypipename), CLIENT_FIFO_TEMPLATE, getpid()); /* 拼接客户端FIFO路径 */
    if (mkfifo(mypipename, 0600) == -1)                                       /* 创建客户端FIFO */
    {
        perror(mypipename); /* 输出创建失败原因 */
        exit(EXIT_FAILURE); /* 退出 */
    }

    /* 以非阻塞方式打开客户端FIFO用于接收回复。 */
    my_fifo = open(mypipename, O_RDONLY | O_NONBLOCK); /* 非阻塞打开客户端FIFO */
    if (my_fifo == -1)                                 /* 打开失败 */
    {
        perror(mypipename);    /* 打开失败 */
        cleanup(EXIT_FAILURE); /* 清理并退出 */
    }

    /* 将回复FIFO路径写入请求包。 */
    strcpy(info.myfifo, mypipename); /* 填写回复FIFO路径 */

    /* 通过公共FIFO向服务器发送请求。 */
    server_fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK); /* 打开服务器FIFO用于写入 */
    if (server_fd == -1)                                /* 打开失败 */
    {
        perror(FIFO_NAME);     /* 打开失败 */
        cleanup(EXIT_FAILURE); /* 清理并退出 */
    }

    if (write(server_fd, &info, sizeof(CLIENTINFO)) != sizeof(CLIENTINFO)) /* 发送请求 */
    {
        perror("write");       /* 写入失败 */
        close(server_fd);        /* 关闭服务器FIFO */
        cleanup(EXIT_FAILURE); /* 清理并退出 */
    }
    close(server_fd); /* 关闭服务器FIFO */

    /* 轮询客户端FIFO直到收到回复。 */
    memset(buffer, '\0', sizeof(buffer)); /* 清空回复缓冲区 */
    while (1)                             /* 等待回复 */
    {
        nread = read(my_fifo, buffer, sizeof(buffer)); /* 尝试读取回复 */
        if (nread > 0)                                 /* 收到回复 */
        {
            printf("Received from server: %s\n", buffer); /* 打印结果 */
            break;                                        /* 结束等待 */
        }

        if (nread == -1 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) /* 非可重试错误 */
        {
            perror("read");        /* 读取失败 */
            cleanup(EXIT_FAILURE); /* 清理并退出 */
        }

        usleep(10000); /* 10ms轮询间隔 */
    }

    printf("Client %d is terminating\n", getpid()); /* 提示结束 */
    cleanup(EXIT_SUCCESS);                          /* 正常退出 */
}
