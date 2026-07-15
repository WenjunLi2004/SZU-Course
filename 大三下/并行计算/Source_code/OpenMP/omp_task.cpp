#include <iostream>
#include <omp.h>
int main()
{
#pragma omp parallel num_threads(4)
    {
#pragma omp single // 单线程生成任务，避免重复创建
        {
            std::cout << "主线程开始创建任务\n";
#pragma omp task // 创建任务1
            {
                printf("任务1由线程%d执行\n", omp_get_thread_num());
            }
#pragma omp task // 创建任务2
            {
                printf("任务2由线程%d执行\n", omp_get_thread_num());
            }
        }
    }
    return 0;
}
