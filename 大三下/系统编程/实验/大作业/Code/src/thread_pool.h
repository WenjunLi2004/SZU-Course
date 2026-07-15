/*
 * thread_pool.h
 *
 * 固定大小的工作线程池。
 *
 * 设计要点：
 *   - 启动时创建恰好 poolsize 个工作线程。
 *   - 空闲线程用“栈”管理（LIFO）：刚回收的线程在栈顶，下一次优先被分派，
 *     符合“刚回收的线程倾向于下一次被分派”。
 *   - 主线程只负责 select + 读取完整请求，然后把请求打包成 job 派发给空闲线程；
 *     业务逻辑在工作线程里跑。
 *   - 派发不阻塞主事件循环：无空闲线程时 dispatch 返回 -1（忙）。
 *
 * 线程池本身与具体业务解耦：job 是一个不透明指针，由调用方提供
 * handle_job（执行）与 free_job（释放）回调。threads.log 的写入由
 * handle_job 内部完成（它能拿到 worker_index / tid / job 内容）。
 */

#ifndef CHAT_THREAD_POOL_H
#define CHAT_THREAD_POOL_H

typedef struct ThreadPool ThreadPool;

/*
 * 创建线程池并启动 size 个工作线程，全部置为空闲。
 *   handle_job(job, worker_index, tid)：在工作线程上下文执行一个 job。
 *   free_job(job)：handle_job 返回后释放 job。
 * 成功返回句柄；失败返回 NULL。
 */
ThreadPool *thread_pool_create(int size,
                               void (*handle_job)(void *job, int worker_index,
                                                  unsigned long tid),
                               void (*free_job)(void *job));

/*
 * 把 job 派发给一个空闲线程（LIFO 取栈顶）。
 * 成功返回 0；当前无空闲线程返回 -1（调用方据此回 "server busy" 并丢弃 job）。
 * 注意：返回 -1 时 job 的所有权仍属于调用方。
 */
int  thread_pool_dispatch(ThreadPool *pool, void *job);

/* 当前空闲线程数（用于日志/观测）。 */
int  thread_pool_idle_count(ThreadPool *pool);

/* 线程池容量。 */
int  thread_pool_size(ThreadPool *pool);

/*
 * 通知所有工作线程退出并 join，然后释放线程池。
 * 已在处理中的 job 会先跑完再退出。
 */
void thread_pool_destroy(ThreadPool *pool);

#endif
