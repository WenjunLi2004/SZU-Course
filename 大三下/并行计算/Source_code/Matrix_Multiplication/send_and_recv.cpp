#include "mpi.h"
#include <iostream>
using namespace std;

int main(void)
{
    const int n = 1000;
    double *a = new double[n * n];
    double *b = new double[n * n];
    double *c = new double[n * n];
    int rank, size;
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (rank == 0)
    {
        srand(0);
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                a[i * n + j] = rand() * 1.0 / RAND_MAX;
                b[i * n + j] = rand() * 1.0 / RAND_MAX;
            }
        }
    }
    double start = MPI_Wtime();

    int rows = n / size;
    int remain = n % size;
    int my_rows = rows + (rank < remain ? 1 : 0);
    int local_n = my_rows * n;

    double *local_a = new double[local_n];
    double *local_c = new double[local_n];
    for (int i = 0; i < local_n; i++)
        local_c[i] = 0.0;

    if (rank == 0)
    {
        for (int i = 0; i < local_n; i++)
        {
            local_a[i] = a[i];
        }
        int offset = local_n;
        for (int dest = 1; dest < size; dest++)
        {
            int dest_rows = rows + (dest < remain ? 1 : 0);
            int dest_n = dest_rows * n;
            MPI_Send(&a[offset], dest_n, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD);
            offset += dest_n;
        }
    }
    else
    {
        MPI_Recv(local_a, local_n, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    MPI_Bcast(b, n * n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    for (int i = 0; i < my_rows; i++)
    {
        for (int k = 0; k < n; k++)
        {
            double temp = local_a[i * n + k];
            for (int j = 0; j < n; j++)
            {
                local_c[i * n + j] += temp * b[k * n + j];
            }
        }
    }

    if (rank == 0)
    {
        for (int i = 0; i < local_n; i++)
            c[i] = local_c[i];
        int offset = local_n;
        for (int source = 1; source < size; source++)
        {
            int source_row = rows + (source < remain ? 1 : 0);
            int source_n = source_row * n;
            MPI_Recv(&c[offset], source_n, MPI_DOUBLE, source, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            offset += source_n;
        }
    }
    else
    {
        MPI_Send(local_c, local_n, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    }
    delete[] local_a;
    delete[] local_c;

    double finish = MPI_Wtime();
    if (rank == 0)
    {
        cout << "execution time = " << finish - start << endl;
        for (int i = 0; i < n; i++)
        {
            for (int j = 0; j < n; j++)
            {
                double s = 0;
                for (int k = 0; k < n; k++)
                    s += a[i * n + k] * b[k * n + j];
                if (s != c[i * n + j])
                    cout << "error" << endl;
            }
        }
    }
    MPI_Finalize();
    delete[] a;
    delete[] b;
    delete[] c;
    return 0;
}