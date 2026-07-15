/* File:     cuda_ssort1.cu
 * Purpose:  Implement parallel sample sort of a list of ints.
 *           This version does a minimal implementation:
 *
 *              n = number of elements in list
 *              t = number of threads (not including host thread)
 *
 *              - Host takes sample and finds splitters:  t+1 ints
 *              - Each thread counts the number of elements in
 *                its sublist going to each bucket.
 *              - Take exclusive prefix sums:  t ints
 *              - Copy my elements from original list into 
 *                shared memory
 *              - Sort my bucket in shared memory
 *              - Copy my bucket into original list
 *
 *           This version only uses one thread block.  Total device 
 *           storage required on the device is 2n + t ints. 
 *
 *           This version uses shared memory for local storage.
 *
 * Compile:  nvcc -o s1 cuda_ssort1.cu set_device.cu stats.cu
 * Run:      ./s1 <th_per_blk> <n> <s> [m]
 *              th_per_blk = number of threads (only one thread block)
 *              n = list size
 *              s = sample size
 *              m = if m is not present, get list from stdin.  
 *                  Otherwise generate the list using the 
 *                  random() function with m as the modulus.
 *
 * Input:    If m is on the command line, none.  Otherwise, the
 *              list
 * Output:   The run time for sorting the list.
 *           If PRINT_LIST is defined, the list before and after
 *           sorting.
 *
 * Notes:
 * 1.  The number of buckets is the same as th_per_blk.
 * 2.  We assume that 1 < th_per_blk < s < n.  We also 
 *     assume that th_per_blk evenly divides both s and n.
 * 3.  Define DEBUG for output after each stage of the sort.
 * 4.  Define PRINT_LIST to print the lists.
 * 5.  Define INSERT to use insertion sort on device.  Otherwise use
 *     heapsort on device
 * 6.  Define SORT_DEBUG for debug info on the sorts
 * 7.  Use qsort for sorts on host.
 *
 * IPP2:   7.2.9 (pp. 429 and ff.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  /* For memset */
#include <math.h>    /* For ceil   */
#include "stats.h"
#include "timer.h"
#include "set_device.h"
#include "simple_printf.h"

/* Seed for generating list */
#define SEED1 1

/* Seed for picking sample */
#define SEED2 2

/* Only used when DEBUG is defined */
#define MAX_STRING 256

/* Max shared mem per block in ints */
#define MAX_SHMEM 12288  /* = 49152 bytes/4 */

#ifdef INSERT
#define SORT Ins_srt
#else
#define SORT Heap_sort
#endif

// For stats.  Global so that they can be used in various functions
// Using start1 and finish1 for Sample_sort
// Using start2 and finish2 for choosing sample and choosing splitters,
//    i.e., the part of the code that runs on the host
double start1, finish1; 
double start2, finish2;

/*---------------------------------------------------------------------
 * Function prototypes
 */
/* Only called by host */
void  Get_args(int argc, char* argv[], int* th_per_blk_p,
      int* n_p, int* s_p, int* m_p);
void  Usage(char progname[]); 
void  Read_list(const char prompt[], int list[], int n);
void  Gen_list(int list[], int n, int modulus);
int   Check_sorted(int list[], int serial_list[], int n);
int   Compare(const void *a_p, const void *b_p);

int   In_chosen(int chosen[], int sz, int j);
void  Choose_sample(int list[], int n, int sample[], int s);
void  Choose_splitters(int sample[], int s, int splitters[], 
         int th_per_blk);

/* Host driver for sample sort */
void  Sample_sort(int list[], int sample[], int splitters[],
      int counts[], int n, int s, int th_per_blk);

/* Called by host and device */
__host__ __device__ void  Print_list(const char title[], 
      int list[], int n);

/* Kernel */
__global__ void Dev_ssort(int list[], int splitters[], int counts[], 
      int n);

/* Called on device */
__device__ void Get_min_max(int splitters[], int my_rank, int* my_min_p, 
      int* my_max_p); 
__device__ void Find_counts(int list[], int n, int splitters[], 
      int counts[], int th_per_blk, int my_rank);
__device__ void Copy_to_my_bkt(int list[], int n, int splitters[],
      int my_bkt[], int my_count, int my_rank);
__device__ void Copy_fr_my_bkt(int list[], int my_bkt[], 
      int my_rank, int my_first, int my_last);
