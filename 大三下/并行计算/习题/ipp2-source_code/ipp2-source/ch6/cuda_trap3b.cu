/* File:     trap3b.cu
 * Purpose:  Implement the trapezoidal rule on a GPU using CUDA.  This 
 *           version uses shared memory to add the values within a warp. 
 *           It then uses shared memory to add the warp sums in a block.
 *           It adds the block sums using atomicAdd.  
 *
 *           We assume that
 *
 *              (blk_ct-1)*th_per_blk < n <= blk_ct*th_per_blk.  
 * 
 *           This insures that every block gets some work.  It also 
 *           assumes that the maximum block size is 1024 and the block 
 *           size is a multiple of the warp size.
 *
 * Compile:  nvcc -o trap3b trap3b.cu -arch=sm_32 
 *              arch must be >= 3.2
 * Run:      ./trap3b <n> <a> <b> <blk_ct> <th_per_blk>
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
 * 3.  The total number of threads is blk_ct*th_per_blk, and 
 *     blk_ct should be chosen to satisfy
 *
 *        (blk_ct-1)*th_per_blk < n <= blk_ct*th_per_blk
 *     
 *     So every blk will do some meaningful work.  It should
 *     also be chosen to be a multiple of the warp size.
 *
 * Runtimes X/X/XX n = 2^20, a = -3, b = 3, blk_ct = 1024, th_per_blk = 1024
 *    -arch=sm_32 on jet, -arch=sm_52 on msan
 * jet06 cpu: 20.2   ms 
 * jet06 gpu:  3.16  ms  
 * msan cpu:   4.51  ms
 * msan gpu:   0.114 ms 
 */
#include <stdio.h>
#include <stdlib.h>
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
#define MAX_BLKSZ 1024

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
 *              assumes block has at most 1024 threads.  It
 *              computes the sum of the values computed within
 *              a warp using Warp_sum.  It then finds the total
 *              of the warp sums within a block using Warp_sum
 *              a second time.  It uses atomicAdd to sum the
 *              results computed by the blocks.
 */
__global__ void Dev_trap(
      const float  a        /* in  */, 
      const float  b        /* in  */, 
      const float  h        /* in  */, 
      const int    n        /* in  */,
      float*       trap_p   /* out */) {
   __shared__ float thread_calcs[MAX_BLKSZ];
   __shared__ float warp_sum_arr[WARPSZ];
   int my_i = blockDim.x * blockIdx.x + threadIdx.x;
   int my_warp = threadIdx.x / warpSize;
   int my_lane = threadIdx.x % warpSize;
   float* shared_vals = thread_calcs + my_warp*warpSize;
   float blk_result = 0.0;

   shared_vals[my_lane] = 0.0f;
   if (0 < my_i && my_i < n) {
      float my_x = a + my_i*h;
      shared_vals[my_lane] = f(my_x);
   }

   float my_result = Shared_mem_sum(shared_vals);
   if (my_lane == 0) warp_sum_arr[my_warp] = my_result;
   __syncthreads();

#  ifdef DEBUG
   printf("GPU: th = %d, blk = %d, my_warp = %d, my_trap = %e, my_result = %e\n",
         my_i, blockIdx.x, my_warp, my_trap, my_result);
#  endif

   if (my_warp == 0) {
      /* Ensure that there are warpSize values in warp_sum_arr */
      if (threadIdx.x >= blockDim.x/warpSize) 
         warp_sum_arr[threadIdx.x] = 0.0;
      blk_result = Shared_mem_sum(warp_sum_arr);
#     ifdef DEBUG
      printf("GPU:  th = %d, blk_result = %e\n", my_i, blk_result);
#     endif
   }

   if (threadIdx.x == 0) {
      atomicAdd(trap_p, blk_result);
   }
}  /* Dev_trap */    


/*-------------------------------------------------------------------
 * Host code 
 */
void  Get_args(const int argc, char* argv[], int* n_p, 
      float* a_p, float* b_p, int* blk_ct_p, int* th_per_blk_p);
void  Trap_wrapper(const float a, const float b, const int n, 
      float* trap_p, const int blk_ct, const int th_per_blk);
float Serial_trap(const float a, const float b, const int n);
void  Update_stats(double start, double finish, 
      double* min_p, double* max_p, double* total_p);
float Simulate_trap(
      const float  a           /* in */, 
      const float  b           /* in */, 
      const int    n           /* in */,
      const int    blk_ct      /* in */,
      const int    th_per_blk  /* in */);
float Serial_tree_sum(float vals[]);


/*-------------------------------------------------------------------
 * main
 */
int main(int argc, char* argv[]) {
   int n, blk_ct, th_per_blk;
   float a, b, *trap_p, strap;
   double start, finish;  
   double dmin = 1.0e6, dmax = 0.0, dtotal = 0.0;
   double hmin = 1.0e6, hmax = 0.0, htotal = 0.0;

   Get_args(argc, argv, &n, &a, &b, &blk_ct, &th_per_blk);
   cudaMallocManaged(&trap_p, sizeof(float));
#  ifdef DEBUG
   printf("main:  n = %d, a = %f, b = %f, blk_ct = %d, th_per_blk = %d, trap_p = %p\n",
         n, a, b, blk_ct, th_per_blk, trap_p);
#  endif

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
      strap = Serial_trap(a, b, n);
      GET_TIME(finish);
      Update_stats(start, finish, &hmin, &hmax, &htotal);
//    printf("Elapsed time for cpu = %e seconds\n", finish-start);
   }
   printf("The area as computed by cuda is:         %e\n", *trap_p);
   printf("The area as computed by simulated gpu is %e\n", 
         Simulate_trap(a, b, n, blk_ct, th_per_blk));
   printf("The area as computed by cpu is:          %e\n", strap);
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
 *            number, or the total number of threads doesn't satisfy
 *
 *               (blk_ct-1) * WARPSZ < n <= blk_ct*WARPSZ,
 *
 *            or the block size isn't a multiple of the warp size,
 *            then print a message and quit.
 */
