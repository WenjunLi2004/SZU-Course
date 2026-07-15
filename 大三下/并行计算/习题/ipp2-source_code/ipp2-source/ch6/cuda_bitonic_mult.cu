/* File:    cuda_bitonic_mult.cu
 *
 * Purpose: Implement parallel bitonic sort using the computations
 *          in serial_bitonic.c.  So at each stage of the sort
 *          each thread can operate independently of the other threads,
 *          deciding what to do on the basis of its rank and
 *          the size of the current sublists.  This version 
 *          uses multiple thread blocks, and if n is the number of 
 *          elements in the list, n should be twice the *total*
 *          number of threads.  Note that n should be a power of 2.
 *
 * Compile: nvcc -arch=sm_XY -o cbm cuda_bitonic_mult.cu
 *             X must be >= 3 since the program uses unified memory
 * Usage:   ./cbm <n> <blk_ct> <th_per_blk> [mod]
 *             n :  number of elements in list (a power of 2 =
 *                  2*blk_ct*th_per_blk)
 *         blk_ct:  number of thread blocks            
 *     th_per_blk:  number of threads per block
 *            mod:  if mod is present it is used as the modulus
 *                  with the C random function to generate the
 *                  elements of the list.  If mod is not present
 *                  the user should enter the list.
 *
 * Input:   none if mod is on the command line
 *          list of n ints if mod is not on the command line
 * Output:  elapsed wall clock time for sort and whether the
 *          list is sorted
 *
 * Notes:
 * 1.  In order to see input and output lists, define the macro PRINT_LIST 
 *     at compile time
 * 2.  Very verbose output is enabled by defining the compiler macro
 *     DEBUG
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "timer.h"

#define ITERS 1

#define INC 0
#define SWAP(x,y) {int z = x; x = y; y = z;}
#define MAX_TITLE 1000
#define ALL_ONES (~0)
/* Number of bits in an unsigned int */
#define U_SZ (8*sizeof(unsigned))


/*---------------------------------------------------------------------
 * Host Functions
 */
void     Usage(char* prog_name);
void     Get_args(int argc, char* argv[], int* n_p, int* blk_ct_p,
               int* th_per_blk_p, int* mod_p);
void     Generate_list(int a[], int n, int mod);
void     Print_list(int a[], int n, const char* title);
void     Read_list(int a[], int n);
void     Bitonic_sort(int list[], int n);
int      Check_sort(int a[], int n);
void     Update_stats(double start, double finish, 
            double* min_p, double* max_p, double* total_p);
void     Par_bitonic_sort(int a[], int n, int blk_ct, int th_per_blk);

/*---------------------------------------------------------------------
 * Host and Device Functions
 */
__host__ __device__ unsigned Get_width(unsigned val);
__host__ __device__ void     Print_unsigned(unsigned val, 
      unsigned field_width);
__host__ __device__ unsigned Insert_zero(unsigned val, unsigned j);
__host__ __device__ void     Compare_swap(int list[], unsigned my_elt1, 
      unsigned my_elt2, unsigned inc_dec);
__host__ __device__ unsigned Which_bit(unsigned stage);
__host__ __device__ void     Swap(int* x_p, int* y_p);


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
         my_elt1 = Insert_zero(th, which_bit);
         my_elt2 = my_elt1 ^ stage;
         /* my_elt1 & bf_sz is increasing (0) or decreasing (nonzero) */
         Compare_swap(list, my_elt1, my_elt2, my_elt1 & bf_sz);
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
   unsigned my_elt1 = Insert_zero(th, which_bit);
   unsigned my_elt2 = my_elt1 ^ stage;
   /* elt & len is increasing (0) or decreasing (nonzero) */
   Compare_swap(list, my_elt1, my_elt2, my_elt1 & bf_sz);

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
      my_elt1 = Insert_zero(th, which_bit);
      my_elt2 = my_elt1 ^ stage;
      /* my_elt1 & bf_sz is increasing (0) or decreasing (nonzero) */
      Compare_swap(list, my_elt1, my_elt2, my_elt1 & bf_sz);
      __syncthreads();
   }

}  /* Pbutterfly_finish */


