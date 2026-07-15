/* File:     cuda_trap1.cu
 * Purpose:  Implement the trapezoidal on a GPU using CUDA.  This version
 *           uses atomicAdd to sum the threads' results.
 *
 * Compile:  nvcc -arch=sm_XX -o trap1 trap1.cu 
 *              XX >= 30 to allow unified memory use
 *              See note 4 below if you're saving the runtimes
 * Run:      ./trap1 <n> <a> <b> <blk_ct> <th_per_blk>
 *              n is the number of trapezoids
 *              a is the left endpoint
 *              b is the right endpoint
 *
 * Input:    None
 * Output:   Result of trapezoidal applied to f(x).
 *
 * Notes:
 * 1.  The function f(x) = x^2 + 1 is hardwired
 * 2.  The total number of threads must be >= n
 * 3.  Define SAVE_STATS to save the runtimes to a file named
 *     
 *         trap1-1048576-1024-1024.txt
 *
 *     Here trap1 is the name of the executable (argv[0]), 1048576
 *     is n, 1024 is both the block count and the number of threads
 *     per block.
 * 4.  If you're saving the runtimes, you'll need to link save_stats.o
 *     into the executable:
 *
 *        nvcc -arch=sm_XX -o trap1 trap1.cu save_stats.c
 *           XX >= 30 to allow use of unified memory
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"

#ifndef SAVE_STATS
#define ITERS 50
#else
#include "save_stats.h"
#endif

/*-------------------------------------------------------------------
 * Function:    f
 * Purpose:     The function we're integrating, callable from 
 *              host or device.
 */
__host__ __device__ float f(
      float  x  /* in */) {
   return x*x + 1;
// return sin(exp(x));
}  /* Dev_f */


/*-------------------------------------------------------------------
 * Function:    Dev_trap  (kernel)
 * Purpose:     Implement the trapezoidal rule
 *
 * Note:        The return value, *trap_p, has been initialized to 
 *
 *                 0.5*(f(a) + f(b))
 *
 *              on the host
 */
__global__ void Dev_trap(
      const float  a       /* in     */, 
      const float  b       /* in     */, 
      const float  h       /* in     */, 
      const int    n       /* in     */, 
      float*       trap_p  /* in/out */) {
   int my_i = blockDim.x * blockIdx.x + threadIdx.x;

   /* f(x_0) and f(x_n) were computed on the host.  So */
   /* compute f(x_1), f(x_2), ..., f(x_(n-1))          */
   if (0 < my_i && my_i < n) {
      float my_x = a + my_i*h;
      float my_trap = f(my_x);
      atomicAdd(trap_p, my_trap);
   }
}  /* Dev_trap */    


/*-------------------------------------------------------------------
 * Host code 
 */
void  Get_args(const int argc, char* argv[], int* n_p, 
      float* a_p, float* b_p,
      int* blk_ct, int* th_per_blk_p);
void  Trap_wrapper(const float a, const float b, const int n, 
      float* trap_p, const int blk_ct, const int th_per_blk);
float Serial_trap(const float a, const float b, const int n);
void  Update_stats(double start, double finish, 
      double* min_p, double* max_p, double* total_p);

/*-------------------------------------------------------------------
 * main
 */
