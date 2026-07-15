#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <d>\n", argv[0]);
        return 1;
    }

    int d = atoi(argv[1]);
    int numb = 1;
    int i;

    /* i从1开始，代表当前处于树的第1层，循环控制一直执行到生成第d-1层的子进程 */
    for (i = 1; i < d; i++)
    {
        /* 只有非叶子节点会进入循环体并执行打印和分裂 */
        printf("I am process no %5d  with PID %5d and PPID %5d\n", numb, getpid(), getppid());

        pid_t pid1 = fork();
        if (pid1 == 0)
        {
            /* 左子进程分支：更新自身编号并继续外层循环深入下一层 */
            numb = 2 * numb;
            continue;
        }
        else if (pid1 > 0)
        {
            /* 父进程保留执行权，继续创建右子进程 */
            pid_t pid2 = fork();
            if (pid2 == 0)
            {
                /* 右子进程分支：更新自身编号并继续外层循环深入下一层 */
                numb = 2 * numb + 1;
                continue;
            }
            else if (pid2 > 0)
            {
                /* 父进程分支：阻塞等待它的左右两个孩子完全退出，然后自己再退出 */
                wait(NULL);
                wait(NULL);
                exit(0);
            }
            else
            {
                perror("fork() for pid2 failed");
                exit(1);
            }
        }
        else
        {
            perror("fork() for pid1 failed");
            exit(1);
        }
    }

    /* 叶子节点以及d=1时的根节点将越过循环，在这里统一执行打印并安全退出 */
    printf("I am process no %5d  with PID %5d and PPID %5d\n", numb, getpid(), getppid());
    exit(0);
}