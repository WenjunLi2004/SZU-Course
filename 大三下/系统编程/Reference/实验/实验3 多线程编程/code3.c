#include <stdio.h>
#include <pthread.h>

int counter = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *new(void *vptr) {
    while (counter < 30) {
        pthread_mutex_lock(&mutex); // 加锁
        for (int i = 0; i < 2; i++) {
            printf("New %d ", i);
            counter++;
        }
        pthread_cond_signal(&cond);
        if (counter < 30)
            pthread_cond_wait(&cond, &mutex);
        pthread_mutex_unlock(&mutex); // 解锁
    }
    return NULL;
}

int main(int argc, char **argv) {
    pthread_t thread;
    pthread_create(&thread, NULL, &new, NULL);
    while (counter < 30) {
        pthread_mutex_lock(&mutex); // 加锁
        if (counter < 30)
            pthread_cond_wait(&cond, &mutex);
        for (int i = 0; i < 4; i++) {
            printf("Main %d ", i);
            counter++;
        }
        putchar('\n');
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex); // 解锁
    }
    pthread_join(thread, NULL);
    pthread_mutex_destroy(&mutex); // 销毁互斥锁
    return 0;
}