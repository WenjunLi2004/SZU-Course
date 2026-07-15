#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
const int MAX_THREADS = 1024;
const int MSG_MAX = 100;
int thread_count = 1;
char **messages;
void *Send_msg(void *rank)
{
    long my_rank = (long)rank;
    long dest = (my_rank + 1) % thread_count;
    long source = (my_rank + thread_count - 1) % thread_count;
    char *my_msg = (char *)malloc(MSG_MAX * sizeof(char));
    sprintf(my_msg, "Hello to %ld from %ld", dest, my_rank);
    messages[dest] = my_msg;
    if (messages[my_rank] != NULL)
        printf("Thread %ld > %s\n", my_rank, messages[my_rank]);
    else
        printf("Thread %ld > No message from %ld\n", my_rank, source);
    return NULL;
}
int main(int argc, char *argv[])
{
    long thread;
    pthread_t *thread_handles;
    if (argc > 1)
        thread_count = atoi(argv[1]);
    thread_handles = (pthread_t *)malloc(thread_count * sizeof(pthread_t));
    messages = (char **)malloc(thread_count * sizeof(char *));
    for (thread = 0; thread < thread_count; thread++)
        messages[thread] = NULL;
    for (thread = 0; thread < thread_count; thread++)
        pthread_create(&thread_handles[thread], (pthread_attr_t *)NULL, Send_msg, (void *)thread);
    for (thread = 0; thread < thread_count; thread++)
    {
        pthread_join(thread_handles[thread], NULL);
    }
    for (thread = 0; thread < thread_count; thread++)
        free(messages[thread]);
    free(messages);
    free(thread_handles);
    return 0;
}
