/* File:     cuda_ssort2.cu
 * Purpose:  Implement parallel sample sort of a list of ints.
 *           This is based on the algorithm outlined in
 *
 *              Frank Dehne and Hamidreza Zaboli, "Deterministic
 *              Sample Sort for GPUs," Parallel Processing Letters,
 *              vol 22, no 3, 2010.
 *
 *           This version adds some code for simple profiling of
 *           the sample sort.
 *
 *
 * Compile:  nvcc -o cuda_ssort2 cuda_ssort2.cu pbitonic_mult.cu 
 *              set_device.cu stats.cu simple_printf.cu
 *           Needs pbitonic_mult.h, timer.h, set_device.h, and stats.h
 *           Note that even though set_device.cu, stats.cu, simple_printf.cu
 *              are ordinary C code that runs on the host, the current 
 *              version of nvcc (10.0.130) won't compile them unless 
 *              they're made .cu files
 * Run:      ./cuda_ssort2 <blk_ct> <th_per_blk> <n> <s> [mod]
 *              blk_ct = number of thread blocks in initial local sort
 *              th_per_blk = number of threads per block in initial local sort
 *              n = list size, a power of 2
 *              s = sample size, also a power of 2
 *              mod = if mod is not present, get list from stdin.  
 *                  Otherwise generate the list with mod as the 
 *                  modulus for the random numbers:
 *
 *                        val = random() % mod;
 *
 *             blk_ct <= s <  n; blk_ct should evenly divide s, and
 *             s should evenly divide n, and in this version the 
 *             total number of elements should be twice the total
 *             number of threads.
 *            
 *             So blk_ct, th_per_blk, s, and n should all be powers 
 *             of 2, and n = 2*blk_ct*th_per_blk.
 *
 *             The bucket size must be < 12288 if we use shared memory.
 *             Because of the power of 2 restrictions, this effectively
 *             becomes 8192.
 *
 * Input:    If mod is on the command line, none.  Otherwise, the
 *              list
 * Output:   The input list and the list after processing.  Whether the 
 *              list was sorted, and the run times for the 
 *              CUDA sort and serial qsort library function.
 *
 * Jargon:   The set of contiguous elements assigned to a thread 
 *           block is the *sublist* of the block.  The ordering of
 *           the element sublist will change, but the set of elements
 *           assigned to the block will not change until the mapping
 *           phase.  At the end of the mapping phase, the elements 
 *           assigned to a block are a *bucket*.  The elements in
 *           a particular bucket will be sorted, but they won't
 *           change.
 *           
 * Notes:
 * 1.  The sublist assigned to each thread block should fit into
 *     the shared memory on that block.  With recent (Maxwell, 
 *     Pascal, Volta, Turing) Nvidia processors, this is
 *     12,288 ints.  However, the bitonic sort of the elements 
 *     assigned to a thread block should have exactly two elements
 *     per thread.  With a maximum of 1024 threads per block, the 
 *     limit on elements per block during the local sort should 
 *     be 2048.  (This isn't checked.)  However, the buckets can
 *     double the original blocksize, and the local (thread block)
 *     sorts currently only support #elts = 2*#threads.  So the
 *     maximum number of threads per block should be <= 512 and 
 *     the initial number of elements per block should be <= 1024.
 * 2.  The number of buckets is the same as blk_ct.
 * 3.  We assume that 1 < blk_ct <= s < n.  We also 
 *     assume that blk_ct evenly divides both s and n.
 * 4.  Define DEBUG for output after each stage of the sort.
 *     Also define BIG_DEBUG for details on the sampling and
 *        assignment to the buckets.
 * 5.  Define PRINT_LIST to get output of lists at various points
 * 6.  Command line compiler macros:
 *        DEBUG:      See above
 *        BIG_DEBUG:  See above (need to link simple_printf)
 *        NO_SERIAL:  Skip the serial sort.  Don't check the sort.
 *        PRINT_LIST: Get list output at various points.
 *
 * IPP2:  7.2.9 (pp. 434 and ff.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  /* For memset */
#include <cuda.h>
#include "timer.h"
#include "pbitonic_mult.h"
#include "set_device.h"
#include "stats.h"
#include "simple_printf.h"  /* For simple_printf */
#define sprintf simple_sprintf

/* Seed for generating list */
#define SEED1 1

/* Smallest negative int (largest in absolute value) */
#define MINUS_INFTY (1 << 31)  // = -2^31

/* Largest positive int.  This was ~MINUS_INFTY = 2^31 - 1, */
/*    but we need to be able to add to this...              */
#define INFTY (1 << 30)   // = 2^30 

/* Only used when DEBUG is defined */
#define MAX_STRING 1000
#define INC 0
#define ALL_ONES (~0)

/* Max shared mem per block in 4-byte ints */
#define MAX_SHMEM 12288
#define MAX_TH_PER_BLK 1024
#define WARPSZ 32

// This seems to share shmem among different blocks
// __device__ int shmem[MAX_SHMEM];

double ssort_func_stats[8*3] = {1.0e10, 0, 0,
                                1.0e10, 0, 0,
                                1.0e10, 0, 0,
                                1.0e10, 0, 0,
                                1.0e10, 0, 0,
                                1.0e10, 0, 0,
                                1.0e10, 0, 0,
                                1.0e10, 0, 0};
const char* titles[8] = {"Choose_sample",     // 0
                   "Sort_of_sample",    // 1
                   "Choose_splitters",  // 2
                   "Build_mat_counts",  // 3
                   "Row_prefix_sums",   // 4
                   "Ser_prefix_sums",   // 5
                   "Map",                // 6
                   "Final_sort"};       // 7

double start, finish, elapsed;

void Update_ssort_stats(double elapsed, int row) {
   if (elapsed < ssort_func_stats[row*3 + 0])
      ssort_func_stats[row*3 + 0] = elapsed;
   if (elapsed > ssort_func_stats[row*3 + 1]) 
      ssort_func_stats[row*3 + 1] = elapsed;
   ssort_func_stats[row*3 + 2] += elapsed;
} /* Update_ssort_stats */


void Print_ssort_stats(void) {
   int func;

   for (func = 0; func < 8; func++)
      printf("%16s :  %4.2e  %4.2e  %4.2e\n", 
            titles[func], 
            ssort_func_stats[func*3 + 0],
            ssort_func_stats[func*3 + 1],
            ssort_func_stats[func*3 + 2]/30);  /* THIS SHOULD BE ITERS */
   printf("\n");
}  /* Print_ssort_stats */
/*
 * col 0:  min 
 * col 1:  max
 * col 2:  total
 * 
 * row 0:  Choose_sample
 * row 1:  Sort_of_sample
 * row 2:  Choose_splitters
 * row 3:  Build_mat_counts
 * row 4:  Row_prefix_sums
 * row 5:  Ser_prefix_sums
 * row 6:  Map
 * row 7:  Final_sort
 */



/*---------------------------------------------------------------------
 * Function prototypes
 */
/* Host functions */
void  Get_args(int argc, char* argv[], int* blk_ct, int* th_per_blk_p,
      int* n_p, int* s_p, int* mod_p);
void  Read_list(const char prompt[], int list[], int n);
void  Gen_list(int list[], int n, int modulus);
int   Check_sorted(int dlist[], int hlist[], int n);
int   Compare(const void *a_p, const void *b_p);
void  Find_new_buck_subs(int buck_subs[], int blk_ct, int new_buck_subs[],
         int *new_alloc_sz_p);
