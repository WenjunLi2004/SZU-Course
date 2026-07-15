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
    pid_t pid1, pid2;
    printf("Hello from Parent Process, PID is %d.\n", getpid());

    pid1 = fork();
    if (pid1 < 0)
    {
        perror("first fork failed");
        exit(1);
    }
    else if (pid1 == 0)
    {
        tprintf("Hello from First Child Process, PID is %d.\n", getpid());
        exit(0);
    }

    pid2 = fork();
    if (pid2 < 0)
    {
        perror("second fork failed");
        exit(1);
    }
    else if (pid2 == 0)
    {
        tprintf("Hello from Second Child Process, PID is %d.\n", getpid());
        exit(0);
    }

    tprintf("Parent forked two child processes--%d and %d.\n", pid1, pid2);

    // 父进程等待两个子进程退出，避免父进程先结束。
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    tprintf("Both Child Processes have exited.\n");
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