__device__ void Excl_prefix_sums(int counts[], int n, int my_rank);
__host__ __device__ void Heap_sort(int a[], int n);
__host__ __device__ void Ins_srt(int a[], int n);
__host__ __device__ void Swap(int* a_p, int* b_p);
__host__ __device__ void H_insert(int val, int a[], int curr_sz);
__host__ __device__ int  Del_max(int a[], int curr_sz);


#define MINUS_INFTY (1 << 31) /* = -2^31 */
/* In some implementation, we need to be able to do arithmetic */
/* with INFTY.  This should protect against overflow.          */
#define INFTY (1 << 30)       /* =  2^30 */

int iter, ITERS;


/*---------------------------------------------------------------------
 * Function:  main
 */
int main(int argc, char* argv[]) {
   int th_per_blk, n, s, modulus;
   int *list, *save_list, *serial_list;
   int *sample, *splitters, *counts;
// double start, finish;

   Set_device();

   Get_args(argc, argv, &th_per_blk, &n, &s, &modulus);

   cudaMallocManaged(&list, n*sizeof(int));
   save_list = (int*) malloc(n*sizeof(int));
   serial_list = (int*) malloc(n*sizeof(int));

   if (modulus == 0)
      Read_list("Enter the list", save_list, n);
   else
      Gen_list(save_list, n, modulus);
#  ifdef PRINT_LIST
   Print_list("Before sort, list = ", save_list, n);
#  endif


   sample = (int*) malloc(s*sizeof(int));
   cudaMallocManaged(&splitters, (th_per_blk+1)*sizeof(int));
   cudaMallocManaged(&counts, (th_per_blk+1)*sizeof(int));

   ITERS = Setup_stats();
// sample_times = (double*) malloc(ITERS*sizeof(double));
   for (iter = 0; iter < ITERS; iter++) {
      memcpy(list, save_list, n*sizeof(int));
      GET_TIME(start1);
      if (th_per_blk == 1)
         SORT(list, n);
      else
         Sample_sort(list, sample, splitters, counts, n, 
               s, th_per_blk);
      GET_TIME(finish1);
      /* s_dmin, s_dmax, s_dtotal defined in stats.c */
      Update_stats(start1, finish1, start2, finish2, 
            &s_dmin, &s_dmax, &s_dtotal);

#     ifdef DO_SERIAL
      memcpy(serial_list, save_list, n*sizeof(int));
//    GET_TIME(start);
//    SORT(serial_list, n);
      qsort(serial_list, n, sizeof(int), Compare);
//    GET_TIME(finish);
      /* s_hmin, s_hmax, s_htotal defined in stats.c */
//    Update_stats(start2, finish2, &s_hmin, &s_hmax, &s_htotal);
#     endif
   }

#  ifdef PRINT_LIST
   Print_list("After sort, list = ", list, n);
#  endif
   Print_stats("Sample sort", s_dmin, s_dmax, s_dtotal, ITERS);
// Print_stats("  Host code", s_hmin, s_hmax, s_htotal, ITERS);
// Sample_stats();

#  ifdef DO_SERIAL
   printf("n = %d\n", n);
   if (Check_sorted(list, serial_list, n))
      printf("List is sorted\n");
   else
      printf("List is NOT sorted\n");
//    Print_list("     serial_list = ", serial_list, n);
#  endif

   cudaFree(list);
   free(save_list);
   free(serial_list);

   free(sample);
// free(sample_times);
   cudaFree(splitters);
   cudaFree(counts);

   return 0;
}  /* main */

/*---------------------------------------------------------------------
 * Function:     Usage
 * Purpose:      Print a message explaining how to start the program
 *               and quit
 */
void Usage(char progname[]) {
   fprintf(stderr,"%s <th_per_blk> <n> <s> [m]\n", progname);
   fprintf(stderr,"      n = list size\n");
   fprintf(stderr,"      s = sample size\n");
   fprintf(stderr,"      Both n and s should be evenly divisible by\n");
   fprintf(stderr,"         th_per_blk\n");
   fprintf(stderr,"      m = if m is not present, get list from\n");
   fprintf(stderr,"         stdin.  Otherwise generate the list with\n");
   fprintf(stderr,"         m as the modulus.\n");
   exit(0);
}  /* Usage */


/*---------------------------------------------------------------------
 * Function:     Get_args
 * Purpose:      Get the command line arguments
 * In args:      argc, argv
 * Out args:     th_per_blk_p, n_p, s_p, m_p
 */
