/*
 * q3_no_sync.c —— 题3 共享 counter，未加同步（演示不可再现性）
 * 两个线程各自把 counter 自增 5000 次，期望终值 10000，
 * 但由于 ++ 不是原子操作，实际结果通常小于 10000 且不稳定。
 */
#include <stdio.h>
#include <pthread.h>

static long counter = 0;

static void *worker(void *arg) {
    const char *name = (const char *)arg;
    for (int i = 0; i < 5000; ++i) {
        counter++;                 /* 读 -> 加 1 -> 写，三步可被打断 */
    }
    printf("[%s] 完成 5000 次自增\n", name);
    return NULL;
}

int main(void) {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, worker, "T1");
    pthread_create(&t2, NULL, worker, "T2");

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("期望 counter = 10000，实际 counter = %ld\n", counter);
    return 0;
}
