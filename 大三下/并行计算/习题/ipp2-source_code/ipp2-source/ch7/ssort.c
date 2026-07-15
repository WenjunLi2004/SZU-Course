/* File:     ssort.c
 * Purpose:  Implement serial sample sort that can be converted into 
 *           cuda code.
 *
 * Compile:  gcc -g -Wall -o ssort ssort.c
 *              Define PRINT_LIST to get output of original and sorted lists
 * Run:      ./ssort <n> <s> <b> [m]
 *                 n:  number of elements in list
 *                 s:  sample size
 *                 b:  number of buckets
 *               mod:  if present, host program should generate
 *                         a list using the C library random()
 *                         function taking remainders modulo mod
 *
 *                            val = random() % mod;
 *                     if not present, host program should read
 *                     a list from stdin
 *              Note:  n should be divisible by s and s
 *                     should be divisible by b
 *
 * Input:    If mod is present on the command line, none
 *           If mod is absent from the command line, an n-element list of ints
 * Output:   Run-time of serial sample sort and library sort. 
 *           Whether the result of serial sample sort agrees with the 
 *              result of the library sort.
 *           Output of the original and sorted lists is enabled by 
 *              compiling with -DPRINT_LIST
 *           Extremely verbose output is enabled with -DDEBUG.  This
 *              output can be pared down by labelling desired DEBUG
 *              output with DEBUG1 and compiling with -DDEBUG1
 *
 * Notes:
 * 1.  INFTY = 2^30.  The values in the input list should not exceed this
 *     value.  This allows us to add to the largest ``splitter''.
 *
 * 2.  The buckets are
 *        bucket
 *           0:   MINUS_INFTY = splitters[0] <= x < splitters[1]
 *           1:   splitters[1] <= x < splitters[2]
 *           ...
 *           i:   splitters[i] <= x < splitters[i+1]
 *           ...
 *           b-1: splitters[b-1] <= x < splitters[b] = INFTY
 *
 * 3.  The sample is the union of deterministic samples taken from each 
 *     sublist:
 *
 *           for each sublist {
 *              sort this sublist;
 *              choose n/s equally spaced elements from 
 *                 the sorted sublist;
 *           }
 *     
 * 4.  In this code we use the C library qsort function for sorting of
 *     the sublists and the sample
 *
 * 5.  This code uses a deterministic method for taking the sample:
 *
 *               slist_sz = n/b;
 *               samps_per_slist = s/b;
 *               sample = empty;
 *               for each sublist of n/b elements
 *                  Sort the sublist
 *                  Choose sample elements from this sublist
 *                     by choosing s/b equally spaced elements 
 *                     of the sublist and appending them to
 *                     the sample
 *
 * IPP2:   7.2.2 (pp. 400 and ff.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for memset, memcpy */
#include "timer.h"

/*  Smallest possible value in list */
#define MINUS_INFTY (1 << 31) // = -2^31

/*  Largest possible positive int in the list.  (It's not */
/*  2^31 - 1, since we want to be able to add to it.)     */
#define INFTY (1 << 30)  // = 2^30

void Usage(char pname[]);
void Get_args(int argc, char* argv[], int* n_p, int* s_p, 
         int* b_p, int* mod_p);
void Init_list(int list[], int n, int mod);
void Gen_list(int list[], int n, int mod);
void Sample_sort(int list[], int n, int s, int b);
void Gen_sample(int list[], int n, int s, int b, int sample[]);
void Find_splitters(int sample[], int s, int splitters[], int b);
void Get_dests(int list[], int which_sublist, int elts_per_sublist, 
      int splitters[], int b, int from_to_cts[]);
int  Which_dest(int key, int splitters[], int last_dest, int b);
void Push_from(int sublist, int list[], int n, int b, 
      int from_to_cts[], int row_pref[], int col_pref[], 
      int bkt_starts[], int tlist[]);
void Print_list(char title[], int list[], int n);
void Print_mat(char title[], int mat[], int m, int n);
void Read_list(char prompt[], int list[], int n);
void Sort(int tlist[], int m);
int  Compare(const void* ap, const void* bp);
void In_place_excl_scan(int arr[], int m, int stride);
void Excl_scan(int scan[], int arr[], int m, int stride);
void Check_sort(int list[], int list_ok[], int m);


