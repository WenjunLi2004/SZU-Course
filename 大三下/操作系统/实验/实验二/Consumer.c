#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include "shm_com_sem.h"

int main()
{
    void *shared_memory = (void *)0;
    struct shared_mem_st *shared_stuff;
    int shmid;
    pid_t fork_result;
    sem_t *sem_queue, *sem_queue_empty, *sem_queue_full;

    // 1. 获取共享内存区，并挂入内存 (不使用IPC_CREAT)
    shmid = shmget((key_t)1234, sizeof(struct shared_mem_st), 0666);
    if (shmid == -1)
    {
        fprintf(stderr, "shmget failed. Is Producer running?\n");
        exit(EXIT_FAILURE);
    }
    shared_memory = shmat(shmid, (void *)0, 0);
    if (shared_memory == (void *)-1)
    {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    shared_stuff = (struct shared_mem_st *)shared_memory;

    // 2. 获取 producer 创建的 3 个信号量
    // sem_open(name, oflag):
    // - name: 必须和 Producer 中完全一致，才能打开同一组命名信号量。
    // - oflag: 这里传0，表示“只打开已存在的信号量”，不会重新创建，也不会使用初值。
    // Consumer 只是使用信号量，不负责初始化；初值由 Producer 创建时决定。
    sem_queue = sem_open(queue_mutex, 0);
    sem_queue_empty = sem_open(queue_empty, 0);
    sem_queue_full = sem_open(queue_full, 0);

    // 3. 创建子进程
    fork_result = fork();
    if (fork_result == -1)
    {
        fprintf(stderr, "Fork failure\n");
        exit(EXIT_FAILURE);
    }

    if (fork_result == 0)
    {
        // --- 子进程逻辑 ---
        while (1)
        {
            sem_wait(sem_queue_full); // P(full)
            sem_wait(sem_queue);      // P(mutex)

            if (strcmp(shared_stuff->buffer[shared_stuff->line_read], "quit") == 0)
            {
                sem_post(sem_queue);
                sem_post(sem_queue_full); // 接力唤醒父进程
                break;
            }

            printf("  [Child %d] read: %s\n", getpid(), shared_stuff->buffer[shared_stuff->line_read]);
            shared_stuff->line_read = (shared_stuff->line_read + 1) % NUM_LINE;

            sleep(1); // 时延以便观察交替执行

            sem_post(sem_queue);       // V(mutex)
            sem_post(sem_queue_empty); // V(empty)
        }
    }
    else
    {
        // --- 父进程逻辑 ---
        while (1)
        {
            sem_wait(sem_queue_full); // P(full)
            sem_wait(sem_queue);      // P(mutex)

            if (strcmp(shared_stuff->buffer[shared_stuff->line_read], "quit") == 0)
            {
                sem_post(sem_queue);
                sem_post(sem_queue_full); // 接力唤醒子进程
                break;
            }

            printf("[Parent %d] read: %s\n", getpid(), shared_stuff->buffer[shared_stuff->line_read]);
            shared_stuff->line_read = (shared_stuff->line_read + 1) % NUM_LINE;

            sleep(1); // 时延以便观察交替执行

            sem_post(sem_queue);       // V(mutex)
            sem_post(sem_queue_empty); // V(empty)
        }
    }

    // 4. 释放信号量和共享内存映射
    sem_close(sem_queue);
    sem_close(sem_queue_empty);
    sem_close(sem_queue_full);
    shmdt(shared_memory);

    exit(EXIT_SUCCESS);
}