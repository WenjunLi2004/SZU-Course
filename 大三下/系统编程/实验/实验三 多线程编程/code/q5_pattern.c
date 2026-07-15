/*
 * q5_pattern.c —— 题5 四个线程按 p1*p2**p3***p4**** 模式循环打印 5 轮
 * 编译: gcc -Wall -o q5_pattern q5_pattern.c -lpthread
 *
 * 思路：用一个共享的 turn 表示当前轮到第几号线程；
 *      4 个线程通过条件变量串行接力，每个线程打印 "p<id>" + (id 个 '*')。
 *      整个过程在锁保护下进行，保证输出顺序严格固定。
 */
#include <stdio.h>
#include <pthread.h>

#define ROUNDS 5
#define NTHREAD 4

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  cond = PTHREAD_COND_INITIALIZER;
static int turn = 0;                       /* 当前应当输出的线程索引 0..3 */

typedef struct {
    int idx;                               /* 0~3 */
    pthread_t tid;
} thread_arg_t;

static void *worker(void *arg) {
    thread_arg_t *t = (thread_arg_t *)arg;

    for (int r = 0; r < ROUNDS; ++r) {
        pthread_mutex_lock(&lock);
        while (turn != t->idx)
            pthread_cond_wait(&cond, &lock);

        /* 打印 p<id> 后跟 (idx+1) 个 '*'，pthread_t 用十六进制低 6 位以便阅读 */
        printf("p%lx", (unsigned long)t->tid & 0xFFFFFFul);
        for (int s = 0; s <= t->idx; ++s) putchar('*');

        /* 第 4 号线程一轮结束，换行更清晰 */
        if (t->idx == NTHREAD - 1) putchar('\n');

        turn = (turn + 1) % NTHREAD;
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

int main(void) {
    thread_arg_t args[NTHREAD];

    for (int i = 0; i < NTHREAD; ++i) {
        args[i].idx = i;
        pthread_create(&args[i].tid, NULL, worker, &args[i]);
    }

    /* 主线程预先打印一下每个线程对应的真实 tid，便于查看 */
    for (int i = 0; i < NTHREAD; ++i)
        printf("线程 #%d 的 pthread_t = %lx (低 24 位)\n",
               i + 1, (unsigned long)args[i].tid & 0xFFFFFFul);
    puts("---- 开始按 p1*p2**p3***p4**** 模式输出 ----");

    for (int i = 0; i < NTHREAD; ++i)
        pthread_join(args[i].tid, NULL);

    pthread_mutex_destroy(&lock);
    pthread_cond_destroy(&cond);
    return 0;
}
