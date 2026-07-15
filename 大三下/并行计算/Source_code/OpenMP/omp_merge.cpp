#include <iostream>
#include <algorithm>
#include <omp.h>
int m = 1e8;
int n = 1e8;
using namespace std;
int main(void)
{
    double *a = new double[m];
    double *b = new double[n];
    double *c = new double[m + n];
    double *d = new double[m + n];
    srand(time(NULL));
    a[0] = rand() * 1.0 / RAND_MAX;
    for (int i = 1; i < m; i++)
        a[i] = a[i - 1] + rand() * 1.0 / RAND_MAX;
    b[0] = rand() * 1.0 / RAND_MAX;
    for (int i = 1; i < n; i++)
        b[i] = b[i - 1] + rand() * 1.0 / RAND_MAX;
    double start, finish, parallel_time, serial_time;
    start = omp_get_wtime();
    int p = 8;
    int *u = new int[p + 1];
    int *v = new int[p + 1];
    u[0] = v[0] = 0;
    u[p] = m;
    v[p] = n;
#pragma omp parallel num_threads(p)
    {
#pragma omp for
        for (int i = 1; i < p; i++)
        {
            u[i] = i * m / p;
            v[i] = upper_bound(b, b + n, a[u[i] - 1]) - b;
        }
#pragma omp for
        for (int i = 0; i < p; i++)
            merge(a + u[i], a + u[i + 1], b + v[i], b + v[i + 1], c + u[i] + v[i]);
    }
    finish = omp_get_wtime();
    parallel_time = finish - start;
    cout << "parallel time with " << p << " threads = " << parallel_time << endl;
    start = omp_get_wtime();
    merge(a, a + m, b, b + n, d);
    finish = omp_get_wtime();
    serial_time = finish - start;
    cout << "serial time = " << serial_time << endl;
    cout << "speedup = " << serial_time / parallel_time << endl;
    for (int i = 0; i < m + n; i++)
        if (c[i] != d[i])
            cout << "error" << endl;
    delete[] a;
    delete[] b;
    delete[] c;
    delete[] d;
    delete[] u;
    delete[] v;
}
