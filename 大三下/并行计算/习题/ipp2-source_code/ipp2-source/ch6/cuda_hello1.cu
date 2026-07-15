/* File:     cuda_hello1.cu
 * Purpose:  Implement ``hello, world'' on a gpu using CUDA.  
 *           Each thread started by the call to the kernel, 
 *           prints a message.  This version can start multiple
 *           thread blocks.
 *
 * Compile:  nvcc -o hello hello.cu 
 * Run:      ./hello <number of thread blocks> <number of threads>
 *
 * Input:    None
 * Output:   A message from each thread.
 *
 * Note:     Requires an Nvidia GPU with compute capability >= 2.0
 *
 * IPP2:     6.6 (pp. 299 and ff.)
 */

#include <stdio.h>
#include <cuda.h>   /* Header file for CUDA */

/* Device code:  runs on GPU */
__global__ void Hello(void) {

   printf("Hello from thread %d in block %d\n", 
         threadIdx.x, blockIdx.x);
}  /* Hello */


/* Host code:  Runs on CPU */
int main(int argc, char* argv[]) {  
   int blk_ct;                /* Number of thread blocks */
   int th_per_blk;     /* Number of threads in each block */

   blk_ct = strtol(argv[1], NULL, 10);  
                  /* Get number of blocks from command line */
   th_per_blk = strtol(argv[2], NULL, 10);  
       /* Get number of threads per block from command line */

   Hello <<<blk_ct, th_per_blk>>>();
            /* Start blk_ct*th_per_blk threads on GPU, */

   cudaDeviceSynchronize();       /* Wait for GPU to finish */

   return 0;
}  /* main */

