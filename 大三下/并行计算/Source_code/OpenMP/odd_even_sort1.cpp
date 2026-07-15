#include <stdio.h>
#include <stdlib.h>
#include <utility>
#include <omp.h>
const int RMAX = 10000000;
int thread_count = 4;
int main(int argc, char *argv[])
{
    int n = 100000;
    double start, finish;
    if (argc > 1)
        thread_count = atoi(argv[1]);
    int *a = new int[n];
    int phase, i;
    srand(1);
    for (i = 0; i < n; i++)
        a[i] = rand() % RMAX;

    start = omp_get_wtime();
    for (phase = 0; phase < n; phase++)
    {
        if (phase % 2 == 0)
        // 频繁地创建和销毁线程会导致性能下降
#pragma omp parallel for num_threads(thread_count) default(none) shared(a, n) private(i)
            for (i = 1; i < n; i += 2)
            {
                if (a[i - 1] > a[i])
                    std::swap(a[i - 1], a[i]);
            }
        else
#pragma omp parallel for num_threads(thread_count) default(none) shared(a, n) private(i)
            for (i = 1; i < n - 1; i += 2)
            {
                if (a[i] > a[i + 1])
                    std::swap(a[i], a[i + 1]);
            }
    }
    finish = omp_get_wtime();
    printf("Elapsed time = %e seconds\n", finish - start);
    delete[] a;
    return 0;
}