void  Get_args(int argc, char* argv[], int* th_per_blk_p,
      int* n_p, int* s_p, int* m_p) {
   if (argc != 4 && argc != 5) 
      Usage(argv[0]);
   *th_per_blk_p = strtol(argv[1], NULL, 10);
   *n_p = strtol(argv[2], NULL, 10);
   *s_p = strtol(argv[3], NULL, 10);
   if (*n_p % *th_per_blk_p != 0 ||
       *s_p % *th_per_blk_p != 0)
      Usage(argv[0]);
   if (argc == 5)
      *m_p = strtol(argv[4], NULL, 10);
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
   for (i = 0; i < n; i++) {
      list[i] = random() % modulus;
   }
}  /* Gen_list */


/*---------------------------------------------------------------------
 * Function:   Print_list
 * Purpose:    Print a list of ints to stdout
 * In args:    all
 */
__host__ __device__ void Print_list(const char title[], 
      int list[], int n) {
   int i;

   printf("%s  ", title);
   for (i = 0; i < n; i++)
      printf("%d ", list[i]);
   printf("\n");
}  /* Print_list */


/*---------------------------------------------------------------------
 * Function:        Sample_sort
 * Purpose:         Use sample sort to sort a list of n ints
 * In args:         n:  size of the list
 *                  s:  sample size
 *                  th_per_blk:  number of buckets and number of threads
 * In/out arg:      list
 * Scratch args:    sample, splitters, counts 
 *
 * Note:            splitters has th_per_blk+1 elements
 */
void  Sample_sort(int list[], int sample[], int splitters[],
      int counts[], int n, int s, int th_per_blk) {
// double start, finish;

   GET_TIME(start2);
// printf(" start2 = %.15e\n", start2);
   Choose_sample(list, n, sample, s);
#  ifdef DEBUG
   Print_list("sample =", sample, s);
#  endif

   Choose_splitters(sample, s, splitters, th_per_blk);
#  ifdef DEBUG
   Print_list("splitters =", splitters, th_per_blk+1);
#  endif
   GET_TIME(finish2);
// printf("finish2 = %.15e\n", finish2);
// sample_times[iter] = finish-start;

// GET_TIME(start2);
// printf("In Sample_sort  start2 = %.15e\n", start2);
   Dev_ssort <<<1, th_per_blk>>>(list, splitters, counts, n);
   cudaDeviceSynchronize();
// GET_TIME(finish2);
// printf("In Sample_sort finish2 = %.15e\n", finish2);
   // Stats are updated in main
}  /* Sample_sort */


/*----------------------------------------------------------------------
 * Function:   Compare
 * Purpose:    Comparison function for use in calls to qsort
 * In args:    a_p, b_p
 * Ret val:    See below
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
 * Function:   In_chosen
 * Purpose:    Check whether the subscript j has already been chosen
 *             for the sample.  If it has return 1.  Otherwise return
 *             0.
 * In args:    chosen:  subscripts chosen so far
 *             sz:  number of subscripts chosen so far
 *             j:  current candidate for a new subscript
 * Ret val:    True, if j has already been chosen; False otherwise
 */
int  In_chosen(int chosen[], int sz, int j) {
   int i;

   for (i = 0 ; i < sz; i++)
      if (chosen[i] == j)
         return 1;
   return 0;
}  /* In_chosen */


/*----------------------------------------------------------------------
 * Function:   Choose_sample
 * Purpose:    Choose the sample from the original list.
 * In args:    list, n, s
 * Out arg:    sample
 * Notes:       
 * 1.  The sample is sorted before return.
 * 2.  This version does sampling without replacement. So 
 *     when the sample size is large relative to the list 
 *     size, the number of attempts to choose a new 
 *     element of the sample can get very large.
 */
void Choose_sample(int list[], int n, int sample[], int s) {
   int i, j;
   int chosen[s];

   srandom(SEED2);
   for (i = 0; i < s; i++) {
      j = random() % n;
#     ifdef BIG_DEBUG
      printf("i = %d, j = %d\n", i, j);
#     endif
      /* Make sure j hasn't been chosen yet */
      while (In_chosen(chosen, i, j))
         j = random() % n;
      chosen[i] = j;
      sample[i] = list[j];
   }
// SORT(sample, s);
   qsort(sample, s, sizeof(int), Compare);

#  ifdef BIG_DEBUG
   Print_list("In Choose_sample, chosen = ", chosen, s);
#  endif
}  /* Choose_sample */


