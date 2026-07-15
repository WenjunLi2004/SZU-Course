#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
/* SIGALRM 处理函数：定时器到期时执行 */
void sigalrm_handler(int signum)
{
    printf("Alarming...\n");
}

/* SIGINT 处理函数：空操作，避免 Ctrl+C 默认终止进程 */
void sigint_handler(int signum)
{
}
void bad_restorer(void)
{
    __asm__ volatile("udf #0");
}
int sf_sleep_with_suspend(int nsec)
{
    sigset_t mask;             /* sigsuspend 等待期间使用的信号掩码 */
    sigset_t oldmask, newmask; /* oldmask: 调用前的信号掩码；newmask: 待阻塞的信号集 */

    /* 注册 SIGINT 处理函数（忽略），防止 Ctrl+C 杀死进程 */
    signal(SIGINT, sigint_handler);
    printf("Enter sf_sleep_with_sigsuspend()...\n");

    struct sigaction newact, oldact;
    unsigned int remaining = 0;

    /* 为 SIGALRM 注册新的处理函数，oldact 保存原来的设置以便恢复 */
    newact.sa_handler = sigalrm_handler;
    newact.sa_flags = 0;

    sigaction(SIGALRM, &newact, &oldact);

    /* 构造只包含 SIGALRM 的信号集 */
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGALRM);

    /* 先阻塞 SIGALRM，避免 alarm() 到期后信号在 sigsuspend 调用前被提前处理（竟态条件） */
    sigprocmask(SIG_BLOCK, &newmask, &oldmask);

    /* 设置定时器，nsec 秒后产生 SIGALRM（此时仍被阻塞，处于挂起状态） */
    alarm(nsec);

    /* 构造 sigsuspend 用的掩码：只阻塞 SIGINT，不阻塞 SIGALRM */
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);

    /* 原子地将信号掩码替换为 mask 并挂起进程，
     * 直到收到一个未被阻塞的信号（即 SIGALRM）才返回；
     * 返回时信号掩码恢复为调用前的 oldmask（仍阻塞 SIGALRM） */
    sigsuspend(&mask);

    /* 显式恢复为函数最初的信号掩码 */
    sigprocmask(SIG_SETMASK, &oldmask, NULL);

    /* 取消定时器并取得剩余秒数（正常情况下应为 0） */
    remaining = alarm(0);

    /* 恢复 SIGALRM 原来的处理方式 */
    sigaction(SIGALRM, &oldact, NULL);

    printf("Exit sf_sleep_with_sigsuspend()...\n");
    return remaining;
}

int main()
{
    printf("Start to sleep...\n");
    int remaining = sf_sleep_with_suspend(5); /* 期望睡 5 秒 */
    printf("Wake up with %d seconds remained...\n", remaining);
}
