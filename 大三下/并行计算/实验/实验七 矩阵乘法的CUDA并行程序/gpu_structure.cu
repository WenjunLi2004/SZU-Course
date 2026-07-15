#include <stdio.h>
#include <cuda.h>
__global__ void Hello(void)
{
    printf("Hello from thread(%d,%d,%d) in block(%d,%d,%d)\n", threadIdx.x, threadIdx.y, threadIdx.z, blockIdx.x, blockIdx.y, blockIdx.z);
}
int main(void)
{
    dim3 grid_dims, block_dims;
    grid_dims.x = 1;
    grid_dims.y = 2;
    grid_dims.z = 3;
    block_dims.x = 2;
    block_dims.y = 2;
    block_dims.z = 2;
    Hello<<<grid_dims, block_dims>>>();
    cudaDeviceSynchronize();
    return 0;
}
