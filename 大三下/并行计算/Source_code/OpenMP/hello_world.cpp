#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
void Hello(void); /* 线程函数 */
int main(int argc, char *argv[])
{
    /* 从命令行中获得线程的数量 */
    int thread_count = atoi(argv[1]);
#pragma omp parallel num_threads(thread_count)
    Hello();
    return 0;
}
void Hello(void)
{
    int my_rank = omp_get_thread_num();
    int thread_count = omp_get_num_threads();
    printf("Hello from thread %d of %d\n", my_rank, thread_count);
}
