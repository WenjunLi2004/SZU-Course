/* File:     pass_by_ref2.cu
 * Purpose:  Second attempt to return a value from the device to the 
 *           host using C pass-by-reference.  This approach uses
 *           unified memory, and explicitly allocates storage for
 *           for the pointer.
 *
 * Compile:  nvcc -o pass_by_ref2 pass_by_ref2.cu
 * Run:      ./pass_by_ref2
 * 
 * Input:    None
 * Output:   The result of the addition.
 *
 * IPP2:     6.9 (pp. 312 and ff.)
 */

#include <stdio.h>
#include <cuda.h>

__global__ void Add(int x, int y, int* sum_p) { 
   *sum_p = x + y;
} /* Add */

int main(void) { 
   int* sum_p;

   cudaMallocManaged(&sum_p, sizeof(int));
   *sum_p = −5;
   Add <<<1, 1>>> (2, 3, sum_p); 
   cudaDeviceSynchronize ( ) ; 
   printf("The sum is %d\n", *sum_p); 
   cudaFree ( sum_p ) ;

   return 0; 
}

