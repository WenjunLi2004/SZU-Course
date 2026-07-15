#include <stdio.h>
#include <cuda.h>
__global__ void Matrix_mult(const float dx[], const float dy[], float dz[], const int n)
{
    int r = blockIdx.y * blockDim.y + threadIdx.y;
    int c = blockIdx.x * blockDim.x + threadIdx.x;

    if (r < n && c < n)
    {
        float temp = 0.0;
        for (int i = 0; i < n; i++)
        {
            temp += dx[r * n + i] * dy[i * n + c];
        }
        dz[r * n + c] = temp;
    }
}
__global__ void Sum(const float a[], int n, float *sum_p)
{
    int id = blockIdx.x * blockDim.x + threadIdx.x;

    float temp = 0.0;

    if (id < n)
    {
        temp = a[id];
    }

    unsigned mask = 0xffffffff;
    for (int diff = warpSize / 2; diff > 0; diff /= 2)
    {
        temp += __shfl_down_sync(mask, temp, diff);
    }

    if (threadIdx.x % warpSize == 0)
    {
        atomicAdd(sum_p, temp);
    }
}

int main(void)
{
    int n = 1024;
    float *x, *y, *z, *cz;
    float *dx, *dy, *dz;
    float sum, *sum_p;
    x = (float *)malloc(n * n * sizeof(float));
    y = (float *)malloc(n * n * sizeof(float));
    z = (float *)malloc(n * n * sizeof(float));
    cz = (float *)malloc(n * n * sizeof(float));
    cudaMalloc(&dx, n * n * sizeof(float));
    cudaMalloc(&dy, n * n * sizeof(float));
    cudaMalloc(&dz, n * n * sizeof(float));
    cudaMallocManaged(&sum_p, sizeof(float));
    srand(0);
    for (int i = 0; i < n * n; i++)
    {
        x[i] = rand() * 1.0 / RAND_MAX;
        y[i] = rand() * 1.0 / RAND_MAX;
        cz[i] = 0;
    }
    struct timespec start, finish;
    cudaMemcpy(dx, x, n * n * sizeof(float), cudaMemcpyHostToDevice);
    cudaMemcpy(dy, y, n * n * sizeof(float), cudaMemcpyHostToDevice);
    dim3 block_dim1(16, 16);
    dim3 grid_dim1((n + block_dim1.x - 1) / block_dim1.x,
                    (n + block_dim1.y - 1) / block_dim1.y);
    int block_dim2 = 256;
    int grid_dim2 = (n * n + block_dim2 - 1) / block_dim2;
    timespec_get(&start, TIME_UTC);
    Matrix_mult<<<grid_dim1, block_dim1>>>(dx, dy, dz, n);
    cudaDeviceSynchronize();
    *sum_p = 0;
    Sum<<<grid_dim2, block_dim2>>>(dz, n * n, sum_p);
    cudaDeviceSynchronize();
    cudaMemcpy(z, dz, n * n * sizeof(float), cudaMemcpyDeviceToHost);
    timespec_get(&finish, TIME_UTC);
    printf("GPU time = %f\n", finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9);
    timespec_get(&start, TIME_UTC);
    for (int i = 0; i < n; i++)
        for (int k = 0; k < n; k++)
            for (int j = 0; j < n; j++)
                cz[i * n + j] += x[i * n + k] * y[k * n + j];
    sum = 0;
    for (int i = 0; i < n * n; i++)
        sum += cz[i];
    timespec_get(&finish, TIME_UTC);
    printf("CPU time = %f\n", finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9);
    double diff_norm = 0;
    for (int i = 0; i < n * n; i++)
    {
        double diff = z[i] - cz[i];
        diff_norm += diff * diff;
    }
    diff_norm = sqrt(diff_norm);
    printf("Two-norm of difference between GPU and CPU = %f\n", diff_norm);
    printf("Sum computed by GPU = %e\n", *sum_p);
    printf("Sum computed by CPU = %e\n", sum);
    free(x);
    free(y);
    free(z);
    free(cz);
    cudaFree(dx);
    cudaFree(dy);
    cudaFree(dz);
    cudaFree(sum_p);
    return 0;
}