/*----------------------------------------------------------------------
 * Function:   Choose_splitters 
 * Purpose:    Determine the cutoffs for the buckets.  There will be
 *             b+1 splitters: splitters[0], splitters[1], ..., 
 *             splitters[b], and each interval should get s/b elements
 *             of the sample.
 * In args:    sample, s, b = th_per_blk
 * Out arg:    splitters
 * Note:       The splitters and buckets are
 *             bucket 0 = -infty = splitters[0] <= x < splitters[1] 
 *             bucket 1 =          splitters[1] <= x < splitters[2] 
 *             bucket 2 =          splitters[2] <= x < splitters[3] 
 *                 ...
 *             bucket b-1 =      splitters[b-1] <= x < splitters[b] = infty
 */
void Choose_splitters(int sample[], int s, int splitters[], int b) {
   int per_bucket = s/b; // Note that s is evenly divisible by b
   int i, j;

   splitters[0] = MINUS_INFTY;
   for (i = 1; i <= b-1; i++) {
      j = i*per_bucket;

      // Note that 1 <= j <= b-1.  So j-1 and j are legal subscripts
      //    in sample.
      splitters[i] = sample[j-1] + sample[j];

      // We should round up if sample[j-1] + sample[j] is odd
      if (splitters[i] % 2 != 0) splitters[i]++;
      splitters[i] = splitters[i]/2;
   }
   splitters[b] = INFTY;
}  /* Choose_splitters */



/*---------------------------------------------------------------------
 * Function:      Dev_ssort  
 * Purpose:       Carry out the bucket specific work in Sample
 *                sort:
 *
 *                   - Each thread counts the number of elements
 *                     it is responsible for sorting
 *                   - Store these in counts, and find the exclusive
 *                     prefix sums to get offsets into sorted list
 *                   - Each thread copies its elements into a sublist
 *                     of a temporary list, and each thread sorts its
 *                     sublist.
 *                   - Threads copy their sorted sublists back into
 *                     original list.
 *
 * Note:
 * Both splitters and counts have storage for th_per_blk+1 elements
 *
 */
__global__ void Dev_ssort(int list[], int splitters[], int counts[], 
      int n) {
   int my_rank = threadIdx.x;   // Only one thread block
   int th_per_blk = blockDim.x;
   int my_first, my_last, my_count;
   int* my_bkt;
#  ifdef DEBUG
// int th;
// char title[MAX_STRING];
#  endif
   __shared__ int shmem[MAX_SHMEM];

   /* Since we don't yet know how many elements are assigned to  */
   /* a thread, we leave the list in global memory               */
   /* Note:  only elements 0, 1, ..., th_per_blk-1 of counts are */
   /*        initialized                                         */
   Find_counts(list, n, splitters, counts, th_per_blk, my_rank);
   __syncthreads();

   my_count = counts[my_rank];
#  ifdef DEBUG
   if (my_rank == 0)
      Print_list("counts", counts, th_per_blk);
#  endif

   /* Now use counts[th_per_blk] = n */
   Excl_prefix_sums(counts, th_per_blk+1, my_rank);
   if (my_rank == 0) counts[th_per_blk] = n;
   __syncthreads();
#  ifdef DEBUG
   if (my_rank == 0)
      Print_list("Prefix sums", counts, th_per_blk+1);
#  endif

   my_first = counts[my_rank];
   my_last = counts[my_rank + 1];
   my_bkt = shmem + my_first;
#  ifdef DEBUG
   printf("Th %d > my_first = %d, my_last = %d, my_count = %d\n", 
         my_rank, my_first, my_last, my_count);
#  endif

   Copy_to_my_bkt(list, n, splitters, my_bkt, my_count, my_rank);
// __syncthreads();
#  ifdef DEBUG
   if (my_rank == 0) Print_list("After Copy_to_my_bkt", shmem, n);
#  endif

   SORT(my_bkt, my_count);
// if (my_rank % warpSize == 0) {
//    int warp_ct = counts[my_rank + warpSize] - my_first;
//    SORT(my_bkt, warp_ct);
// }
   // This barrier is needed to avoid conflicts between Copy_to and Copy_fr
   __syncthreads();

   Copy_fr_my_bkt(list, my_bkt, my_rank, my_first, my_last);

}  /* Dev_ssort */


