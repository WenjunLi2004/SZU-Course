#include "mpi.h"
#include <iostream>
#include <cmath>
#include <algorithm>
using namespace std;
int main(void)
{
	int rank, p;
	int n = 2048;
	MPI_Init(NULL, NULL);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);
	int m = n / p;
	int w = n + 1;
	double *a = new double[m * w]; // a[i*w+n]表示b[i]
	double *z = new double[m * w]; // 验证需要
	double *x = new double[n];
	for (int i = 0; i < m; i++)
	{
		srand((rank * m + i) * 10);
		for (int j = 0; j < w; j++)
			a[i * w + j] = z[i * w + j] = rand() * 1.0 / RAND_MAX;
	}
	double start = MPI_Wtime();
	/*用户代码开始位置：只可在此补充代码，其他勿动*/

	// 初始化映射：logical <-> physical
	int *row_map = new int[n];
	int *inv_row_map = new int[n]; // 反向映射：物理行号 -> 当前逻辑行号
	for (int i = 0; i < n; i++)
	{
		// 初始时逻辑行号与物理行号一致
		row_map[i] = i;
		inv_row_map[i] = i;
	}

	// 工作缓冲区
	double *pivot_buffer = new double[w];	   // 用于存储主元行的有效数据,大小为w(n+ 1)
	double *local_x_physical = new double[m];  // 存储进程内物理行的结果，大小为 m
	double *global_x_physical = new double[n]; // 存储所有物理行的结果，大小为 n

	// MPI_MAXLOC 使用 (value, index)
	struct
	{
		double value;
		int index;
	} local_max, global_max;

	int base_row = rank * m;
	// 高斯-约旦：按列 k 进行主元选择、归一化和全行消元
	for (int k = 0; k < n; k++)
	{
		// === A. 主元选择（部分选主元）===
		local_max.value = -1.0;
		local_max.index = -1;

		// 仅扫描本进程本地行；logical_row < k 的行已完成，不再参与
		for (int local_row = 0; local_row < m; local_row++)
		{
			int physical_row = base_row + local_row;
			int logical_row = inv_row_map[physical_row];
			if (logical_row < k)
				continue;
			double val = fabs(a[local_row * w + k]);
			if (val > local_max.value)
			{
				local_max.value = val;
				local_max.index = logical_row; // 记录逻辑行号
			}
		}

		// 全局归约：得到第 k 列绝对值最大的逻辑行号
		MPI_Allreduce(&local_max, &global_max, 1, MPI_DOUBLE_INT, MPI_MAXLOC, MPI_COMM_WORLD);

		// 逻辑交换：只交换映射，不搬动整行数据
		int logical_pivot_row = global_max.index;			// 主元逻辑行号
		int physical_at_pivot = row_map[logical_pivot_row]; // 主元行所在的物理行
		int physical_at_k = row_map[k];						// 当前第 k 步所在的物理行
		row_map[k] = physical_at_pivot;
		row_map[logical_pivot_row] = physical_at_k;
		inv_row_map[physical_at_pivot] = k;
		inv_row_map[physical_at_k] = logical_pivot_row;

		// === B. 主元行归一化并广播 ===
		int pivot_physical_row = row_map[k];
		int root = pivot_physical_row / m;
		int local_pivot_row = pivot_physical_row % m;
		double *pivot_data = nullptr;
		if (rank == root)
		{
			double *pivot_row = &a[local_pivot_row * w];
			double pivot_val = pivot_row[k];
			double inv_pivot = 1.0 / pivot_val;
			for (int j = k; j < w; j++)
				pivot_row[j] *= inv_pivot;
			MPI_Bcast(&pivot_row[k], w - k, MPI_DOUBLE, root, MPI_COMM_WORLD);
			pivot_data = pivot_row;
		}
		else
		{
			MPI_Bcast(&pivot_buffer[k], w - k, MPI_DOUBLE, root, MPI_COMM_WORLD);
			pivot_data = pivot_buffer;
		}

		// === C. 用主元行对其余行做消元===
		if (rank == root)
		{
			// 主元行所在进程需要跳过主元行本身，仅消去其上方行
			for (int i = 0; i < local_pivot_row; i++)
			{
				double *row = &a[i * w];
				double factor = row[k];
				for (int j = k + 1; j < w; j++)
					row[j] -= factor * pivot_data[j];
				row[k] = 0.0;
			}
			// 再消去主元行下方的所有本地行
			for (int i = local_pivot_row + 1; i < m; i++)
			{
				double *row = &a[i * w];
				double factor = row[k];
				for (int j = k + 1; j < w; j++)
					row[j] -= factor * pivot_data[j];
				row[k] = 0.0;
			}
		}
		else
		{
			for (int i = 0; i < m; i++)
			{
				double *row = &a[i * w];
				double factor = row[k];
				for (int j = k + 1; j < w; j++)
					row[j] -= factor * pivot_data[j];
				row[k] = 0.0;
			}
		}
	}
	// === D. 收集并还原解向量 ===
	// 1) 提取本地物理顺序结果（增广矩阵最后一列）
	for (int i = 0; i < m; i++)
		local_x_physical[i] = a[i * w + n]; // n 就是最后一列的索引，即常数列

	// 2) 汇总所有进程的物理顺序结果
	MPI_Allgather(local_x_physical, m, MPI_DOUBLE,
				  global_x_physical, m, MPI_DOUBLE, MPI_COMM_WORLD);

	// 3) 通过 row_map 从物理顺序还原逻辑顺序 x
	for (int k = 0; k < n; k++)
		x[k] = global_x_physical[row_map[k]];

	// 资源释放
	delete[] row_map;
	delete[] inv_row_map;
	delete[] pivot_buffer;
	delete[] local_x_physical;
	delete[] global_x_physical;

	/*用户代码结束位置：只可在此补充代码，其他勿动*/
	MPI_Barrier(MPI_COMM_WORLD);
	double finish = MPI_Wtime();
	if (rank == 0)
		cout << "execution time = " << finish - start << endl;
	// 验证开始
	double max_dif;
	double local_dif = 0;
	for (int i = 0; i < m; i++)
	{
		double dif = z[i * w + n];
		for (int j = 0; j < n; j++)
			dif -= z[i * w + j] * x[j];
		local_dif = max(local_dif, fabs(dif));
	}
	MPI_Reduce(&local_dif, &max_dif, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	if (rank == 0)
		cout << "max difference = " << max_dif << endl;
	delete[] a;
	delete[] z;
	delete[] x;
	MPI_Finalize();
	return 0;
}