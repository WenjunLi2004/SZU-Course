/*
 * q2_original.c —— 题2 图1原始程序（仅在 main 末尾加了打印 counter，
 * 以便观察竞争导致的错误结果，函数体本身与图1完全一致）。
 * 编译: gcc -Wall -o q2_original q2_original.c -lpthread
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NLOOP 5000

int counter;                        /* incremented by threads */

void *increase(void *vptr);

int main(int argc, char **argv)
{
    pthread_t threadIdA, threadIdB;

    pthread_create(&threadIdA, NULL, &increase, NULL);
    pthread_create(&threadIdB, NULL, &increase, NULL);

    /* wait for both threads to terminate */
    pthread_join(threadIdA, NULL);
    pthread_join(threadIdB, NULL);

    printf("期望 counter = %d，实际 counter = %d\n", 2 * NLOOP, counter);
    return 0;
}

void *increase(void *vptr)
{
    int i, val;

    for (i = 0; i < NLOOP; i++) {
        val = counter;
        printf("%x: %d\n", (unsigned int)pthread_self(), val + 1);
        counter = val + 1;
    }

    return NULL;
}
