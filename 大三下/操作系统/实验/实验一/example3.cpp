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
    pid_t pid;
    pid = fork();
    if (pid == 0) // child process
    {
        sleep(5);
        tprintf("Hello from Child Process.\n");
        tprintf("I'm calling exec.\n");
        // 对齐 argv[0] 为 "/bin/ps"
        int exec_ret = execl("/bin/ps", "/bin/ps", "-a", NULL);
        // 只有 execl 执行失败（如找不到路径）时，才会执行到这里并返回 -1
        if (exec_ret == -1)
        {
            perror("execl failed");
            exit(1);
        }
        // //运行 ls 命令，列出 /etc 目录下的文件
        // execl("/bin/ls","/bin/ls", "-l", "/etc", NULL);
        tprintf("You should never see this because the child is already gone.\n");
        exit(0);
    }
    else if (pid > 0) // parent process
    {
        tprintf("Hello from Parent Process, PID is %d.\n", getpid());
        sleep(1);
        tprintf("Parent forked process %d.\n", pid);
        sleep(1);
        tprintf("Parent is waiting for child to exit.\n");
        waitpid(pid, NULL, 0);
        tprintf("Parent had exited.\n");
    }
    else
    {
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