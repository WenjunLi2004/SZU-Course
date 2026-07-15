#include <stdio.h>
#include <cuda.h>

// 矩阵乘法 kernel：每个线程负责计算结果矩阵 dz 中的一个元素。
__global__ void Matrix_mult(const float dx[], const float dy[], float dz[], const int n)
{
	// 将二维 block/grid 坐标映射到矩阵的行 r 和列 c。
	int r = blockIdx.y * blockDim.y + threadIdx.y;
	int c = blockIdx.x * blockDim.x + threadIdx.x;

	// 处理矩阵规模不能被 block 大小整除时的边界线程。
	if (r < n && c < n)
	{
		float temp = 0.0;
		for (int i = 0; i < n; i++)
		{
			// dz[r][c] = dx 的第 r 行与 dy 的第 c 列的点积。
			temp += dx[r * n + i] * dy[i * n + c];
		}
		dz[r * n + c] = temp;
	}
}

// 对数组 a 的所有元素求和，用于统计 GPU 结果矩阵中所有元素之和。
__global__ void Sum(const float a[], int n, float *sum_p)
{
	int idx = blockIdx.x * blockDim.x + threadIdx.x;
	int stride = blockDim.x * gridDim.x;

	float temp = 0.0;

	// grid-stride 循环使线程总数不足时仍能覆盖整个数组。
	for (int i = idx; i < n; i += stride)
	{
		temp += a[i];
	}

	// 使用 warp 内 shuffle 指令对同一 warp 内线程的局部和做归约。
	unsigned mask = 0xffffffff;
	for (int offset = warpSize / 2; offset > 0; offset /= 2)
	{
		// 每一轮从相距 offset 的线程取值并累加，最终每个 warp 的 0 号 lane 得到总和。
		temp += __shfl_down_sync(mask, temp, offset);
	}

	// 由每个 warp 的首线程写入全局和，减少原子操作次数。
	if (threadIdx.x % warpSize == 0)
	{
		atomicAdd(sum_p, temp);
	}
}

int main(void)
{
	// 矩阵阶数，三个矩阵均按一维数组连续存储。
	int n = 1024;
	float *x, *y, *z, *cz;
	float *dx, *dy, *dz;
	float sum, *sum_p;

	// 分配主机端矩阵空间：x、y 为输入矩阵，z 为 GPU 结果，cz 为 CPU 参考结果。
	x = (float *)malloc(n * n * sizeof(float));
	y = (float *)malloc(n * n * sizeof(float));
	z = (float *)malloc(n * n * sizeof(float));
	cz = (float *)malloc(n * n * sizeof(float));

	// 分配设备端矩阵空间，并使用统一内存保存 GPU 端归约结果。
	cudaMalloc(&dx, n * n * sizeof(float));
	cudaMalloc(&dy, n * n * sizeof(float));
	cudaMalloc(&dz, n * n * sizeof(float));
	cudaMallocManaged(&sum_p, sizeof(float));

	// 固定随机种子，便于多次运行时得到可复现的输入数据。
	srand(0);
	for (int i = 0; i < n * n; i++)
	{
		x[i] = rand() * 1.0 / RAND_MAX;
		y[i] = rand() * 1.0 / RAND_MAX;
		cz[i] = 0;
	}

	struct timespec start, finish;

	// 将输入矩阵从主机端复制到设备端。
	cudaMemcpy(dx, x, n * n * sizeof(float), cudaMemcpyHostToDevice);
	cudaMemcpy(dy, y, n * n * sizeof(float), cudaMemcpyHostToDevice);

	// 二维线程块用于矩阵乘法，便于直接对应矩阵的行和列。
	dim3 block_dim1(16, 16);
	dim3 grid_dim1((n + block_dim1.x - 1) / block_dim1.x,
					(n + block_dim1.y - 1) / block_dim1.y);

	// 一维线程布局用于对结果矩阵做并行归约求和。
	int block_dim2 = 256;
	int grid_dim2 = (n * n + block_dim2 - 1) / block_dim2;

	// GPU 计时：包含矩阵乘法、GPU 归约以及结果复制回主机的时间。
	timespec_get(&start, TIME_UTC);
	Matrix_mult<<<grid_dim1, block_dim1>>>(dx, dy, dz, n);
	cudaDeviceSynchronize();

	// 归约前清零统一内存中的累加结果。
	*sum_p = 0;
	Sum<<<grid_dim2, block_dim2>>>(dz, n * n, sum_p);
	cudaDeviceSynchronize();
	cudaMemcpy(z, dz, n * n * sizeof(float), cudaMemcpyDeviceToHost);
	timespec_get(&finish, TIME_UTC);
	printf("GPU time = %f\n", finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9);

	// CPU 串行矩阵乘法，作为结果正确性的参考。
	timespec_get(&start, TIME_UTC);
	for (int i = 0; i < n; i++)
		for (int k = 0; k < n; k++)
			for (int j = 0; j < n; j++)
				cz[i * n + j] += x[i * n + k] * y[k * n + j];

	// 计算 CPU 结果矩阵的元素总和，用于与 GPU 归约结果比较。
	sum = 0;
	for (int i = 0; i < n * n; i++)
		sum += cz[i];
	timespec_get(&finish, TIME_UTC);
	printf("CPU time = %f\n", finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9);

	// 用二范数衡量 GPU 与 CPU 计算结果之间的整体误差。
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

	// 释放主机端和设备端内存。
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