/*-------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
   int  n, mod, blk_ct, th_per_blk;
   int *a, *tmp;
   double start, finish;
   double dmin = 1.0e6, dmax = 0.0, dtotal = 0.0;
   double hmin = 1.0e6, hmax = 0.0, htotal = 0.0;

   Get_args(argc, argv, &n, &blk_ct, &th_per_blk, &mod);
   printf("n = %d, blk_ct = %d, th_per_blk = %d, mod = %d\n",
         n, blk_ct, th_per_blk, mod);
   cudaMallocManaged(&a, n*sizeof(int));
   tmp = (int*) malloc(n*sizeof(int));
   if (mod != 0)
      Generate_list(a, n, mod);
   else
      Read_list(a, n);
   memcpy(tmp, a, n*sizeof(int));

#  ifdef PRINT_LIST
   Print_list(a, n, "Before sort");
#  endif

for (int iter = 0; iter < ITERS; iter++) {
      GET_TIME(start);
      Par_bitonic_sort(a, n, blk_ct, th_per_blk);
//    cudaDeviceSynchronize();   /* Not needed */
      GET_TIME(finish);
//    printf("Elapsed time to sort %d ints using parallel bitonic sort = %e secs\n", 
//          n, finish-start);

      Update_stats(start, finish, &dmin, &dmax, &dtotal);


      GET_TIME(start);
      Bitonic_sort(tmp, n);
      GET_TIME(finish);
//    printf("Elapsed time to sort %d ints using serial bitonic sort = %e secs\n", 
//          n, finish-start);
      Update_stats(start, finish, &hmin, &hmax, &htotal);
      
   }

#  ifdef PRINT_LIST
   Print_list(a, n, "After sort");
#  endif
   if (Check_sort(a, n) != 0) 
      printf("Device list is sorted\n");
   else
      printf("Device list is not sorted\n");
   if (Check_sort(tmp, n) != 0) 
      printf("Host list is sorted\n");
   else
      printf("Host list is not sorted\n");
   printf("Device times:  min = %e, max = %e, avg = %e\n",
         dmin, dmax, dtotal/ITERS);
   printf("  Host times:  min = %e, max = %e, avg = %e\n",
         hmin, hmax, htotal/ITERS);

   cudaFree(a);
   free(tmp);
   return 0;
}  /* main */


/*-----------------------------------------------------------------
 * Function:  Usage
 * Purpose:   Summary of how to run program and exit
 */
void Usage(char* prog_name) {
   fprintf(stderr, "      usage: %s <n> <blk_ct> <th_per_blk> [mod]\n",
         prog_name);
   fprintf(stderr, "         n : number of elements in list (a power\n"); 
   fprintf(stderr, "             of 2 = 2*blk_ct*th_per_blk)\n");
   fprintf(stderr, "     blk_ct: number of thread blocks\n"); 
   fprintf(stderr, " th_per_blk: number of threads per block\n");
   fprintf(stderr, "        mod: if mod is present it is used as the\n");
   fprintf(stderr, "             modulus with the C random function to\n");
   fprintf(stderr, "             generate the elements of the list.  If\n");
   fprintf(stderr, "             mod is not present the user should enter\n"); 
   fprintf(stderr, "             the list.\n");
   exit(0);
}  /* Usage */


/*-----------------------------------------------------------------
 * Function:  Get_args
 * Purpose:   Get and check command line arguments
 * In args:   argc, argv
 * Out args:  n_p, blk_ct_p, th_per_blk_p, mod_p
 */
