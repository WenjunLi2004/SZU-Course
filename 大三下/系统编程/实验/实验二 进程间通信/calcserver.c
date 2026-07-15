/* calcserver.c */

#include <errno.h>      /* errno与错误码 */
#include <fcntl.h>      /* open标志 */
#include <signal.h>     /* signal */
#include <stdio.h>      /* printf/perror */
#include <stdlib.h>     /* exit */
#include <string.h>     /* strlen/snprintf */
#include <sys/stat.h>   /* mkfifo/mkdir */
#include <sys/types.h>  /* 基本类型 */
#include <unistd.h>     /* access/read/write */
#include "clientinfo.h" /* CLIENTINFO与路径宏 */

static int fifo_fd = -1; /* 服务器FIFO描述符 */

/*
 * 若目录不存在则创建。
 * path: 要创建的目录绝对路径。
 * 返回值: 成功0，失败-1。
 */
static int ensure_dir(const char *path)
{
    if (mkdir(path, 0777) == -1 && errno != EEXIST) /* 不存在则创建 */
    {
        perror(path); /* 输出失败原因 */
        return -1;    /* 创建失败 */
    }
    return 0; /* 创建成功或已存在 */
}

/*
 * 关闭服务器FIFO并退出。
 * status: 返回给操作系统的退出码。
 */
static void cleanup(int status)
{
    if (fifo_fd != -1)  /* 描述符有效则关闭 */
        close(fifo_fd); /* 关闭服务器FIFO */
    unlink(FIFO_NAME);  /* 删除公共FIFO */
    exit(status);       /* 退出进程 */
}

/*
 * 处理终止信号并进行清理。
 * sig: 接收到的信号编号（未使用）。
 */
static void handler(int sig)
{
    (void)sig;             /* 避免未使用参数警告 */
    cleanup(EXIT_FAILURE); /* 异常退出时清理 */
}

/*
 * 确保目录存在，若服务器FIFO不存在则创建。
 * 返回值: 成功0，失败-1。
 */
static int prepare_fifo(void)
{
    if (ensure_dir(CALC_USER_HOME) == -1 ||       /* 用户主目录 */
        ensure_dir(CALC_APP_DIR) == -1 ||         /* 应用目录 */
        ensure_dir(CALC_DATA_DIR) == -1 ||        /* 数据目录 */
        ensure_dir(CALC_SERVER_FIFO_DIR) == -1 || /* 服务器FIFO目录 */
        ensure_dir(CALC_CLIENT_FIFO_DIR) == -1)   /* 客户端FIFO目录 */
        return -1;                                /* 任意失败则返回 */

    if (access(FIFO_NAME, F_OK) == -1) /* 不存在则创建公共FIFO */
    {
        if (mkfifo(FIFO_NAME, 0666) == -1) /* 创建公共FIFO */
        {
            perror(FIFO_NAME); /* 输出创建失败原因 */
            return -1;         /* 创建失败 */
        }
    }

    return 0; /* FIFO准备完成 */
}

/*
 * 计算客户端请求结果并格式化为字符串。
 * info: 包含操作数和运算符的请求。
 * buffer: 输出的文本缓冲区。
 * size: 输出缓冲区大小（字节）。
 */
static void format_result(const CLIENTINFOPTR info, char *buffer, size_t size)
{
    int result; /* 计算结果 */

    switch (info->op) /* 根据运算符计算 */
    {
    case '+':
        result = info->leftarg + info->rightarg; /* 加法 */
        break;                                   /* 结束分支 */
    case '-':
        result = info->leftarg - info->rightarg; /* 减法 */
        break;                                   /* 结束分支 */
    case '*':
        result = info->leftarg * info->rightarg; /* 乘法 */
        break;                                   /* 结束分支 */
    case '/':
        if (info->rightarg == 0) /* 除零保护 */
        {
            snprintf(buffer, size, "Error: division by zero"); /* 输出错误 */
            return;                                            /* 提前返回 */
        }
        result = info->leftarg / info->rightarg; /* 除法 */
        break;                                   /* 结束分支 */
    default:
        snprintf(buffer, size, "Error: unsupported operator '%c'", info->op); /* 不支持运算符 */
        return;                                                               /* 提前返回 */
    }

    snprintf(buffer, size, "The result is %d", result); /* 正常结果 */
}

int main()
{
    int fd1;                       /* 客户端FIFO描述符 */
    CLIENTINFO info;               /* 读取到的请求 */
    char buffer[SERVER_REPLY_LEN]; /* 回复缓冲区 */
    ssize_t nread;                 /* 读取字节数 */

    signal(SIGINT, handler);  /* Ctrl+C */
    signal(SIGTERM, handler); /* 终止信号 */
    signal(SIGHUP, handler);  /* 终端挂起 */

    if (prepare_fifo() == -1) /* 创建FIFO目录及公共服务器FIFO */
        exit(EXIT_FAILURE);   /* 准备失败退出 */

    fifo_fd = open(FIFO_NAME, O_RDWR); /* 读写打开公共FIFO，避免读到EOF */
    if (fifo_fd == -1)                 /* 打开失败 */
    {
        perror(FIFO_NAME);  /* 打开失败 */
        exit(EXIT_FAILURE); /* 退出 */
    }

    printf("Server is ready on %s\n", FIFO_NAME); /* 就绪提示 */

    while (1) /* 持续服务 */
    {
        nread = read(fifo_fd, &info, sizeof(CLIENTINFO)); /* 读取请求包 */
        if (nread == -1)                                  /* 读取失败 */
        {
            if (errno == EINTR) /* 被信号打断 */
                continue;       /* 被信号打断则重试 */
            perror("read");     /* 读取失败 */
            break;              /* 跳出主循环 */
        }

        if (nread == 0) /* 暂无数据 */
            continue;   /* 无数据则继续等待 */

        if (nread != sizeof(CLIENTINFO)) /* 请求不完整 */
        {
            fprintf(stderr, "Ignoring incomplete request: %zd bytes\n", nread); /* 报告异常长度 */
            continue;                                                           /* 忽略该请求 */
        }

        printf("Client arrived: %s, expression: %d %c %d\n",
               info.myfifo, info.leftarg, info.op, info.rightarg); /* 打印请求内容 */

        format_result(&info, buffer, sizeof(buffer));   /* 计算并格式化结果 */
        fd1 = open(info.myfifo, O_WRONLY | O_NONBLOCK); /* 打开客户端FIFO */
        if (fd1 == -1)                                  /* 打开失败 */
        {
            perror(info.myfifo); /* 打开失败 */
            continue;            /* 跳过本次回复 */
        }

        if (write(fd1, buffer, strlen(buffer) + 1) == -1) /* 写入回复 */
            perror("write");                              /* 写入失败 */

        close(fd1); /* 关闭客户端FIFO */
    }

    cleanup(EXIT_FAILURE); /* 异常结束清理 */
}
