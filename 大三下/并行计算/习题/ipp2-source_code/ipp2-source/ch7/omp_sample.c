/* File:     omp_sample.c
 * Purpose:  Implement parallel sample sort of a list of ints.
 *           This version
 *
 * Compile:  gcc -g -Wall -lm -fopenmp -o omp_sample omp_sample.c
 * Run:      OMP_NUM_THREADS=<thread_count> ./omp_sample <n> <s> <m>
 *              n = list size
 *              s = sample size
 *              m = if m is not present, get list from stdin.  
 *                  Otherwise generate the list with m as the 
 *                  modulus.
 *
 * Input:    If m is on the command line, none.  Otherwise, the list
 * Output:   The sorted list and the run time for sorting it.
 *
 * Notes:
 * 1.  The number of buckets is the same as thread_count.
 * 2.  We assume that 1 < thread_count < s < n.  We also 
 *     assume that thread_count evenly divides both s and n.
 * 3.  Define DEBUG for output after each stage of the sort.
 *     Also define BIG_DEBUG for details on the sampling and
 *        assignment to the buckets.
 * 4.  Define NO_LIST_OUTPUT to suppress printing of the lists.
 *
 * IPP2: 7.2.6 (pp. 412 and ff.)
 */

#include "timer.h"
#include <math.h>    /* For ceil   */
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  /* For memset */

/* Seed for generating list */
#define SEED1 1

/* Only used when DEBUG is defined */
#define MAX_STRING 1000


/*---------------------------------------------------------------------
 * Function prototypes
 */
int   Check_sorted(int list[], int n);
void  Choose_sample(int list[], int loc_n, int sample[], int subsamp_sz);
void  Choose_splitters(int sample[], int s, int splitters[], int thread_count);
int   Compare(const void *a_p, const void *b_p);
void  Find_counts(long my_rank);
int   Find_my_sublist_sz(long my_rank);
void  Gen_list(int list[], int n, int modulus);
void  Get_args(int argc, char* argv[], int* m_p);
int   Merge(int my_sublist[], int curr_sublist_sz, long my_rank, int th, int *dest);
void  Print_as_2d(const char title[], int arr[], int rows, int cols);
void  Print_list(char title[], int list[], int n);
void  Read_list(char prompt[], int list[], int n);
void  Row_prefix_sums(long my_rank);
void  Sample_sort(void);

/*---------------------------------------------------------------------
 * Shared data structures
 */
int thread_count, n, s;
int *list;
int *splitters;
int *mat_counts;
int *th_sublist_szs;
int *sample;


/*---------------------------------------------------------------------
 * Function:  main
 */
int main(int argc, char* argv[]) {
    int modulus;
    double start, finish;

    Get_args(argc, argv, &modulus);

    list = malloc(n*sizeof(int));

    if (modulus == 0)
        Read_list("Enter the list", list, n);
    else
        Gen_list(list, n, modulus);

#   ifndef NO_LIST_OUTPUT 
    Print_list("Before sort, list = ", list, n);
#   endif

    splitters = malloc((thread_count-1)*sizeof(int));
    mat_counts = malloc(thread_count*thread_count*sizeof(int));
    th_sublist_szs = malloc(thread_count*sizeof(int));
    sample = malloc(s*sizeof(int));

    GET_TIME(start);
#   pragma omp parallel
    Sample_sort();
    GET_TIME(finish);

#   ifndef NO_LIST_OUTPUT 
    Print_list("After sort, list = ", list, n);
#   endif

    printf("Elapsed time for the sort = %e seconds\n", finish-start);
    if (Check_sorted(list, n))
        printf("List is sorted\n");

    free(list);
    free(splitters);
    free(mat_counts);
    free(th_sublist_szs);
    free(sample);

    return 0;
}  /* main */

/*---------------------------------------------------------------------
 * Function:     Get_args
 * Purpose:      Get the command line arguments
 * In args:      argc, argv
 * Out args:     m_p
 * Out globals:  thread_count, n, s
 */
void Get_args(int argc, char* argv[], int* m_p) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr,"%s <n> <s> <m>\n", argv[0]);
        fprintf(stderr,"      n = list size\n");
        fprintf(stderr,"      s = sample size\n");
        fprintf(stderr,"      m = if m is not present, get list from\n");
        fprintf(stderr,"         stdin.  Otherwise generate the list with\n");
        fprintf(stderr,"         m as the modulus.\n");
        exit(0);
    }

#   pragma omp parallel
    {
        thread_count = omp_get_num_threads();
    }

    n = strtol(argv[1], NULL, 10);
    s = strtol(argv[2], NULL, 10);
    if (argc == 4)
        *m_p = strtol(argv[3], NULL, 10);
    else 
        *m_p = 0;
}  /* Get_args */