void Get_args(int argc, char* argv[], int* n_p, int* blk_ct_p,
      int* th_per_blk_p, int* mod_p) {
   int n;

   if (argc != 4 && argc != 5) Usage(argv[0]);

   n = *n_p = strtol(argv[1], NULL, 10);
   if (n <= 0) Usage(argv[0]);
   while ((n & 1) == 0)
      n = n >> 1;
   if (n > 1) Usage(argv[0]);

   *blk_ct_p = strtol(argv[2], NULL, 10);
   *th_per_blk_p = strtol(argv[3], NULL, 10);
   if (*n_p != 2*(*blk_ct_p)*(*th_per_blk_p)) Usage(argv[0]);

   if (argc == 5)
      *mod_p = strtol(argv[4], NULL, 10);
   else
      *mod_p = 0;
}  /* Get_args */


/*-----------------------------------------------------------------
 * Function:  Generate_list
 * Purpose:   Use random number generator to generate list elements
 * In args:   n
 * Out args:  a
 */
void Generate_list(int a[], int n, int mod) {
   int i;

   srandom(1);
   for (i = 0; i < n; i++)
      a[i] = random() % mod;
}  /* Generate_list */


/*-----------------------------------------------------------------
 * Function:  Print_list
 * Purpose:   Print the elements in the list
 * In args:   a, n
 */
void Print_list(int a[], int n, const char* title) {
   int i;

   printf("%s:  ", title);
   for (i = 0; i < n; i++)
      printf("%d ", a[i]);
   printf("\n\n");
}  /* Print_list */


/*-----------------------------------------------------------------
 * Function:  Read_list
 * Purpose:   Read elements of list from stdin
 * In args:   n
 * Out args:  a
 */
void Read_list(int a[], int n) {
   int i;

   printf("Please enter the elements of the list\n");
   for (i = 0; i < n; i++)
      scanf("%d", &a[i]);
}  /* Read_list */


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
   cudaDeviceSynchronize();

   for (bf_sz = 4*th_per_blk; bf_sz <= n; bf_sz = bf_sz*2) {
      for (stage = bf_sz/2; stage >= 2*th_per_blk; stage = stage/2) {
         /* Do a single iteration of a butterfly */
         Pbutterfly_one_stage <<<blk_ct, th_per_blk>>> (list, n, bf_sz, stage);
      } 
      cudaDeviceSynchronize();

      /* Finish the current butterfly working with ``short'' sublists */
      Pbutterfly_finish <<<blk_ct, th_per_blk>>> (list, n, bf_sz); 

      cudaDeviceSynchronize();
   }
   cudaDeviceSynchronize();

}  /* Par_bitonic_sort */

/*---------------------------------------------------------------------
 * Function:  Get_width
 * Purpose:   Determine the number of bits in the binary rep of val
 *            from the least significant bit to the leftmost nonzero
 *            bit.  The number of bits in zero is zero.
 */
