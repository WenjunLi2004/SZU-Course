#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
const int m = 8000000;
const int n = 8;
double A[m][n];
double x[n];
double y[m];
int main(void)
{
    int i, j;
    double start, finish;
    printf("Matrix %d×%d\n", m, n);
    for (int thread_count = 1; thread_count <= 8; thread_count *= 2)
    {
        start = omp_get_wtime();
#pragma omp parallel for num_threads(thread_count) private(i, j)
        for (i = 0; i < m; i++)
        {
            y[i] = 0.0;
            for (j = 0; j < n; j++)
                y[i] += A[i][j] * x[j];
        }
        finish = omp_get_wtime();
        double elapsed = finish - start;
        printf("Elapsed time of %ld threads = %f seconds\n", thread_count, elapsed);
    }
    return 0;
}
