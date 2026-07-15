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

    int *row_map = new int[n];
    int *inv_row_map = new int[n];
    for (int i = 0; i < n; i++)
    {
        row_map[i] = i;
        inv_row_map[i] = i;
    }

    double *pivot_buffer = new double[w];
    double *local_x_physical = new double[m];
    double *global_x_physical = new double[n];

    struct
    {
        double value;
        int index;
    } local_max, global_max;

    int base_row = rank * m;
    for (int k = 0; k < n; k++)
    {
        local_max.value = -1.0;
        local_max.index = -1;

        for (int local_row = 0; local_row < m; local_row++)
        {
            int physical_row = base_row + local_row;
            int logical_row = inv_row_map[physical_row];
            if (logical_row < k)
            {
                continue;
            }

            double val = fabs(a[local_row * w + k]);
            if (val > local_max.value)
            {
                local_max.value = val;
                local_max.index = logical_row;
            }
        }

        MPI_Allreduce(&local_max, &global_max, 1, MPI_DOUBLE_INT, MPI_MAXLOC, MPI_COMM_WORLD);

        int logical_pivot_row = global_max.index;
        int physical_at_pivot = row_map[logical_pivot_row];
        int physical_at_k = row_map[k];

        row_map[k] = physical_at_pivot;
        row_map[logical_pivot_row] = physical_at_k;
        inv_row_map[physical_at_pivot] = k;
        inv_row_map[physical_at_k] = logical_pivot_row;

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
            {
                pivot_row[j] *= inv_pivot;
            }
            MPI_Bcast(&pivot_row[k], w - k, MPI_DOUBLE, root, MPI_COMM_WORLD);
            pivot_data = pivot_row;
        }
        else
        {
            MPI_Bcast(&pivot_buffer[k], w - k, MPI_DOUBLE, root, MPI_COMM_WORLD);
            pivot_data = pivot_buffer;
        }

        if (rank == root)
        {
            for (int i = 0; i < local_pivot_row; i++)
            {
                double *row = &a[i * w];
                double factor = row[k];
                for (int j = k + 1; j < w; j++)
                {
                    row[j] -= factor * pivot_data[j];
                }
                row[k] = 0.0;
            }
            for (int i = local_pivot_row + 1; i < m; i++)
            {
                double *row = &a[i * w];
                double factor = row[k];
                for (int j = k + 1; j < w; j++)
                {
                    row[j] -= factor * pivot_data[j];
                }
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
                {
                    row[j] -= factor * pivot_data[j];
                }
                row[k] = 0.0;
            }
        }
    }

    for (int i = 0; i < m; i++)
    {
        local_x_physical[i] = a[i * w + n];
    }

    MPI_Allgather(local_x_physical, m, MPI_DOUBLE,
                  global_x_physical, m, MPI_DOUBLE, MPI_COMM_WORLD);

    for (int k = 0; k < n; k++)
    {
        x[k] = global_x_physical[row_map[k]];
    }

    delete[] row_map;
    delete[] inv_row_map;
    delete[] pivot_buffer;
    delete[] local_x_physical;
    delete[] global_x_physical;

    /*用户代码结束位置：只可在此补充代码，其他勿动*/
    MPI_Barrier(MPI_COMM_WORLD);
    double finish = MPI_Wtime();
    if (rank == 0)
        // cout << "execution time = " << finish - start << endl;
        cout << finish - start;
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
        // cout << "max difference = " << max_dif << endl;
        delete[] a;
    delete[] z;
    delete[] x;
    MPI_Finalize();
    return 0;
}