int main(int argc, char* argv[]) {
   int n, s, b, mod = 0;
   int *list, *list_cpy;
   double start, finish;

   Get_args(argc, argv, &n, &s, &b, &mod);
   
   list = malloc(n*sizeof(int));
   list_cpy = malloc(n*sizeof(int));
   Init_list(list, n, mod);
   memcpy(list_cpy, list, n*sizeof(int));
#  ifdef PRINT_LIST
   Print_list("Original list", list, n);
#  endif
   GET_TIME(start);
   Sample_sort(list, n, s, b);
   GET_TIME(finish);
#  ifdef PRINT_LIST
   Print_list("Sorted list", list, n);
#  endif

   printf("Elapsed time for sample sort = %e seconds\n", finish-start);

   GET_TIME(start);
   Sort(list_cpy, n);
   GET_TIME(finish);
   printf("      Elapsed time for qsort = %e seconds\n", finish-start);

   Check_sort(list, list_cpy, n);

   free(list);
   free(list_cpy);

   return 0;
}  /* main */


/*---------------------------------------------------------------------
 * Function:  Usage
 * Purpose:   Print info on command line for starting program
 */
void Usage(char pname[]) {
   fprintf(stderr, "usage:  %s <n> <s> <b> [mod]\n", pname);
   fprintf(stderr, "    n:  number of elements in list\n");
   fprintf(stderr, "    s:  sample size\n");
   fprintf(stderr, "    b:  number of buckets\n");
   fprintf(stderr, "  mod:  if present, program should generate the list\n");
   fprintf(stderr, "           using random() % mod\n");
   fprintf(stderr, "        if absent, user should enter list on stdin\n");
   fprintf(stderr, " Note:  n should be divisible by s and s\n");
   fprintf(stderr, "           should be divisible by b\n");
}  /* Usage */


/*---------------------------------------------------------------------
 * Function:  Get_args
 * Purpose:   Get the command line args, exit if incorrect
 */
void Get_args(int argc, char* argv[], int* n_p, int* s_p, 
         int* b_p, int* mod_p) {

   if (argc != 4 && argc != 5) {
      Usage(argv[0]);
      exit(0);
   }

   *n_p = strtol(argv[1], NULL, 10);
   *s_p = strtol(argv[2], NULL, 10);
   *b_p = strtol(argv[3], NULL, 10);
   if (argc == 5)
      *mod_p = strtol(argv[4], NULL, 10);
   else
      *mod_p = 0;

   if (*n_p % *s_p != 0) {
      Usage(argv[0]);
      exit(0);
   } else if (*s_p % *b_p != 0) {
      Usage(argv[0]);
      exit(0);
   }
}  /* Get_args */


/*---------------------------------------------------------------------
 * Function:  Init_list
 * Purpose:   Initialize the list
 */
void Init_list(int list[], int n, int mod) {
   if (mod == 0)
      Read_list("Enter the list", list, n);
   else
      Gen_list(list, n, mod);

}  /* Init_list */


/*---------------------------------------------------------------------
 * Function:     Sample_sort
 * Purpose:      Sort the elements of list into increasing order using
 *               sample sort
 * In args:      n:  number of elements in list
 *               s:  the number of elements in the sample
 *               b:  the number of buckets
 * In/out args:  list
 *
 * Notes:
 * 1.  n is evenly divisible by s
 * 2.  s is evenly divisible by b
 * 3.  row_pref corresponds to bkt_starts_in_slists in IPP2
 * 4.  col_pref corresponds to slist_starts_in_bkts in IPP2
 */