/*----------------------------------------------------------------------
 * Function:    Get_min_max
 * Purpose:     Find the smallest and largest values that can be
 *              stored on this thread.  For each x on this thread
 *
 *                 splitters[my_rank] = my_min 
 *                          <= x < my_max = splitters[my_rank+1]
 *
 *              So splitters contains th_per_blk+1 elements with
 *              subscripts ranging from 0 to th_per_blk.
 * Note:        if (my_rank == 0) my_min = -INFTY
 *              if (my_rank == th_per_blk-1) my_max = INFTY
 * In args:     splitters, my_rank
 * Out args:    my_min_p, my_max_p
 *
 * 
 */
__device__ void Get_min_max(int splitters[], int my_rank, int* my_min_p, 
      int* my_max_p) {

   *my_min_p = splitters[my_rank];
   *my_max_p = splitters[my_rank+1];
}  /* Get_min_max */


/*----------------------------------------------------------------------
 * Function:    Find_counts
 * Purpose:     Find the number of elements in the list
 *              that should go to the calling thread.
 * In args:     list, splitters, n, th_per_blk, my_rank
 * Out arg:     counts
 */
__device__ void  Find_counts(int list[], int n, int splitters[], 
      int counts[], int th_per_blk, int my_rank) {

   int i, my_min, my_max, my_count = 0;

   Get_min_max(splitters, my_rank, &my_min, &my_max);

   for (i = 0; i < n; i++) {
      if (my_min <= list[i] && list[i] < my_max)
         my_count++;
   }
   counts[my_rank] = my_count;
}  /* Find_counts */


/*----------------------------------------------------------------------
 * Function:       Excl_prefix_sums
 * Purpose:        Overwrite counts with the exclusive prefix sums
 * In arg:         th_per_blk
 * In/out arg:     counts
 *
 */
__device__ void Excl_prefix_sums(int counts[], int n, int my_rank) {
   unsigned shift;
   int tmp;

   if (my_rank == 0) 
      tmp = my_rank;
   else
      tmp = counts[my_rank-1];
   __syncthreads();
   counts[my_rank] = tmp;
   __syncthreads();

   for (shift = 1; shift < n; shift <<= 1) {
      if (my_rank >= shift)
         counts[my_rank] += counts[my_rank-shift];
      __syncthreads();     
   }
}  /* Excl_prefix_sums */



/*----------------------------------------------------------------------
 * Function:       Copy_to_my_bkt
 * Purpose:        Copy calling thread's sublist into my_bkt
 * In args:        list, n, splitters, my_rank
 * Out arg:        my_bkt
 */
__device__ void Copy_to_my_bkt(int list[], int n, int splitters[], 
      int my_bkt[], int my_count, int my_rank) {
   int li, lli, my_min, my_max;

   Get_min_max(splitters, my_rank, &my_min, &my_max);

   for (li = lli = 0; li < n && lli < my_count; li++)
      if (my_min <= list[li] && list[li] < my_max) {
          my_bkt[lli++] = list[li];
       } 
}  /* Copy_to_my_bkt */


/*-----------------------------------------------------------------
 * Function:     Heap_sort
 * Purpose:      In place sort of array of ints using heap sort
 * In args:      n
 * In/out args:  a
 */
__device__ void Heap_sort(int a[], int n) {
   int i, val, curr_sz = 0;
   
   curr_sz = 0;
   for (i = 0; i < n; i++) {
      val = a[i];
      H_insert(val, a, curr_sz);
      curr_sz++;
#     ifdef SORT_DEBUG
      Print_list("Heap", a, curr_sz);
      printf("\n");
#     endif
   }

#  ifdef SORT_DEBUG
   Print_list("Final Heap", a, n);
   printf("\n");
#  endif

   for (i = 0; i < n; i++) {
      val = Del_max(a, curr_sz);
      curr_sz--;
      a[n-i-1] = val;
#     ifdef SORT_DEBUG
      printf("i = %d, curr_sz = %d, val = %d\n",
            i, curr_sz, val);
      Print_list("Heap", a, curr_sz);
      Print_list("List", a + curr_sz, n-curr_sz);
      printf("\n");
#     endif
   }
}  /* Heap_sort */


/*-----------------------------------------------------------------
 * Function:     Swap
 */