void  Ser_prefix_sums(int buck_subs[], int blk_ct);
int   Get_max_bucksz(int buck_subs[], int blk_ct);

/* Host driver for device sample sort */
void  Sample_sort(int list[], int tmp_list[], int sample[], 
      int splitters[], int mat_counts[], int buck_subs[], 
      int n, int s, int blk_ct, int th_per_blk, int* max_bucksz_p);

/* Host and Device functions */
__host__ __device__ void Print_list(const char title[], int list[], int n);
__host__ __device__ void Print_list_strided(const char title[], 
      int list[], int n, int stride);
__host__ __device__ void Print_as_2d(const char title[], int arr[], int rows, 
         int cols);
__host__ __device__ int   Ceiling(int num, int denom);
__host__ __device__ int   Min_power2(int x);

/* Kernels */
__global__ void Choose_sample(int list[], int n, int sample[], int s);
__global__ void Choose_splitters(int sample[], int s, int splitters[], int b);
__global__ void Build_mat_counts(int list[], int n, int splitters[], 
      int mat_counts[]);
__global__ void Row_prefix_sums(int mat_counts[], int buck_subs[]);
__global__ void Map(int list[], int tmp_list[], int n, 
      int mat_counts[], int buck_subs[], int splitters[]);
__global__ void Final_sort(int list[], int tmp_list[], int n,
      int buck_subs[], int* big_buck_arr, int* new_buck_subs);

/* Device functions */
// __device__ void Local_sort(int slist[], int my_list_sz, int my_rk);
__device__ void Local_sort(int slist[], int sublist_sz, int my_rk,
      int th_per_blk);
__device__ int Find_bucket(int elt, int splitters[], int min, int max);
__device__ void Compare_swap(int my_list[], unsigned elt, unsigned partner,
      unsigned inc_dec);
__device__ unsigned Insert_zero(unsigned val, unsigned j);
__device__ int Warp_sum(const int my_val);
__device__ int Get_dest_buck(int elt, int splitters[], int last_buck, 
      int blk_ct);
__device__ int Get_first_elt_sub(int list[], int k, int splitter);


/*---------------------------------------------------------------------
 * Function:  main
 */
int main(int argc, char* argv[]) {
   int blk_ct, th_per_blk, n, s, mod;
   int *olist, *hlist, *dlist, *tmp_list;
   int *sample;
   int *splitters;
   int *mat_counts, *buck_subs;
   int max_bucksz;
   int it, iters;
   double start, finish;

   Get_args(argc, argv, &blk_ct, &th_per_blk, &n, &s, &mod);

#  ifdef DEBUG
   printf("blk_ct = %d, th_per_blk = %d, n = %d, s = %d, mod = %d\n",
         blk_ct, th_per_blk, n, s, mod);
#  endif

   Set_device();

   olist = (int*) malloc(n*sizeof(int));
   hlist = (int*) malloc(n*sizeof(int));
   cudaMallocManaged(&dlist, n*sizeof(int));
   cudaMallocManaged(&tmp_list, n*sizeof(int));

   if (mod == 0)
      Read_list("Enter the list", olist, n);
   else
      Gen_list(olist, n, mod);
#  ifdef PRINT_LIST
   Print_list("Before sort, list = ", olist, n);
#  endif

   cudaMallocManaged(&sample, s*sizeof(int));
   cudaMallocManaged(&splitters, (blk_ct+1)*sizeof(int));
   splitters[blk_ct] = INFTY;
   cudaMallocManaged(&mat_counts, blk_ct*blk_ct*sizeof(int));
   cudaMallocManaged(&buck_subs, (blk_ct+1)*sizeof(int));

   iters = Setup_stats();
   for (it = 0; it < iters; it++) {
      memcpy(dlist, olist, n*sizeof(int));
#     ifdef DEBUG
      printf("Device Start\n");
#     endif
      GET_TIME(start);
      memset(mat_counts, 0, blk_ct*blk_ct*sizeof(int));
      buck_subs[blk_ct] = n;
      Sample_sort(dlist, tmp_list, sample, splitters, mat_counts, 
            buck_subs, n, s, blk_ct, th_per_blk, &max_bucksz);
      GET_TIME(finish);
      /* s_dmin, s_dmax, s_dtotal defined in stats.c */
      Update_stats(start, finish, &s_dmin, &s_dmax, &s_dtotal);

#     ifdef DEBUG
      printf("Device finish\n\n");
      printf("Host Start\n");
#     endif

#     ifndef NO_SERIAL
      memcpy(hlist, olist, n*sizeof(int));
      GET_TIME(start);
      qsort(hlist, n, sizeof(int), Compare);
      GET_TIME(finish);
      /* s_hmin, s_hmax, s_htotal defined in stats.c */
      Update_stats(start, finish, &s_hmin, &s_hmax, &s_htotal);
#     endif
   }  /* for iter */

#  ifdef PRINT_LIST
   Print_list("After sort, dlist = ", dlist, n);
#  endif

   Print_stats("Device", s_dmin, s_dmax, s_dtotal, iters);
   Print_stats("  Host", s_hmin, s_hmax, s_htotal, iters);
   printf("The maximum bucket size in sample sort was %d\n",
         max_bucksz);
   Print_ssort_stats();

   if (Check_sorted(dlist, hlist, n)) 
      printf("Dlist is sorted\n");
#  ifdef PRINT_LIST 
   else 
      Print_list("            hlist = ", hlist, n);
#  endif

   free(olist);
   free(hlist);
   cudaFree(dlist);
   cudaFree(tmp_list);
   cudaFree(sample);
   cudaFree(splitters);
   cudaFree(mat_counts);
   cudaFree(buck_subs);

   return 0;
}  /* main */


/*---------------------------------------------------------------------
 * Function:     Get_args
 * Purpose:      Get the command line arguments
 * In args:      argc, argv
 * Out args:     th_per_blk_p, n_p, s_p, m_p
 */
void Get_args(int argc, char* argv[], int* blk_ct_p, int* th_per_blk_p, 
      int* n_p, int* s_p, int* m_p) {
   if (argc != 5 && argc != 6) {
      fprintf(stderr,"%s <blk_ct> <th_per_blk> <n> <s> [m]\n", argv[0]);
      fprintf(stderr,"      n = list size\n");
      fprintf(stderr,"      s = sample size\n");
      fprintf(stderr,"      m = if m is not present, get list from\n");
      fprintf(stderr,"         stdin.  Otherwise generate the list with\n");
      fprintf(stderr,"         m as the modulus for random numbers.\n");
      exit(0);
   }
   *blk_ct_p = strtol(argv[1], NULL, 10);
   *th_per_blk_p = strtol(argv[2], NULL, 10);
   *n_p = strtol(argv[3], NULL, 10);
   *s_p = strtol(argv[4], NULL, 10);
   if (argc == 6)
      *m_p = strtol(argv[5], NULL, 10);
   else 
      *m_p = 0;
}  /* Get_args */


/*---------------------------------------------------------------------
 * Function:   Read_list
 * Purpose:    Read a list of ints from stdin
 * In args:    prompt, n
 * Out arg:    list
 */
void Read_list(const char prompt[], int list[], int n) {
   int i;

   printf("%s\n", prompt);
   for (i = 0; i < n; i++)
      scanf("%d", &list[i]);
}  /* Read_list */


/*---------------------------------------------------------------------
 * Function:   Gen_list
 * Purpose:    Generate a list of ints using the random function
 * In args:    n, modulus 
 * Out arg:    list
 */
