#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    int rank, p;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    // 假设全局大小 N 是 p^2 的 100000 倍，确保整除，方便使用 MPI_Alltoall
    long long N = (long long)p * p * 1000000;
    long long local_n = N / p;     // 每个进程拥有的总数据量
    long long chunk = local_n / p; // 每次发给单个目标进程的数据量

    // 分配内存
    int *block_data = (int *)malloc(local_n * sizeof(int));
    int *cyclic_data = (int *)malloc(local_n * sizeof(int));
    int *send_buf = (int *)malloc(local_n * sizeof(int));
    int *recv_buf = (int *)malloc(local_n * sizeof(int));
    int *counts = (int *)malloc(p * sizeof(int));

    // 1. 初始化块分布数据
    for (long long i = 0; i < local_n; i++)
    {
        block_data[i] = rank;
    }

    // 测试 1: 从块分布转换为循环分布
    MPI_Barrier(MPI_COMM_WORLD);
    double start_b2c = MPI_Wtime();

    // 1a. 打包数据
    for (int i = 0; i < p; i++)
        counts[i] = 0; // 重置计数器
    for (long long i = 0; i < local_n; i++)
    {
        long long g = rank * local_n + i; // 计算全局索引
        int dest = g % p;                 // 循环分布的目标进程
        // 将数据放入对应目标进程的发送槽位
        send_buf[dest * chunk + counts[dest]] = block_data[i];
        counts[dest]++;
    }

    // 1b. 全局全交换
    MPI_Alltoall(send_buf, chunk, MPI_INT, recv_buf, chunk, MPI_INT, MPI_COMM_WORLD);

    // 1c. 解包数据
    for (long long i = 0; i < local_n; i++)
    {
        cyclic_data[i] = recv_buf[i];
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double end_b2c = MPI_Wtime();
    double time_b2c = end_b2c - start_b2c;

    // 测试 2: 从 循环分布转换为块分布 
    MPI_Barrier(MPI_COMM_WORLD);
    double start_c2b = MPI_Wtime();

    // 2a. 打包数据
    for (int i = 0; i < p; i++)
        counts[i] = 0;
    for (long long i = 0; i < local_n; i++)
    {
        long long g = rank + i * p; // 循环数据的全局索引
        int dest = g / local_n;     // 块分布的目标进程
        send_buf[dest * chunk + counts[dest]] = cyclic_data[i];
        counts[dest]++;
    }

    // 2b. 全局全交换
    MPI_Alltoall(send_buf, chunk, MPI_INT, recv_buf, chunk, MPI_INT, MPI_COMM_WORLD);

    // 2c. 解包数据
    for (long long i = 0; i < local_n; i++)
    {
        block_data[i] = recv_buf[i];
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double end_c2b = MPI_Wtime();
    double time_c2b = end_c2b - start_c2b;

    // 结果统计与输出 (取所有进程耗时的最大值)
    double max_time_b2c, max_time_c2b;
    MPI_Reduce(&time_b2c, &max_time_b2c, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&time_c2b, &max_time_c2b, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        printf("总数据量 N = %lld 整数 (约 %lld MB)\n", N, N * sizeof(int) / 1024 / 1024);
        printf("进程数量 P = %d\n", p);
        printf("--------------------------------------------------\n");
        printf("块分布 -> 循环分布耗时: %f 秒\n", max_time_b2c);
        printf("循环分布 -> 块分布耗时: %f 秒\n", max_time_c2b);
        printf("--------------------------------------------------\n");
    }

    free(block_data);
    free(cyclic_data);
    free(send_buf);
    free(recv_buf);
    free(counts);

    MPI_Finalize();
    return 0;
}