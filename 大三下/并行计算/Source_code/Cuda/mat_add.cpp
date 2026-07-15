#include <stdio.h>
#include <cuda.h>
__global__ void Vec_add(const float x[], const float y[], float z[], const int n)
{
    int my_elt = blockDim.x * blockIdx.x + threadIdx.x;
    if (my_elt < n)
        z[my_elt] = x[my_elt] + y[my_elt];
}
int main(void)
{
    int n = 1e8;
    int th_per_blk = 1024;
    int blk_ct = ceil(n * 1.0 / th_per_blk);
    float *x, *y, *z, *cz;
    cudaMallocManaged(&x, n * sizeof(float));
    cudaMallocManaged(&y, n * sizeof(float));
    cudaMallocManaged(&z, n * sizeof(float));
    cz = (float *)malloc(n * sizeof(float));
    for (int i = 0; i < n; i++)
    {
        x[i] = rand() * 1.0 / RAND_MAX;
        y[i] = rand() * 1.0 / RAND_MAX;
    }
    struct timespec start, finish;
    timespec_get(&start, TIME_UTC);
    Vec_add<<<blk_ct, th_per_blk>>>(x, y, z, n);
    cudaDeviceSynchronize();
    timespec_get(&finish, TIME_UTC);
    printf("GPU time = %f\n", finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9);
    timespec_get(&start, TIME_UTC);
    for (int i = 0; i < n; i++)
        cz[i] = x[i] + y[i];
    timespec_get(&finish, TIME_UTC);
    printf("CPU time = %f\n", finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9);
    double sum = 0;
    for (int i = 0; i < n; i++)
    {
        double diff = z[i] - cz[i];
        sum += diff * diff;
    }
    double diff_norm = sqrt(sum);
    printf("Two-norm of difference between host and device = %f\n", diff_norm);
    cudaFree(x);
    cudaFree(y);
    cudaFree(z);
    free(cz);
    return 0;
}
