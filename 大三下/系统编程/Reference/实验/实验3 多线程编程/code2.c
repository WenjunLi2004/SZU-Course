#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#define NLOOP 5000
int counter;

void *increase(void *vptr);

sem_t sem; // 信号量
int main(int argc, char **argv) {
    pthread_t threadIdA, threadIdB;
    sem_init(&sem, 0, 1); // 初始化信号量，初值为1
    pthread_create(&threadIdA, NULL, &increase, NULL);
    pthread_create(&threadIdB, NULL, &increase, NULL);
    pthread_join(threadIdA, NULL);
    pthread_join(threadIdB, NULL);
    sem_destroy(&sem); // 销毁信号量
    return 0;
}

void *increase(void *vptr) {
    int i, val;
    for (i = 0; i < NLOOP; i++) {
        sem_wait(&sem); // 等待信号量
        val = counter;
        printf("%x: %d\n", (unsigned int) pthread_self(), val + 1);
        counter = val + 1;
        sem_post(&sem); // 发送信号量
    }
    return NULL;
}