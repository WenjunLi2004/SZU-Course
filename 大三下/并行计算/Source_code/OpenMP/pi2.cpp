#include <iostream>
#include <iomanip>
#include <omp.h>
using namespace std;
int main()
{
    int k;
    int n = 100000000;
    double factor = 1.0;
    double sum = 0.0;
#pragma omp parallel for num_threads(4) reduction(+ : sum) private(factor)
    for (k = 0; k < n; k++)
    {
        if (k % 2 == 0)
            factor = 1.0;
        else
            factor = -1.0;
        sum += factor / (2 * k + 1);
    }
    double pi_approx = 4.0 * sum;
    cout << setprecision(15) << pi_approx << endl;
}
