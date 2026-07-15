#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

/*
 * 题目（Google 面试题改编）：
 *   4 个线程，线程 t (t = 1,2,3,4) 的功能是输出 t 个字符 t（即 "1" "22" "333" "4444"）。
 *   4 个文件 A B C D 初始为空，要求最终内容呈下列“错位轮转”格式：
 *      A: 1 22 333 4444 1 22 333 4444 ...
 *      B: 22 333 4444 1 22 333 4444 1 ...
 *      C: 333 4444 1 22 333 4444 1 22 ...
 *      D: 4444 1 22 333 4444 1 22 333 ...
 *
 * 思路（按轮 + barrier，而非全局令牌一次只放一个线程）：
 *   - 把连续的 4 次写入看作一“轮” round (0,1,2,...)。
 *   - 第 round 轮里，线程 t 写入文件下标 f = (t-1-round) mod 4（0=A,1=B,2=C,3=D）。
 *     对固定 round，t=1..4 时 f 恰好取遍 0..3 的一个排列，
 *     即同一轮里 4 个线程写的是 4 个不同的文件，互不冲突，可以真正并行写。
 *   - 但文件 A 第 round+1 个 token 必须等文件 A 第 round 个 token 写完（顺序约束），
 *     而这两次写来自不同线程，所以需要一个“轮与轮之间”的 barrier：
 *     4 个线程都写完第 round 轮，才能一起进入第 round+1 轮。
 */

#define ROUNDS 8                 /* 轮数：每个文件写 ROUNDS 个 token（此处 2 个完整周期） */
#define NTHREAD 4

const char *fileNames[NTHREAD] = {"A", "B", "C", "D"};
int fds[NTHREAD];                /* A B C D 的文件描述符 */

pthread_mutex_t barrier_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  barrier_cond  = PTHREAD_COND_INITIALIZER;
int arrived = 0;                 /* 本轮已到达 barrier 的线程数 */
int generation = 0;              /* 轮次代数，唤醒后据此区分新旧轮，防止虚假唤醒误判 */

/* 轮与轮之间的同步点：4 个线程都到达后才一起放行进入下一轮 */
void barrier_wait(void)
{
    int my_gen;

    pthread_mutex_lock(&barrier_mutex);
    my_gen = generation;
    if (++arrived == NTHREAD) {
        arrived = 0;
        generation++;
        pthread_cond_broadcast(&barrier_cond);
    } else {
        while (generation == my_gen)
            pthread_cond_wait(&barrier_cond, &barrier_mutex);
    }
    pthread_mutex_unlock(&barrier_mutex);
}

void *worker(void *arg)
{
    int t = *(int *)arg;         /* 本线程的编号 1..4，也是要输出的字符与重复次数 */
    char token[NTHREAD + 2];     /* 形如 "4444 " ，最多 4 个字符 + 1 个空格 + '\0' */
    int round, f, j;

    for (round = 0; round < ROUNDS; round++) {
        /* 计算本轮 token 应写入的文件下标 f = (t-1-round) mod 4 */
        f = ((t - 1 - round) % NTHREAD + NTHREAD) % NTHREAD;

        /* 拼出 token 字符串："t" 重复 t 次，后跟一个空格分隔符 */
        for (j = 0; j < t; j++)
            token[j] = '0' + t;
        token[t] = ' ';
        write(fds[f], token, t + 1); /* 本轮 4 个线程的目标文件互不相同，无需加锁 */

        barrier_wait();           /* 等所有线程写完本轮，再一起进入下一轮 */
    }
    return NULL;
}

int main(void)
{
    pthread_t threads[NTHREAD];
    int ids[NTHREAD];
    int i;

    /* 打开（创建/清空）A B C D 四个文件，保证初始为空 */
    for (i = 0; i < NTHREAD; i++) {
        fds[i] = open(fileNames[i], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fds[i] == -1) { perror("open"); exit(1); }
    }

    /* 创建 4 个线程，编号 1..4 */
    for (i = 0; i < NTHREAD; i++) {
        ids[i] = i + 1;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }

    for (i = 0; i < NTHREAD; i++)
        pthread_join(threads[i], NULL);

    for (i = 0; i < NTHREAD; i++)
        close(fds[i]);

    pthread_mutex_destroy(&barrier_mutex);
    pthread_cond_destroy(&barrier_cond);
    return 0;
}
