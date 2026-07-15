#include <iostream>
#include <omp.h>
using namespace std;
int fib(int n)
{
    int i = 0;
    int j = 0;
    if (n < 2)
        return n;
#pragma omp task shared(i) if (n > 20)
    i = fib(n - 1);
#pragma omp task shared(j) if (n > 20)
    j = fib(n - 2);
#pragma omp taskwait
    return i + j;
}
int main()
{
    int res;
    double start, finish;
    start = omp_get_wtime();
#pragma omp parallel num_threads(4)
#pragma omp single
    res = fib(30);
    finish = omp_get_wtime();
    cout << "fib(10) = " << res << endl;
    cout << "Elapsed time = " << finish - start << " seconds" << endl;
    return 0;
}
