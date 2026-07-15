/* File:    cuda_bitonic_one.cu
 *
 * Purpose: Implement parallel bitonic sort using the computations
 *          in serial_bitonic.c.  So at each stage of the sort
 *          each thread can operate independently of the other threads,
 *          deciding what to do on the basis of its rank and
 *          the size of the current sublists.  This version only
 *          uses one thread block, and if n is the number of elements
 *          in the list, n should be twice the number of threads.
 *          Note that n should be a power of 2.
 *
 * Compile: nvcc -arch=sm_XY -o cbo cuda_bitonic_one.cu
 *             X must be >= 3 since the program uses unified memory
 * Usage:   ./cbo <n> [mod]
 *             n :  number of elements in list
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

#ifdef DEBUG
#define ITERS 1
#else
#define ITERS 50
#endif

#define INC 0
#define SWAP(x,y) {int z = x; x = y; y = z;}
#define MAX_TITLE 1000
#define ALL_ONES (~0)
#define U_SZ (8*sizeof(unsigned))


/*---------------------------------------------------------------------
 * Host Functions
 */
void     Usage(char* prog_name);
void     Get_args(int argc, char* argv[], int* n_p, int* mod_p);
void     Generate_list(int a[], int n, int mod);
void     Print_list(int a[], int n, const char* title);
void     Read_list(int a[], int n);
void     Bitonic_sort(int a[], int n);
int      Check_sort(int a[], int n);
void     Update_stats(double start, double finish, 
            double* min_p, double* max_p, double* total_p);

/*---------------------------------------------------------------------
 * Host and Device Functions
 */
__host__ __device__ unsigned Get_width(unsigned val);
__host__ __device__ void     Print_unsigned(unsigned val, 
      unsigned field_width);
__host__ __device__ unsigned Insert_zero(unsigned val, unsigned j);
__host__ __device__ void     Compare_swap(int a[], unsigned elt, 
      unsigned partner, unsigned inc_dec);
__host__ __device__ unsigned Which_bit(unsigned part_mask);
__host__ __device__ void     Swap(int* x_p, int* y_p);


/*---------------------------------------------------------------------
 * Device Functions and Kernels
 */
__global__ void Pbitonic_sort(int a[], int n);


/*---------------------------------------------------------------------
 * Function:  Pbitonic_sort (kernel)
 * Purpose:   Sort a list of n ints using one thread block and n/2 
 *            threads.  The sorting algorithm is bitonic sort.  Note 
 *            that n is a power of 2.
 */
__global__ void Pbitonic_sort(
      int   list[]  /* in/out */, 
      int   n       /* in     */) {
   unsigned bf_sz, stage, my_elt1, my_elt2, which_bit;

   /* Only threadIdx.x is needed, since there's only one block */
   unsigned th = blockDim.x*blockIdx.x + threadIdx.x;
#  ifdef DEBUG
   printf("Th %u > Starting Pbitonic_sort\n", th);
#  endif

   for (bf_sz = 2; bf_sz <= n; bf_sz = bf_sz << 1) {
      for (stage = bf_sz >> 1; stage > 0;  
            stage = stage >> 1) {
         which_bit = Which_bit(stage);
         my_elt1 = Insert_zero(th, which_bit);
         my_elt2 = my_elt1 ^ stage;
         Compare_swap(list, my_elt1, my_elt2, my_elt1 & bf_sz);
         __syncthreads();
      }
   }
}  /* Pbitonic_sort */

/*-------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
   int  n, mod;
   int *a, *tmp;
   double start, finish;
   double dmin = 1.0e6, dmax = 0.0, dtotal = 0.0;
   double hmin = 1.0e6, hmax = 0.0, htotal = 0.0;

   Get_args(argc, argv, &n, &mod);
   cudaMallocManaged(&a, n*sizeof(int));
   tmp = (int*) malloc(n*sizeof(int));
   if (mod != 0)
      Generate_list(a, n, mod);
   else
      Read_list(a, n);
   memcpy(tmp, a, n*sizeof(int));

#  ifdef PRINT_LIST
   printf("n = %d, mod = %d\n", n, mod);
   Print_list(a, n, "Before sort");
#  endif

for (int iter = 0; iter < ITERS; iter++) {
      GET_TIME(start);
#     ifdef DEBUG 
      printf("Calling Pbitonic_sort with 1 block and %d threads\n",
            n/2);
      fflush(stdout);
#     endif
      Pbitonic_sort<<<1, n/2>>>(a, n);
      cudaDeviceSynchronize();
#     ifdef DEBUG 
      printf("Returned from call to Pbitonic_sort\n");
      fflush(stdout);
#     endif
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
 * Purpose:   Summary of how to run program
 */
void Usage(char* prog_name) {
   fprintf(stderr, "usage:   %s <n> [mod]\n", prog_name);
   fprintf(stderr, "   n :  number of elements in list\n");
   fprintf(stderr, "  mod:  if present generate list using the random\n");
   fprintf(stderr, "        number generator random and modulus mod.\n");
   fprintf(stderr, "        If absent user will enter list\n");
   exit(0);
}  /* Usage */


/*-----------------------------------------------------------------
 * Function:  Get_args
 * Purpose:   Get and check command line arguments
 * In args:   argc, argv
 * Out args:  n_p, mod_p
 */
void Get_args(int argc, char* argv[], int* n_p, int* mod_p) {
   if (argc != 2 && argc != 3)
      Usage(argv[0]);
    
   *n_p = strtol(argv[1], NULL, 10);

   if (argc == 3)
      *mod_p = strtol(argv[2], NULL, 10);
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
 * Function:  Get_width
 * Purpose:   Determine the number of bits in the binary rep of val
 *            from the least significant bit to the leftmost nonzero
 *            bit.  The number of bits in zero is zero.
 */
unsigned Get_width(unsigned val) {
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

void Print_unsigned(unsigned val, unsigned field_width) {
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
unsigned Insert_zero(unsigned val, unsigned j) {
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
 * In args:     elt, partner:  subscripts of elements of a
 *                 elt should always be < partner
 *              inc_dec:   whether pair should increase (0)
 *                 or decrease (!=0)
 * In/out arg:  a:  the list
 */
void Compare_swap(int a[], unsigned elt, unsigned partner,
      unsigned inc_dec) {
   int tmp;

   if (inc_dec == INC) {
      if (a[elt] > a[partner]) {
         tmp = a[elt];
         a[elt] = a[partner];
         a[partner] = tmp;
      }
   } else {  /* inc_dec != INC */
      if (a[elt] < a[partner]) {
         tmp = a[elt];
         a[elt] = a[partner];
         a[partner] = tmp;
      }
   }
}  /* Compare_swap */


/*-----------------------------------------------------------------
 * Function:   Which_bit
 * Purpose:    Find the place of the nonzero bit in part_mask
 *
 * Note:       part_mask is a power of 2.  So it has exactly one 
 *             nonzero bit.
 */
unsigned Which_bit(unsigned part_mask) {
   unsigned bit = 0;

   while (part_mask > 1) {
      bit++;
      part_mask = part_mask >> 1;
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
            /* elt & len = INC or !INC */
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
void Swap(int* x_p, int* y_p) {
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


