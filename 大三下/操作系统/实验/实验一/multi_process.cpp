#include <unistd.h>
#include <stdarg.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>

int tprintf(const char *fmt, ...);

int main(void)
{
    int i;
    // 计划创建的子进程数量。
    const int N = 5;
    pid_t pid;
    printf("Hello from Parent Process, PID is %d.\n", getpid());

    for (i = 0; i < N; i++)
    {
        // 每次循环调用 fork，父进程获得子进程 PID，子进程获得 0。
        pid = fork();
        if (pid < 0)
        {
            perror("fork failed");
            exit(1);
        }
        else if (pid == 0) // child process
        {
            // 略微错开输出，便于观察多进程打印顺序。
            usleep(10000);
            printf("Hello from Child Process, PID is %d. Parent PID is %d\n", getpid(), getppid());
            // 子进程完成一次打印后立即退出，避免继续参与后续 fork。
            exit(0);
        }
    }
    // 只有父进程会执行到这里并统计创建结果。
    tprintf("Parent forked %d child processes.\n", N);
    // 回收 N 个子进程，避免产生僵尸进程。
    for (i = 0; i < N; i++)
        waitpid(-1, NULL, 0);
    tprintf("All Child Processes have exited.\n");
    tprintf("Parent had exited.\n");
    return 0;
}

int tprintf(const char *fmt, ...)
{
    va_list args;
    struct tm *tstruct;
    time_t tsec;
    tsec = time(NULL);
    tstruct = localtime(&tsec);
    // 输出统一的时间戳和 PID，便于观察父子进程执行顺序。
    printf("%02d:%02d:%02d: %5d|", tstruct->tm_hour, tstruct->tm_min, tstruct->tm_sec, getpid());
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);
    return ret;
}