void Sample_sort(int list[], int n, int s, int b) {
   int i, k, which_sublist, elts_per_sublist = n/b;
   int *sample, *splitters, *tlist;
   int first_elt, last_elt;
   int *from_to_cts, *row_pref, *col_pref, *bkt_starts;

   sample = malloc(s*sizeof(int));
   Gen_sample(list, n, s, b, sample);
    
#  ifdef DEBUG
   Print_list("Sample after Gen_sample", sample, s);
#  endif

   Sort(sample, s);

#  ifdef DEBUG
   Print_list("Sorted sample", sample, s);
#  endif

   splitters = malloc((b+1)*sizeof(int));
   splitters[0] = MINUS_INFTY;
   splitters[b] = INFTY;
   Find_splitters(sample, s, splitters, b);

#  ifdef DEBUG
   Print_list("Splitters", splitters, b+1);
#  endif

   /* from_to_cts[i,j] = from_to_counts[i*b + j] 
    * number of elements going *from* sublist i
    * *to* bucket j. */
   from_to_cts = malloc(b*b*sizeof(int));
   memset(from_to_cts, 0, b*b*sizeof(int));
   row_pref = malloc(b*b*sizeof(int)); 
   col_pref = malloc((b+1)*b*sizeof(int));
   bkt_starts = malloc(b*sizeof(int));
   for (which_sublist = 0; which_sublist < b; which_sublist++) 
      /* Determine the destination bucket of each element of 
       *    which_sublist 
       * Also determine the total number of elements in 
       *    which_sublist that should go to each bucket.
       * tlist[k] returns the destination bucket 
       * from_to_cts[i, d] = from_to_counts[i*b + d] is the
       *    number of elements going from sublist i to 
       *    bucket d
       */
      Get_dests(list, which_sublist, elts_per_sublist, splitters, 
            b, from_to_cts);

#  ifdef DEBUG
   Print_mat("from_to_cts", from_to_cts, b, b);
#  endif

   /* Now form the row and column prefix sums of from_to_cts:  
    *    If 0 <= i, j < b, row_pref[i,j] = loc in slist i of 1st
    *                              elt that goes to bkt j
    *    If i < b, col_pref[i,j] = loc in bkt j of 1st elt in slist i 
    *                              that goes to bkt j.  
    *    If i = b, col_pref[i,j] = number of elts in bkt j */
   for (k = 0; k < b; k++) {
      Excl_scan(row_pref + k*b, from_to_cts + k*b, b, 1);
      Excl_scan(col_pref + k, from_to_cts + k, b+1, b);
   }

   /* Find location of first element of each bucket in tlist */
   Excl_scan(bkt_starts, col_pref + b*b, b, 1);

#  ifdef DEBUG
   Print_mat("After prefix sums, row_pref", row_pref, b, b);
   Print_mat("After prefix sums, col_pref", col_pref, b+1, b);
   Print_list("After prefix sums, bkt_starts", bkt_starts, b);
#  endif

   tlist = malloc(n*sizeof(int));
   /* The arrays have the following functions:
    *
    * - from_to_cts:  
    *      ftc[i,j] = number of elements of sublist i
    *         going to bucket j.  
    * - row_pref:
    *      rp[i,j] = index of first element in sublist i going to bkt j
    * - col_pref:
    *      i < b: cp[i,j] = index of first element in bkt j coming from
    *         sublist i      
    *      i = b: cp[b,j] = number of elements in bucket j
    * - bkt_starts:
    *      bkt_start[j] = index of first element in bucket j
    */


   /* Now iterate through the sublists, push each element of sublist
    * to its correct location in its destination bucket
    */
   for (which_sublist = 0; which_sublist < b; which_sublist++)
      Push_from(which_sublist, list, n, b, from_to_cts, row_pref, 
            col_pref, bkt_starts, tlist);

#  ifdef DEBUG
   Print_list("After Push_from", tlist, n);
#  endif

   for (i = 0; i < b; i++) {
      first_elt = bkt_starts[i];
      if (i < b-1)
         last_elt = bkt_starts[i+1];
      else
         last_elt = n;
      Sort(tlist + first_elt, last_elt - first_elt);
   }

   memcpy(list, tlist, n*sizeof(int));
   
   free(sample);
   free(splitters);
   free(from_to_cts);
   free(row_pref);
   free(col_pref);
   free(bkt_starts);
   free(tlist);
}  /* Sample_sort */


/*---------------------------------------------------------------------
 * Function:  Gen_sample
 * Purpose:   Generate the sample.  This particular version uses
 *            a deterministic method to generate the sample:
 *
 *               slist_sz = n/b;
 *               samps_per_slist = s/b;
 *               sample = empty;
 *               for each sublist of n/b elements
 *                  Sort the sublist
 *                  Choose sample elements from this sublist
 *                     by choosing s/b equally spaced elements 
 *                     of the sublist and appending them to
 *                     the sample
 */
