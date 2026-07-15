/*
 * thread_pool.c
 *
 * 固定大小线程池实现：LIFO 空闲栈 + 每线程条件变量 + 非阻塞派发。
 */

#include "thread_pool.h"

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

typedef enum { WORKER_IDLE = 0, WORKER_BUSY = 1 } WorkerState;

typedef struct {
    int             index;
    pthread_t       tid;
    WorkerState     state;
    int             has_job;      /* 1=已分派待处理的 job */
    void           *job;
    pthread_mutex_t mu;           /* 保护 has_job / job */
    pthread_cond_t  cv;           /* 唤醒该线程处理 job 或退出 */
    ThreadPool     *pool;
} Worker;

struct ThreadPool {
    int      size;
    Worker  *workers;

    int     *idle_stack;          /* 空闲 worker 下标，LIFO */
    int      idle_top;            /* 栈内元素个数；idle_stack[idle_top-1] 为栈顶 */
    pthread_mutex_t stack_mu;     /* 保护 idle_stack / idle_top */

    void   (*handle_job)(void *job, int worker_index, unsigned long tid);
    void   (*free_job)(void *job);

    volatile int shutting_down;
};

static void idle_push(ThreadPool *p, int idx)
{
    pthread_mutex_lock(&p->stack_mu);
    p->idle_stack[p->idle_top++] = idx;
    pthread_mutex_unlock(&p->stack_mu);
}

/* 取栈顶空闲 worker 下标；无空闲返回 -1。 */
static int idle_pop(ThreadPool *p)
{
    int idx = -1;
    pthread_mutex_lock(&p->stack_mu);
    if (p->idle_top > 0) idx = p->idle_stack[--p->idle_top];
    pthread_mutex_unlock(&p->stack_mu);
    return idx;
}

static void *worker_main(void *arg)
{
    Worker     *w = (Worker *)arg;
    ThreadPool *p = w->pool;
    unsigned long tid = (unsigned long)(uintptr_t)pthread_self();

    for (;;)
    {
        void *job = NULL;

        pthread_mutex_lock(&w->mu);
        /* 用 while 而非 if：既挡虚假唤醒，也让 shutting_down 能把等待中的线程唤出来退出。 */
        while (!w->has_job && !p->shutting_down)
            pthread_cond_wait(&w->cv, &w->mu);
        if (!w->has_job && p->shutting_down) { pthread_mutex_unlock(&w->mu); break; }
        job = w->job;
        w->job = NULL;
        w->has_job = 0;
        pthread_mutex_unlock(&w->mu);

        if (job)
        {
            p->handle_job(job, w->index, tid);
            p->free_job(job);
        }

        /* 处理完毕：标记空闲并压回 LIFO 栈，等待下一次派发。 */
        w->state = WORKER_IDLE;
        idle_push(p, w->index);
    }
    return NULL;
}

ThreadPool *thread_pool_create(int size,
                               void (*handle_job)(void *job, int worker_index,
                                                  unsigned long tid),
                               void (*free_job)(void *job))
{
    ThreadPool *p;
    int i;

    if (size <= 0 || !handle_job || !free_job) return NULL;

    p = calloc(1, sizeof(*p));
    if (!p) return NULL;
    p->size = size;
    p->handle_job = handle_job;
    p->free_job = free_job;
    p->shutting_down = 0;
    p->workers = calloc((size_t)size, sizeof(Worker));
    p->idle_stack = calloc((size_t)size, sizeof(int));
    if (!p->workers || !p->idle_stack) { free(p->workers); free(p->idle_stack); free(p); return NULL; }
    pthread_mutex_init(&p->stack_mu, NULL);
    p->idle_top = 0;

    for (i = 0; i < size; i++)
    {
        Worker *w = &p->workers[i];
        w->index = i;
        w->state = WORKER_IDLE;
        w->has_job = 0;
        w->job = NULL;
        w->pool = p;
        pthread_mutex_init(&w->mu, NULL);
        pthread_cond_init(&w->cv, NULL);
    }

    /* 先把所有 worker 压入空闲栈，再启动线程：保证派发只会命中已就绪线程。 */
    for (i = 0; i < size; i++) p->idle_stack[p->idle_top++] = i;

    for (i = 0; i < size; i++)
    {
        if (pthread_create(&p->workers[i].tid, NULL, worker_main, &p->workers[i]) != 0)
        {
            /* 部分创建失败：通知已创建的退出并清理。 */
            int j;
            p->shutting_down = 1;
            for (j = 0; j < i; j++)
            {
                pthread_mutex_lock(&p->workers[j].mu);
                pthread_cond_signal(&p->workers[j].cv);
                pthread_mutex_unlock(&p->workers[j].mu);
            }
            for (j = 0; j < i; j++) pthread_join(p->workers[j].tid, NULL);
            for (j = 0; j < size; j++) { pthread_mutex_destroy(&p->workers[j].mu); pthread_cond_destroy(&p->workers[j].cv); }
            pthread_mutex_destroy(&p->stack_mu);
            free(p->workers); free(p->idle_stack); free(p);
            return NULL;
        }
    }
    return p;
}

int thread_pool_dispatch(ThreadPool *p, void *job)
{
    int idx;
    Worker *w;

    if (!p || p->shutting_down) return -1;
    idx = idle_pop(p);
    if (idx < 0) return -1;            /* 无空闲线程：忙 */

    w = &p->workers[idx];
    pthread_mutex_lock(&w->mu);
    w->state = WORKER_BUSY;
    w->job = job;
    w->has_job = 1;
    pthread_cond_signal(&w->cv);
    pthread_mutex_unlock(&w->mu);
    return 0;
}

int thread_pool_idle_count(ThreadPool *p)
{
    int n;
    if (!p) return 0;
    pthread_mutex_lock(&p->stack_mu);
    n = p->idle_top;
    pthread_mutex_unlock(&p->stack_mu);
    return n;
}

int thread_pool_size(ThreadPool *p) { return p ? p->size : 0; }

void thread_pool_destroy(ThreadPool *p)
{
    int i;
    if (!p) return;

    p->shutting_down = 1;
    for (i = 0; i < p->size; i++)
    {
        pthread_mutex_lock(&p->workers[i].mu);
        pthread_cond_signal(&p->workers[i].cv);
        pthread_mutex_unlock(&p->workers[i].mu);
    }
    for (i = 0; i < p->size; i++) pthread_join(p->workers[i].tid, NULL);
    for (i = 0; i < p->size; i++)
    {
        pthread_mutex_destroy(&p->workers[i].mu);
        pthread_cond_destroy(&p->workers[i].cv);
    }
    pthread_mutex_destroy(&p->stack_mu);
    free(p->workers);
    free(p->idle_stack);
    free(p);
}