void Gen_list(int list[], int n, int modulus) {
   int i;

   srandom(SEED1);
   for (i = 0; i < n; i++)
      list[i] = random() % modulus;
}  /* Gen_list */


/*---------------------------------------------------------------------
 * Function:   Print_list
 * Purpose:    Print a list of ints to stdout
 * In args:    all
 */
__host__ __device__ void Print_list(const char title[], int list[], int n) {
   int i;

   printf("%s  ", title);
   for (i = 0; i < n; i++)
      printf("%d ", list[i]);
   printf("\n");
}  /* Print_list */


/*---------------------------------------------------------------------
 * Function:   Print_list_strided
 * Purpose:    Print a strided list of ints to stdout
 * In args:    all
 */
__host__ __device__ void Print_list_strided(const char title[], 
      int list[], int n, int stride) {
   int i;

   printf("%s  ", title);
   for (i = 0; i < n; i++)
      printf("%d ", list[i*stride]);
   printf("\n");
}  /* Print_list_strided */


/*---------------------------------------------------------------------
 * Function:   Check_sorted
 * Purpose:    Determine whether dlist is sorted in increasing order
 *             by comparing it to hlist, which has been sorted in
 *             increasing order
 * In args:    all
 * Ret val:    1 if the list is sorted, 0 otherwise
 */
int Check_sorted(int dlist[], int hlist[], int n) {
   int i;

   for (i = 0; i < n-1; i++)
      if (dlist[i] != hlist[i]) {
         printf("dlist isn't sorted: dlist[%d] = %d != %d = hlist[%d]\n",
               i, dlist[i], hlist[i], i);
         return 0;
      }
   return 1;
}  /* Check_sorted */


/*---------------------------------------------------------------------
 * Function:        Sample_sort
 * Purpose:         Use sample sort to sort a list of n ints
 * In args:         n:  size of the list
 *                  s:  sample size
 *                  blk_ct:  number of thread blocks for local sort and 
 *                     number of buckets
 *                  th_per_blk:  number of threads per block for local
 *                     sort
 * In/out arg:      list
 * Out arg:         max_bucksz_p:  the maximum number of elements in
 *                     a bucket
 * Scratch:         sample
 *                  splitters
 *                  mat_counts
 *                  buck_subs:  first number of elements going to each block
 *                              then first element going to each block
 *                  tmp_list
 */
void  Sample_sort(int list[], int tmp_list[], int sample[], 
      int splitters[], int mat_counts[], int buck_subs[], 
      int n, int s, int blk_ct, int th_per_blk, int* max_bucksz_p) {
   int sblk_ct, sth_per_blk, th_per_blk1;

   GET_TIME(start);
   Choose_sample<<<blk_ct, th_per_blk>>>(list, n, sample, s);
   cudaDeviceSynchronize();
   GET_TIME(finish);
   elapsed = finish-start;
   Update_ssort_stats(elapsed, 0);

#  ifdef DEBUG
   printf("After Choose_sample\n");
   Print_list("list =", list, n);
   Print_list("sample =", sample, s);
#  endif

   // Sort the list of all of the samples
   // s = 2*sblk_ct*sth_per_blk
   // sth_per_blk = min(s/2, th_per_blk)
   GET_TIME(start);
   sth_per_blk = ((s/2 < th_per_blk) ? s/2 : th_per_blk);
   sblk_ct = s/(2*sth_per_blk);
#  ifdef DEBUG
   printf("Before Par_bitonic_sort: sblk_ct = %d, sth_per_blk = %d\n", 
         sblk_ct, sth_per_blk);
#  endif
   Par_bitonic_sort(sample, s, sblk_ct, sth_per_blk);
   cudaDeviceSynchronize();
   GET_TIME(finish);
   elapsed = finish-start;
   Update_ssort_stats(elapsed, 1);

#  ifdef DEBUG
   printf("After Par_bitonic_sort\n");
   Print_list("sample =", sample, s);
#  endif

   // Choose the splitters and build the other data structures:
   // Use as few blocks as possible:  maximize the number of threads
   // Remember blk_ct = number of splitters
   GET_TIME(start);
   int spl_blk_ct = Ceiling(blk_ct, MAX_TH_PER_BLK);
// printf("For call to Choose_splitters, spl_blk_ct = %d\n",
//         spl_blk_ct);
   splitters[0] = MINUS_INFTY;
   splitters[blk_ct] = INFTY;
   Choose_splitters<<<spl_blk_ct, MAX_TH_PER_BLK>>>(sample, s, 
         splitters, blk_ct); 
   cudaDeviceSynchronize();
   GET_TIME(finish);
   elapsed = finish-start;
   Update_ssort_stats(elapsed, 2);
#  ifdef DEBUG
   Print_list("splitters = ", splitters, blk_ct+1);
#  endif

   //    mat_counts[i*blk_ct + j] = number of elements of
   //                 list going from block j to block i
   GET_TIME(start);
   Build_mat_counts<<<blk_ct, th_per_blk>>>(list, n, splitters, mat_counts);
   cudaDeviceSynchronize();
   GET_TIME(finish);
   elapsed = finish-start;
   Update_ssort_stats(elapsed, 3);
#  ifdef DEBUG
   printf("After Build_mat_counts\n");
   Print_as_2d("mat_counts", mat_counts, blk_ct, blk_ct); 
#  endif

   // Get exclusive prefix sums of the rows of elts in mat_counts
   // This will give us the indexes in the sorted list of the elements 
   // that each block is responsible for.  This will also store the
   // number of elements going to each block in buck_subs
   GET_TIME(start);
   int tpb = ((blk_ct > MAX_TH_PER_BLK) ? MAX_TH_PER_BLK : blk_ct);
   Row_prefix_sums<<<blk_ct, tpb>>>(mat_counts, buck_subs);
   // Crashes in Final_sort call to Local_sort without the call
   //    cudaDeviceSynchronize
   cudaDeviceSynchronize();
   GET_TIME(finish);
   elapsed = finish-start;
   Update_ssort_stats(elapsed, 4);
#  ifdef DEBUG
   printf("After Row_prefix_sums\n");
   Print_as_2d("mat_counts = ", mat_counts, blk_ct, blk_ct);
   Print_list("buck_subs = ", buck_subs, blk_ct);
#  endif

   // Serial prefix sums of the elements in buck_subs
   GET_TIME(start);
   Ser_prefix_sums(buck_subs, blk_ct);
   GET_TIME(finish);
   elapsed = finish-start;
   Update_ssort_stats(elapsed, 5);
#  ifdef DEBUG
   printf("After Prefix_sums\n");
   Print_list("buck_subs = ", buck_subs, blk_ct+1);
#  endif

   GET_TIME(start);
   Map<<<blk_ct, th_per_blk>>>(list, tmp_list, n, mat_counts, 
            buck_subs, splitters);
   cudaDeviceSynchronize();
   GET_TIME(finish);
   elapsed = finish-start;
   Update_ssort_stats(elapsed, 6);
#  ifdef DEBUG
   printf("After Map\n");
   Print_list("tmp_list = ", tmp_list, n);
#  endif

   // Find the smallest integer max_bucksz such that 
   //    no bucket has more than max_bucksz elements,
   //    and max_bucksz is a power of 2.
   GET_TIME(start);
   *max_bucksz_p = Get_max_bucksz(buck_subs, blk_ct);
// printf("max_bucksz = %d\n", *max_bucksz_p);
   if (*max_bucksz_p > 2*MAX_TH_PER_BLK) {
      th_per_blk1 = MAX_TH_PER_BLK;
   } else {
      th_per_blk1 = th_per_blk;
   }

#  ifdef FORCE_GLBL_MEM
   *max_bucksz_p = MAX_SHMEM+1;
#  endif

   int new_alloc_sz = 0, *new_buck_subs = NULL, *big_buck_arr = NULL;
   if (*max_bucksz_p > MAX_SHMEM) {
      cudaMallocManaged(&new_buck_subs, (blk_ct+1)*sizeof(int));
      Find_new_buck_subs(buck_subs, blk_ct, new_buck_subs, &new_alloc_sz);
      cudaError_t err = cudaMallocManaged(&big_buck_arr, 
            new_alloc_sz*sizeof(int));
      if (err != cudaSuccess) {
         fprintf(stderr, "***Can't allocate big_buck_arr!***\n");
         exit(-1);
      }
//    Print_list("buck_subs", buck_subs, blk_ct+1);
//    Print_list("new_buck_subs", new_buck_subs, blk_ct+1);
   }

   // Maximum number of elements in a bucket is max_bucksz
   //    So we may need as many as max_bucksz/2 threads for
   //    the local sorts
   Final_sort<<<blk_ct, th_per_blk1>>>(list, tmp_list, n,
      buck_subs, big_buck_arr, new_buck_subs);
   cudaDeviceSynchronize();
   GET_TIME(finish);
   elapsed = finish-start;
   Update_ssort_stats(elapsed, 7);
   if (big_buck_arr != NULL) cudaFree(big_buck_arr);
   if (new_buck_subs != NULL) cudaFree(new_buck_subs);
}  /* Sample_sort */


