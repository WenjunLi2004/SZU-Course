#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
void Mat_vect_mult(double local_A[], double local_x[], double local_y[], int n, int local_n)
{
    double *x = (double *)malloc(n * sizeof(double));
    // Allgather是关键：每个核心把自己的local_x发给所有核心，最终每个核心都能得到完整的x向量
    MPI_Allgather(local_x, local_n, MPI_DOUBLE, x, local_n, MPI_DOUBLE, MPI_COMM_WORLD);
    for (int i = 0; i < local_n; i++)
    {
        local_y[i] = 0.0;
        for (int j = 0; j < n; j++)
            local_y[i] += local_A[i * n + j] * x[j];
    }
    free(x);
}
int main(void)
{
    double *local_A;
    double *local_x;
    double *local_y;
    int n, local_n;
    int my_rank, comm_sz;
    double start, finish, loc_elapsed, elapsed;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_sz);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    if (my_rank == 0)
    {
        printf("Enter order of the matrix\n");
        fflush(stdout);
        scanf_s("%d", &n);
    }
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    local_n = n / comm_sz;
    local_A = (double *)malloc(local_n * n * sizeof(double));
    local_x = (double *)malloc(local_n * sizeof(double));
    local_y = (double *)malloc(local_n * sizeof(double));
    srand(my_rank);
    for (int i = 0; i < local_n; i++)
        for (int j = 0; j < n; j++)
            local_A[i * n + j] = ((double)rand()) / ((double)RAND_MAX);
    for (int i = 0; i < local_n; i++)
        local_x[i] = ((double)rand()) / ((double)RAND_MAX);
    MPI_Barrier(MPI_COMM_WORLD);
    start = MPI_Wtime();
    Mat_vect_mult(local_A, local_x, local_y, n, local_n);
    finish = MPI_Wtime();
    loc_elapsed = finish - start;
    MPI_Reduce(&loc_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    if (my_rank == 0)
        printf("Elapsed time = %e\n", elapsed);
    free(local_A);
    free(local_x);
    free(local_y);
    MPI_Finalize();
    return 0;
}