__host__ __device__ unsigned Get_width(unsigned val) {
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

__host__ __device__   void Print_unsigned(unsigned val, unsigned field_width) {
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
 * Function:    Insert_zero
 * Purpose:     Insert a zero in the binary representation of 
 *              val between bits j and j-1
 */
__host__ __device__ unsigned Insert_zero(unsigned val, unsigned j) {
   unsigned left_bits, right_bits, left_ones, right_ones;

   left_ones = ALL_ONES << j;  
   right_ones = ~left_ones;
   left_bits = left_ones & val;
   right_bits = right_ones & val;
   return (left_bits << 1) | right_bits;
}  /* Insert_zero */




/*-----------------------------------------------------------------
 * Function:    Compare_swap
 * Purpose:     Compare two elements in the list, and if out of order
 *              swap:
 *
 *                 inc_dec = INC => pair should increase
 *                 inc_dec != INC => pair should decrease
 *             
 * In args:     my_elt1, my_elt2:  subscripts of elements of a
 *                 my_elt1 should always be < my_elt2
 *              inc_dec:   whether pair should increase (0)
 *                 or decrease (nonzero)
 * In/out arg:  list:  the list
 */
__host__ __device__ void Compare_swap(int list[], unsigned my_elt1, 
      unsigned my_elt2, unsigned inc_dec) {
   int tmp;

   if (inc_dec == INC) {
      if (list[my_elt1] > list[my_elt2]) {
         tmp = list[my_elt1];
         list[my_elt1] = list[my_elt2];
         list[my_elt2] = tmp;
      }
   } else {  /* inc_dec != INC */
      if (list[my_elt1] < list[my_elt2]) {
         tmp = list[my_elt1];
         list[my_elt1] = list[my_elt2];
         list[my_elt2] = tmp;
      }
   }
}  /* Compare_swap */


/*-----------------------------------------------------------------
 * Function:   Which_bit
 * Purpose:    Find the place of the nonzero bit in stage
 *
 * Note:       stage is a power of 2.  So it has exactly one 
 *             nonzero bit.
 */
__host__ __device__ unsigned Which_bit(unsigned stage) {
   unsigned bit = 0;

   while (stage > 1) {
      bit++;
      stage = stage >> 1;
   }

   return bit;
}  /* Which_bit */


/*-----------------------------------------------------------------
 * Function:     Bitonic_sort
 * Purpose:      Sort list of n elements using bitonic sort
 * In args:      n
 * In/out args:  a
 *
 * Note:         n should be a power of 2
 */
void Bitonic_sort(int a[], int n) {
   unsigned len, part_mask, partner, bit, i, elt;
#  ifdef DEBUG
   char title[MAX_TITLE];
#  endif

   for (len = 2; len <= n; len = len << 1) {
#     ifdef DEBUG
      printf("len = %u\n", len);
#     endif 
      for (part_mask = len >> 1; part_mask > 0; part_mask = part_mask >> 1) {
         bit = Which_bit(part_mask);
#        ifdef DEBUG
         printf("   part_mask = %u, bit = %u\n", part_mask, bit);
#        endif 
         for (i = 0; i < n/2; i++) {
            elt = Insert_zero(i, bit);
            partner = elt ^ part_mask;
#           ifdef DEBUG
            printf("      i = %u = ", i);
            Print_unsigned(i, Get_width(n)); 
            printf(", elt = %u = ", elt);
            Print_unsigned(elt, 1 + Get_width(n)); 
            printf(", partner = %u = ", partner);
            Print_unsigned(partner, 1 + Get_width(n)); 
            printf("\n");
            printf("      Before swap: elt = %u, a[%u] = %d, partner = %u, a[%u] = %d\n", 
                                elt, elt, a[elt], 
                                partner, partner, a[partner]);
#           endif 
            /* elt & len = INC or DEC */
            Compare_swap(a, elt, partner, elt & len);
#           ifdef DEBUG
            printf("      After swap:  elt = %u, a[%u] = %d, partner = %u, a[%u] = %d\n", 
                                elt, elt, a[elt], 
                                partner, partner, a[partner]);
#           endif 
         }
      }
#     ifdef DEBUG
      sprintf(title, "len = %d, after bitonic", len);
      Print_list(a, n, title);
      printf("\n");
#     endif
   }
}  /* Bitonic_sort */

      
/*-----------------------------------------------------------------
 * Function:     Swap
 * Purpose:      Swap contents of x_p and y_p
 * In/out args:  x_p, y_p
 */
__host__ __device__ void Swap(int* x_p, int* y_p) {
   int temp = *x_p;
   *x_p = *y_p;
   *y_p = temp;
}  /* Swap */


/*-----------------------------------------------------------------
 * Function:     Check_sort
 * Purpose:      Check to see if a list is sorted in increasing order
 * In args:      a, n
 */
int Check_sort(int a[], int n) {
   int i;

   for (i = 0; i < n-1; i++)
//    if (a[i] > a[i+1]) {
      if (*(a+i) > *(a+i+1)) {
         printf("a[%d] = %d > %d = a[%d]\n",
               i, *(a+i), *(a+i+1), i+1);
//             i, a[i], a[i+1], i+1);
         return 0;
      }
   return 1; 
}  /* Check_sort */


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


