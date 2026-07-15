/* File:     cuda_hello.cu
 * Purpose:  Implement ``hello, world'' on a gpu using CUDA.  
 *           Each thread started by the call to the kernel, 
 *           prints a message.
 *
 * Compile:  nvcc -o hello hello.cu 
 * Run:      ./hello  <number of threads>
 *
 * Input:    None
 * Output:   A message from each thread.
 *
 * Notes:    
 * 1.  Requires an Nvidia GPU with compute capability >= 2.0
 * 2.  This version starts only one block of threads.
 *
 * IPP2:     6.4.1 (pp. 296 and ff.)
 */

#include <stdio.h>
#include <cuda.h>   /* Header file for CUDA */

/* Device code:  runs on GPU */
__global__ void Hello(void) {

   printf("Hello from thread %d!\n", threadIdx.x);
}  /* Hello */


/* Host code:  Runs on CPU */
int main(int argc, char* argv[]) {
   int thread_count;     /* Number of threads to run on GPU */

   thread_count = strtol(argv[1], NULL, 10);  
                      /* Get thread_count from command line */

   Hello <<<1, thread_count>>>();  
                      /* Start thread_count threads on GPU, */

   cudaDeviceSynchronize();       /* Wait for GPU to finish */

   return 0;
}  /* main */


