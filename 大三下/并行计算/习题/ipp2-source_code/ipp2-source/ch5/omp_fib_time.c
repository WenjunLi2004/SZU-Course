/* File:     omp_fib_time.c
 * Purpose:  OpenMP implementation of program for generating the
 *           first n fibonacci numbers.  Attempts to use task directive
 *           for recursive calls.  This version reports the execution
 *           time
 *
 * Compile:  gcc -g -Wall -o omp_fib omp_fib.c
 * Run:      ./omp_fib <n>
 *           n should be >= 1
 *
 * Input:    None
 * Output:   The first n fibonacci numbers and the time it took
 *           to generate them
 * IPP2:     5.10 (p. 273)
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include "timer.h"

int* fibs;

int fib(int fibs[], int n, int from) {
    int i = 0;
    int j = 0;

    if (n <= 1) return 1;

//#   pragma omp task shared(i)
       i = fib(fibs, n - 1, n);
#   pragma omp task shared(j) if(n > 20)
       j = fib(fibs, n - 2, n);
#   pragma omp taskwait
    fibs[n] = i + j;
    return fibs[n];
} /* fib */


int main(int argc, char* argv[]) {
   int n, i;
   double start, finish;

   if (argc != 2) {
      fprintf(stderr, "usage: %s <n>\n", argv[0]);
      exit(0);
   }

   n = strtol(argv[1], NULL, 10);
   fibs = malloc((n+1)*sizeof(int));
   fibs[0] = fibs[1] = 1;

   GET_TIME(start);
#  pragma omp parallel
#     pragma omp single
      fib(fibs, n, n+1);
   GET_TIME(finish);

   for (i = 0; i <= n; i++)
      printf("fibs[%d] = %d\n", i, fibs[i]);

   printf("Elapsed time = %e secs\n", finish-start);

   free(fibs);
   return 0;
}  /* main */

