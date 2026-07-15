#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
double f(int i)
{
    int j, start = i * (i + 1) / 2, finish = start + i;
    double return_val = 0.0;
    for (j = start; j <= finish; j++)
    {
        return_val += sin(j);
    }
    return return_val;
}
int main(int argc, char *argv[])
{
    int thread_count = 4;
    int n = 20000;
    double start, finish;
    if (argc > 1)
        thread_count = atoi(argv[1]);
        
    start = omp_get_wtime();
    double sum = 0.0;
#pragma omp parallel num_threads(thread_count) reduction(+ : sum)
    {
        int rank = omp_get_thread_num();
#pragma omp for // schedule(static, 1)
        for (int i = 0; i <= n; i++)
        {
            sum += f(i);
            // printf("iteration %d is assigned to thread %d\n", i, rank);
        }
    }
    finish = omp_get_wtime();
    printf("Elapsed time = %f seconds\n", finish - start);
    printf("sum = %f\n", sum);
    return 0;
}
