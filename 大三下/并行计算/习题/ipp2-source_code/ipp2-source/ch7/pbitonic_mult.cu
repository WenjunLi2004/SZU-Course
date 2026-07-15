/* File:    pbitonic_mult.cu
 *
 * Purpose: Implement parallel bitonic sort using the computations
 *          in serial_bitonic1.c.  So at each stage of the sort
 *          each thread can operate independently of the other threads,
 *          deciding what to do on the basis of its rank and
 *          the size of the current sublists.  This version 
 *          uses multiple thread blocks, and if n is the number of 
 *          elements in the list, n should be twice the *total*
 *          number of threads.  Note that n should be a power of 2.
 *          This only includes a host driver function, various
 *          kernels, and device code.
 *
 * Notes:
 * 1.  In order to see input and output lists, define the macro PRINT_LIST 
 *     at compile time
 * 2.  Very verbose output is enables by defining the compiler macro
 *     DEBUG
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pbitonic_mult.h"
#include "timer.h"


#define INC 0
#define MAX_TITLE 1000
#define ALL_ONES (~0)
/* Number of bits in an unsigned int */
#define U_SZ (8*sizeof(unsigned))


/*---------------------------------------------------------------------
 * Host Function
 */
void     Par_bitonic_sort(int a[], int n, int blk_ct, int th_per_blk);

/*---------------------------------------------------------------------
 * Host and Device Functions
 */
__device__ unsigned Get_width(unsigned val);
__device__ void     Print_unsigned(unsigned val, 
      unsigned field_width);
 __device__ unsigned Insert_zero1(unsigned val, unsigned j);
 __device__ void     Compare_swap1(int list[], unsigned my_elt1, 
      unsigned my_elt2, unsigned inc_dec);
__device__ unsigned Which_bit(unsigned stage);


/*---------------------------------------------------------------------
 * Device Functions and Kernels
 */
__global__ void Pbitonic_start(int list[], int n);
__global__ void Pbutterfly_one_stage(int list[], int n);
__global__ void Pbutterfly_finish(int list[], int n);


/*---------------------------------------------------------------------
 * Function:  Pbitonic_start (kernel)
 * Purpose:   Sort consecutive sublists of length 2*th_per_blk into 
 *            alternately increasing and decreasing sublists.
 *            The sorting algorithm is bitonic sort.  Note that 
 *            th_per_blk is a power of 2.
 */
__global__ void Pbitonic_start(int list[], int n) {
   unsigned bf_sz, stage, my_elt1, my_elt2, which_bit;

   unsigned th = blockDim.x*blockIdx.x + threadIdx.x;

   for (bf_sz = 2; bf_sz <= 2*blockDim.x; bf_sz = bf_sz << 1) {
      for (stage = bf_sz >> 1; stage > 0;  
            stage = stage >> 1) {
         which_bit = Which_bit(stage);
         my_elt1 = Insert_zero1(th, which_bit);
         my_elt2 = my_elt1 ^ stage;
         /* my_elt1 & bf_sz is increasing (0) or decreasing (nonzero) */
         Compare_swap1(list, my_elt1, my_elt2, my_elt1 & bf_sz);
         __syncthreads();
      }
   }
}  /* Pbitonic_start */



/*---------------------------------------------------------------------
 * Function:  Pbutterfly_one_stage (kernel)
 * Purpose:   Execute one stage of a butterfly on consecutive
 *            sublists of length bf_sz using stage to pair list
 *            elements.  The length of the sublists is > 2*th_per_blk,
 *            requiring a call to cudaDeviceSynchronize after 
 *            the kernel returns.  (So there's no call to
 *            __syncthreads in this kernel.)
 */
__global__ void Pbutterfly_one_stage(int list[], int n, unsigned bf_sz,
      unsigned stage) {

   unsigned th = blockDim.x*blockIdx.x + threadIdx.x;

   unsigned which_bit = Which_bit(stage);
   unsigned my_elt1 = Insert_zero1(th, which_bit);
   unsigned my_elt2 = my_elt1 ^ stage;
   /* elt & len is increasing (0) or decreasing (nonzero) */
   Compare_swap1(list, my_elt1, my_elt2, my_elt1 & bf_sz);

}  /* Pbutterfly_one_stage */



/*---------------------------------------------------------------------
 * Function:  Pbutterfly_finish (kernel)
 * Purpose:   Complete a butterfly that was started on sublists of
 *            length bf_sz > 2*th_per_blk.  The butterfly should 
 *            continue with sublists of length bf_sz <= 2*th_per_blk.
 * Note:      Each thread block can operate independently of the
 *            other thread blocks in this kernel.
 */
__global__ void Pbutterfly_finish(int list[], int n, unsigned bf_sz) {

   unsigned stage, which_bit, my_elt1, my_elt2;

   unsigned th = blockDim.x*blockIdx.x + threadIdx.x;

   for (stage = blockDim.x; stage > 0; stage = stage/2) {
      which_bit = Which_bit(stage);
      my_elt1 = Insert_zero1(th, which_bit);
      my_elt2 = my_elt1 ^ stage;
      /* my_elt1 & bf_sz is increasing (0) or decreasing (nonzero) */
      Compare_swap1(list, my_elt1, my_elt2, my_elt1 & bf_sz);
      __syncthreads();
   }

}  /* Pbutterfly_finish */