void Gen_sample(int list[], int n, int s, int b, int sample[]) {
   int slist_sz = n/b;
   int samps_per_slist = s/b;
   /* step = slist_sz/samps_per_slist = n/s */
   int step = n/s;
   int which_sublist;
   int which_samp, sub;
   int *tlist, *tsamp ;

#  ifdef DEBUG
   printf("Gen_sample:  n = %d, s = %d, b = %d\n", n, s, b);
   printf("Gen_sample:  slist_sz = %d, samps_per_slist = %d, step = %d\n", 
         slist_sz, samps_per_slist, step);
#  endif

   for (which_sublist = 0; which_sublist < b; which_sublist++) {
      tlist = list + which_sublist*slist_sz;

#     ifdef DEBUG
      printf("sublist = %d\n", which_sublist);
      Print_list("Before sublist sort:", tlist, slist_sz);
#     endif

      Sort(tlist, slist_sz);

#     ifdef DEBUG
      Print_list("After sublist sort:", tlist, slist_sz);
#     endif

      tsamp = sample + which_sublist*samps_per_slist;
      for (which_samp = 0; which_samp < samps_per_slist; which_samp++) {
         sub = (which_samp+1)*step - 1;
         tsamp[which_samp] = tlist[sub];
      }
#     ifdef DEBUG
      Print_list("Sample", tsamp, samps_per_slist);
      printf("\n");
#     endif
   }
}  /* Gen_sample */


/*---------------------------------------------------------------------
 * Function:  Get_dests
 * Purpose:   Find the number of elements going from the sublist 
 *            which_sublist to each possible destination bucket.
 * In args:   list:  the list of all keys 
 *            which_sublist:  which sublist we're working on 
 *               (one of 0, 1, ..., b-1)
 *            elts_per_sublist:  n/b 
 *            splitters:  array of values demarcating the bucket boundaries 
 *            b:  the number of sublists in the original list, and the
 *               number of buckets in the final list
 * Out args:  from_to_counts:  from_to_cts[which_sublist, dest_buck] =
 *               from_to_cts[which_sublist*b + dest_buck] is the 
 *               number of elements in which_sublist that go to dest_buck
 * Note:      The elements of from_to_cts have been initialized
 *               by the caller to 0.
 */

void Get_dests(int list[], int which_sublist, int elts_per_sublist, 
      int splitters[], int b, int from_to_cts[]) {
   int j, dest_bkt = 0;

#  ifdef DEBUG
   printf("In Get_dests, which_sublist = %d\n", which_sublist);
   Print_list("In Get_dests, from_to_cts", from_to_cts + which_sublist*b, b);
#  endif

   for (j = which_sublist*elts_per_sublist; 
         j < (which_sublist+1)*elts_per_sublist; j++) {
      dest_bkt = Which_dest(list[j], splitters, dest_bkt, b);
      from_to_cts[which_sublist*b + dest_bkt]++;
   }
#  ifdef DEBUG
   Print_list("In Get_dests, from_to_cts", from_to_cts + which_sublist*b, b);
#  endif

}  /* Get_dests */


/*---------------------------------------------------------------------
 * Function:  Which_dest
 * Purpose:   Determine which bucket key should belong to in the
 *            sorted list.
 * In args:   key:  the key whose destination bucket is being found
 *            splitters:  the list of bucket boundaries (see below)
 *            last_dest:  in the previous call to Which_dest, the
 *               destination bucket was last_dest.  Since the keys
 *               in a sublist are searched in increasing order, we
 *               know that the current key must go to bucket last_dest
 *               or higher
 *            b:  the total number of buckets
 * Note:      Since the sublists are sorted, the input key can't 
 *            belong to a bucket with lower index than the bucket
 *            for the key that was last checked.
 * Note:      splitters stores the cutoffs between pairs of consecutive 
 *            buckets.  Since there are b buckets, there are b-1
 *            splitters.  
 *
 *               bucket
 *                  0  :  splitter[0] <= x < splitter[1]  
 *                  1  :  splitter[1] <= x < splitter[2]  
 *                  ...   ...
 *                  i  :  splitter[i] <= x < splitter[i+1]  
 *                  ...   ...
 *                 b-1 :  splitter[b-1] <= x < splitter[b]
 *
 *            splitter[0] = MINUS_INFTY
 *            splitter[1] = INFTY
 */
int Which_dest(int key, int splitters[], int last_dest, int b) {
   int i;

   for (i = last_dest; i < b; i++)
      if (key < splitters[i+1]) return i;
   return -1;  /* Never executed */
}  /* Which_dest */


/*---------------------------------------------------------------------
 * Function:  Print_list
 * Purpose:   Print the list to stdout
 */
void Print_list(char title[], int list[], int n) {
   int i;

   printf("%s:  ", title);
   for (i = 0; i < n; i++)
      printf("%d ", list[i]);
   printf("\n");
}  /* Print_list */


/*---------------------------------------------------------------------
 * Function:  Print_mat
 * Purpose:   Print a 2d array stdout
 */