/*----------------------------------------------------------------------
 * Function:   Get_max_bucksz
 * Purpose:    Find the smallest p power of 2 such that no bucket 
 *             stores more than 2^p elements.
 * In args:    All
 * Ret val:    2^p
 */
int Get_max_bucksz(int buck_subs[], int blk_ct) {
   int max = 0;
   int power2, buck, bucksz;

   for (buck = 0; buck < blk_ct;  buck++) {
      bucksz = buck_subs[buck+1] - buck_subs[buck];
      power2 = Min_power2(bucksz);
      if (power2 > max) max = power2;
   }
   return max;
}  /* Get_max_bucksz */


/*----------------------------------------------------------------------
 * Function:  Min_power2
 * Purpose:   Find the smallest power of 2 that is >= x
 * Note:      if x <= 0, the function just returns x
 */
int Min_power2(int x) {
   if (x <= 0) return x;
   unsigned y = 1;
   while (y < x) y <<= 1;
   return y;
}  /* Min_power2 */


/*----------------------------------------------------------------------
 * Function:  Ceiling
 * Purpose:   Find the ceiling of num/denom
 */
__host__ __device__ int Ceiling(int num, int denom) {
   int remainder = num % denom;
   int quotient = num/denom;

   // If remainder != 0, then (remainder != 0) == 1
   int retval = quotient + (remainder != 0);
   return retval;
}  /* Ceiling */


/*----------------------------------------------------------------------
 * Function:   Compare
 * Purpose:    Comparison function for use in calls to qsort
 * In args:    a_p, b_p
 * Ret val:    if *a_p < *b_p:  -1 
 *             if *a_p > *b_p:   1 
 *             if *a_p = *b_p:   0 
 */
int  Compare(const void *a_p, const void *b_p) {
   int a = *((int*) a_p);
   int b = *((int*) b_p);

   if (a < b)
      return -1;
   else if (a > b)
      return 1;
   else /* a == b */
      return 0;
}  /* Compare */


/*----------------------------------------------------------------------
 * Function:   Choose_sample
 * Purpose:    Choose the sample from the original list.
 * In args:    n, s
 * In/out arg: list
 * Out arg:    sample
 * Note:       
 * 1.  The sample is not sorted when the function returns
 * 2.  The elements assigned to each thread block are sorted.   
 */
__global__ void Choose_sample(int list[], int n, int sample[], int s) {
   int blk_ct = gridDim.x;
   int th_per_blk = blockDim.x;
   int my_blk = blockIdx.x;
   int my_rk = threadIdx.x;
   int sublist_sz = n/blk_ct;
   int blk_offset = my_blk*sublist_sz;
   int loc_n = n/blk_ct;
   __shared__ int slist[MAX_SHMEM];  
   int* my_list = list + blk_offset;

   /* Copy my_list in global memory into slist in shared memory */
   /*    Note we assume 2 list elements per thread              */
   slist[my_rk] = my_list[my_rk];
   slist[my_rk + sublist_sz/2] = my_list[my_rk + sublist_sz/2];
   __syncthreads();

   Local_sort (slist, sublist_sz, my_rk, th_per_blk);

   /* Copy slist in shared memory back into my_list in global memory */
   my_list[my_rk] = slist[my_rk];
   my_list[my_rk + sublist_sz/2] = slist[my_rk + sublist_sz/2];

   // Now get the sample, using the copy of the sorted sublist 
   //    stored in shared memory
   int loc_s = s/blk_ct;
   int samp_offset = my_blk*loc_s;
#  ifdef DEBUG
   printf("Blk %d Th %d > loc_s = %d, samp_offset = %d\n",
         my_blk, my_rk, loc_s, samp_offset);
#  endif

   if (my_rk < loc_s) {
      sample[samp_offset + my_rk] =  slist[my_rk*loc_n/loc_s];
#     ifdef DEBUG
      printf("Blk %d Th %d > sample[samp_offset + my_rk] = %d, slist[my_rk] = %d\n",
            my_blk, my_rk, sample[samp_offset + my_rk], slist[my_rk]);
#     endif
   }

}  /* Choose_sample */

/*---------------------------------------------------------------------
 * Function:    Local_sort
 * Purpose:     The original per thread block bitonic sort required that 
 *              the number of elements in the sublist assigned to the calling
 *              thread block be twice the number of threads, and the number
 *              of threads be a power of two.  This version generalizes the
 *              size of the sublist to be any power of two, regardless of
 *              of the number of threads in the thread block.
 * In args:     my_list_sz:  the size of the sublist assigned to this block
 *              th_per_blk:  the number of threads in the block.
 *              my_rk:       my thread rank in this block
 * In/out arg:  slist:       unsorted list on input, sorted list on
 *                           return
 */
__device__ void Local_sort(int slist[], int sublist_sz, int my_rk,
      int th_per_blk) {
   unsigned bf_sz, stage, my_elt1, my_elt2, which_bit, wb_st, i;

   for (bf_sz = 2, wb_st = 0; bf_sz <= sublist_sz; 
         bf_sz = bf_sz << 1, wb_st++) {
      for (stage = bf_sz >> 1, which_bit = wb_st; stage > 0;  
            stage = stage >> 1, which_bit--) {
         for (i = my_rk; i < sublist_sz/2; i += th_per_blk) {
            my_elt1 = Insert_zero(i, which_bit);
            my_elt2 = my_elt1 ^ stage;

            Compare_swap(slist, my_elt1, my_elt2, my_elt1 & bf_sz);
         }
         __syncthreads();
      }
   }
}  /* Local_sort */

