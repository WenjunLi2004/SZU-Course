#include <algorithm>
#include <float.h>
#include <iostream>
#include <omp.h>
using namespace std;

int main(int argc, char *argv[])
{
    int n = 1e7; // 待排序元素个数
    int p = 8;   // OpenMP 线程数，同时也是 PSRS 中的分段数 / 桶数

    if (argc > 1)
        p = atoi(argv[1]);
    if (argc > 2)
        n = atoi(argv[2]);
    double *a = new double[n]; // 并行排序使用的数组
    double *b = new double[n]; // PSRS 并行排序后的结果数组
    double *c = new double[n]; // 串行排序对照数组，用于验证正确性

    // 初始化随机数据，同时复制一份到 c 中用于串行 sort 对照
    for (int i = 0; i < n; i++)
        a[i] = c[i] = rand() * 1.0 / RAND_MAX;

    double start, finish;

    int local_n = n / p;           // 每个线程初始分到的元素个数
    int sample_step = local_n / p; // 正则采样间隔：每个线程从局部有序段中取 p 个样本

    double *samples = new double[p * p]; // 全部样本：p 个线程，每个线程 p 个样本
    double *pivots = new double[p - 1];  // 主元：p 个桶需要 p-1 个分界点

    // cuts 用一维数组模拟 cuts[thread_id][bucket_id]
    // 每个线程的局部有序段被 p-1 个主元切成 p 个桶，因此需要 p+1 个边界
    int *cuts = new int[p * (p + 1)];

    // bucket_start[j] 表示第 j 个全局桶在结果数组 b 中的起始位置
    // 第 j 个桶最终写入 b[bucket_start[j], bucket_start[j+1])
    int *bucket_start = new int[p + 1];

    start = omp_get_wtime();

#pragma omp parallel num_threads(p)
    {
        int rank = omp_get_thread_num();

        // =========================
        // 1. 均匀划分 + 局部排序
        // =========================

        int left = rank * local_n;
        int right = left + local_n;

        // 最后一个线程负责到 n，避免 n 不能被 p 整除时丢失元素
        if (rank == p - 1)
            right = n;

        // 每个线程只排序自己负责的局部段，区间为 [left, right)
        sort(a + left, a + right);

        // =========================
        // 2. 正则采样
        // =========================

        // 每个线程从自己的局部有序段中等间隔选取 p 个样本
        for (int j = 0; j < p; j++)
            samples[rank * p + j] = a[left + j * sample_step];

        // 必须等待所有线程完成采样，才能对 samples 做全局排序
#pragma omp barrier

        // =========================
        // 3. 样本排序 + 4. 选择主元
        // =========================

#pragma omp single
        {
            // 对 p^2 个样本排序，近似估计全局数据分布
            sort(samples, samples + p * p);

            // 从排好序的样本中每隔 p 个取一个主元
            // 共选出 p-1 个主元，用于划分 p 个全局桶
            for (int i = 1; i < p; i++)
                pivots[i - 1] = samples[i * p];
        }
        // single 结尾默认有隐式 barrier，保证所有线程看到已经选好的 pivots

        // =========================
        // 5. 主元划分
        // =========================

        int cut_base = rank * (p + 1);

        // 当前线程局部段的第一个边界和最后一个边界
        cuts[cut_base] = left;
        cuts[cut_base + p] = right;

        // 在当前局部有序段中查找每个主元对应的切分点
        // upper_bound 返回第一个大于 pivots[j-1] 的位置
        for (int j = 1; j < p; j++)
            cuts[cut_base + j] =
                upper_bound(a + left, a + right, pivots[j - 1]) - a;

        // 必须等待所有线程完成 cuts 计算，才能统计每个桶的总大小
#pragma omp barrier

        // =========================
        // 6. 计算每个全局桶在 b 中的起始位置
        // =========================

#pragma omp single
        {
            bucket_start[0] = 0;

            for (int j = 0; j < p; j++)
            {
                int size = 0;

                // 第 j 个全局桶由所有线程的第 j 个局部桶组成
                for (int i = 0; i < p; i++)
                {
                    size += cuts[i * (p + 1) + j + 1] - cuts[i * (p + 1) + j];
                }

                // 前缀和：确定第 j+1 个桶的起始位置
                bucket_start[j + 1] = bucket_start[j] + size;
            }
        }
        // single 结尾默认有隐式 barrier，保证 bucket_start 已经计算完成

        // =========================
        // 7. p 路归并
        // =========================

        // 当前线程 rank 负责第 rank 个全局桶
        // 它需要归并来自所有线程的第 rank 个局部桶

        int pos[p];     // pos[i] 表示第 i 个线程的当前读取位置
        int end[p];     // end[i] 表示第 i 个线程对应局部桶的结束位置
        double head[p]; // head[i] 缓存第 i 个局部桶当前头元素，减少重复访存

        for (int i = 0; i < p; i++)
        {
            // 第 i 个线程的第 rank 个桶范围：
            // [cuts[i][rank], cuts[i][rank+1])
            pos[i] = cuts[i * (p + 1) + rank];
            end[i] = cuts[i * (p + 1) + rank + 1];

            // 如果该局部桶非空，则记录当前头元素；否则用 DBL_MAX 表示无有效元素
            if (pos[i] < end[i])
                head[i] = a[pos[i]];
            else
                head[i] = DBL_MAX;
        }

        // 当前线程负责写入 b 中第 rank 个全局桶的位置
        double *write = b + bucket_start[rank];

        // 当前全局桶的元素总数
        int size = bucket_start[rank + 1] - bucket_start[rank];

        // 朴素 p 路归并：每次从 p 个局部桶的头元素中选出最小值
        for (int i = 0; i < size; i++)
        {
            double min_value = head[0];
            int min_rank = 0;

            // 找到当前最小的桶头元素
            for (int j = 1; j < p; j++)
            {
                if (head[j] < min_value)
                {
                    min_value = head[j];
                    min_rank = j;
                }
            }

            // 将最小值写入结果数组
            *write++ = min_value;

            // 对应局部桶的读取位置后移
            pos[min_rank]++;

            // 更新该局部桶的头元素
            if (pos[min_rank] < end[min_rank])
                head[min_rank] = a[pos[min_rank]];
            else
                head[min_rank] = DBL_MAX;
        }
    }

    finish = omp_get_wtime();
    cout << finish - start << endl;

    // cout << "parallel time with " << p << " threads = " << finish - start << endl;

    // 串行排序作为正确性和性能对照
    start = omp_get_wtime();
    sort(c, c + n);
    finish = omp_get_wtime();
    // cout << "serial time = " << finish - start << endl;

    // 验证 PSRS 排序结果是否与串行 sort 完全一致
    for (int i = 0; i < n; i++)
        if (b[i] != c[i])
        {
            cout << "error" << endl;
            return 1;
        }

    delete[] a;
    delete[] b;
    delete[] c;
    delete[] samples;
    delete[] pivots;
    delete[] cuts;
    delete[] bucket_start;

    return 0;
}
