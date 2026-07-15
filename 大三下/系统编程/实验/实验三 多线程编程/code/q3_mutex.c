/*
 * q3_mutex.c —— 题3 使用 pthread_mutex 互斥锁保护共享变量
 * 编译: gcc -Wall -o q3_mutex q3_mutex.c -lpthread
 */
#include <stdio.h>
#include <pthread.h>

static long counter = 0;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void *worker(void *arg) {
    const char *name = (const char *)arg;
    for (int i = 0; i < 5000; ++i) {
        pthread_mutex_lock(&lock);
        counter++;
        pthread_mutex_unlock(&lock);
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

    pthread_mutex_destroy(&lock);
    printf("期望 counter = 10000，实际 counter = %ld\n", counter);
    return 0;
}
