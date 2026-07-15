/*
 * q3_sem.c —— 题3 使用 POSIX 信号量 (sem_wait/sem_post) 实现互斥
 * 编译: gcc -Wall -o q3_sem q3_sem.c -lpthread
 *
 * 信号量初值设为 1，等价于二元信号量，可起到互斥锁的作用。
 */
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

static long counter = 0;
static sem_t sem;

static void *worker(void *arg) {
    const char *name = (const char *)arg;
    for (int i = 0; i < 5000; ++i) {
        sem_wait(&sem);            /* P 操作：信号量 -1，若 <0 则阻塞 */
        counter++;
        sem_post(&sem);            /* V 操作：信号量 +1 */
    }
    printf("[%s] 完成 5000 次自增\n", name);
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    /* 第二个参数 0 表示线程间共享；初始值 1 表示资源可用 */
    sem_init(&sem, 0, 1);

    pthread_create(&t1, NULL, worker, "T1");
    pthread_create(&t2, NULL, worker, "T2");

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    sem_destroy(&sem);
    printf("期望 counter = 10000，实际 counter = %ld\n", counter);
    return 0;
}