int main(int argc, char* argv[]) {
   int n, blk_ct, th_per_blk;
   float a, b, *trap_p, trap;
   double start, finish;  /* Only used on host */
   double dmin = 1.0e6, dmax = 0.0, dtotal = 0.0;
   double hmin = 1.0e6, hmax = 0.0, htotal = 0.0;
   cudaError_t err;


   Get_args(argc, argv, &n, &a, &b, &blk_ct, &th_per_blk);
   err = cudaMallocManaged(&trap_p, sizeof(float));
   if (err != cudaSuccess) {
      fprintf(stderr, "cudaMallocManaged error = %s\n", 
            cudaGetErrorString(err));
      fprintf(stderr, "Quitting\n");
      exit(-1);
   }

   for (int iter = 0; iter < ITERS; iter++) {
      GET_TIME(start);
      Trap_wrapper(a, b, n, trap_p, blk_ct, th_per_blk);
      GET_TIME(finish);

//    printf("Elapsed time for cuda = %e seconds\n", finish-start);
      Update_stats(start, finish, &dmin, &dmax, &dtotal);
#     ifdef SAVE_STATS
      runtimes[iter] = finish-start;
#     endif
   
      GET_TIME(start)
      trap = Serial_trap(a, b, n);
      GET_TIME(finish);
      Update_stats(start, finish, &hmin, &hmax, &htotal);
//    printf("Elapsed time for cpu = %e seconds\n", finish-start);
   }
   printf("The area as computed by cuda is: %e\n", *trap_p);
   printf("The area as computed by cpu is: %e\n", trap);
   printf("Device times:  min = %e, max = %e, avg = %e\n",
         dmin, dmax, dtotal/ITERS);
   printf("  Host times:  min = %e, max = %e, avg = %e\n",
         hmin, hmax, htotal/ITERS);

#  ifdef SAVE_STATS
   Save_stats(argv[0], n, blk_ct, th_per_blk);
#  endif

   cudaFree(trap_p);

   return 0;
}  /* main */


/*-------------------------------------------------------------------
 * Function:  Get_args
 * Purpose:   Get command line args.  If there aren't the right
 *            number or there aren't enough threads, print a message 
 *            and quit.
 */
void Get_args(const int argc, char* argv[], int* n_p, 
      float* a_p, float* b_p, 
      int* blk_ct_p, int* th_per_blk_p) {

   if (argc != 6) {
      fprintf(stderr, "usage: %s <n> <a> <b> <blk_ct> <th_per_blk>\n", 
            argv[0]);
      exit(0);
   }
   *n_p = strtol(argv[1], NULL, 10);
   *a_p = strtod(argv[2], NULL);
   *b_p = strtod(argv[3], NULL);
   *blk_ct_p = strtol(argv[4], NULL, 10);
   *th_per_blk_p = strtol(argv[5], NULL, 10);
   if (*n_p > (*blk_ct_p)*(*th_per_blk_p)) {
      fprintf(stderr, "Number of threads must be >= n\n");
      exit(0);
   }
}  /* Get_args */


/*-------------------------------------------------------------------
 * Function:  Trap_wrapper
 * Purpose:   CPU wrapper function for GPU trapezoidal rule
 * Note:      Assumes trap_p has been allocated on host and
 *            device
 */
void Trap_wrapper(const float a, const float b, const int n, 
     float* trap_p,
     const int blk_ct, const int th_per_blk) {

   /* Invoke kernel */
   *trap_p = 0.5*(f(a) + f(b));
   float h = (b-a)/n;
   Dev_trap<<<blk_ct, th_per_blk>>>(a, b, h, n, trap_p);
   cudaDeviceSynchronize();
   *trap_p = h*(*trap_p);

}  /* Trap_wrapper */


/*-------------------------------------------------------------------
 * Function:  Serial_trap
 * Purpose:   Implement a single CPU trapezoidal rule
 */
float Serial_trap(
      const float  a  /* in */, 
      const float  b  /* in */, 
      const int    n  /* in */) {
   float x, h = (b-a)/n;
   float trap = 0.5*(f(a) + f(b));

   for (int i = 1; i <= n-1; i++) {
       x = a + i*h;
       trap += f(x);
   }
   trap = trap*h;
   
   return trap;
}  /* Serial_trap */


/*-------------------------------------------------------------------
 * Function:  Update_stats
 * Purpose:   Update timing stats
 */
void Update_stats(double start, double finish, 
      double* min_p, double* max_p, double* total_p) {
   double elapsed = finish - start;
   if (elapsed < *min_p) *min_p = elapsed;
   if (elapsed > *max_p) *max_p = elapsed;
   *total_p += elapsed;
}  /* Update_stats */
