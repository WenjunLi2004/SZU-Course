#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>
double f(double x)
{
    double return_val;
    // return_val = x * x;
    return_val = 4 / (1 + x * x);
    return return_val;
}

int main(int argc, char *argv[])
{
    double global_result = 0.0;
    double a, b;
    int n, i;
    int thread_count = 4;
    if (argc > 1)
        thread_count = atoi(argv[1]);
    printf("Enter a, b, and n\n");
    scanf_s("%lf %lf %d", &a, &b, &n);
    double h = (b - a) / n;
#pragma omp parallel for num_threads(thread_count) reduction(+ : global_result)
    for (i = 1; i <= n - 1; i++)
    {
        double x = a + i * h;
        global_result += f(x);
    }

    printf("With n = %d trapezoids, our estimate\n", n);
    printf("of the integral from %f to %f = %.14e\n", a, b, global_result);
    return 0;
}
