#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>

int main()
{
    pid_t pid;
    pid = fork();

    if (pid < 0)
        printf("error occurred!\n");
    else if (pid == 0) {
        // 子进程
        printf("Hi father! I'm a ZOMBIE\n");
        exit(0);     // 没有人等待这个进程
    }
    else {
        // 父进程
        sleep(10);   // 休眠10秒
        wait(NULL);   // 僵尸进程现在将被回收
    }
}
