#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <cuda.h>

// 在二进制表示中向第 j 位插入 0。
// 这样可以用一个线程编号生成一对待比较元素中的第一个下标。
__device__ unsigned Insert_zero(unsigned val, unsigned j)
{
    unsigned left_bits, right_bits, left_ones, right_ones;
    left_ones = (~0) << j;
    right_ones = ~left_ones;
    left_bits = left_ones & val;
    right_bits = right_ones & val;
    return (left_bits << 1) | right_bits;
}

// 根据 inc_dec 指定的方向完成一次 compare-swap：
// inc_dec 为 0 时按升序交换，否则按降序交换。
__device__ void Compare_swap(float a[], unsigned elt, unsigned partner, unsigned inc_dec)
{
    float tmp;
    if ((inc_dec == 0 && a[elt] > a[partner]) || (inc_dec != 0 && a[elt] < a[partner]))
    {
        tmp = a[elt];
        a[elt] = a[partner];
        a[partner] = tmp;
    }
}

// 每个线程块处理 2048 个元素：1024 个线程各负责一组比较。
// 本阶段先在共享内存中把每个 2048 元素的小段排成双调有序段。
__global__ void Block_sort(float a[])
{
    __shared__ float shared[2048];
    unsigned th = threadIdx.x;
    unsigned base = blockIdx.x * 2048;

    // 一个线程读取两个元素，充分利用 1024 线程块完成 2048 元素段排序。
    shared[th] = a[base + th];
    shared[th + 1024] = a[base + th + 1024];

    __syncthreads();

    unsigned bf_sz, stage, my_elt1, my_elt2, initial_bit, which_bit;

    for (bf_sz = 2, initial_bit = 0; bf_sz <= 2048; bf_sz <<= 1, initial_bit++)
    {
        // stage 控制当前比较距离，which_bit 用于由线程号生成不重复的比较对。
        for (stage = bf_sz >> 1, which_bit = initial_bit; stage > 0; stage >>= 1, which_bit--)
        {
            my_elt1 = Insert_zero(th, which_bit);
            my_elt2 = my_elt1 ^ stage;
            Compare_swap(shared, my_elt1, my_elt2, (base + my_elt1) & bf_sz);
            __syncthreads();
        }
    }

    a[base + th] = shared[th];
    a[base + th + 1024] = shared[th + 1024];
}

// 当比较距离不小于 2048 时，配对元素可能位于不同线程块。
// 此时直接在全局内存中完成跨块 compare-swap。
__global__ void Cross_block(float a[], unsigned bf_sz, unsigned stage, unsigned which_bit)
{
    unsigned th = blockIdx.x * 1024 + threadIdx.x;
    unsigned my_elt1 = Insert_zero(th, which_bit);
    unsigned my_elt2 = my_elt1 ^ stage;

    Compare_swap(a, my_elt1, my_elt2, my_elt1 & bf_sz);
}

// 跨块合并完成后，剩余比较距离小于 2048，可回到共享内存中完成块内合并。
__global__ void In_block(float a[], unsigned bf_sz)
{
    __shared__ float shared[2048];

    unsigned th = threadIdx.x;
    unsigned base = blockIdx.x * 2048;

    shared[th] = a[base + th];
    shared[th + 1024] = a[base + th + 1024];

    __syncthreads();

    unsigned stage, which_bit, my_elt1, my_elt2;

    for (stage = 2048 >> 1, which_bit = 10; stage > 0; stage >>= 1, which_bit--)
    {
        my_elt1 = Insert_zero(th, which_bit);
        my_elt2 = my_elt1 ^ stage;

        Compare_swap(shared, my_elt1, my_elt2, (base + my_elt1) & bf_sz);

        __syncthreads();
    }

    a[base + th] = shared[th];
    a[base + th + 1024] = shared[th + 1024];
}

int main(int argc, char *argv[])
{
    // 支持从命令行传入 n，便于脚本批量测试；未传参时默认 n = 2^25。
    // n 必须为 2 的幂；每个 CUDA block 固定 1024 个线程，处理 2048 个元素。
    int n = (argc > 1) ? atoi(argv[1]) : (1 << 25);
    int th_per_blk = 1024, blk_ct = n / 2 / th_per_blk;
    if (n < 2048 || (n & (n - 1)) != 0)
    {
        fprintf(stderr, "n must be a power of two and at least 2048\n");
        return 1;
    }
    float *a, *b, *da;
    a = (float *)malloc(n * sizeof(float));
    b = (float *)malloc(n * sizeof(float));
    cudaMalloc(&da, n * sizeof(float));
    srand(0);
    for (int i = 0; i < n; i++)
        a[i] = b[i] = rand() * 1.0 / RAND_MAX;
    cudaMemcpy(da, a, n * sizeof(float), cudaMemcpyHostToDevice);
    struct timespec start, finish;
    timespec_get(&start, TIME_UTC);

    // 先完成 2048 元素粒度的局部排序，再逐级扩大双调序列长度。
    Block_sort<<<blk_ct, th_per_blk>>>(da);

    for (unsigned bf_sz = 4096, initial_bit = 11; bf_sz <= (unsigned)n; bf_sz <<= 1, initial_bit++)
    {
        // 大步长阶段需要跨线程块访问，只能使用全局内存。
        for (unsigned stage = bf_sz >> 1, which_bit = initial_bit;
             stage >= 2048;
             stage >>= 1, which_bit--)
        {
            Cross_block<<<blk_ct, th_per_blk>>>(da, bf_sz, stage, which_bit);
        }

        // 小步长阶段的比较对象落在同一 2048 元素段内，使用共享内存加速。
        In_block<<<blk_ct, th_per_blk>>>(da, bf_sz);
    }
    cudaDeviceSynchronize();

    timespec_get(&finish, TIME_UTC);
    printf("GPU time = %f\n", finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9);
    timespec_get(&start, TIME_UTC);
    std::sort(b, b + n);
    timespec_get(&finish, TIME_UTC);
    printf("CPU time = %f\n", finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9);
    cudaMemcpy(a, da, n * sizeof(float), cudaMemcpyDeviceToHost);
    // 与 CPU std::sort 的结果逐项对比，验证 GPU 双调排序正确性。
    for (int i = 0; i < n; i++)
        if (a[i] != b[i])
            printf("error\n");
    free(a);
    free(b);
    cudaFree(da);
}
