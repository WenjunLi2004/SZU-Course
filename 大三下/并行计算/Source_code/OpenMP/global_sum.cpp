#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
using namespace std;
int main(void)
{
    int k;
    int n = 100000000;
    double factor;
    double sum;
    double my_sum[1000];
    double start, finish, elapsed;
    for (int thread_count = 1; thread_count <= 8; thread_count *= 2)
    {
        sum = 0.0;
        for (int i = 0; i < 1000; i++)
            my_sum[i] = 0.0;
        start = omp_get_wtime();
#pragma omp parallel num_threads(thread_count) private(factor, k)
        {
            int rank = omp_get_thread_num();
#pragma omp for
            for (k = 0; k < n; k++)
            {
                if (k % 2 == 0)
                    factor = 1.0;
                else
                    factor = -1.0;
                my_sum[rank] += factor / (2 * k + 1);
            }
        }
        finish = omp_get_wtime();
        elapsed = finish - start;
        for (int i = 0; i < 1000; i++)
            sum += my_sum[i];
        sum = 4.0 * sum;
        printf("thread_count = %d:\n", thread_count);
        printf("Multi-threaded estimate of pi  = %.15f\n", sum);
        printf("Elapsed time = %f seconds\n", elapsed);
    }
}
