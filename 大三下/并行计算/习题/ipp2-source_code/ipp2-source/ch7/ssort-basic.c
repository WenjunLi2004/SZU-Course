/* File:     ssort-basic.c
 * Purpose:  Implement the basic version of serial sample sort from IPP2
 *
 * Compile:  gcc -g -Wall -o ssort-b ssort-basic.c
 *              Define PRINT_LIST to get output of original and sorted lists
 * Run:      ./ssort-b <n> <s> <b> [mod]
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
 * 3.  The sample is generated using the random() function:
 *
 *           int chosen[s];
 *
 *           srandom(...);
 *           for (int i = 0; i < s; i++) {
 *              int j = random() % n;
 *              while (In_chosen(chosen, i, j))
 *                 j = random() % n;
 *              chosen[i] = j;
 *              sample[i] = list[j];
 *           }
 *     
 * 4.  In this code we use the C library qsort function for sorting of
 *     the sublists and the sample
 *
 * IPP2:  7.2.1 (pp. 398 and ff.)
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

#define LIST_SEED 1
#define SAMPLE_SEED 2

void Usage(char pname[]);
void Get_args(int argc, char* argv[], int* n_p, int* s_p, 
         int* b_p, int* mod_p);
void Init_list(int list[], int n, int mod);
void Gen_list(int list[], int n, int mod);
void Print_list(char title[], int list[], int n);
void Sample_sort(int list[], int n, int s, int b);
int  In_chosen(int chosen[], int size, int sub);
void Gen_sample(int list[], int n, int s, int b, int sample[]);
void Find_splitters(int sample[], int s, int splitters[], int b);
int  Find_bkt(int elt, int splitters[], int b);
void Append_elt(int elt, int* buckets[], int in_bkt_cts[], int bkt_allocs[], 
            int which_bkt);
void Print_bkts(char title[], int* buckets[], int in_bkt_cts[], int b);

void Read_list(char prompt[], int list[], int n);
void Sort(int tlist[], int m);
int  Append_bkt(int list[], int curr_sz, int bucket[], int bkt_sz);
int  Compare(const void* ap, const void* bp);
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
 */
void Sample_sort(int list[], int n, int s, int b) {
   int i, next_bkt_start, which_bkt, default_bkt_sz = 2*n/b;
   int *sample, *splitters;
   int **buckets, *in_bkt_cts, *bkt_allocs;
#  ifdef DEBUG
   char title[1000];
#  endif

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

   /* Allocate storage for buckets, in_bkt_cts, and bkt_allocs */
   /* Initialize in_bkt_cts and bkt_allocs */
   buckets = malloc(b*sizeof(int*));
   in_bkt_cts = malloc(b*sizeof(int));
   bkt_allocs = malloc(b*sizeof(int));
   for (i = 0; i < b; i++) {
      buckets[i] = malloc(default_bkt_sz*sizeof(int));
      in_bkt_cts[i] = 0;
      bkt_allocs[i] = default_bkt_sz;
   }

   /* Determine which bucket each element of list goes to */
   /* Append the element to the appropriate bucket */
   for (i = 0; i < n; i++) {
      which_bkt = Find_bkt(list[i], splitters, b);
      Append_elt(list[i], buckets, in_bkt_cts, bkt_allocs, 
            which_bkt);
   }
#  ifdef DEBUG
   Print_bkts("After copying to bkts", buckets, in_bkt_cts, b);
#  endif

   /* Sort each bucket, and append its contents to the list */
   next_bkt_start = 0;
   for (i = 0; i < b; i++) {
      Sort(buckets[i], in_bkt_cts[i]);
      next_bkt_start = Append_bkt(list, next_bkt_start, buckets[i], 
            in_bkt_cts[i]);
   }
#  ifdef DEBUG
   Print_list("After appending bkts to list", list, n);
#  endif

   free(sample);
   free(splitters);
   for (i = 0; i < b; i++)
      free(buckets[i]);
   free(buckets);
   free(in_bkt_cts);
   free(bkt_allocs);
}  /* Sample_sort */


/*---------------------------------------------------------------------
 * Function:  In_chosen
 * Purpose:   Check to see whether list[sub] has been chosen for
 *            for the sample yet
 */
int In_chosen(int chosen[], int size, int sub) {
   int k;

   for (k = 0; k < size; k++)
      if (chosen[k] == sub) return 1;
   return 0;
}  /* In_chosen */


