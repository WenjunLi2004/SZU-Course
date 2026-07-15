#include <stdio.h>
#include <cuda.h>
__global__ void Hello(void)
{
    printf("Hello from thread %d in block %d\n", threadIdx.x, blockIdx.x);
}
int main(void)
{
    int blk_ct = 2;
    int th_per_blk = 4;
    Hello<<<blk_ct, th_per_blk>>>();
    cudaDeviceSynchronize();
    return 0;
}
