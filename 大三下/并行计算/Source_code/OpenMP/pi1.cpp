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
#pragma omp parallel for num_threads(2) reduction(+ : sum)
    for (k = 0; k < n; k++)
    {
        sum += factor / (2 * k + 1);
        factor = -factor;
    }
    double pi_approx = 4.0 * sum;
    cout << setprecision(15) << pi_approx << endl;
}
