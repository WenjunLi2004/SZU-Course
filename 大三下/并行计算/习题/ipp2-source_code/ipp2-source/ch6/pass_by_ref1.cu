/* File:     pass_by_ref1.cu
 * Purpose:  Attempt to return a value from the device to the host using
 *           C pass-by-reference.  This approach does *not* work.
 *
 * Compile:  nvcc -o pass_by_ref1 pass_by_ref1.cu
 * Run:      ./pass_by_ref1
 * 
 * Input:    None
 * Output:   Depends on system
 *
 * IPP2:     6.9 (p. 312)
 */
#include <stdio.h>

__global__ void Add(int x, int y, int* sum_p) { 
   *sum_p = x + y;
} /* Add */

int main(void) {
   int sum = −5;

   Add <<<1, 1>>> (2, 3, &sum); 
   cudaDeviceSynchronize ( ) ; 
   printf("The sum is %d\n", sum);

   return 0; 
}  /* main */
