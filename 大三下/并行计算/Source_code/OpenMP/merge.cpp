#include <iostream>
#include <algorithm>
int m = 1e8;
int n = 1e8;
using namespace std;
int main(void)
{
    double *a = new double[m];
    double *b = new double[n];
    double *c = new double[m + n];
    struct timespec start, finish;
    int i, j, k;
    srand(time(NULL));
    a[0] = rand() * 1.0 / RAND_MAX;
    for (i = 1; i < m; i++)
        a[i] = a[i - 1] + rand() * 1.0 / RAND_MAX;
    b[0] = rand() * 1.0 / RAND_MAX;
    for (j = 1; j < n; j++)
        b[j] = b[j - 1] + rand() * 1.0 / RAND_MAX;
    timespec_get(&start, TIME_UTC);
    i = j = k = 0;
    while (1)
    {
        if (a[i] < b[j])
        {
            c[k++] = a[i++];
            if (i >= m)
                break;
        }
        else
        {
            c[k++] = b[j++];
            if (j >= n)
                break;
        }
    }
    while (i < m)
        c[k++] = a[i++];
    while (j < n)
        c[k++] = b[j++];
    timespec_get(&finish, TIME_UTC);
    cout << "serial time = " << finish.tv_sec - start.tv_sec + (finish.tv_nsec - start.tv_nsec) / 1e9 << endl;
    delete[] a;
    delete[] b;
    delete[] c;
}