///*----------------------------------------------------------------------
// * Function:    Local_sort
// *
// * Purpose:     Bitonic sort of sublist assigned to the calling thread
// *              block
// * In args:     my_list_sz:  the size of the sublist assigned to this block
// *              my_rk:       my thread rank in this block
// * In/Out arg:  slist:       unsorted list on input, sorted list on
// *                           return
// * Note:        Assumes sublist_sz = 2*th_per_blk, and th_per_blk (and
// *              hence sublist_sz) are both powers of two.
// */
//__device__ void Local_sort(int slist[], int my_list_sz, int my_rk) {
//   unsigned bf_sz, stage, my_elt1, my_elt2, which_bit, wb_st;
//
//   for (bf_sz = 2, wb_st = 0; bf_sz <= my_list_sz; 
//         bf_sz = bf_sz << 1, wb_st++) {
//      for (stage = bf_sz >> 1, which_bit = wb_st; stage > 0;  
//            stage = stage >> 1, which_bit--) {
//         my_elt1 = Insert_zero(my_rk, which_bit);
//         my_elt2 = my_elt1 ^ stage;
//         Compare_swap(slist, my_elt1, my_elt2, my_elt1 & bf_sz);
//         __syncthreads();
//      }
//   }
//}  /* Local_sort */


/*---------------------------------------------------------------------
 * Function:    Insert_zero
 * Purpose:     Insert a zero in the binary representation of 
 *              val between bits j and j-1
 */
__device__ unsigned Insert_zero(unsigned val, unsigned j) {
   unsigned left_bits, right_bits, left_ones, right_ones;

   left_ones = ALL_ONES << j;
   right_ones = ~left_ones;
   left_bits = left_ones & val;
   right_bits = right_ones & val;
   return (left_bits << 1) | right_bits;
}  /* Insert_zero */


/*-----------------------------------------------------------------
 * Function:    Compare_swap
 * Purpose:     Compare two elements in the list, and if out of order,
 *              swap:
 *
 *                 inc_dec = INC => pair should increase
 *                 inc_dec != INC => pair should decrease
 *             
 * In args:     elt, partner:  subscripts of the pair of elements being 
                      compared
 *                 elt should always be < partner
 *              inc_dec:   whether pair should increase (0)
 *                 or decrease (!=0)
 * In/out arg:  my_list:  the list
 */
__device__ void Compare_swap(int my_list[], unsigned elt, unsigned partner,
      unsigned inc_dec) {
   int tmp;

   if (inc_dec == INC) {
      if (my_list[elt] > my_list[partner]) {
         tmp = my_list[elt];
         my_list[elt] = my_list[partner];
         my_list[partner] = tmp;
      }
   } else {  /* inc_dec != INC */
      if (my_list[elt] < my_list[partner]) {
         tmp = my_list[elt];
         my_list[elt] = my_list[partner];
         my_list[partner] = tmp;
      }
   }
}  /* Compare_swap */


/*----------------------------------------------------------------------
 * Function:   Choose_splitters
 * Purpose:    Assign values to splitters as follows:
 *             bucket 0 =  -infty = splitters[0] <=  x < splitters[1] 
 *             bucket 2 =           splitters[1] <=  x < splitters[2] 
 *                 ...
 *           bucket b-1 =         splitters[b-1] <=  x < splitters[b] = infty
 * In arg:     sample, sample_sz, b
 * Out arg:    splitters
 * Note:       splitters[0] and splitters[b] are assigned on the host.
 */
__global__ void Choose_splitters(int sample[], int sample_sz, 
      int splitters[], int b) {
   int glb_rk = blockDim.x*blockIdx.x + threadIdx.x;
   int splitter_gap = sample_sz/b;

   if (1 <= glb_rk && glb_rk < b) {
//    printf("Blk %d, Th %d >  Choose_splitters, glb_rk = %d, splitter_gap = %d\n", blockIdx.x, threadIdx.x, glb_rk, splitter_gap);
      splitters[glb_rk] = sample[glb_rk*splitter_gap];
   }
}  /* Choose_splitters */


/*----------------------------------------------------------------------
 * Function:   Build_mat_counts
 * Purpose:    Determine the cutoffs for the buckets.  There are
 *             b + 1 = blk_ct + 1 splitters: see note 1 below.
 *             Use these to determine the number of elements in 
 *             the sublist belonging to the current block that 
 *             should go to each of the blocks.  The idea is that 
 *
 *                mat_counts[i][j] = number of elements of
 *                   list going from block j to block i
 *
 *             and if the current block is block j, we figure
 *             mat_counts[i][j] for each block i.
 *                 
 *             However we use a one-dimensional array for mat_counts.
 *             So mat_counts[i][j] is actually mat_counts[i*blk_ct + j].
 *
 * In args:    list:       the list being sorted 
 *             n:          the total number of elements in the list 
 *             splitters:  the cutoffs for the blocks to which elements
 *                         in the list should go.  See note 1.
 * Out args:   mat_counts: See "Purpose" above.
 *
 * Notes:
 * 1.  The splitters and buckets are
 *
 *             bucket 0 : -infty = splitters[0] <=  x < splitters[1] 
 *             bucket 2 :          splitters[1] <=  x < splitters[2] 
 *                 ...
 *           bucket b-1 :        splitters[b-1] <=  x < splitters[b] = infty
 *
 * 2. mat_counts is initialized to 0 on the host.
 */
__global__ void Build_mat_counts(int list[], int n, int splitters[], 
      int mat_counts[]) {
   int b = gridDim.x;
   int loc_blk = blockIdx.x;
   int loc_rk = threadIdx.x;
   int loc_n = n/b;  // Number of elements currently assigned to this block
   int* loc_list = list + blockIdx.x*loc_n;  
                     // Elements currently assigned to this block
   __shared__ int ssplit[MAX_SHMEM]; // Fast storage for the splitters
   int bucket, bucket1;  // Each thread is responsible for two buckets.

   // Load the splitters into shared memory
   for (int i = loc_rk; i <= b; i += blockDim.x)
      ssplit[i] = splitters[i]; 
   __syncthreads();

   bucket = Find_bucket(loc_list[loc_rk], ssplit, 0, b);
// bucket = Find_bucket(loc_list[loc_rk], splitters, b);
#  ifdef DEBUG
   printf("Blk %d, Th %d > val = %d, bucket = %d\n",
         blockIdx.x, loc_rk, loc_list[loc_rk], bucket);
#  endif
   atomicAdd(&mat_counts[bucket*b + loc_blk], 1);

   bucket1 = Find_bucket(loc_list[loc_rk + loc_n/2], 
         ssplit, bucket, b);
#  ifdef DEBUG
   printf("Blk %d, Th %d > val = %d, bucket = %d\n",
         blockIdx.x, loc_rk, loc_list[loc_rk+loc_n/2], bucket1);
#  endif
   atomicAdd(&mat_counts[bucket1*b + loc_blk], 1);

   // This takes no advantage of the fact that we know the
   //    bucket *follows* the first bucket
// bucket = Find_bucket(loc_list[loc_rk + loc_n/2], ssplit, b);
// bucket = Find_bucket(loc_list[loc_rk + loc_n/2], splitters, b);
// printf("Blk %d, Th %d > val = %d, bucket = %d\n",
//       blockIdx.x, loc_rk, loc_list[loc_rk + loc_n/2], bucket);
// atomicAdd(&mat_counts[bucket*b + loc_blk], 1);

   // Need a barrier here.  So return to host.

}  /* Build_mat_counts */


/*---------------------------------------------------------------------
 * Function:      Find_bucket
 * Purpose:       Binary search of the array splitters for the
 *                subscript i in splitters with the property that
 *                splitters[i] <= elt < splitters[i+1].
 * In args:       all
 * Note:          splitters[b] = infty
 */
