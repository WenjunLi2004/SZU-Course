/* File:     serial_fib.c
 * Purpose:  Serial implementation of program for generating the
 *           first n fibonacci numbers
 *
 * Compile:  gcc -g -Wall -o serial_fib serial_fib.c
 * Run:      ./serial_fib <n>
 *           n should be >= 1
 *
 * Input:    None
 * Output:   The first n fibonacci numbers
 *
 * IPP2:     5.10 (p. 271 and ff.)
 */ 
#include <stdio.h>
#include <stdlib.h>

int* fibs;

int fib(int n) {
    int i = 0;
    int j = 0;

    if (n <= 1) return 1;

    i = fib(n - 1);
    j = fib(n - 2);
    printf("Assigning fibs[%d] = %d\n", n, i+j);
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
   fibs[0] = 1;
   fibs[1] = 1;

   /* Ignore return value */
   fib(n);

   for (int i = 0; i <= n; i++)
      printf("fibs[%d] = %d\n", i, fibs[i]);

   free(fibs);
   return 0;
}  /* main */

