#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/shm.h>
#include "shm_com_sem.h"

int main(void)
{
    void *shared_memory = (void *)0;
    struct shared_mem_st *shared_stuff;
    char key_line[LINE_SIZE];
    int shmid;
    sem_t *sem_queue, *sem_queue_empty, *sem_queue_full;

    // 1. 获取（或创建）共享内存区，并挂入内存
    // shmget(key, size, shmflg):
    // - key: 共享内存键值。这里用固定值1234，Producer和Consumer必须一致，才能访问同一块共享内存。
    // - size: 共享内存大小。这里是struct shared_mem_st的大小，足够存放缓冲区和读写指针。
    // - shmflg: 权限和创建标志。0666表示所有用户可读写；IPC_CREAT表示不存在时自动创建；
    //   这里的 `|` 只是把“权限”和“创建标志”组合到一起，不表示“自动创建”本身。
    shmid = shmget((key_t)1234, sizeof(struct shared_mem_st), 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }

    // shmat(shmid, addr, flag):
    // - shmid: 由shmget返回的共享内存ID。
    // - addr: 传0表示由内核自动选择映射地址。
    // - flag: 传0表示默认可读写挂接（不是SHM_RDONLY只读挂接）。
    shared_memory = shmat(shmid, (void *)0, 0);
    if (shared_memory == (void *)-1)
    {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    printf("Memory attached at %p\n", shared_memory);
    shared_stuff = (struct shared_mem_st *)shared_memory;

    // 2. 创建三个信号量
    // sem_queue: 互斥锁，保护共享缓冲区和读写指针（初值1）。
    // sem_queue_empty: 空槽计数，Producer写入前要wait它（初值NUM_LINE）。
    // sem_queue_full: 满槽计数，Consumer读取前要wait它（初值0）。

    // sem_open(name, oflag, mode, value):
    // - name: 信号量名字，Producer和Consumer必须用同名对象才能操作同一组信号量。
    // - oflag: O_CREAT表示不存在就创建。
    // - mode: 0666表示读写权限
    // - value: 只在创建时生效，作为初值。
    // 这里三个信号量的初值分别是：mutex=1, empty=NUM_LINE, full=0。
    sem_queue = sem_open(queue_mutex, O_CREAT, 0666, 1);
    sem_queue_empty = sem_open(queue_empty, O_CREAT, 0666, NUM_LINE);
    sem_queue_full = sem_open(queue_full, O_CREAT, 0666, 0);

    // 初始化读写指针
    shared_stuff->line_write = 0;
    shared_stuff->line_read = 0;

    // 生产者和消费者用的是同一组信号量，但wait/post方向相反：
    // Producer: wait(empty)->wait(mutex)->写->post(mutex)->post(full)
    // Consumer: wait(full)->wait(mutex)->读->post(mutex)->post(empty)
    // 3. 核心循环：不断从控制台读入并写入缓冲区
    while (1)
    {
        printf("Enter your text ('quit' for exit): ");
        fgets(key_line, LINE_SIZE, stdin);     // 使用fgets替代gets更安全
        key_line[strcspn(key_line, "\n")] = 0; // 移除换行符

        // 如果输入 quit 则跳出循环准备退出
        if (strcmp(key_line, "quit") == 0)
        {
            // 注意：我们仍然要把 quit 写进缓冲区，让消费者知道
            sem_wait(sem_queue_empty);
            sem_wait(sem_queue);
            strcpy(shared_stuff->buffer[shared_stuff->line_write], key_line);
            shared_stuff->line_write = (shared_stuff->line_write + 1) % NUM_LINE;
            sem_post(sem_queue);
            sem_post(sem_queue_full);
            break;
        }

        // 将输入的行写入缓冲区，包含信号量操作
        sem_wait(sem_queue_empty); // P(empty)
        sem_wait(sem_queue);       // P(mutex)

        strcpy(shared_stuff->buffer[shared_stuff->line_write], key_line);
        shared_stuff->line_write = (shared_stuff->line_write + 1) % NUM_LINE;

        sem_post(sem_queue);      // V(mutex)：离开临界区，允许其他进程再进入
        sem_post(sem_queue_full); // V(full)：通知“已有一条新数据可读”
    }

    // 4. 释放信号量，结束映射，删除共享内存
    sem_close(sem_queue);
    sem_close(sem_queue_empty);
    sem_close(sem_queue_full);

    sem_unlink(queue_mutex);
    sem_unlink(queue_empty);
    sem_unlink(queue_full);

    shmdt(shared_memory);
    shmctl(shmid, IPC_RMID, 0);

    printf("Producer exit.\n");
    exit(EXIT_SUCCESS);
}