__device__ int Find_bucket(int elt, int splitters[], int min, int max) {
   int splmin = splitters[min], splmax = splitters[max];

   while (min <= max) {
      int mid = (min+max)/2;
//    printf("Blk %d Th %d > elt = %d, min = %d, max = %d, mid = %d, spl[mid] = %d, spl[mid+1] = %d\n",
//          blockIdx.x, threadIdx.x, elt, min, max, mid,
//          splitters[mid], splitters[mid+1]);
      if ((splitters[mid] <= elt) && (elt < splitters[mid+1]))
         return mid;
      else if (splitters[mid] < elt)
         min = mid + 1;
      else // splitters[mid] > elt
         max = mid - 1;
   }
   // Something went wrong ... 
   printf("Blk %d, Th %d > elt = %d, Can't find bucket, min = %d, max = %d\n",
         blockIdx.x, threadIdx.x, elt, splmin, splmax);
   return 0;
}  /* Find_bucket */


/*-------------------------------------------------------------------
 * Function:   Warp_sum
 * Purpose:    Tree-structured Sum of the values in a warp
 *
 * Note:       The thread with lane id 0 in the warp is the only
 *             thread that returns the correct sum
 *
 * Note:       The function assumes that a full warp of WARPSZ
 *             threads is running the function.
 */
__device__ int Warp_sum(const int my_val) {
   int my_result = my_val;

   for (unsigned diff = warpSize/2; diff > 0; diff = diff/2) {
      my_result += __shfl_down_sync(0xFFFFFFFF, my_result, diff);
   }

   return my_result;
}  /* Warp_sum */

/*----------------------------------------------------------------------
 * Function:    Row_prefix_sums
 * Purpose:     Find the locations in each block's sublist of mat_counts of 
 *              the starting point of all the blocks' sublists.  
 *              This is done by taking the exclusive prefix sums 
 *              of the rows of mat_counts.  
 *              
 * In arg:      n:  number of elements in global mat_counts
 * Out arg:     buck_subs:  buck_subs[i] = number of elements
 *                 going to block i = bucket i in final, sorted mat_counts.
 * In/out arg:  mat_counts:  
 *              On input
 *
 *                 mat_counts[j][i] = mat_counts[j*blk_ct + i]
 *
 *              is the number of elements from sublist i going to
 *              bucket j, 0 <= i, j < blk_ct.
 *
 *              On output:
 *
 *                 mat_counts[j][0] = 0, 0 <= j < blk_ct
 *                 mat_counts[j][i] = Sum of the first i-1 
 *                    entries in row j, 0 < i < blk_ct
 *                 buck_subs[j] = total number of
 *                    of elements going to bucket j = sum
 *                    of the input elements in row j of 
 *                    mat_counts.
 *
 * Notes:
 * 1.  mat_counts has blk_ct rows and blk_ct cols.
 * 2.  th_per_blk should be evenly divisible by blk_ct.
 */
__global__ void Row_prefix_sums(int mat_counts[], int buck_subs[]) {
   int blk_ct = gridDim.x;
   int my_blk = blockIdx.x;
   int th_per_blk = blockDim.x;
   int my_rk = threadIdx.x;
   int* my_row = mat_counts + my_blk*blk_ct;
   __shared__ int temp[MAX_SHMEM];  // Double buffer

   unsigned shift;
   int iters, i, offout, offin;
   int pout = 0, pin = 1;  // Switch between the two buffers in temp

   iters = blk_ct/th_per_blk;  

   // Shift my_row to right one element, and copy last element in
   //    my_row into buck_subs
   if (my_rk == 0) {
      temp[0] = 0;
      buck_subs[my_blk] = my_row[blk_ct-1];
   }
   for (i = my_rk; i < blk_ct-1; i += th_per_blk) {
      temp[i + 1] = my_row[i];
   }
   for (i = my_rk; i < blk_ct; i += th_per_blk) { 
      temp[blk_ct + i] = 0;
   }
   __syncthreads();
#  ifdef DEBUG
   if (my_blk == 0 && my_rk == 0)
      Print_as_2d("after init", temp, 2, blk_ct);
   __syncthreads();
#  endif

   // Now do an exclusive prefix sum on the elements in temp
   // First do a prefix sum on each sublist of th_per_blk elts
   for (shift = 1; shift < th_per_blk; shift <<= 1) {
      pout = 1 - pout;
      pin = 1 - pin;
      offout = pout*blk_ct;
      offin = pin*blk_ct;
      for (i = my_rk; i < blk_ct; i += th_per_blk) {
         if (my_rk >= shift) {
            temp[offout + i] = temp[offin + i] + temp[offin + i - shift];
         } else {
            temp[offout + i] = temp[offin + i];
         }
         __syncthreads(); 
#        ifdef DEBUG
         if (my_blk == 0 && my_rk == 0)
            Print_as_2d("after shift", temp, 2, blk_ct);
         __syncthreads();
#        endif
      }
   }
  
   // When i >= th_per_blk, add in the last value in temp 
   //    from the previous iteration
   for (int it = 1; it < iters; it++) {
      pout = 1 - pout;
      pin = 1 - pin;
      offout = pout*blk_ct;
      offin = pin*blk_ct;
      int curr_start = it*th_per_blk;
      int curr_end = curr_start + th_per_blk;
      int add_in = temp[offin + it*th_per_blk - 1];
      for (i = my_rk; i < blk_ct; i += th_per_blk) {
         if (curr_start <= i && i < curr_end)
            temp[offout + i] = temp[offin + i] + add_in;
         else
            temp[offout + i] = temp[offin + i];
      }
      __syncthreads();
   }
#  ifdef DEBUG
   if (my_blk == 0 && my_rk == 0)
      Print_as_2d("after cleanup", temp, 2, blk_ct);
   __syncthreads();
#  endif

   // Copy back into mat_counts
   for (i = my_rk; i < blk_ct; i += th_per_blk) {
      my_row[i] = temp[offout + i];
   }

   // Don't need to sync, since tmp_row was already synced
   if (my_rk == 0) buck_subs[my_blk] += temp[offout + blk_ct - 1]; 
}  /* Row_prefix_sums */