void Get_args(const int argc, char* argv[], int* n_p, 
      float* a_p, float* b_p, int* blk_ct_p, int* th_per_blk_p) {
   int n, blk_ct, th_per_blk;

   if (argc != 6) {
      fprintf(stderr, "usage: %s <n> <a> <b> <blk_ct> <th_per_blk>\n", 
            argv[0]);
      exit(0);
   }
   n = strtol(argv[1], NULL, 10);
   *a_p = strtod(argv[2], NULL);
   *b_p = strtod(argv[3], NULL);
   blk_ct = strtol(argv[4], NULL, 10);
   th_per_blk = strtol(argv[5], NULL, 10);
   if (n <= (blk_ct - 1)*th_per_blk ||
       n > blk_ct*th_per_blk || 
       th_per_blk % WARPSZ != 0) {
      fprintf(stderr, "n = %d, blk_ct = %d, th_per_blk = %d\n",
           n, blk_ct, th_per_blk);
      fprintf(stderr, "blk_ct should be chosen so that\n");
      fprintf(stderr, "(blk_ct-1)*th_per_blk < n <= blk_ct*th_per_blk\n");
      fprintf(stderr, "th_per_blk should be a multiple of %d\n",
            WARPSZ);
      exit(0);
   }
   *n_p = n;
   *blk_ct_p = blk_ct;
   *th_per_blk_p = th_per_blk;
}  /* Get_args */


/*-------------------------------------------------------------------
 * Function:  Trap_wrapper
 * Purpose:   CPU wrapper function for GPU trapezoidal rule
 * Note:      Assumes trap_p has been allocated on host and
 *            device
 */
void Trap_wrapper(const float a, const float b, const int n, 
     float* trap_p, const int blk_ct, const int th_per_blk) {

#  ifdef DEBUG
   printf("wrapper:  n = %d, a = %f, b = %f, blk_ct = %d, th_per_blk = %d\n",
         n, a, b, blk_ct, th_per_blk);
#  endif
   /* Invoke kernel */
   *trap_p = 0.5*(f(a) + f(b));
   float h = (b-a)/n;
   Dev_trap<<<blk_ct, th_per_blk>>>(a, b, h, n, trap_p);
   cudaDeviceSynchronize();

#  ifdef DEBUG
   printf("wrapper:  *trap_p returned from GPU = %e\n",
         *trap_p);
#  endif
   *trap_p = h*(*trap_p);
#  ifdef DEBUG
   printf("wrapper:  final value of *trap_p = %e\n",
         *trap_p);
#  endif
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
      const int    blk_ct      /* in */,
      const int    th_per_blk  /* in */) {
   float x, h = (b-a)/n;
   float temp[WARPSZ];
   float warp_sums[WARPSZ];
   float trap = 0.0f;
   unsigned lane;
   int th;
   int warps_per_blk = th_per_blk/WARPSZ;

   trap += 0.5*(f(a) + f(b));
#  ifdef DEBUG
   printf("Sim:  a = %.1f, b = %.1f, n = %d, blk_ct = %d, trap =  %e\n",
         a, b, n, blk_ct, trap);
#  endif

   for (int blk = 0; blk < blk_ct; blk++) {
      memset(warp_sums, 0, WARPSZ*sizeof(float));
      for (int warp = 0; warp < warps_per_blk; warp++) {
         int first = blk*th_per_blk + warp*WARPSZ;
         int last = first + WARPSZ;

         /* Find each thread's contribution to the warp sum */
         for (th = first; th < last; th++) {
            lane = th % WARPSZ;
            if (0 < th && th < n) {
               x = a + th*h;
               temp[lane] = f(x);
            } else {
               temp[lane] = 0.0f;
            }
         }
#        ifdef DEBUG
         printf("Sim: blk = %d, warp = %d, temp = ",
                  blk, warp);
         for (int i = 0; i < WARPSZ; i++)
            printf("%e ", temp[i]);
         printf("\n");
#        endif

         /* Tree structured sum of warp values*/
         warp_sums[warp] = Serial_tree_sum(temp);
#        ifdef DEBUG
         printf("Sim: blk = %d,  warp = %d, th = %d, sum = %e\n", 
               blk, warp, first, warp_sums[warp]);
#        endif
      }  /* for warp */

      /* Tree structured sum of warp_sums*/
      trap += Serial_tree_sum(warp_sums);
#     ifdef DEBUG
      printf("Sim: blk = %d,  trap = %e\n", 
            blk, trap);
#     endif

   }  /* for blk */
   trap = trap*h;
   
   return trap;
}  /* Simulate_trap */


/*-------------------------------------------------------------------
 * Function:  Serial_tree_sum
 * Purpose:   Simulate a tree structured sum
 */
float Serial_tree_sum(float vals[]) {

   for (unsigned bit_mask = WARPSZ >> 1; bit_mask > 0; 
      bit_mask >>= 1) {
      for (unsigned lane = 0; lane < bit_mask; lane++) {
         unsigned partner = lane | bit_mask;
         vals[lane] += vals[partner];
      }
   }  /* for bit_mask */

   return vals[0];
}  /* Serial_tree_sum */


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
