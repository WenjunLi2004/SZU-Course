#include <iostream>
#include <omp.h>
using namespace std;
const int n = 20;
int main()
{
    int i;
    int fibo[n];
    fibo[0] = fibo[1] = 1;
#pragma omp parallel for num_threads(4) private(i)
    for (i = 2; i < n; i++)
        fibo[i] = fibo[i - 1] + fibo[i - 2];
    for (i = 0; i < n; i++)
        cout << fibo[i] << endl;
}
