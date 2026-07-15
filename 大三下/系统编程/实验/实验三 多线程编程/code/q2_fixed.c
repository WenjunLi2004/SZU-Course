/*
 * q2_fixed.c —— 题2 修改后的图1程序
 * 编译: gcc -Wall -o q2_fixed q2_fixed.c -lpthread
 *
 * 针对原程序的问题作了如下修改：
 *   1) increase() 中 "val = counter; ...; counter = val + 1;" 是
 *      读-改-写三步非原子操作，两个线程并发执行存在数据竞争。
 *      解决：引入 pthread_mutex_t lock 把临界区整体锁起来。
 *   2) 原 main 没有打印最终 counter，无法看到竞争效果。
 *      解决：join 完成后由主线程统一打印。
 *   3) 把 printf 移到锁的临界区内部，让屏幕输出顺序与 counter 更新顺序一致，
 *      读者更容易从日志看出每次自增都是正确的。
 *   4) 减少 NLOOP 内部 printf 的频率（可选）：本文件保留原有的逐次打印，
 *      便于课堂演示，工程中应根据需要去掉以避免 I/O 主导耗时。
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NLOOP 5000

int counter = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *increase(void *vptr);

int main(int argc, char **argv)
{
    pthread_t threadIdA, threadIdB;

    pthread_create(&threadIdA, NULL, &increase, NULL);
    pthread_create(&threadIdB, NULL, &increase, NULL);

    pthread_join(threadIdA, NULL);
    pthread_join(threadIdB, NULL);

    pthread_mutex_destroy(&lock);
    printf("期望 counter = %d，实际 counter = %d\n", 2 * NLOOP, counter);
    return 0;
}

void *increase(void *vptr)
{
    int i, val;

    for (i = 0; i < NLOOP; i++) {
        pthread_mutex_lock(&lock);              /* 进入临界区 */
        val = counter;
        printf("%lx: %d\n", (unsigned long)pthread_self(), val + 1);
        counter = val + 1;
        pthread_mutex_unlock(&lock);            /* 离开临界区 */
    }

    return NULL;
}
