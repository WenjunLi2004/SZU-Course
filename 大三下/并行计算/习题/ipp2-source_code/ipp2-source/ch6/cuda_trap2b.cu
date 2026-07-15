/* File:     trap2b.cu
 * Purpose:  Implement the trapezoidal rule on a GPU using CUDA.  This 
 *           version assumes that the block size is the same as the warp
 *           size.  It uses shared memory to add the values within a warp. 
 *           This version also adds the warp sums using atomicAdd.
 *           We assume that (blk_ct-1)*32 < n <= blk_ct*32.  This insures 
 *           that every block gets some work.
 *
 * Compile:  nvcc -arch=sm_XX -o trap2b trap2b.cu 
 *              XX >= 3.0
 * Run:      ./trap2b <n> <a> <b> <blk_ct>
 *              n is the number of trapezoids
 *              a is the left endpoint
 *              b is the right endpoint
 *
 * Input:    None
 * Output:   Result of trapezoidal applied to f(x).
 *
 * Notes:
 * 1.  The function f(x) = x^2 + 1 is hardwired
 * 2.  The warp size is hard coded as a preprocessor macro WARPSZ.
 * 3.  The total number of threads is blk_ct*WARPSZ, and 
 *     blk_ct should be chosen to satisfy
 *
 *        (blk_ct-1)*WARPSZ < n <= blk_ct*WARPSZ
 *     
 *     So every warp will do some meaningful work.
 *
 * Runtimes 3/XX/17 n = 2^20, a = -3, b = 3, blk_ct = 32768
 *    -arch=sm_32 on jet, -arch=sm_52 on msan
 * jet22 cpu:  20.8   ms
 * jet22 gpu:   3.62  ms  
 * msan cpu:    4.47  ms
 * msan gpu:    0.169 ms  
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"

#ifdef DEBUG
#define ITERS 1
#else
#  ifdef SAVE_STATS
#  include "save_stats.h"
#  else
#  define ITERS 50
#  endif
#endif

/* Make the warp size available on the host and at compile time */
#define WARPSZ 32 

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
 * Function:   Shared_mem_sum
 * Purpose:    Dissemination reduction sum of the values in a warp
 *             stored in shared memory
 *
 */
__device__ float Shared_mem_sum(float shared_vals[]) {
   int my_lane = threadIdx.x % warpSize;

   /* At the start diff = 10000 (binary), then diff = 01000 (binary), etc */
   for (unsigned diff = 16; diff > 0; diff >>= 1) {
      int source = (my_lane + diff) % warpSize;
      shared_vals[my_lane] += shared_vals[source];
   }

   return shared_vals[my_lane];
}  /* Shared_mem_sum */


/*-------------------------------------------------------------------
 * Function:    Dev_trap  (kernel)
 * Purpose:     Implement the trapezoidal rule.  This version
 *              assumes each block is a single warp.  It
 *              computes the sum of the results computed within
 *              a block, and then uses atomicAdd to sum the
 *              results computed by the blocks.
 */
__global__ void Dev_trap(
      const float  a         /* in  */, 
      const float  b         /* in  */, 
      const float  h         /* in  */, 
      const int    n         /* in  */,
      float*       trap_p    /* out */) {
   __shared__ float shared_vals[WARPSZ];
   int my_i = blockDim.x * blockIdx.x + threadIdx.x;
   int my_lane = threadIdx.x % warpSize;
   
   shared_vals[my_lane] = 0.0f;
   if (0 < my_i && my_i < n) {
      float my_x = a + my_i*h;
      shared_vals[my_lane] = f(my_x);
   }

   float result = Shared_mem_sum(shared_vals);

   /* result is the same on all threads in a block.  Ignore result 
      unless threadIdx.x = 0 */
   if (threadIdx.x == 0) {
#     ifdef DEBUG
      printf("GPU:  th = %d, add = %e\n", my_i, result);
#     endif
      atomicAdd(trap_p, result);
   }
}  /* Dev_trap */    


/*-------------------------------------------------------------------
 * Host code 
 */
void  Get_args(const int argc, char* argv[], int* n_p, 
      float* a_p, float* b_p, int* blk_ct);
void  Trap_wrapper(const float a, const float b, const int n, 
      float* trap_p, const int blk_ct);
float Serial_trap(const float a, const float b, const int n);
void  Update_stats(double start, double finish, 
      double* min_p, double* max_p, double* total_p);
float Simulate_trap(
      const float  a           /* in */, 
      const float  b           /* in */, 
      const int    n           /* in */,
      const int    blk_ct      /* in */);


/*-------------------------------------------------------------------
 * main
 */
