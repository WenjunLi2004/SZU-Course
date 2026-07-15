/* File:     omp_broken_fib2.c
 * Purpose:  Second attempt to parallelize the Fibonacci number 
 *           computation using task directives
 *
 * Compile:  gcc -g -Wall fopenmp -o omp_broken_fib2 omp_broken_fib2.c
 * Run:      ./omp_broken_fib2 <n>
 *           n should be >= 1
 *
 * Input:    None
 * Output:   Probably incorrect values for the first n fibonacci numbers
 *
 * IPP2:     5.10 (pp. 272 and ff.)
 */ 
#include <stdio.h>
#include <stdlib.h>

int* fibs;

int fib(int n) {
    int i = 0;
    int j = 0;

    if (n <= 1) return 1;

#   pragma omp task shared(i)
    i = fib(n - 1);
#   pragma omp task shared(j)
    j = fib(n - 2);
    fibs[n] = i + j;
    return fibs[n];
} /* fib */

int main(int argc, char* argv[]) {
   int n;

   if (argc != 2) {
      fprintf(stderr, "usage: %s <n>\n", argv[0]);
      exit(0);
   }

   n = strtol(argv[1], NULL, 10);
   fibs = malloc((n+1)*sizeof(int));
   fibs[0] = fibs[1] = 1;

#  pragma omp parallel
#  pragma omp single
   /* Ignore return value */
   fib(n);

   for (int i = 0; i <= n; i++)
      printf("fibs[%d] = %d\n", i, fibs[i]);

   free(fibs);
   return 0;
}  /* main */

