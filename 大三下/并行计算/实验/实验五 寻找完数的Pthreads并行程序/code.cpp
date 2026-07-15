#include <iostream>
#include <pthread.h>
#include <algorithm>
#include <cstdlib>
#include <ctime>
using namespace std;

const int MAX = 100; // 保存完数结果的最大个数
int p = 8;           // 线程数，默认使用 8 个线程
int n = 1000000;     // 查找范围上界，默认查找小于等于 1000000 的所有完数
int a[MAX];          // 保存找到的完数
int num = 0;         // 当前已经找到的完数个数

// 互斥锁用于保护全局结果数组 a 和全局计数 num。
// 主计算过程使用线程私有数组，不需要加锁；只有合并结果时进入临界区。
pthread_mutex_t mutex;

// Pthreads 线程函数。
// 每个线程负责 [2, n] 中的一段连续整数，并判断这些整数是否为完数。
void *Perfect_number(void *r)
{
    long rank = (long)r; // 当前线程编号，范围为 0 到 p - 1

    // [2, n] 中共有 n - 1 个待测试整数。
    // 按线程编号把这些整数均匀划分为 p 个闭区间。
    int local_n = (n - 1) / p;      // 待测试整数个数
    int start = 2 + rank * local_n; // 当前线程负责区间的起点
    int end = start + local_n - 1;  // 当前线程负责区间的终点

    // 当线程数大于待测试整数个数时，某些线程可能没有任务。
    if (local_n <= 0)
        return NULL;

    // sum[i] 表示整数 start + i 的所有真因数之和。
    // 该数组是线程私有的，所以不同线程之间不会发生写冲突。
    int *sum = new int[local_n];

    // 每个线程先把自己找到的完数存入本地数组，
    // 线程结束前再一次性合并到全局数组 a 中，减少锁竞争。
    int local_a[MAX];
    int local_num = 0;

    // 枚举可能的真因数 d，并把 d 加到当前线程负责区间中所有 d 的倍数上。
    // 这样仍然是在统计每个整数的真因数和，符合完数定义。
    for (int d = 1; d <= end / 2; d++)
    {
        // 找到 [start, end] 中第一个 d 的倍数。
        // 向上调整到不小于 start 的位置
        int first = ((start + d - 1) / d) * d;

        // d 不能作为自身的真因数，因此倍数至少应从 2*d 开始。
        if (first < 2 * d)
            first = 2 * d;

        // 对区间内所有 d 的倍数 x，累加真因数 d。
        for (int x = first; x <= end; x += d)
            sum[x - start] += d;
    }

    // 逐个测试当前线程负责的整数：
    // 若所有真因数之和等于它本身，则该整数是完数。
    for (int x = start; x <= end; x++)
        if (sum[x - start] == x && local_num < MAX)
            local_a[local_num++] = x;

    // 将本地结果合并到全局数组。
    // a 和 num 是共享变量，必须用互斥锁保护。
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < local_num && num < MAX; i++)
        a[num++] = local_a[i];
    pthread_mutex_unlock(&mutex);

    // 释放当前线程申请的私有数组。
    delete[] sum;
    return NULL;
}

int main(int argc, char *argv[])
{
    // 支持命令行指定线程数和问题规模：
    
    if (argc > 1)
        p = atoi(argv[1]);
    if (argc > 2)
        n = atoi(argv[2]);

    // 为 p 个线程分配线程句柄，并初始化互斥锁。
    pthread_t *thread_handles = (pthread_t *)malloc(p * sizeof(pthread_t));
    pthread_mutex_init(&mutex, NULL);

    struct timespec start, finish;

    // 开始计时，计入线程创建、并行计算和线程回收的总时间。
    timespec_get(&start, TIME_UTC);

    // 创建 p 个线程执行完数查找。
    for (long thread = 0; thread < p; thread++)
        pthread_create(&thread_handles[thread], NULL, Perfect_number, (void *)thread);

    // 等待所有线程结束。
    for (long thread = 0; thread < p; thread++)
        pthread_join(thread_handles[thread], NULL);

    // 停止计时。
    timespec_get(&finish, TIME_UTC);

    // 各线程合并结果的先后顺序不固定，排序后保证按从小到大输出。
    sort(a, a + num);

    // 输出并行执行时间和所有找到的完数。
    cout << "parallel execution time = " << finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9 << endl;
    for (int i = 0; i < num; i++)
        cout << a[i] << " ";
    cout << endl;

    // 销毁互斥锁并释放线程句柄数组。
    pthread_mutex_destroy(&mutex);
    free(thread_handles);
    return 0;
}