///*----------------------------------------------------------------------
// * Function:    Row_prefix_sums
// * Purpose:     Find the locations in each block's sublist
// *              the starting point of all the blocks' sublists.  
// *              This is done by taking the exclusive prefix sums 
// *              of the rows of mat_counts.  
// *              
// * In arg:      n:  number of elements in global list
// * Out arg:     buck_subs:  buck_subs[i] = number of elements
// *                 going to bucket i in final, sorted list.
// * In/out arg:  mat_counts:  
// *              On input
// *
// *                 mat_counts[j][i] = mat_counts[j*blk_ct + i]
// *
// *              is the number of elements from block i going to
// *              block j, 0 <= i, j < blk_ct.
// *
// *              On output:
// *
// *                 mat_counts[j][0] = 0
// *                 mat_counts[j][i] = Sum of the first i-1 
// *                    entries in row j, 0 < i < blk_ct
// *                 buck_subs[j] = total number of
// *                    of elements going to block j = sum
// *                    of the input elements in row j of 
// *                    mat_counts.
// *
// * Note that mat_counts has blk_ct rows and blk_ct cols.
// *
// * Note that we assume blk_ct is evenly divisible by th_per_blk.
// */
//__global__ void Row_prefix_sums(int mat_counts[], int buck_subs[]) {
//   int blk_ct = gridDim.x;
//   int my_blk = blockIdx.x;
//   int th_per_blk = blockDim.x;
//   int my_rk = threadIdx.x;
//   int* my_row = mat_counts + my_blk*blk_ct;
//   __shared__ int tmp_row[MAX_SHMEM];
//
//   unsigned shift;
//   int iters, i, offset;
//
//   iters = blk_ct/th_per_blk;  
//
//   // Shift my_row to right one element, and copy last element in
//   //    my_row into buck_subs
//   if (my_rk == 0) {
//      tmp_row[0] = 0;
//      buck_subs[my_blk] = my_row[blk_ct-1];
//   }
//   for (i = my_rk; i < blk_ct-1; i += th_per_blk) {
//      tmp_row[i + 1] = my_row[i];
//   }
//   __syncthreads();
//
//#  ifdef DEBUG
//   if (my_rk == 0) Print_list("After shift", tmp_row, blk_ct);
//#  endif
//
//   // Now do an exclusive prefix sum on the elements in tmp_row
//   for (i = my_rk; i < blk_ct; i += th_per_blk) {
//      // Write and read shmem
//      for (shift = 1; shift < th_per_blk; shift <<= 1) {
//         if (my_rk >= shift) {
//            tmp_row[i] += tmp_row[i - shift];
//         }
//         __syncthreads(); 
//      }
//      // When i >= th_per_blk, add in the last value in tmp_row 
//      //    from the previous iteration
//      if (i >= th_per_blk) tmp_row[i] += tmp_row[i- my_rk - 1];
//      __syncthreads();
//#     ifdef DEBUG
//      if (my_rk == 0) Print_list("After iter", tmp_row, blk_ct);
//#     endif
//   }
//
//   // Copy back into mat_counts
//   for (i = 0; i < iters; i++) {
//      offset = i*th_per_blk;
//      my_row[my_rk + offset] = tmp_row[my_rk + offset];
//   }
//
//   // Don't need to sync, since tmp_row was already synced
//   if (my_rk == 0) buck_subs[my_blk] += tmp_row[blk_ct-1]; 
//}  /* Row_prefix_sums */
//
//
//__global__ void Row_prefix_sums(int mat_counts[], int buck_subs[], int n) {
//   __shared__ float temp[SHMEM_MAX]; // allocated on invocation
//   int thid = threadIdx.x;
//   int pout = 0, pin = 1;
//   // Load input into shared memory.
//    // This is exclusive scan, so shift right by one
//    // and set first element to 0
//   temp[pout*n + thid] = (thid > 0) ? g_idata[thid-1] : 0;
//   __syncthreads();
//   for (int offset = 1; offset < n; offset *= 2)
//   {
//     pout = 1 - pout; // swap double buffer indices
//     pin = 1 - pout;
//     if (thid >= offset)
//       temp[pout*n+thid] += temp[pin*n+thid - offset];
//     else
//       temp[pout*n+thid] = temp[pin*n+thid];
//     __syncthreads();
//   }
//   g_odata[thid] = temp[pout*n+thid]; // write output
//}


/*----------------------------------------------------------------------
 * Function:    Find_new_buck_subs
 * Purpose:     Find bucket sizes after adjusting so that each
 *                 bucket size is a power of 2.  Also determine
 *                 whether there is enough shared memory to
 *                 for each bucket, and the total allocation
 *                 needed for the buckets after adjusting sizes
 *                 to be powers of 2. 
 * In arg:      blk_ct:  number of buckets   
 *              buck_subs:  sizes of buckets before adjusting them
 *                 to be powers of 2.
 * Out args:    new_buck_subs:  smallest power of 2 >= corresponding
 *                 bucket size
 *              new_alloc_sz:  the total amount of memory needed
 *                 for all the adjusted buckets.
 */
void Find_new_buck_subs(int buck_subs[], int blk_ct, int new_buck_subs[], 
      int *new_alloc_sz_p) {
   int bkt, new_alloc_sz = 0, blk_alloc, bs_old, bs_new;

   for (bkt = 0; bkt < blk_ct; bkt++) {
      blk_alloc = Min_power2(buck_subs[bkt+1] - buck_subs[bkt]);
      new_buck_subs[bkt] = blk_alloc;
      new_alloc_sz += blk_alloc;
   }

   bs_old = new_buck_subs[0];
   new_buck_subs[0] = 0;
   for (bkt = 1; bkt < blk_ct; bkt++) {
      bs_new = new_buck_subs[bkt];
      new_buck_subs[bkt] = bs_old + new_buck_subs[bkt-1];
      bs_old = bs_new;
   }
   new_buck_subs[blk_ct] = new_alloc_sz;

   *new_alloc_sz_p = new_alloc_sz;
}  /* Find_new_buck_subs */


/*----------------------------------------------------------------------
 * Function:    Ser_prefix_sums
 * Purpose:     Find the exclusive prefix sums of the elements in the
 *              buck_subs.  
 * In arg:      blk_ct:  the number of rows in mat_counts, blk_ct + 1 is
 *                 the number of cols.
 * In/out arg:  buck_subs:  On input buck_subs[k] is the number of elements 
 *                 assigned to block k for the final sort.
 *                 On return, this will be replaced by the sum of the
 *                 elements preceding buck_subs[k] (0 for buck_subs[0]).
 *                 So on return buck_subs[k] is the subscript of the 
 *                 first element assigned to block k (i.e., bucket k)
 *                 in the sorted list.
 */
void Ser_prefix_sums(int buck_subs[], int blk_ct) {
   int i, sum_so_far, curr_elt; 
   
   curr_elt = buck_subs[0];
   sum_so_far = 0;
   buck_subs[0] = 0;
   for (i = 1; i < blk_ct; i++) {
      sum_so_far += curr_elt;
      curr_elt = buck_subs[i];
      buck_subs[i] = sum_so_far;
   }
      
}  /* Ser_prefix_sums */

/*---------------------------------------------------------------------
 * Function:    Map
 * Purpose:     Assign each element in list (the src) to the correct
 *              location in its destination bucket (the dest). 
 * In args:     
 *              list: The global list. Each sublist of elements assigned 
 *                 to a block (n/blk_ct elements) is sorted.  
 *              n:  the total number of elements in the list
 *              mat_counts:  mat_counts[k][j] = subscript in bucket
 *                 of first element in sublist of list belonging to block j 
 *                 that is getting assigned to bucket k.
 *                 Order:  blk_ct x blk_ct
 *              buck_subs:  buck_subs[k] = index in sorted list of
 *                 first element in bucket k.
 *                 Order:  blk_ct + 1:  last element is blk_ct
 *              splitters: If x is an element of the list, and
 *                 splitters[k] <= x < splitters[k+1], 
 *                 0 <= k < blk_ct-1, then x should be mapped 
 *                 to bucket k.  splitters[0] = -infty, 
 *                 splitters[blk_ct] = infty.
 *                 Order:  blk_ct+1
 * Out args:    tmp_list:  Storage for the buckets.  When the
 *                 kernel returns each bucket occupies a contiguous
 *                 subarray.  Note that this needs to be global 
 *                 memory, since, in general, all the blocks are 
 *                 writing to each bucket.
 */
