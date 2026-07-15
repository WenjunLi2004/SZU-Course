/* File:     pass_by_ref2_cp.cu
 * Purpose:  Third attempt to return a value from the device to the 
 *           host using C pass-by-reference.  This approach explicitly
 *           allocates storage on the host and device, and copies
 *           the device result back to the host.
 *
 * Compile:  nvcc -o pass_by_ref2_cp pass_by_ref2_cp.cu
 * Run:      ./pass_by_ref2_dp
 * 
 * Input:    None
 * Output:   The result of the addition
 *
 * IPP2:     6.9 (pp. 312 and ff.)
 */

#include <stdio.h>
#include <cuda.h>

__global__ void Add(int x, int y, int *sum_p) { 
   *sum_p = x + y;
} /* Add */

int main(void) {
   int *hsum_p , *dsum_p ;
   hsum_p = (int*) malloc(sizeof(int)); 
   cudaMalloc(&dsum_p , sizeof ( int ));
   *hsum_p = −5;
   Add <<<1, 1>>> (2, 3, dsum_p);
   cudaMemcpy(hsum_p , dsum_p , sizeof ( int ) , cudaMemcpyDeviceToHost ); 
   printf("The sum is %d\n", *hsum_p);
   free ( hsum_p ) ;
   cudaFree ( dsum_p ) ;
   return 0; 
}