/*---------------------------------------------------------------------
 * Function:   Read_list
 * Purpose:    Read a list of ints from stdin
 * In args:    prompt, n
 * Out arg:    list
 */
void Read_list(char prompt[], int list[], int n) {
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
void Print_list(char title[], int list[], int n) {
    int i;

    printf("%s  ", title);
    for (i = 0; i < n; i++)
        printf("%d ", list[i]);
    printf("\n");
}  /* Print_list */


/*---------------------------------------------------------------------
 * Function:   Check_sorted
 * Purpose:    Determine whether the list is sorted in increasing order
 * In args:    all
 * Ret val:    1 if the list is sorted, 0 otherwise
 */
int Check_sorted(int list[], int n) {
    int i;

    for (i = 0; i < n-1; i++)
        if (list[i] > list[i+1]) {
            printf("list isn't sorted: list[%d] = %d > %d = list[%d]\n",
                    i, list[i], list[i+1], i+1);
            return 0;
        }
    return 1;
}

/*---------------------------------------------------------------------
 * Function:        Sample_sort
 * Purpose:         Use sample sort to sort a list of n ints
 * globals in:      n:  size of the list
 *                  s:  sample size
 *                  thread_count:  number of buckets and number of threads
 * In/out global:   list
 * Scratch globals: splitters, bucket_sizes, bucket_bdries
 *                  mat_counts
 */
void Sample_sort(void) {
    int my_rank = omp_get_thread_num();
    int loc_n = n/thread_count, th;
    int my_first = my_rank*loc_n;
    int my_sublist_sz, curr_sublist_sz, *my_sublist;
    int my_offset;
    int subsample_sz = s / thread_count;
    int subsample_idx = subsample_sz * my_rank;

    // Local sort of each thread's keys
    qsort(list + my_first, loc_n, sizeof(int), Compare);

    Choose_sample(list + my_first, loc_n, sample + subsample_idx, subsample_sz);

#   pragma omp barrier
    if (my_rank == 0) {
        // Sort the sample so we can choose splitters
        qsort(sample, s, sizeof(int), Compare);
#       ifdef DEBUG
            Print_list("sample =", sample, s);
#       endif

        Choose_splitters(sample, s, splitters, thread_count);
#       ifdef DEBUG
            Print_list("splitters =", splitters, thread_count-1);
#       endif
    }

#   pragma omp barrier

    Find_counts(my_rank);

#   pragma omp barrier

    my_sublist_sz = Find_my_sublist_sz(my_rank);
    th_sublist_szs[my_rank] = my_sublist_sz;
    my_sublist = malloc(my_sublist_sz*sizeof(int));

#   pragma omp barrier

    Row_prefix_sums(my_rank);

#   pragma omp barrier

    // Copy the elements from Thread 0's list into the first slots in the
    // caller's sublist 
    void *dest = malloc(my_sublist_sz * sizeof(int));
    curr_sublist_sz= 0;
    for (th = 0; th < thread_count; th++)
        curr_sublist_sz = Merge(my_sublist, curr_sublist_sz, my_rank, th, dest);
    free(dest);

    my_offset = 0;
    for (th = 0; th < my_rank; th++)
        my_offset += th_sublist_szs[th];

#   pragma omp barrier
    memcpy(list + my_offset, my_sublist, my_sublist_sz * sizeof(int));
    free(my_sublist);
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
    return a - b;
}  /* Compare */


/*----------------------------------------------------------------------
 * Function:   Choose_sample
 * Purpose:    Choose the sample from the original list.
 * In args:    list, loc_n, subsamp_sz
 * Out arg:    sample
 */
void Choose_sample(int list[], int loc_n, int sample[], int subsamp_sz) {
    int i;
    for (i = 0; i < subsamp_sz; i++) {
        sample[i] = list[i * (loc_n / subsamp_sz)];
    }
}  /* Choose_sample */


/*----------------------------------------------------------------------
 * Function:   Choose_splitters 
 * Purpose:    Determine the cutoffs for the buckets.  There will be
 *             b-1 splitters: splitters[0], splitters[1], ..., 
 *             splitters[b-2], and each bucket should get s/b elements
 *             of the sample.
 * In args:    sample, s, b = thread_count
 * Out arg:    splitters
 * Note:       The splitters and buckets are
 *             bucket 0 =      -infty    <= x < splitters[0] 
 *             bucket 1 =   splitters[0] <= x < splitters[1] 
 *             bucket 2 =   splitters[1] <= x < splitters[2] 
 *                 ...
 *           bucket b-1 = splitters[b-2] <= x < infty
 */
void Choose_splitters(int sample[], int s, int splitters[], int b) {
    int per_bucket = s/b; // Note that s is evenly divisible by b
    int i, j;

    for (i = 0; i < b-1; i++) {
        j = (i+1)*per_bucket;
        // Note that j >= 1, since i+1 >= 0 and per_bucket >= 1
        // Also note that dividing by 2.0 is necessary:  otherwise
        //    integer division will take the floor and the call
        //    to ceil will have no effect
        splitters[i] = ceil((sample[j-1] + sample[j])/2.0);
    }
}  /* Choose_splitters */


/*----------------------------------------------------------------------
 * Function:    Find_counts
 * Purpose:     Find the number of elements in the calling thread's
 *              sublist of list that should go to each thread.
 * In globals:  list, splitters
 * Out global:  mat_counts
 *              mat_count[my_rank*thread_count, th] = number 
 *                 of keys in my_rank's block that should 
 *                 go to thread th 
 *              So each thread initializes one "row" of 
 *                 mat_counts
 */
void Find_counts(long my_rank) {
    int th, curr_count, total, loc_n = n/thread_count, i; 
    int* my_sublist = list + my_rank*loc_n;
    int* my_mat_counts = mat_counts + my_rank*thread_count;

    i = total = 0;
    for (th = 0; th < thread_count-1; th++) {
        curr_count = 0;
        while (i < loc_n && my_sublist[i] < splitters[th]) {
            curr_count++;
            i++;
        }
        total += curr_count;
        my_mat_counts[th] = curr_count;
    }
    my_mat_counts[thread_count-1] = loc_n - total;

}  /* Find_counts */


/*----------------------------------------------------------------------
 * Function:   Find_my_sublist_sz
 * Purpose:    Find the size of the sublist that the calling thread
 *             will be working with.  This is obtained by summing
 *             the entries in the appropriate column of mat_counts.
 * In arg:     my_rank
 * In global:  mat_counts
 * Ret val:    Sum of the entries in column my_rank
 */
int Find_my_sublist_sz(long my_rank) {
    int count = 0, i;

    for (i = 0; i < thread_count; i++)
        count += mat_counts[i*thread_count + my_rank];
    return count;
}  /* Find_my_list_sz */


/*----------------------------------------------------------------------
 * Function:       Row_prefix_sums
 * Purpose:        Overwrite mat_counts with the prefix sums across
 *                    each row
 * In arg:         my_rank
 * In global:      thread_count
 * In/out global:  mat_counts
 */
void Row_prefix_sums(long my_rank) {
    int i;
    int* my_row = mat_counts + my_rank*thread_count;
    for (i = 1; i < thread_count; i++)
        my_row[i] += my_row[i-1];
}  /* Row_prefix_sums */


/*----------------------------------------------------------------------
 * Function:    Merge
 * Purpose:     Merge thread th's sublist of list into the 
 *              calling threads my_sublist
 * In args:     curr_sublist_sz, my_rank, th, dest
 *              Note: 'dest' is for temporary storage; pre-allocated outside
 *              Merge() to reduce memory allocation overhead.
 * In/out arg:  my_sublist 
 * In globals:  list, mat_counts
 * Ret val:     Updated value of curr_sublist_sz
 */
int  Merge(int my_sublist[], int curr_sublist_sz, long my_rank, int th, int *dest) {
    int tlist_i, slist_i, dest_i, my_first, my_last;
    int dest_sz;
    int *tlist = list + th*n/thread_count;

    if (my_rank == 0)
        my_first = 0;
    else
        my_first = mat_counts[th*thread_count + my_rank - 1];
    my_last = mat_counts[th*thread_count + my_rank];
    dest_sz = curr_sublist_sz + my_last - my_first;

    tlist_i = my_first; slist_i = dest_i = 0;
    while (tlist_i < my_last && slist_i < curr_sublist_sz)
        if (tlist[tlist_i] < my_sublist[slist_i])
            dest[dest_i++] = tlist[tlist_i++];
        else
            dest[dest_i++] = my_sublist[slist_i++];

    while (tlist_i < my_last)
        dest[dest_i++] = tlist[tlist_i++];
    while (slist_i < curr_sublist_sz)
        dest[dest_i++] = my_sublist[slist_i++];

    memcpy(my_sublist, dest, dest_sz*sizeof(int));

    return dest_sz;
}  /* Merge */


/*----------------------------------------------------------------------
 * Function:   Print_as_2d
 * Purpose:    Print a one dimensional array as a two dimensional
 *                with 
 * In args:    all
 */
void Print_as_2d(const char title[], int arr[], int rows, 
        int cols) {
    int i, j;

    printf("%s: \n", title);
    for (i = 0; i < rows; i++) {
        for (j = 0; j < cols; j++)
            printf("%2d ", arr[i*cols + j]);
        printf("\n");
    }

}  /* Print_as_2d */