__global__ void Map(int list[], int tmp_list[], int n, 
      int mat_counts[], int buck_subs[], int splitters[]) {
   int blk_ct = gridDim.x;
   int th_per_blk = blockDim.x;
   int my_blk = blockIdx.x;
   int my_rk = threadIdx.x;
   int sublist_sz = n/blk_ct;
   int *sublist = list + my_blk*sublist_sz;
   int k, dest_buck, dest_sub, last_buck;
   int first_elt_sub;
   int offset_into_buck, dest_buck_start;
   __shared__ int ssplit[MAX_SHMEM];

   // Load the splitters into shared memory
   for (int i = my_rk; i <= blk_ct; i += th_per_blk)
      ssplit[i] = splitters[i];
   __syncthreads();

#  ifdef DEBUG
   printf("Bl %d Th %d > sublist_sz = %d, sublist_start = %d, sublist_end = %d, th_per_blk = %d\n",
         my_blk, my_rk, sublist_sz, sublist_start, sublist_end, th_per_blk);
#  endif

   // last_buck is used to reduce the cost of the search for dest_buck 
   last_buck = 0;
   for (k = my_rk; k < sublist_sz; k += th_per_blk) {
      // Determine which bucket list[k] should be assigned to
      int elt = sublist[k];
      dest_buck = Get_dest_buck(elt, ssplit, last_buck, blk_ct);
      last_buck = dest_buck;
#     ifdef DEBUG
//    printf("Bl %d Th %d > k = %d, elt = %d, dest_buck = %d, last_buck = %d\n",
//          my_blk, my_rk, k, elt, dest_buck, last_buck);
#     endif

      // Get the subscript of the first element in the sublist
      //    that maps to the same bucket as sublist[k]
      first_elt_sub = Get_first_elt_sub(sublist, k, ssplit[dest_buck]);
      offset_into_buck = k - first_elt_sub;
      dest_buck_start = mat_counts[dest_buck*blk_ct + my_blk]
                        + buck_subs[dest_buck];
      dest_sub = dest_buck_start + offset_into_buck;
      tmp_list[dest_sub] = elt;

#     ifdef DEBUG
      printf("Bl %d Th %d > k = %d, first_elt_sub = %d, offset_into_buck = %d, dest_buck_start = %d, dest_sub = %d\n",
            my_blk, my_rk, k, first_elt_sub, offset_into_buck, 
            dest_buck_start, dest_sub);
#     endif
   }

}  /* Map */


/*---------------------------------------------------------------------
 * Function:    Get_dest_buck
 * Purpose:     Given an element of the list, determine which bucket
 *              it should be assigned to.
 * In args:     element, splitters, blk_ct
 *              last_buck:  the last bucket this thread had as a
 *                 destination.  Since the subscript of the element arg
 *                 is increasing over successive calls to Get_src_blk,
 *                 there's no need to search splitters corresponding
 *                 to earlier buckets
 * Ret val:     The bucket to which element should be assigned in
 *              the sorted list.
 */
__device__ int Get_dest_buck(int elt, int splitters[], int last_buck, 
      int blk_ct) {
   int buck;

   buck = Find_bucket(elt, splitters, last_buck, blk_ct);
   return buck;
} /* Get_dest_buck */


/*---------------------------------------------------------------------
 * Function:   Get_first_elt_sub
 * Purpose:    Given a subscript k of an element x = sublist[k] in the 
 *             calling block's sublist, and the splitter = splitters[i] 
 *             satisfying 
 *
 *                  splitters[i] <= x < splitters[i+1],
 *
 *             find the subscript j of the first element y = sublist[j] in 
 *             this block's sublist such that either
 *
 *                  sublist[j-1] < splitters[i] or
 *                  j-1 < first element of the block
 *
 * In args:    sublist:  the current thread/blocks sublist of the global list
 *             k:  k the subscript of the element x
 *             splitter:  splitters[i]
 * Ret val:    j as specified in Purpose
 */
__device__ int Get_first_elt_sub(int sublist[], int k, int splitter) {

   int j = k-1;
   while (j >= 0 && sublist[j] >= splitter) 
      j--;
   return ++j;
} /* Get_first_elt_sub */


/*----------------------------------------------------------------------
 * Function:  Final_sort
 * Purpose:   Each bucket is stored in a sublist of tmp_list.
 *            Each thread block sorts one of the buckets and then copies
 *            the sorted bucket into list.
 * In args:   tmp_list:  the unsorted buckets stored in subarrays.
 *               order n.
 *            n:  the number of elements in list and tmp_list
 *            buck_subs:  subscripts of the first element in
 *               each of the buckets.  Last element is n.
 *               order blk_ct+1.
 *            new_buck_subs:  subscripts of the first elements
 *               in each of the buckets, after the bucket sizes
 *               have been adjusted to be powers of 2.  NULL
 *               if no adjust bucket size is > MAX_SHMEM
 * Out arg:   list:  the sorted list.
 *               order n.
 * Scratch:   big_buck_arr:  global array capable of storing
 *               adjusted bucket sizes.  NULL if no adjusted bucket
 *               size is > MAX_SHMEM.
 */
__global__ void Final_sort(int list[], int tmp_list[], int n,
      int buck_subs[], int* big_buck_arr, int* new_buck_subs) {
// int blk_ct = gridDim.x;
   int my_blk = blockIdx.x;
   int my_rk = threadIdx.x;
   int th_per_blk = blockDim.x;
   __shared__ int shmem[MAX_SHMEM];
   int my_start = buck_subs[my_blk];
   int buck_sz = buck_subs[my_blk+1] - my_start;
   int* subarr = tmp_list + my_start;
   int* my_buck;
   int k;

   /* Smallest power of 2 >= buck_sz */
   int buck_sz_p2 = Min_power2(buck_sz);
#  ifdef FORCE_GLBL_MEM
   if (buck_sz_p2 > 0) 
#  else
   if (buck_sz_p2 > MAX_SHMEM)
#  endif
      my_buck = big_buck_arr + new_buck_subs[my_blk];
   else
      my_buck = shmem;

   for (k = my_rk; k < buck_sz; k += th_per_blk)
      my_buck[k] = subarr[k];
   for ( ; k < buck_sz_p2; k += th_per_blk)
      my_buck[k] = INFTY;
   __syncthreads();

   // Bitonic sort of a single thread block
   // Note that Local_sort should correctly handle the case
   //    when 2*buck_sz_p2 < th_per_blk
   Local_sort(my_buck, buck_sz_p2, my_rk, th_per_blk);
   __syncthreads();

#  ifdef DEBUG
   if (my_rk == 0) Print_list("my_buck", my_buck, buck_sz_p2);
#  endif

   // Copy my_buck into list in global memory 
   int* my_list = list + my_start;
   for (k = my_rk; k < buck_sz; k += th_per_blk)
      my_list[k] = my_buck[k];
}  /* Final_sort */


/*----------------------------------------------------------------------
 * Function:  Clog2
 * Purpose:   Find ceiling of log_2 of n
 * In arg:    n
 * Ret val:   ceil(log_2(n))
 */
__device__ int Clog2(int n) {
   unsigned tn = n, clog = 0;

   while (tn > 1) {
      tn = tn >> 1;
      clog++;
   }
   if (n > (1 << clog)) clog++;
   return clog;
}  /* Clog2 */



/*----------------------------------------------------------------------
 * Function:   Print_as_2d
 * Purpose:    Print a one dimensional array as a two dimensional
 *                with 
 * In args:    all
 */
__host__ __device__ void Print_as_2d(const char title[], int arr[], int rows, 
      int cols) {
   int i, j;

   printf("%s: \n", title);
   for (i = 0; i < rows; i++) {
      for (j = 0; j < cols; j++)
         printf("%2d ", arr[i*cols + j]);
      printf("\n");
   }

}  /* Print_as_2d */

