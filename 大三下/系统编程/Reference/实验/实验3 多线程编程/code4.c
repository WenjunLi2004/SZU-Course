#include <stdio.h>
#include <pthread.h>

int counter = 0;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void *new(void *vptr) {
    int*p=(int*)vptr;
    int times=*p;
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&mutex); // 加锁
        while (counter%4 != times-1)
            pthread_cond_wait(&cond, &mutex);
        printf("%x", (unsigned int)pthread_self());
        counter++;
        for(int j=0;j<times;j++)
            putchar('*');
        if(times==4)
            putchar('\n');
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex); // 解锁
    }
    return NULL;
}

int main(int argc, char **argv) {
    pthread_t threads[4];
    int times[4];
    for (int i = 0; i < 4; i++){
        times[i]=i+1;
        pthread_create(&threads[i], NULL, &new, &times[i]);
    }
    for (int i = 0; i < 4; i++)
        pthread_join(threads[i], NULL);
    pthread_mutex_destroy(&mutex); // 销毁互斥锁
    return 0;
}