__device__ void Swap(int* a_p, int* b_p) {
   int tmp = *a_p;
   *a_p = *b_p;
   *b_p = tmp;
}  /* Swap */


/*-----------------------------------------------------------------
 * Function:     H_insert
 * Purpose:      Insert an element into the heap
 * Note:         This is a max heap
 */
__device__ void H_insert(int val, int a[], int curr_sz) {
   int pos = curr_sz;
   int parent;

   a[pos] = val;
   parent = (pos == 0 ? 0 : (pos - 1)/2);
   while (pos > 0 && a[pos] > a[parent]) {
      Swap(&a[pos], &a[parent]);
      pos = parent;
      parent = (pos - 1)/2;
   }
}  /* H_insert */


/*-----------------------------------------------------------------
 * Function:     Del_max
 * Purpose:      Delete max value from heap
 */
__device__ int  Del_max(int a[], int curr_sz) {
   int lc, rc, pos, done = 0;
   int ret_val = a[0];
   a[0] = a[curr_sz-1];

   pos = 0;
   while ( !done ) {
      lc = 2*pos+1;
      rc = lc+1;
      if (rc < curr_sz) {
         // Both lc and rc < curr_sz
         if (a[lc] >= a[rc]) {
            if (a[pos] < a[lc]) {
               Swap(&a[pos], &a[lc]);
               pos = lc;
            } else if (a[pos] < a[rc]) {
               Swap(&a[pos], &a[rc]);
               pos = rc;
            } else {
               done = 1;
            }
         } else { // a[rc] > a[lc]
            if (a[pos] < a[rc]) {
               Swap(&a[pos], &a[rc]);
               pos = rc;
            } else if (a[pos] < a[lc]) {
               Swap(&a[pos], &a[lc]);
               pos = lc;
            } else {
               done = 1;
            }
         }
      } else if (lc < curr_sz) {
         // Only lc < curr_sz
         if (a[pos] < a[lc]) {
            Swap(&a[pos], &a[lc]);
            pos = lc;
         } else {
            done = 1;
         }
      } else {
         // At end of tree
         done = 1;
      }
   }

   return ret_val;
}  /* Del_max */


/*----------------------------------------------------------------------
 * Function:       Copy_fr_my_bkt
 * Purpose:        Copy calling thread's sublist into original list
 * In arg:         my_bkt, my_rank, my_first, my_last
 * Out arg:        list
 */
__device__ void Copy_fr_my_bkt(int list[], int my_bkt[], 
      int my_rank, int my_first, int my_last) {
   int li, lli;

   for (li = my_first, lli = 0; li < my_last; li++, lli++)
      list[li] = my_bkt[lli];
}  /* Copy_fr_my_bkt */


/*---------------------------------------------------------------------
 * Function:   Excl_prefix_sums
 * Purpose:    Compute exclusive prefix sums of values in array counts
 * In args:    th_per_blk
 * In/out arg: counts
 */
//__device__ void Excl_prefix_sums(int counts[], int th_per_blk, int my_rank) {
//   unsigned shift;
//
//   /* First compute inclusive prefix sums */
//   for (shift = 1; shift < n; shift <<= 1) {
//      if (my_rank >= shift)
//         vals[my_rank] += vals[my_rank-shift];
//      __syncthreads();     
//   }
//
//   /* Now shift right to get exclusive scan */
//   if (my_rank < th_per_blk-1)
//      vals[my_rank+1] = vals[my_rank];
//   __syncthreads();
//   if (my_rank == 0) vals[0] = 0;
//   __syncthreads();
//}  /* Excl_prefix_sums */


/*---------------------------------------------------------------------
 * Function:   Check_sorted
 * Purpose:    Determine whether list is sorted in increasing order
 *             by comparing it to serial_list, which has been sorted in
 *             increasing order
 * In args:    all
 * Ret val:    1 if the list is sorted, 0 otherwise
 */
int Check_sorted(int list[], int serial_list[], int n) {
   int i;

   for (i = 0; i < n-1; i++)
      if (list[i] != serial_list[i]) {
         printf("list isn't sorted: list[%d] = %d != %d = serial_list[%d]\n",
               i, list[i], serial_list[i], i);
         return 0;
      }
   return 1;
}  /* Check_sorted */


/*-----------------------------------------------------------------
 * Function:     Ins_srt
 * Purpose:      Sort array of ints using insertion sort
 * In args:      n
 * In/out args:  A
 */
