/* File:     pass_by_ref2_glbl.cu
 * Purpose:  Return a value from the device to the device to the
 *           host using a global variable in CUDA managed memory.
 *
 * Compile:  nvcc -o pass_by_ref2_glbl pass_by_ref2_glbl.cu
 * Run:      ./pass_by_ref2_glbl
 * 
 * Input:    None
 * Output:   The result of the addition.
 *
 * IPP2:     6.9 (pp. 312 and ff.)
 */

#include <stdio.h>

__managed__ int sum ;

__global__ void Add(int x, int y) { 
   sum = x + y;
} /* Add */

int main(void) { 
   sum = −5;
   Add <<<1, 1>>> (2, 3);
   cudaDeviceSynchronize ( ) ;
   printf("After kernel: The sum is %d\n", sum);
   return 0;
}
