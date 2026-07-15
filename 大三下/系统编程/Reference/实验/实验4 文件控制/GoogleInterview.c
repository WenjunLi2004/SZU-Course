#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>

int counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
char*files[]={"A", "B", "C", "D"};
int orders[4]={0,3,2,1};
int fds[4];
void *new(void *vptr) {
    int*p=(int*)vptr;
    int times=*p;
    for (int i = 0; i < 32; i++) {
        pthread_mutex_lock(&mutex); // ¼ÓËø
        while (counter%4 != times-1)
            pthread_cond_wait(&cond, &mutex);
        counter++;
        char buffer[16];
        for(int j=0;j<times;j++)
            buffer[j]=times+'0';
        buffer[times]=' ';
        write(fds[orders[(4-times+1+i)%4]],buffer,times+1);
        if(i==31)
            write(fds[orders[(4-times+1+i)%4]],"\n",1);
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex); // ½âËø
    }
    return NULL;
}

int main(int argc, char **argv) {
    pthread_t threads[4];
    int times[4];
    for (int i = 0; i < 4; i++){
        times[i]=i+1;
        fds[i]=open(files[i],O_WRONLY|O_TRUNC);
        pthread_create(&threads[i], NULL, &new, &times[i]);
    }
    for (int i = 0; i < 4; i++)
        pthread_join(threads[i], NULL);
    pthread_mutex_destroy(&mutex); // Ïú»Ù»¥³âËø
    return 0;
}