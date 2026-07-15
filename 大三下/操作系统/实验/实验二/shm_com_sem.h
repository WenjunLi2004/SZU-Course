#ifndef SHM_COM_SEM_H
#define SHM_COM_SEM_H

#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

#define LINE_SIZE 256
#define NUM_LINE 16

// 用于创建有名信号量时的识别名字（需在系统中唯一）
const char * queue_mutex = "queue_mutex";
const char * queue_empty = "queue_empty";
const char * queue_full = "queue_full";

// 生产者消费者公用的缓冲区结构体
struct shared_mem_st {
    char buffer[NUM_LINE][LINE_SIZE]; // 16行，每行256个字符
    int line_write;                   // 写指针（生产者用）
    int line_read;                    // 读指针（消费者用）
};

#endif 