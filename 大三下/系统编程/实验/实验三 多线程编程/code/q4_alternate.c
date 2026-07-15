/*
 * q4_alternate.c —— 题4 主线程与子线程交替执行
 * 子线程循环 2 次 -> 主线程循环 4 次 -> ... 如此交替 5 轮
 * 编译: gcc -Wall -o q4_alternate q4_alternate.c -lpthread
 *
 * 思路：用两个条件变量 + 一个共享 turn 标志，
 *      turn = 0 时子线程跑，turn = 1 时主线程跑，跑完互相唤醒。
 */
#include <stdio.h>
#include <pthread.h>

#define ROUNDS 5

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;
static int turn = 0;               /* 0: 子线程；1: 主线程 */

static void *child(void *arg) {
    (void)arg;
    for (int r = 0; r < ROUNDS; ++r) {
        pthread_mutex_lock(&lock);
        while (turn != 0)
            pthread_cond_wait(&cond, &lock);

        for (int i = 0; i < 2; ++i)
            printf("  [子线程] 第 %d 轮，第 %d 次循环\n", r + 1, i + 1);

        turn = 1;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main(void) {
    pthread_t tid;
    pthread_create(&tid, NULL, child, NULL);

    for (int r = 0; r < ROUNDS; ++r) {
        pthread_mutex_lock(&lock);
        while (turn != 1)
            pthread_cond_wait(&cond, &lock);

        for (int i = 0; i < 4; ++i)
            printf("[主线程] 第 %d 轮，第 %d 次循环\n", r + 1, i + 1);

        turn = 0;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&lock);
    }

    pthread_join(tid, NULL);
    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
    return 0;
}