void Print_mat(char title[], int mat[], int m, int n) {
   int i, j;

   printf("%s:\n", title);
   for (i = 0; i < m; i++) {
      for (j = 0; j < n; j++)
         printf("%2d ", mat[i*n + j]);
      printf("\n");
   }
}  /* Print_mat */

/*---------------------------------------------------------------------
 * Function:  Find_splitters
 * Purpose:   Find the bucket boundaries
 */
void Find_splitters(int sample[], int s, int splitters[], int b) {
   int i, samp_sub, sum, samps_per_slist = s/b;

   for (i = 1; i < b; i++) {
      samp_sub = i*samps_per_slist;
      sum = sample[samp_sub-1] + sample[samp_sub];
      if (sum % 2 != 0) sum++;
      splitters[i] = sum/2;
   }
}  /* Find_splitters */


/*---------------------------------------------------------------------
 * Function:  Read_list
 * Purpose:   Read the list from stdin
 */
void Read_list(char prompt[], int list[], int n) {
   int i;

   printf("%s\n", prompt);
   for (i = 0; i < n; i++)
      scanf("%d", &list[i]);
}  /* Read_list */


/*---------------------------------------------------------------------
 * Function:  Gen_list
 * Purpose:   Use the random function to generate a list
 */
void Gen_list(int list[], int n, int mod) {
   int i;

   srandom(1);
   for (i = 0; i < n; i++)
      list[i] = random() % mod;
}  /* Gen_list */


/*---------------------------------------------------------------------
 * Function:  Sort
 * Purpose:   Function for sorting the sublists and the sample
 */
void Sort(int tlist[], int m) {
   qsort(tlist, m, sizeof(int), Compare);
}  /* Sort */

/*---------------------------------------------------------------------
 * Function:  Compare
 * Purpose:   Comparison function used by the C library qsort function
 */
int  Compare(const void* ap, const void* bp) {
   int a = *((int*) ap);
   int b = *((int*) bp);

   if (a < b)
      return -1;
   else if (a > b)
      return 1;
   else /* a == b */
      return 0;
}  /* Compare */


/*---------------------------------------------------------------------
 * Function:     In_place_excl_scan
 * Purpose:      Form in place exclusive scan or exclusive prefix sums of the
 *               elements in arr:
 *
 *                  scan[0] = 0
 *                  scan[stride] = arr[0]
 *                  scan[2*stride] = arr[0] + arr[stride]
 *                      ...
 *                  scan[(m-1)*stride] = arr[0] + ... + arr[(m-2) * stride]
 *
 * In args:     m: number of elements in arr involved in the scan
 *              stride:  stride between successive elements involved in
 *                     scan
 * In/out arg:  arr:  see Purpose above
 */
void In_place_excl_scan(int arr[], int m, int stride) {
   int i, tnew, told;

#  ifdef DEBUG_ES
   printf("m = %d, stride = %d\n", m, stride);
#  endif

   told = arr[0];
   arr[0] = 0;
#  ifdef DEBUG_ES
   printf("i = 0, told = %d, tnew = undef, arr[i] = 0\n", told);
#  endif
   for (i = stride; i < m; i += stride) {
      tnew = arr[i];
#     ifdef DEBUG_ES
      printf("i = %d, tnew = %d, told = %d, arr[i] = %d\n",
           i, tnew, told, arr[i]);
#     endif
      arr[i] = told + arr[i-stride];
      told = tnew;
#     ifdef DEBUG_ES
      printf("i = %d, tnew = %d, told = %d, arr[i] = %d\n",
           i, tnew, told, arr[i]);
#     endif
   }
#  ifdef DEBUG_ES
   printf("\n");
#  endif
}  /* In_place_excl_scan */


/*---------------------------------------------------------------------
 * Function:     Excl_scan
 * Purpose:      Form exclusive scan or exclusive prefix sums of the
 *               elements in arr:
 *
 *                  scan[0] = 0
 *                  scan[1*stride] = scan[0] + arr[0] 
 *                                 = arr[0*stride]
 *                  scan[2*stride] = scan[1*stride] + arr[1*stride] 
 *                                 = arr[0*stride] + arr[1*stride]
 *                                ...
 *              scan[(m-1)*stride] = scan[(m-2)*stride] + arr[(m-2)*stride]
 *                                 = arr[0*stride] + arr[1*stride] 
 *                                   + ... + arr[(m-2)*stride]
 *
 * In args:     arr:  the values being scanned
 *              m: number of elements in arr involved in the scan
 *              stride:  stride between successive elements involved in
 *                     scan
 * In/out arg:  arr:  see Purpose above
 */
