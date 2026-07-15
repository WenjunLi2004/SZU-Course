#include<stdio.h>
#include<pthread.h>
#include<string.h>
#include<stdlib.h>
#include <unistd.h>

typedef struct {
    int value;
    char string[128];
} thread_parm_t;

void *threadfunc(void *parm) {
    sleep(2);
    thread_parm_t *p = (thread_parm_t *) parm;
    printf("%s,parm=%d\n", p->string, p->value);
    free(p);
    return NULL;
}
int main(){
    pthread_t thread1,thread2;
    int rc=0;
    thread_parm_t *parm=NULL;
    printf("Creat a thread attributes object\n");
    parm= malloc(sizeof(thread_parm_t));
    parm->value=5;
    strcpy(parm->string,"Inside first thread");
    rc= pthread_create(&thread1,NULL,threadfunc,(void*)parm);
    parm= malloc(sizeof(thread_parm_t));
    parm->value=77;
    strcpy(parm->string,"Inside the second thread");
    rc= pthread_create(&thread2,NULL,threadfunc,(void*)parm);
    printf("Main completed\n");
    pthread_join(thread1,NULL);
    pthread_join(thread2,NULL);
    return 0;
}