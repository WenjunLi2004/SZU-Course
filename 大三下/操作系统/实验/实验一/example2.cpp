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
    int i = 0;
    pid_t pid;
    printf("Hello from Parent Process, PID is %d.\n", getpid());

    pid = fork();
    if (pid == 0) // child process
    {
        sleep(1);
        for (i = 0; i < 3; i++)
        {
            printf("Hello from Child Process, PID is %d. %d times\n", getpid(), i + 1);
            sleep(1);
        }
        exit(0);
    }
    else if (pid > 0) // parent process
    {
        tprintf("Parent forked one child process--%d.\n", pid);
        tprintf("Parent is waiting for child to exit.\n");
        // 阻塞等待指定子进程退出，避免父进程先结束。
        waitpid(pid, NULL, 0);
        tprintf("Child Process has exited.\n");
        tprintf("Parent had exited.\n");
    }
    else
    {
        // 这里是例程错误之处
        tprintf("Fork failed.\n");
    }
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