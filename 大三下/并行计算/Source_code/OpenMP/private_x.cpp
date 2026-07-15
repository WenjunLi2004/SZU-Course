#include <iostream>
#include <omp.h>
using namespace std;
int main()
{
    int x = 5;
#pragma omp parallel num_threads(4) private(x)
    {
        int my_rank = omp_get_thread_num();
        printf("Thread %d > before initialization, x = %d\n", my_rank, x);
        x = 2 * my_rank + 2;
        printf("Thread %d > after initialization, x = %d\n", my_rank, x);
    }
    printf("After parallel block, x = %d\n", x);
}