int main(int argc, char* argv[]) {
   int n, blk_ct;
   float a, b, *trap_p, strap;
   double start, finish;  
   double dmin = 1.0e6, dmax = 0.0, dtotal = 0.0;
   double hmin = 1.0e6, hmax = 0.0, htotal = 0.0;

   Get_args(argc, argv, &n, &a, &b, &blk_ct);
   cudaMallocManaged(&trap_p, sizeof(float));

   for (int iter = 0; iter < ITERS; iter++) {
      GET_TIME(start);
      Trap_wrapper(a, b, n, trap_p, blk_ct);
      GET_TIME(finish);

//    printf("Elapsed time for cuda = %e seconds\n", finish-start);
      Update_stats(start, finish, &dmin, &dmax, &dtotal);
#     ifdef SAVE_STATS
      runtimes[iter] = finish-start;
#     endif
   
      GET_TIME(start)
      strap = Serial_trap(a, b, n);
      GET_TIME(finish);
      Update_stats(start, finish, &hmin, &hmax, &htotal);
//    printf("Elapsed time for cpu = %e seconds\n", finish-start);
   }
   printf("The area as computed by cuda is:         %e\n", *trap_p);
   printf("The area as computed by simulated gpu is %e\n", 
         Simulate_trap(a, b, n, blk_ct));
   printf("The area as computed by cpu is:          %e\n", strap);
   printf("Device times:  min = %e, max = %e, avg = %e\n",
         dmin, dmax, dtotal/ITERS);
   printf("  Host times:  min = %e, max = %e, avg = %e\n",
         hmin, hmax, htotal/ITERS);

#  ifdef SAVE_STATS
   Save_stats(argv[0], n, blk_ct, 32);
#  endif
   cudaFree(trap_p);

   return 0;
}  /* main */

/*-------------------------------------------------------------------
 * Function:  Get_args
 * Purpose:   Get command line args.  If there aren't the right
 *            number or the total number of threads doesn't satisfy
 *
 *               (blk_ct-1) * WARPSZ < n <= blk_ct*WARPSZ,
 *
 *            then print a message and quit.
 */
void Get_args(const int argc, char* argv[], int* n_p, 
      float* a_p, float* b_p, int* blk_ct_p) {
   int n, blk_ct;

   if (argc != 5) {
      fprintf(stderr, "usage: %s <n> <a> <b> <blk_ct>\n", 
            argv[0]);
      exit(0);
   }
   n = strtol(argv[1], NULL, 10);
   *a_p = strtod(argv[2], NULL);
   *b_p = strtod(argv[3], NULL);
   blk_ct = strtol(argv[4], NULL, 10);

   if ((n <= (blk_ct-1)*WARPSZ) ||
       (n >  blk_ct*WARPSZ)) {
      fprintf(stderr, "blk_ct should be chosen so that\n");
      fprintf(stderr, "(blk_ct-1)*%d < n <= blk_ct*%d\n",
            WARPSZ, WARPSZ);
      exit(0);
   }
   *n_p = n;
   *blk_ct_p = blk_ct;
}  /* Get_args */


/*-------------------------------------------------------------------
 * Function:  Trap_wrapper
 * Purpose:   CPU wrapper function for GPU trapezoidal rule
 * Note:      Assumes trap_p has been allocated on host and
 *            device
 */
void Trap_wrapper(const float a, const float b, const int n, 
     float* trap_p, const int blk_ct) {

   /* Invoke kernel */
   *trap_p = 0.5*(f(a) + f(b));
   float h = (b-a)/n;
   Dev_trap<<<blk_ct, WARPSZ>>>(a, b, h, n, trap_p);
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
#      ifdef DEBUG
       printf("Ser:  i = %d, x = %e, f(x) = %e\n", i, x, f(x));
#      endif
   }
   trap = trap*h;
   
   return trap;
}  /* Serial_trap */


/*-------------------------------------------------------------------
 * Function:  Simulate_trap
 * Purpose:   Simulate the GPU computation of the trapezoidal rule
 * Note:      This only implements a tree-structured sum to lane 0
 *            in each "warp," since only the final result in lane
 *            0 is used.  So it does not implement a full butterfly
 *            structured sum across each warp.
 * Note:      This version of the GPU sum is nondeterministic, since
 *            the order in which the warp results are added into the
 *            the global sum is nondeterministic.  So the simulated
 *            result may be quite different from the gpu result
 */
float Simulate_trap(
      const float  a           /* in */, 
      const float  b           /* in */, 
      const int    n           /* in */,
      const int    blk_ct      /* in */) {
   float x, h = (b-a)/n;
   float temp[WARPSZ];
   float trap = 0.0f;
   unsigned lane;
   int th;
#  ifdef DEBUG
   printf("Sim:  a = %.1f, b = %.1f, n = %d, blk_ct = %d\n",
         a, b, n, blk_ct);
#  endif

   trap += 0.5*(f(a) + f(b));
   for (int blk = 0; blk < blk_ct; blk++) {
      int first = blk*WARPSZ;
      int last = first + WARPSZ;
      for (th = first; th < last; th++) {
         lane = th % WARPSZ;
         if (0 < th && th < n) {
            x = a + th*h;
            temp[lane] = f(x);
         } else {
            temp[lane] = 0.0f;
         }
#        ifdef DEBUG
         printf("Sim: blk = %d, warp = %d, th = %d, x = %e, f(x) = %e\n",
                  blk, wrp, th, x, temp[lane]);
#        endif
      }  /* for th */

      /* Dissemination sum to lane 0 */
      for (unsigned diff = WARPSZ >> 1; diff > 0; 
            diff >>= 1) {
         /* Ignore lanes past diff */
         for (lane = 0; lane < diff; lane++) {
            int source = (lane + diff) % WARPSZ;
            temp[lane] += temp[source];
         }
      }  /* for bit_mask */
#     ifdef DEBUG
      printf("Sim: warp = %d, th = %d, add = %e\n", 
            wrp, first, temp[0]);
#     endif

      trap += temp[0];
   }  /* for blk */
   trap = trap*h;
   
   return trap;
}  /* Simulate_trap */


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