__host__ __device__ void Ins_srt(int A[], int n) {
   int i, pos, val;

   for (i = 1; i < n; i++) {
      val = A[i];
      pos = i-1;
      while (pos >= 0 && A[pos] > val) {
         A[pos+1] = A[pos];
         pos--;
      }
      A[pos+1] = val;
   }

}  /* Ins_srt */


/*
 * Copyright (c) 2016, Matt Redfearn
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>


__device__ int puutchar(int c) {
   printf("%c", c);
   return c;
}  /* puutchar */

__device__ void simple_outpuutchar(char **str, char c)
{
   if (str) {
      **str = c;
      ++(*str);
   } else {
      puutchar(c);
   }
}

enum flags {
   PAD_ZERO   = 1,
   PAD_RIGHT   = 2,
};

__device__ int prints(char **out, const char *string, int width, int flags)
{
   int pc = 0, padchar = ' ';

   if (width > 0) {
      int len = 0;
      const char *ptr;
      for (ptr = string; *ptr; ++ptr) ++len;
      if (len >= width) width = 0;
      else width -= len;
      if (flags & PAD_ZERO)
         padchar = '0';
   }
   if (!(flags & PAD_RIGHT)) {
      for ( ; width > 0; --width) {
         simple_outpuutchar(out, padchar);
         ++pc;
      }
   }
   for ( ; *string ; ++string) {
      simple_outpuutchar(out, *string);
      ++pc;
   }
   for ( ; width > 0; --width) {
      simple_outpuutchar(out, padchar);
      ++pc;
   }

   return pc;
}

#define PRINT_BUF_LEN 64

__device__ int simple_outputi(char **out, long long i, int base, int sign, int width, int flags, int letbase)
{
   char print_buf[PRINT_BUF_LEN];
   char *s;
   int t, neg = 0, pc = 0;
   unsigned long long u = i;

   if (i == 0) {
      print_buf[0] = '0';
      print_buf[1] = '\0';
      return prints(out, print_buf, width, flags);
   }

   if (sign && base == 10 && i < 0) {
      neg = 1;
      u = -i;
   }

   s = print_buf + PRINT_BUF_LEN-1;
   *s = '\0';

   while (u) {
      t = u % base;
      if( t >= 10 )
         t += letbase - '0' - 10;
      *--s = t + '0';
      u /= base;
   }

   if (neg) {
      if( width && (flags & PAD_ZERO) ) {
         simple_outpuutchar (out, '-');
         ++pc;
         --width;
      }
      else {
         *--s = '-';
      }
   }

   return pc + prints (out, s, width, flags);
}