/*---------------------------------------------------------------------
 * Function:    Par_bitonic_sort
 * Purpose:     Wrapper for kernels implementing parallel bitonic sort.  
 * In args:     n:  number of elements in the list, a power of 2, should
 *                  = 2*blk_ct*th_per_blk
 *              blk_ct:  the number of thread blocks
 *              th_per_blk:  the number threads in each thread block
 * In/out arg:  list:  the list
 *
 * Note:        Since there is a call to cudaDeviceSynchronize after
 *              the last kernel call, the caller does not need to
 *              call cudaDeviceSynchronize.
 */
void     Par_bitonic_sort(int list[], int n, int blk_ct, int th_per_blk) {
   unsigned bf_sz, stage;

   /* Sort sublists with 2*th_per_blk elements */
   Pbitonic_start <<<blk_ct, th_per_blk>>> (list, n);
// Not needed
// cudaDeviceSynchronize();  

   for (bf_sz = 4*th_per_blk; bf_sz <= n; bf_sz = bf_sz*2) {
      for (stage = bf_sz/2; stage >= 2*th_per_blk; stage = stage/2) {
         /* Do a single iteration of a butterfly */
         Pbutterfly_one_stage <<<blk_ct, th_per_blk>>> (list, n, bf_sz, stage);
      } 
//    cudaDeviceSynchronize();

      Pbutterfly_finish <<<blk_ct, th_per_blk>>> (list, n, bf_sz); 

//    cudaDeviceSynchronize();
   }
// cudaDeviceSynchronize();

}  /* Par_bitonic_sort */

/*---------------------------------------------------------------------
 * Function:  Get_width
 * Purpose:   Determine the number of bits in the binary rep of val
 *            from the least significant bit to the leftmost nonzero
 *            bit.  The number of bits in zero is zero.
 */
__device__ unsigned Get_width(unsigned val) {
   unsigned field_width = 0;

   while (val != 0) {
      val >>= 1;
      field_width++;
   }
   return field_width;
}  /* Get_width */


/*---------------------------------------------------------------------
 * Function:  Print_unsigned
 * Purpose:   Print the binary representation of an unsigned int
 */

__device__   void Print_unsigned(unsigned val, unsigned field_width) {
   unsigned curr_bit, i;
   /* +1 for null char terminating string */
   char bin_str[U_SZ+1];

   for (i = 0; i < field_width; i++)
      bin_str[i] = '0';
   bin_str[field_width] = '\0';

   if (val == 0) {
      printf("%s", bin_str);
      return;
   }

   /* val != 0 */
   curr_bit = field_width-1;
   while (val > 0) {
      if (val & 1) bin_str[curr_bit] = '1';
      val >>= 1;
      curr_bit--;
   }

   printf("%s", bin_str);
}  /* Print_unsigned */


/*---------------------------------------------------------------------
 * Function:    Insert_zero1
 * Purpose:     Insert a zero in the binary representation of 
 *              val between bits j and j-1
 *
 */
__device__ unsigned Insert_zero1(unsigned val, unsigned j) {
   unsigned left_bits, right_bits, left_ones, right_ones;

   left_ones = ALL_ONES << j;  
   right_ones = ~left_ones;
   left_bits = left_ones & val;
   right_bits = right_ones & val;
   return (left_bits << 1) | right_bits;
}  /* Insert_zero1 */




/*-----------------------------------------------------------------
 * Function:    Compare_swap1
 * Purpose:     Compare two elements in the list, and if out of order,
 *              swap:
 *
 *                 inc_dec = INC => pair should increase
 *                 inc_dec != INC => pair should decrease
 *             
 * In args:     my_elt1, my_elt2:  subscripts of elements of list
 *                 my_elt1 should always be < my_elt2
 *              inc_dec:   whether pair should increase (0)
 *                 or decrease (nonzero)
 * In/out arg:  list:  the list
 *
 */
__device__ void Compare_swap1(int list[], unsigned my_elt1, 
      unsigned my_elt2, unsigned inc_dec) {
   int tmp;

   if (inc_dec == INC) {
      if (list[my_elt1] > list[my_elt2]) {
         tmp = list[my_elt1];
         list[my_elt1] = list[my_elt2];
         list[my_elt2] = tmp;
      }
   } else {  // inc_dec != INC 
      if (list[my_elt1] < list[my_elt2]) {
         tmp = list[my_elt1];
         list[my_elt1] = list[my_elt2];
         list[my_elt2] = tmp;
      }
   }
}  /* Compare_swap1 */


/*-----------------------------------------------------------------
 * Function:   Which_bit
 * Purpose:    Find the place of the nonzero bit in stage
 *
 * Note:       stage is a power of 2.  So it has exactly one 
 *             nonzero bit.
 */
__device__ unsigned Which_bit(unsigned stage) {
   unsigned bit = 0;

   while (stage > 1) {
      bit++;
      stage = stage >> 1;
   }

   return bit;
}  /* Which_bit */