/*---------------------------------------------------------------------
 * Function:  Gen_sample
 * Purpose:   Generate the sample using a random number generator
 */
void Gen_sample(int list[], int n, int s, int b, int sample[]) {
   int i, j, chosen[s];

   srandom(SAMPLE_SEED);
   for (i = 0; i < s; i++) {
      j = random() % n;
      while (In_chosen(chosen, i, j))
         j = random() % n;
      chosen[i] = j;
      sample[i] = list[j];
   }
}  /* Gen_sample */


/*---------------------------------------------------------------------
 * Function:  Find_bkt
 * Purpose:   Determine which bucket elt should go to.
 * Note:  The buckets are
 *        bucket
 *           0:   MINUS_INFTY = splitters[0] <= x < splitters[1]
 *           1:   splitters[1] <= x < splitters[2]
 *           ...
 *           i:   splitters[i] <= x < splitters[i+1]
 *           ...
 *           b-1: splitters[b-1] <= x < splitters[b] = INFTY
 *
 */
int  Find_bkt(int elt, int splitters[], int b) {
   int min = 0, max = b+1, mid;

   while (min <= max) {
      mid = (min + max)/2;
      if (splitters[mid] <= elt) {
         if (elt < splitters[mid+1])
            return mid;
         else  /* splitters[mid+1] <= elt */ 
            min = mid+1;
      } else { /* elt < splitters[mid] */
         max = mid-1;
      }
   } /* while */

   return -1;  /* Never executed */
}  /* Find_bkt */


/*---------------------------------------------------------------------
 * Function:  Which_dest  (Unused)
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
 * Function:    Append_elt
 * Purpose:     Append an element to the bucket, buckets[which_bkt].
 *              If there isn't room in the bucket call realloc.  
 *              If the call to realloc fails, exit.
 * In args:     elt:  the element being added to the bucket
 *              which_bkt:  the bucket to which the element is added
 *
 * In/out args: If there's a successful call to realloc:
 *                 buckets[which_bkt] will have its allocation doubled, and
 *                    elt will be appended to buckets[which_bucket]
 *                 in_bkt_cts[which_bucket] will be incremented
 *                 bkt_allocs[which_bucket] will be doubled
 *              If there isn't a call to realloc:
 *                 buckets[which_bkt] will have elt appended
 *                 in_bkt_cts[which_bucket] will be incremented
 */
void Append_elt(int elt, int* buckets[], int in_bkt_cts[], int bkt_allocs[], 
            int which_bkt) {
   int* new_ptr;

   if (in_bkt_cts[which_bkt] == bkt_allocs[which_bkt]) {
      new_ptr = realloc(buckets[which_bkt], 
            2*bkt_allocs[which_bkt]*sizeof(int));
      if (new_ptr == NULL) {
         fprintf(stderr, "\n***Can't allocate %d ints for bucket %d***\n",
               2*bkt_allocs[which_bkt], which_bkt);
         exit(-1);
      } else {
         buckets[which_bkt] = new_ptr;
         bkt_allocs[which_bkt] *= 2;
#        ifdef DEBUG
         printf("Allocated %d ints for bucket %d\n", bkt_allocs[which_bkt],
               which_bkt);
#        endif
      }
   }

   /* There is enough space in buckets[which_bkt] for an additional element */
   buckets[which_bkt][in_bkt_cts[which_bkt]] = elt;
   in_bkt_cts[which_bkt]++;
}  /* Append_elt */


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
 * Function:  Print_bkts
 * Purpose:   Print the buckets
 */
void Print_bkts(char title[], int* buckets[], int in_bkt_cts[], int b) {
   int i, j;

   printf("%s:\n", title);
   for (i = 0; i < b; i++) {
      for (j = 0; j < in_bkt_cts[i]; j++) {
         printf("%d ", buckets[i][j]);
      }
      printf("\n");
   }
}  /* Print_bkts */


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
 * Function:  Append_bkt
 * Purpose:   Append a bucket to the list
 *            Return the number of elements in the new list
 */
int  Append_bkt(int list[], int curr_sz, int bucket[], int bkt_sz) {
   int i, *tlist = list + curr_sz;

   for (i = 0; i < bkt_sz; i++)
      tlist[i] = bucket[i];
   return curr_sz + bkt_sz;
}  /* Append_bkt */


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