__device__ int simple_vsprintf(char **out, const char *in_format, va_list ap)
{
   int width, flags;
   int pc = 0, i = 0;
   char scr[2];
   char f[256], *format;  

   while (i < 256 && in_format[i] != '\0') {
      f[i] = in_format[i];
      i++;
   }
   if (i == 256) 
      f[255] = '\0';
   else
      f[i] = '\0';
   format = &f[0];

   union {
      char c;
      char *s;
      int i;
      unsigned int u;
      long li;
      unsigned long lu;
      long long lli;
      unsigned long long llu;
      short hi;
      unsigned short hu;
      signed char hhi;
      unsigned char hhu;
      void *p;
   } u;

   for (; *format != 0; ++format) {
      if (*format == '%') {
         ++format;
         width = flags = 0;
         if (*format == '\0')
            break;
         if (*format == '%')
            goto out;
         if (*format == '-') {
            ++format;
            flags = PAD_RIGHT;
         }
         while (*format == '0') {
            ++format;
            flags |= PAD_ZERO;
         }
         if (*format == '*') {
            width = va_arg(ap, int);
            format++;
         } else {
            for ( ; *format >= '0' && *format <= '9'; ++format) {
               width *= 10;
               width += *format - '0';
            }
         }
         switch (*format) {
            case('d'):
               u.i = va_arg(ap, int);
               pc += simple_outputi(out, u.i, 10, 1, width, flags, 'a');
               break;

            case('u'):
               u.u = va_arg(ap, unsigned int);
               pc += simple_outputi(out, u.lli, 10, 0, width, flags, 'a');
               break;

            case('x'):
               u.u = va_arg(ap, unsigned int);
               pc += simple_outputi(out, u.lli, 16, 0, width, flags, 'a');
               break;

            case('X'):
               u.u = va_arg(ap, unsigned int);
               pc += simple_outputi(out, u.lli, 16, 0, width, flags, 'A');
               break;

            case('c'):
               u.c = va_arg(ap, int);
               scr[0] = u.c;
               scr[1] = '\0';
               pc += prints(out, scr, width, flags);
               break;

            case('s'):
               u.s = va_arg(ap, char *);
               pc += prints(out, u.s ? u.s : "(null)", width, flags);
               break;
            case('l'):
               ++format;
               switch (*format) {
                  case('d'):
                     u.li = va_arg(ap, long);
                     pc += simple_outputi(out, u.li, 10, 1, width, flags, 'a');
                     break;

                  case('u'):
                     u.lu = va_arg(ap, unsigned long);
                     pc += simple_outputi(out, u.lli, 10, 0, width, flags, 'a');
                     break;

                  case('x'):
                     u.lu = va_arg(ap, unsigned long);
                     pc += simple_outputi(out, u.lli, 16, 0, width, flags, 'a');
                     break;

                  case('X'):
                     u.lu = va_arg(ap, unsigned long);
                     pc += simple_outputi(out, u.lli, 16, 0, width, flags, 'A');
                     break;

                  case('l'):
                     ++format;
                     switch (*format) {
                        case('d'):
                           u.lli = va_arg(ap, long long);
                           pc += simple_outputi(out, u.lli, 10, 1, width, flags, 'a');
                           break;

                        case('u'):
                           u.llu = va_arg(ap, unsigned long long);
                           pc += simple_outputi(out, u.lli, 10, 0, width, flags, 'a');
                           break;

                        case('x'):
                           u.llu = va_arg(ap, unsigned long long);
                           pc += simple_outputi(out, u.lli, 16, 0, width, flags, 'a');
                           break;

                        case('X'):
                           u.llu = va_arg(ap, unsigned long long);
                           pc += simple_outputi(out, u.lli, 16, 0, width, flags, 'A');
                           break;

                        default:
                           break;
                     }
                     break;
                  default:
                     break;
               }
               break;
            case('h'):
               ++format;
               switch (*format) {
                  case('d'):
                     u.hi = va_arg(ap, int);
                     pc += simple_outputi(out, u.hi, 10, 1, width, flags, 'a');
                     break;

                  case('u'):
                     u.hu = va_arg(ap, unsigned int);
                     pc += simple_outputi(out, u.lli, 10, 0, width, flags, 'a');
                     break;

                  case('x'):
                     u.hu = va_arg(ap, unsigned int);
                     pc += simple_outputi(out, u.lli, 16, 0, width, flags, 'a');
                     break;

                  case('X'):
                     u.hu = va_arg(ap, unsigned int);
                     pc += simple_outputi(out, u.lli, 16, 0, width, flags, 'A');
                     break;

                  case('h'):
                     ++format;
                     switch (*format) {
                        case('d'):
                           u.hhi = va_arg(ap, int);
                           pc += simple_outputi(out, u.hhi, 10, 1, width, flags, 'a');
                           break;

                        case('u'):
                           u.hhu = va_arg(ap, unsigned int);
                           pc += simple_outputi(out, u.lli, 10, 0, width, flags, 'a');
                           break;

                        case('x'):
                           u.hhu = va_arg(ap, unsigned int);
                           pc += simple_outputi(out, u.lli, 16, 0, width, flags, 'a');
                           break;

                        case('X'):
                           u.hhu = va_arg(ap, unsigned int);
                           pc += simple_outputi(out, u.lli, 16, 0, width, flags, 'A');
                           break;

                        default:
                           break;
                     }
                     break;
                  default:
                     break;
               }
               break;
            default:
               break;
         }
      }
      else {
out:
         simple_outpuutchar (out, *format);
         ++pc;
      }
   }
   if (out) **out = '\0';
   return pc;
}

__device__ int simple_printf(char *fmt, ...)
{
   va_list ap;
   int r;

   va_start(ap, fmt);
   r = simple_vsprintf(NULL, fmt, ap);
   va_end(ap);

   return r;
}

__device__ int simple_sprintf(char *buf, const char *fmt, ...)
{
   va_list ap;
   int r;

   va_start(ap, fmt);
   r = simple_vsprintf(&buf, fmt, ap);
   va_end(ap);

   return r;
}