void Excl_scan(int scan[], int arr[], int m, int stride) {
   int i, j;

#  ifdef DEBUG
   printf("In Excl_scan, m = %d, stride = %d\n", m, stride);
#  endif

   scan[0] = 0;
   for (i = 1; i < m; i++) {
      j = i*stride;
      scan[j] = scan[j-stride] + arr[j-stride];
#     ifdef DEBUG
      printf("In Excl_scan, j = %d, scan[j] = %d\n", j, scan[j]);
#     endif
   }

#  ifdef DEBUG
   printf("\n");
#  endif
}  /* Excl_scan */


/*---------------------------------------------------------------------
 * Function:     Push_from
 * Purpose:      Push the contents of sublist into the correct buckets
 *               in dlist
 *
 * In args:      sublist:  the sublist whose contents we're push
 *               list:  the (modified) input list, which is logically
 *                  divided into b sublists, each of which has n/b
 *                  elements.  
 *               n:  the number of elements in list
 *               b:  the number of buckets
 *               from_to_cts:  
 *                  logical two-dimensional array with b rows and b cols:
 *                  from_to_cts[i,j] = number of elements going from
 *                     sublist i to bucket j
 *               row_pref:
 *                  logical two-dimensional array with b rows and b cols:
 *                  row_pref[i,j] = index of first element in sublist 
 *                     i going to bkt j
 *               col_pref:
 *                  logical two-dimensional array with b+1 rows and b cols:
 *                  i < b: col_pref[i,j] = index of first element in bkt j 
 *                     coming from sublist i      
 *                  i = b: col_pref[b,j] = number of elements in bucket j
 *               bkt_starts:
 *                  one dimensional array with b elements
 *                  bkt_start[j] = index of first element in bucket j
 *
 * Out arg:      tlist:  the destination for the contents of list.
 *                  It is logically divided into b buckets. 
 *
 * Note: Given the various data structures, we could implement a similar
 *    function Pull_to, that pulls each element from list into its
 *    destination bucket.  In a shared memory parallel implementation
 *    this could avoid issues with false sharing that might arise using
 *    Push_from.  However, since there can be considerable variation
 *    in bucket sizes, Push_from avoids load imbalance issues.
 */
void Push_from(int sublist, int list[], int n, int b, 
   int from_to_cts[], int row_pref[], int col_pref[], 
   int bkt_starts[], int tlist[])  {

   int first_src_sub, last_src_sub;
   int bkt, src_sub, dest_sub;
   int *slist_arr = list + sublist*n/b; 
   int *bkt_arr;
#  ifdef DEBUG
   char title[100];
#  endif

   for (bkt = 0; bkt < b; bkt++) {
      bkt_arr = tlist + bkt_starts[bkt] + col_pref[sublist*b + bkt] ;
      first_src_sub = row_pref[sublist*b + bkt];
      last_src_sub = (bkt < b-1 ? row_pref[sublist*b + bkt+1]: n/b);
      dest_sub = 0;
      for (src_sub = first_src_sub; src_sub < last_src_sub; src_sub++)
         bkt_arr[dest_sub++] = slist_arr[src_sub];
#     ifdef DEBUG
      sprintf(title, "Slist %d, Bkt %d", sublist, bkt);
      printf("Slist %d, Bkt %d:  first_src_sub = %d, last_src_sub = %d, bkt_start = %d\n",
            sublist, bkt,  first_src_sub, last_src_sub, bkt_starts[bkt]);
      Print_list(title, bkt_arr, last_src_sub - first_src_sub);
      printf("\n");
#     endif
   }

}  /* Push_from */


/*---------------------------------------------------------------------
 * Function:     Check_sort
 * Purpose:      Compare the contents of list to list_ok.  list_ok is
 *                  believed to have been correctly sorted.
 * In args:      all
 */
void Check_sort(int list[], int list_ok[], int m) {
   int i, diff_ct = 0;

   for (i = 0; i < m; i++)
      if (list[i] != list_ok[i]) {
         diff_ct++;
         if (diff_ct == 1) 
            printf("First diff: list[%d] = %d, list_ok[%d] = %d\n",
                  i, list[i], i, list_ok[i]);
      }
   if (diff_ct == 0)
      printf("No differences between list and list_ok\n");
   else
      printf("There were %d differences between list and list_ok\n", 
            diff_ct);
}  /* Check_sort */
