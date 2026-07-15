#include <iostream>
#include <pthread.h>
#include <cstdlib>
#include <cstdint>
#include <ctime>
#include <cmath>

using namespace std;

int p = 8;      // 线程数，可由命令行第 1 个参数指定
int n = 2e8;    // 数组大小，可由命令行第 2 个参数指定
double *a;      // 输入数组
double *b;      // 并行前缀和结果数组
double *c;      // 串行前缀和结果数组，用于验证并行结果
double *offset; // 每个线程负责区间前面所有区间的累加偏移量
pthread_mutex_t barrier_mutex;
pthread_cond_t cond;
int barrier_count = 0; // 当前已经到达屏障的线程数
int phase = 0;         // 屏障轮次，用于避免条件变量虚假唤醒导致提前通过

// 线程屏障：保证所有线程都完成当前阶段后，才能一起进入下一阶段。
void Barrier()
{
    pthread_mutex_lock(&barrier_mutex);
    int my_phase = phase;
    barrier_count++;
    if (barrier_count == p)
    {
        barrier_count = 0;
        phase++;
        pthread_cond_broadcast(&cond);
    }
    else
        while (my_phase == phase)
            pthread_cond_wait(&cond, &barrier_mutex);
    pthread_mutex_unlock(&barrier_mutex);
}

// Pthreads 线程函数：每个线程负责数组中的一个连续分块。
// 并行前缀和分为三阶段：
// 1. 各线程独立计算本地分块内部的前缀和；
// 2. 根据各分块末尾值计算每个分块应加的 offset；
// 3. 各线程把对应 offset 加回自己的本地分块，得到全局前缀和。
void *Prefix_sum(void *r)
{
    long rank = (long)r;                  // 当前线程编号，范围为 0 到 p - 1
    int local_n = n / p;                  // 每个线程处理的元素个数
    int local_start = rank * local_n;     // 当前线程负责区间的起始下标
    int local_end = (rank + 1) * local_n; // 当前线程负责区间的结束下标（不含）

    // 阶段1：局部前缀和。
    // 每个线程只读写自己的连续区间 [local_start, local_end)，
    // 因此该阶段不需要互斥；计算完成后，区间末尾 b[local_end - 1]
    // 就是该分块内所有元素的总和。
    b[local_start] = a[local_start];
    for (int i = local_start + 1; i < local_end; i++)
        b[i] = b[i - 1] + a[i];

    // 等待所有线程完成阶段1，确保后续计算 offset 时每个分块末尾值都已就绪。
    Barrier();

    // 阶段2：计算分块偏移量。
    // offset[i] 表示第 i 个分块之前所有分块元素的总和；
    // 例如 offset[2] = 第 0 块总和 + 第 1 块总和。
    // 第 0 个分块前面没有元素，因此 offset[0] 为 0。
    if (rank == 0)
    {
        offset[0] = 0;
        for (int i = 1; i < p; i++)
            offset[i] = offset[i - 1] + b[i * local_n - 1];
    }

    // 等待 offset 数组计算完成，避免阶段3读取到未完成的偏移量。
    Barrier();

    // 阶段3：加回全局偏移量。
    // 阶段1得到的是“分块内部”的前缀和；除第 0 块外，
    // 其他分块的每个元素都还需要加上它前面所有分块的总和，
    // 才能变成相对于整个数组的全局前缀和。
    if (rank != 0)
    {
        double my_offset = offset[rank];
        for (int i = local_start; i < local_end; i++)
            b[i] += my_offset;
    }

    return NULL;
}
static uint64_t seed = 1;

// 简单线性同余随机数生成器，用于快速初始化大规模测试数据。
inline double fast_rand()
{
    seed = seed * 6364136223846793005ULL + 1;
    return (double)(seed >> 32) / (1ULL << 32);
}
int main(int argc, char *argv[])
{
    if (argc > 1)
        p = atoi(argv[1]);
    if (argc > 2)
        n = atoll(argv[2]);
    a = new double[n];
    b = new double[n];
    c = new double[n];
    offset = new double[p];
    for (int i = 0; i < n; i++)
        a[i] = fast_rand() * 2 - 1;
    long thread;
    pthread_t *thread_handles;
    thread_handles = (pthread_t *)malloc(p * sizeof(pthread_t));
    pthread_mutex_init(&barrier_mutex, NULL);
    pthread_cond_init(&cond, NULL);
    struct timespec start, finish;
    timespec_get(&start, TIME_UTC);

    for (thread = 0; thread < p; thread++)
        pthread_create(&thread_handles[thread], NULL, Prefix_sum, (void *)thread);
    for (thread = 0; thread < p; thread++)
        pthread_join(thread_handles[thread], NULL);

    timespec_get(&finish, TIME_UTC);
    cout << "parallel execution time = " << finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9 << endl;
    timespec_get(&start, TIME_UTC);
    c[0] = a[0];
    for (int i = 1; i < n; i++)
        c[i] = c[i - 1] + a[i];
    timespec_get(&finish, TIME_UTC);
    cout << "serial execution time = " << finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9 << endl;
    double error = 0;
    for (int i = 0; i < n; i++)
        if (abs(b[i] - c[i]) > error)
            error = abs(b[i] - c[i]);
    cout << "max error = " << error << endl;
    delete[] a;
    delete[] b;
    delete[] c;
    delete[] offset;
    pthread_mutex_destroy(&barrier_mutex);
    pthread_cond_destroy(&cond);
    free(thread_handles);
    return 0;
}
