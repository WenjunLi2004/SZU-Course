/* File:      mpi-ssort-bfly.c
 * Purpose:   Implement sample sort of a list of ints using MPI and 
 *               - random number generator to generate sample, 
 *               - distributed odd-even sort to sort sample
 *               - distributed choice of splitters
 *               - binary search to identify contents of buckets
 *                 on each thread
 *               - Hand-coded butterfly to redistribute list
 *               - This version allocates an array in Butterfly
 *                 on the stack
 *
 * Compile:   mpicc -g -Wall -o mpi-ssort-aaoe mpi-ssort-aaoe.c mpi-oe.c stats.c
 *            note:  stats.h header file should be in current directory
 * Run:       mpiexec -n <p> ./mpi-ssort-aaoe <n> <s> [mod]
 *               p:  number of MPI processes
 *               n:  number of elements in the list
 *               s:  size of the sample
 *             mod:  if present, modulus for calls to random()
 *                   used in generating the list
 *            note:  p should be a power of 2 and
 *                   it should evenly divide both n and s
 *                   Also s should evenly divide n.
 *
 * Input:    If mod is on the command line, none.
 *           Otherwise, the n elements of the list
 * Output:   Run-time of the sample sort and run time of qsort
 *           on process 0.  Whether list was correctly sorted.
 *           If PRINT_LIST is defined, initial list and sorted
 *              list
 *           If DEBUG is defined, verbose output on progress of
 *              program.
 * Notes:     
 * 1.  ITERS is a macro used to specify the number of times
 *     the program will be run.  Currently ITERS = 1 if
 *     DEBUG is set.  Otherwise ITERS = 10.
 * 2.
 *
 * IPP2:     7.2.8 (pp. 423 and ff.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>


/* stats.c defines variables
 *   s_dmin, s_dmax, s_dtotal  // MPI runtimes 
 *   s_hmin, s_hmax, s_htotal  // Qsort runtimes
 * and functions
 *   Setup_stats (not used), Update_stats and Print_stats
 */
#include "stats.h"

/* Code for MPI odd-even transposition sort */
/* We call 
 * void OE_sort(int** local_A_p, int* temp_B, int** temp_C_p, int loc_n, 
 *       int my_rank, int p, MPI_Comm comm);
 */
#include "mpi-oe.h"


#ifdef DEBUG
#define ITERS 1
#else
//#define ITERS 2
#define ITERS 10
#endif

/* Seed when list is generated using random() */
#define SEED_GEN 1

/* Add to process rank for seeds when generating sample */
#define SEED_SAMPLE 2

/* <= every element in the input list */
#define MINUS_INFTY (1 << 31) /* = -2^31 */

/* > every element in input list */
/* (It's not 2^31 - 1, since we want to be able to add to it.) */
#define INFTY (1 << 30)  /* = 2^30 */


/*---------------------Functions Only Run by Proc 0-------------------*/
void Usage(char progname[]);
void Print_list(const char title[], int list[], int n);
void Read_list(int list[], int n);
void Gen_list(int list[], int n, int mod);
void Find_splitters(int sample[], int s, int splitters[], int p);
int  Check_sorted(int list1[], int list2[], int n);


/*----------------------------Local Functions-------------------------*/
int  Compare(const void *a_p, const void *b_p);
void Gen_sample(int my_rank, int loc_list[], int loc_n, int loc_samp[],
      int loc_s);
int  In_chosen(int chosen[], int chosen_sz, int sub);
int Bin_srch(int list[], int splj, int spljp1, int min, int max);
int Bin_lin_srch(int list[], int splj, int min, int prev);
void Count_elts_going_to_procs(int loc_list[], int loc_n, int splitters[], 
      int p, int my_to_counts[]);
void Excl_prefix_sums(int in[], int out[], int m);
void Send_upper_args(int local_A[], int loc_n, int* offset_p, 
      int* count_p, double splitter);
void Send_lower_args(int local_A[], int loc_n, int* count_p, 
      double splitter);
void Merge(int** loc_list_p, int* loc_n_p, int offset, 
      int rcv_buf[], int rcv_buf_sz, int** tmp_buf_p);


/*--------------------Functions Involving Communication---------------*/
void Get_args(int argc, char* argv[], int my_rank, int p, MPI_Comm comm,
      int* n_p, int* s_p, int* mod_p);
void Ssort(int my_rank, int p, MPI_Comm comm, int tlist[], int n, 
      int loc_list[], int loc_n, int s);
void Dist_find_splitters(int my_rank, int p, MPI_Comm comm, 
      int loc_samp[], int loc_s, int splitters[]);
void Print_loc_lists(int my_rank, int p, MPI_Comm comm, 
      const char title_in[], int list[], int n);
void Get_list(int my_rank, int p, MPI_Comm comm, int list[], int n, int mod,
      int loc_list[], int loc_n);
void Butterfly(int loc_list[], int* loc_n_p, int splitters[],
      int tlist[], int n,
      int my_rank, int p, MPI_Comm comm);
void Send_recv(int loc_list[], int count, int rcv_buf[], 
      int* rcv_buf_sz_p, int n, int partner, MPI_Comm comm);

#ifdef START_DEBUGGERS
/* Function:  Block_for_debugger
 * Purpose:   Block execution of the program so that a debugger can
 *            be attached to each MPI process in a separate terminal
 *            window
 */
void Block_for_debugger(int my_rank, MPI_Comm comm) {
#  include <sys/types.h>
#  include <unistd.h>
   char c;

   printf("Proc %d > Process ID = %d\n", my_rank, getpid());
   if (my_rank == 0) {
      printf("Hit enter after you've attached a debugger to each MPI process\n");
      scanf("%c", &c);
   }
   MPI_Barrier(comm);
}  /* Block_for_debugger */
#endif


/*--------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
   int n, p, s, mod, my_rank;
   int *list = NULL, *tlist1 = NULL, *tlist2 = NULL, *loc_list;
   int loc_n;
   int iter;
   MPI_Comm comm;
   double start, finish, elapsed, my_start, my_finish, my_elapsed; 

   MPI_Init(&argc, &argv);
   comm = MPI_COMM_WORLD;
   MPI_Comm_size(comm, &p);
   MPI_Comm_rank(comm, &my_rank);
#  ifdef START_DEBUGGERS
   Block_for_debugger(my_rank, comm);
#  endif
   printf("Proc %d > pid = %d\n", my_rank, getpid());
   if (my_rank == 0) {
      char c;
      printf("Hit Enter to continue\n");
      scanf("%c", &c);
   }
   MPI_Barrier(comm);

   Get_args(argc, argv, my_rank, p, comm, &n, &s, &mod);
   if (my_rank == 0) {
      list = malloc(n*sizeof(int));
      tlist2 = malloc(n*sizeof(int));
   }
   tlist1 = malloc(n*sizeof(int));
   loc_n = n/p;
   /* Note the increased size for loc_list! */
   loc_list = malloc(n*sizeof(int));

   Get_list(my_rank, p, comm, list, n, mod, loc_list, loc_n);
#  ifdef PRINT_LIST
   if (my_rank == 0)
      Print_list("Original list", list, n);
#  endif 

   for (iter = 0; iter < ITERS; iter++) {
      MPI_Scatter(list, loc_n, MPI_INT, loc_list, loc_n, MPI_INT, 
            0, comm);

      MPI_Barrier(comm);
      my_start = MPI_Wtime();
      Ssort(my_rank, p, comm, tlist1, n, loc_list, loc_n, s);
      my_finish = MPI_Wtime();
      my_elapsed = my_finish - my_start;
      MPI_Reduce(&my_elapsed, &elapsed, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
      
      if (my_rank == 0)
         Update_stats(0.0, elapsed, &s_dmin, &s_dmax, &s_dtotal);

      if (my_rank == 0) {
         memcpy(tlist2, list, n*sizeof(int));
         start = MPI_Wtime();
         qsort(tlist2, n, sizeof(int), Compare);
         finish = MPI_Wtime();
         Update_stats(start, finish, &s_hmin, &s_hmax, &s_htotal);
      }
   } /* for iter */

#  ifdef PRINT_LIST 
   if (my_rank == 0)
      Print_list("After sort, list = ", tlist1, n);
#  endif

   if (my_rank == 0) {
      Print_stats("Sample", s_dmin, s_dmax, s_dtotal, ITERS);
      Print_stats(" qsort", s_hmin, s_hmax, s_htotal, ITERS);
   }

   if (my_rank == 0) {
      if (Check_sorted(tlist1, tlist2, n)) 
         printf("List is sorted\n");
      else
         Print_list("             list = ", tlist1, n);
   }

   if (my_rank == 0) {
      free(list);
      free(tlist2);
   }
   free(tlist1);
// free(loc_list);

   MPI_Finalize(); 
   return 0;

}  /* main */


/*--------------------------------------------------------------------
 * Function:  Usage
 * Purpose:   Print info on command line
 * Note:      Only called by process 0
 */
void Usage(char progname[]) {
   fprintf(stderr, "usage: mpiexec -n <p> %s <n> <s> [mod]\n", progname);
   fprintf(stderr, "    p:  number of MPI processes\n");
   fprintf(stderr, "    n:  number of elements in the list\n");
   fprintf(stderr, "    s:  size of the sample\n");
   fprintf(stderr, "  mod:  if present, modulus for calls to random()\n");
   fprintf(stderr, "        used in generating the list\n");
   fprintf(stderr, " note:  p should evenly divide both n and s, and\n");
   fprintf(stderr, "        s should evenly divide n.\n");
}  /* Usage */


/*--------------------------------------------------------------------
 * Function:  Get_args
 * Purpose:   Get and distribute the command line arguments.  If they're 
 *            incorrect, print a message and quit.
 */           
void Get_args(int argc, char* argv[], int my_rank, int p, MPI_Comm comm,
      int* n_p, int* s_p, int* mod_p) {
   int keep_going = 1;

   if (my_rank == 0) 
      if (argc != 3 && argc != 4) {
         Usage(argv[0]);
         keep_going = 0;
      }
   MPI_Bcast(&keep_going, 1, MPI_INT, 0, comm);
   if (!keep_going) {
      MPI_Finalize();
      exit(0);
   }

   if (my_rank == 0) {
      *n_p = strtol(argv[1], NULL, 10);
      *s_p = strtol(argv[2], NULL, 10);
      if (argc == 4)
         *mod_p = strtol(argv[3], NULL, 10);
      else
         *mod_p = 0;
      if (*s_p % p != 0 || *n_p % *s_p != 0)
         keep_going = 0;
   }
   MPI_Bcast(&keep_going, 1, MPI_INT, 0, comm);
   if (!keep_going) {
      MPI_Finalize();
      exit(0);
   }
   MPI_Bcast(n_p, 1, MPI_INT, 0, comm);
   MPI_Bcast(s_p, 1, MPI_INT, 0, comm);
   MPI_Bcast(mod_p, 1, MPI_INT, 0, comm);
}  /* Get_args */


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

/*---------------------------------------------------------------------
 * Function:   Print_list
 * Purpose:    Print a list of ints to stdout
 */
void Print_list(const char title[], int list[], int n) {
   int i;

   printf("%s  ", title);
   for (i = 0; i < n; i++)
      printf("%d ", list[i]);
   printf("\n");
}  /* Print_list */


/*---------------------------------------------------------------------
 * Function:   Print_loc_lists
 * Purpose:    Print the list belonging to each process
 */
void Print_loc_lists(int my_rank, int p, MPI_Comm comm, 
      const char title_in[], int list[], int n) {
   int elt_cts[p];
   int max_n = 0, i;
   int *tlist = NULL;
   char title[100];
   MPI_Status status;

   MPI_Gather(&n, 1, MPI_INT, elt_cts, 1, MPI_INT, 0, comm);
   if (my_rank == 0) {
      for (i = 0; i < p; i++)
         if (elt_cts[i] > max_n) max_n = elt_cts[i];
      tlist = malloc(max_n*sizeof(int));
      printf("%s\n", title_in);
      sprintf(title, "Proc 0 > ");
      Print_list(title, list, n);
      for (i = 1; i < p; i++) {
         MPI_Recv(tlist, max_n, MPI_INT, i, 0, comm, &status);
         sprintf(title, "Proc %d > ", i);
         Print_list(title, tlist, elt_cts[i]);
      }
      printf("\n");
      fflush(stdout);
      free(tlist);
   } else {
      MPI_Send(list, n, MPI_INT, 0, 0, comm);
   }
}  /* Print_loc_lists */


/*---------------------------------------------------------------------
 * Function:   Read_list
 * Purpose:    Read a list of ints from stdin
 */
void Read_list(int list[], int n) {
   int i;

   printf("Enter the %d elements of the list\n", n);
   for (i = 0; i < n; i++)
      scanf("%d", &list[i]);
}  /* Read_list */


/*---------------------------------------------------------------------
 * Function:   Gen_list
 * Purpose:    Generate a list of ints using random() and taking
 *             remainders modulo mod
 */
void Gen_list(int list[], int n, int mod) {
   int i;

   srandom(SEED_GEN);
   for (i = 0; i < n; i++)
      list[i] = random() % mod;
}  /* Gen_list */


/*---------------------------------------------------------------------
 * Function:   Get_list
 * Purpose:    Either read the list from stdin or generate it using
 *             the random() fcn and taking remainders modulo mod
 */
void Get_list(int my_rank, int p, MPI_Comm comm, int list[], int n, 
      int mod, int loc_list[], int loc_n) {
   if (my_rank == 0) {
      if (mod == 0)
         Read_list(list, n);
      else
         Gen_list(list, n, mod);
   }
   MPI_Scatter(list, loc_n, MPI_INT, loc_list, loc_n, MPI_INT, 
         0, comm);
}  /* Get_list */


/*---------------------------------------------------------------------
 * Function:   Ssort
 * Purpose:    Implement MPI sample sort using
 *               - random number generator to generate sample, 
 *               - All processes to sort sample and choose splitters,
 *               - Butterfly to redistribute list
 * In args:    my_rank:  calling process' rank in communicator
 *             p:  number of processes in communicator
 *             comm:  communicator used by all processes
 *             n:  size of global list
 *             loc_n:  number of elements in calling process'
 *                 sublist (n = allocated storage)
 *             s:  size of sample
 *
 * In/out:     loc_list:  calling process' sublist
 *            
 * Scratch/out:tlist: temporary storage on each process.
 *             Can store up to n elements.  Sorted global 
 *             list on process 0 on return
 */
void Ssort(int my_rank, int p, MPI_Comm comm, int tlist[], int n, 
      int* loc_list, int loc_n, int s) {
   int *loc_samp, *temp_b, *temp_c, loc_s = s/p;
   int splitters[p+1];
   int bkt_counts[p];
   int bkt_offsets[p];

#  ifdef DEBUG
   Print_loc_lists(my_rank, p, comm, "Start of Ssort", loc_list, loc_n);
#  endif
   
   /* Generate the sample and find the splitters */
   loc_samp = malloc(loc_s*sizeof(int));
   temp_b = malloc(loc_s*sizeof(int));
   temp_c = malloc(loc_s*sizeof(int));
   Gen_sample(my_rank, loc_list, loc_n, loc_samp, loc_s);
   
   /* Call to odd-even transposition sort */
   OE_sort(&loc_samp, temp_b, &temp_c, loc_s, my_rank, p, comm);
   /* Get splitters */
   Dist_find_splitters(my_rank, p, comm, loc_samp, loc_s, splitters);

#  ifdef DEBUG
   Print_loc_lists(my_rank, p, comm, "Sample", loc_samp, loc_s);
   Print_loc_lists(my_rank, p, comm, "Splitters", splitters, p+1);
#  endif
   free(loc_samp);
   free(temp_b);
   free(temp_c);

   /* Sort local list */
   qsort(loc_list, loc_n, sizeof(int), Compare);
#  ifdef DEBUG
   Print_loc_lists(my_rank, p, comm, "After qsort loc_lists", loc_list, loc_n);
#  endif

   if (p > 1) 
      /* Note that loc_list is freed in Butterfly */
      Butterfly(loc_list, &loc_n, splitters, tlist, n, 
         my_rank, p, comm);
   else 
      memcpy(tlist, loc_list, loc_n*sizeof(int));

#  ifdef DEBUG
   Print_loc_lists(my_rank, p, comm, "After Butterfly loc_lists", tlist, loc_n);
#  endif

   /* Gather buckets to Process 0 */
   MPI_Gather(&loc_n, 1, MPI_INT, bkt_counts, 1, MPI_INT, 
         0, comm);
#  ifdef DEBUG
   if (my_rank == 0) Print_list("Bucket counts", bkt_counts, p);
#  endif
   if (my_rank == 0) {
      Excl_prefix_sums(bkt_counts, bkt_offsets, p);
#     ifdef DEBUG
      if (my_rank == 0) Print_list("Prefix sums of bucket counts", 
            bkt_offsets, p);
#     endif
      MPI_Gatherv(MPI_IN_PLACE, loc_n, MPI_INT, tlist, bkt_counts,
            bkt_offsets, MPI_INT, 0, comm);
   } else {
      MPI_Gatherv(tlist, loc_n, MPI_INT, tlist, bkt_counts,
            bkt_offsets, MPI_INT, 0, comm);
   }
#  ifdef DEBUG
   if (my_rank == 0) Print_list("After Gatherv", tlist, n);
#  endif
}  /* Ssort */


/*---------------------------------------------------------------------
 * Function:   In_chosen
 * Purpose:    Check whether sub has already been chosen for the sample
 */
int  In_chosen(int chosen[], int chosen_sz, int sub) {
   int j;

   for (j = 0; j < chosen_sz; j++)
      if (chosen[j] == sub) return 1;
   return 0;
}  /* In_chosen */


/*---------------------------------------------------------------------
 * Function:   Gen_sample
 * Purpose:    Generate calling process' part of the sample
 */
void Gen_sample(int my_rank, int loc_list[], int loc_n, int loc_samp[],
      int loc_s) {
   int i, sub;
   int chosen[loc_s];

   srandom(SEED_SAMPLE + my_rank);
   for (i = 0; i < loc_s; i++) {
      sub = random() % loc_n;
      while (In_chosen(chosen, i, sub))
         sub = random() % loc_n;
      chosen[i] = sub;
      loc_samp[i] = loc_list[sub];
   }
}  /* Gen_sample */


/*---------------------------------------------------------------------
 * Function:   Dist_find_splitters
 * Purpose:    Select splitters from sample
 * Note:       splitters has storage for p+1 elements
 */
void Dist_find_splitters(int my_rank, int p, MPI_Comm comm, 
      int loc_samp[], int loc_s, int splitters[]) {
   int my_splitter, prev_max, my_min, my_max;
   MPI_Request rreq;
   MPI_Status status;

   if (my_rank > 0)
      my_min = loc_samp[0];
   if (my_rank < p-1)
      my_max = loc_samp[loc_s-1];
      
   if (my_rank > 0)
      MPI_Irecv(&prev_max, 1, MPI_INT, my_rank-1, 0, comm, &rreq);
   if (my_rank < p-1) 
      MPI_Send(&my_max, 1, MPI_INT, my_rank+1, 0, comm);
   if (my_rank > 0) {
      MPI_Wait(&rreq, &status);
      my_splitter = my_min + prev_max;
      if (my_splitter % 2 != 0) my_splitter++;
      my_splitter /= 2;
   } else { /* my_rank == 0 */
      my_splitter = MINUS_INFTY;
   }

   MPI_Allgather(&my_splitter, 1, MPI_INT, splitters, 1, MPI_INT, comm);
   splitters[p] = INFTY;
}  /*  Dist_find_splitters */


/*---------------------------------------------------------------------
 * Function:   Find_splitters
 * Purpose:    Select splitters from sample
 * Note:       splitters has storage for p+1 elements
 */
void Find_splitters(int sample[], int s, int splitters[], int p) {
   int i, samp_sub, sum, samps_per_slist = s/p;
   
   qsort(sample, s, sizeof(int), Compare);
   splitters[0] = MINUS_INFTY;
   splitters[p] = INFTY;

   for (i = 1; i < p; i++) {
      samp_sub = i*samps_per_slist;
      sum = sample[samp_sub-1] + sample[samp_sub];
      if (sum % 2 != 0) sum++;
      splitters[i] = sum/2;
   }
}  /* Find_splitters */


/*---------------------------------------------------------------------
 * Function:  Bin_srch
 * Purpose:   Carry out the first part of a binary/linear search for 
 *            the element splj in list.  We want the smallest element
 *            of list that is >= splj.  (This is the smallest element
 *            of bucket j.) So we look for a subscript i such that 
 *            min <= i <= max, and
 *             
 *               list[i-1] < splj <= list[i] < spljp1 <= list[i+1]
 *
 *            Here 
 *
 *               splj = splitters[j], 
 *
 *            and
 *
 *               spljp1 = splitters[j+1], 
 *
 *            for some j such that 0 <= j < p, where the array splitters
 *            has p+1 elements.
 *
 *            We first carry out a binary search for a subscript k
 *            such that 
 *
 *                  splj <= list[k] < spljp1 
 *
 *            Then we use Bin_lin_srch to find the smallest such k,
 *            and return this.
 *
 */
int Bin_srch(int list[], int splj, int spljp1, int min, int max) {
      int mid;

   while (min <= max) {
      mid = (min + max)/2;
//    printf("   mid = %d\n", mid);
      if (splj <= list[mid]) {
         if (list[mid] < spljp1) {
            /* splj <= list[mid] < spljp1.  Find smallest such mid */
            return Bin_lin_srch(list, splj, min, mid);
         } else  /* spljp1 <= list[mid] */
            max = mid - 1;
      } else { /* list[mid] < splj */
         min = mid + 1;
      }
   }

   /* We didn't find an element list[i] such that 
    *
    *    splj <= list[i] < spljp1
    *
    * So this bucket must be empty.
    */
// printf("   returning mid = min\n");
   return min;
}  /* Bin_srch */


/*---------------------------------------------------------------------
 * Function:   Bin_lin_srch
 * Purpose:    From Bin_srch we know that
 *
 *                splj <= list[prev] < spljp1
 *
 *             Find the smallest subscript i >= min such that 
 *
 *                splj <= list[i] < spljp1
 *
 * Algorithm:  Run a binary search until we find a subscript
 *             mid with min <= mid < prev and
 *
 *                list[mid] < splj
 *
 *             Then back up to the predecessor of mid in the
 *             search and use linear search until we 
 *             find the smallest subscript rv >= min such 
 *
 *                splj <= list[rv];
 */
int Bin_lin_srch(int list[], int splj, int min, int prev) {
   int mid, max = prev - 1;

   while (min <= max) {
      mid = (min + max)/2;
      if (list[mid] < splj) {
         /* Linear search */
         int rv = prev;
         while (rv > min && list[rv-1] >= splj) 
            rv--;
         return rv;
      } else /* list[mid] >= splj */ {
         prev = mid;
         max = mid-1;
      }
   }

   /* Can't get here: we know that list[prev] satisfies
    *
    *    splj <= list[prev] < spljp1 
    */
   return min;
}  /* Bin_lin_srch */

/*---------------------------------------------------------------------
 * Function:   Count_elts_going_to_procs
 * Purpose:    Count the number of elements going from this process
 *             to every process.  This version uses binary search
 *             to determine which bucket each element belongs to
 *
 * Note: The buckets are
 *        bucket
 *           0:   MINUS_INFTY = splitters[0] <= x < splitters[1]
 *           1:   splitters[1] <= x < splitters[2]
 *           ...
 *           i:   splitters[i] <= x < splitters[i+1]
 *           ...
 *           p-1: splitters[p-1] <= x < splitters[p] = INFTY
 * 
 */
void Count_elts_going_to_procs(int loc_list[], int loc_n, int splitters[], 
      int p, int my_to_counts[]) {
   int bkt, sub, min, max;
   int bkt_starts[p+1];
   
   min = 0;
   max = loc_n-1;
   for (bkt = 0; bkt < p; bkt++) {
      sub = Bin_srch(loc_list, splitters[bkt], splitters[bkt+1], min, max);
      bkt_starts[bkt] = sub;
      min = sub;
   }
   bkt_starts[p] = loc_n;

   for (bkt = 0; bkt < p; bkt++) 
      my_to_counts[bkt] = bkt_starts[bkt+1] - bkt_starts[bkt];
}  /* Count_elts_going_to_procs */



/*---------------------------------------------------------------------
 * Function:   Excl_prefix_sums
 * Purpose:    Find exclusive prefix sums of the elements in array in.
 *             Store the result in array out.
 */
void Excl_prefix_sums(int in[], int out[], int m) {
   int i;

   out[0] = 0;
   for (i = 1; i < m; i++)
      out[i] = out[i-1] + in[i-1];
}  /* Excl_prefix_sums */


/*---------------------------------------------------------------------
 * Function:   Check_sorted
 * Purpose:    Determine whether list1 is sorted correctly by comparing 
 *             it to list2, which is known to be sorted correctly.
 */
int  Check_sorted(int list1[], int list2[], int n) {
   int i, sorted = 1;

   for (i = 0; i < n; i++)
      if (list1[i] != list2[i]) {
         printf("list1[%d] = %d != %d = list2[%d]\n", i, list1[i],
               list2[i], i);
         sorted = 0;
      }
   return sorted;
}  /* Check_sorted */


/*-------------------------------------------------------------------
 * Function:         Butterfly
 * Purpose:          Sort the global list using a butterfly communication
 *                   scheme and the splitters to determine which keys to
 *                   send.  Local lists are sorted.
 * In args:          splitters, my_rank, p, comm, n
 * In arg/scratch:   loc_list
 * Out arg/scratch:  rcv_buf
 * In/out/scratch:   loc_n_p
 *
 * Notes:       
 * 1. Which splitter to use is determined as follows:
 *
 *             0   1   2   3   4   5   6   7
 *     Pass 0: 4   4   4   4   4   4   4   4   = p/2
 *             100 100 100 100 100 100 100 100 = p/2
 *     Pass 1: 2   2   2   2   6   6   6   6   = p/2 +/- p/4
 *             010 010 010 010 110 110 110 110
 *     Pass 2: 1   1   3   3   5   5   7   7   = p/2 +/- p/4 +/- p/8
 *             001 001 011 011 101 101 111 111
 * 
 * 2. Array sizes are all n.
 * 3. loc_list is freed here
 */
void Butterfly(int loc_list[], int* loc_n_p, int splitters[],
      int rcv_buf[], int n,
      int my_rank, int p, MPI_Comm comm) {
   // rcv_buf is used for exchanged data
   // tmp_buf is used for merged data
   int rcv_buf_sz = 0;
   int tmp_buf[n];
   int **llp = &loc_list;
   int **tbp = &tmp_buf;

// if (my_rank == 0) {
//    printf("Proc %d > loc_list = %p, tmp_buf = %p\n", 
//          my_rank, loc_list, tmp_buf);
//    printf("Proc %d >     *llp = %p,    *tbp = %p\n", 
//          my_rank, *llp, *tbp);
// }

   // offset specifies where in loc_list to start send
   // count is the amount of data in loc_list to send
   int offset, count, new_loc_n, partner;
   unsigned which_splitter = p >> 1, flip_bit = p >> 1, bitmask = p >> 1;
#  ifdef DEBUG
   char title[100];
   printf("Proc %d > loc_list = %p, rcv_buf = %p, tmp_buf = %p\n",
         my_rank, *llp, rcv_buf, *tbp);
   fflush(stdout);
   MPI_Barrier(comm);
#  endif

   while (bitmask >= 1) {
      partner = my_rank ^ bitmask;
#     ifdef DEBUG
      printf("Proc %d > partner = %d, splitter = %d, bitmask = %d\n", 
            my_rank, partner, splitters[which_splitter],
            bitmask);
#     endif
      if (my_rank < partner) {
         // Determine which data should be sent to partner
         //   Note that this is a local operation:  it does
         //   no communication
         Send_upper_args(*llp, *loc_n_p, &offset, &count, 
               splitters[which_splitter]);

         // How many ints will be in loc_list after Send_recv
         new_loc_n = offset;
         flip_bit >>= 1;
         which_splitter -= flip_bit;  // For next time
      } else {
         // Determine which data should be sent to partner
         //   Note that this is a local operation:  it does 
         //   no communication
         Send_lower_args(*llp, *loc_n_p, &count, 
               splitters[which_splitter]);

         // How many ints will be in loc_list after Send_recv
         new_loc_n = *loc_n_p - count;
         offset = 0;
         flip_bit >>= 1;
         which_splitter += flip_bit;  // For next time
      }

#     ifdef DEBUG
      printf("Proc %d > Before 'before Send_recv' call to Print_loc_lists\n",
            my_rank);
      printf("Proc %d > offset = %d, count = %d\n", my_rank, offset, count);
      fflush(stdout);
      sprintf(title, "Before Send_recv, bitmask = %d, loc_list = ", bitmask);
      Print_loc_lists(my_rank, p, comm, title, *llp, *loc_n_p);
//    printf("Proc %d > After 'before Send_recv' call to Print_loc_lists\n",
//          my_rank);
      fflush(stdout);
#     endif
      // Exchange data with partner:  rcv_buf and rcv_buf_sz
      // will be updated by the Send_recv function.
      Send_recv(*llp + offset, count, rcv_buf, &rcv_buf_sz, n,
            partner, comm);

#     ifdef DEBUG
      printf("Proc %d > Completed Send_recv, bitmask = %d\n",
            my_rank, bitmask);
      printf("Proc %d > rcv_buf_sz = %d\n", my_rank, rcv_buf_sz);
      sprintf(title, "After Send_recv, bitmask = %d, rcv_buf =", bitmask);
      Print_loc_lists(my_rank, p, comm, title, rcv_buf, rcv_buf_sz);
#     endif

      *loc_n_p = new_loc_n; 
      if (my_rank < partner) {
         offset = 0;
      } else {
         offset = count;
      }

      // Merge the received data into local storage
      Merge(llp, loc_n_p, offset, rcv_buf, rcv_buf_sz, 
            tbp);

#     ifdef DEBUG
      sprintf(title, "After merge, bitmask = %d, loc_list =", bitmask);
      Print_loc_lists(my_rank, p, comm, title, *llp, *loc_n_p);
#     endif

      bitmask >>= 1;
   }

#  ifdef DEBUG
   printf("Proc %d > loc_list = %p, rcv_buf = %p, tmp_buf = %p\n",
         my_rank, *llp, rcv_buf, *tbp);
   fflush(stdout);
   MPI_Barrier(comm);
#  endif
   memcpy(rcv_buf, *llp, *loc_n_p*sizeof(int));

// if (my_rank == 0) {
//    printf("Proc %d > loc_list = %p, tmp_buf = %p\n", 
//          my_rank, loc_list, tmp_buf);
//    printf("Proc %d >     *llp = %p,    *tbp = %p\n", 
//          my_rank, *llp, *tbp);
//    fflush(stdout);
// }
// free(tmp_buf);
}  /* Butterfly */


/*-------------------------------------------------------------------
 * Function:  Send_upper_args
 * Purpose:   Determine which data in loc_list should be sent
 *            to partner.  
 * In args:   loc_list:  local list
 *            loc_n:  number of elements in local list
 *            splitter:  cutoff:  if loc_list[i] >= splitter,
 *               loc_list[i] should be sent to partner
 * Out args:  offset_p: if i >= *offset_p, loc_list[i] should
 *               be sent to partner
 *            count_p:  number of elements of loc_list that 
 *               should be sent:  loc_n - *offset_p;
 *
 * Notes:      
 * 1.  loc_list is sorted.  So this could be done with a binary
 *     search.
 */
void Send_upper_args(int loc_list[], int loc_n, int* offset_p, 
      int* count_p, double splitter) {
   int i;

   for (i = 0; i < loc_n; i++) 
      if (loc_list[i] >= splitter) {
         *offset_p = i;
         *count_p = loc_n - i;
         return;
      }

   // Not sending anything
   *offset_p = loc_n;
   *count_p = 0;
}  /* Send_upper_args */


/*-------------------------------------------------------------------
 * Function:  Send_lower_args
 * Purpose:   Determine which data in loc_list should be sent
 *            to partner.  
 * In args:   loc_list:  local list
 *            loc_n:  number of elements in local list
 *            splitter:  cutoff:  if loc_list[i] < splitter,
 *               loc_list[i] should be sent to partner
 * Out arg:   count_p:  number of elements of loc_list that 
 *               should be sent
 *
 * Notes:      
 * 1.  loc_list is sorted.  So this could be done with a binary
 *     search.
 */
void Send_lower_args(int loc_list[], int loc_n, int* count_p, 
      double splitter) {
   int i;

   for (i = loc_n-1; i >= 0; i--) {
      if (loc_list[i] < splitter) {
         *count_p = i + 1;
         return;
      }
   }

   // Not sending anything
   *count_p = 0;
}  /* Send_lower_args */


/*-------------------------------------------------------------------
 * Function:     Send_recv
 * Purpose:      Send block of loc_list to partner, receive block from
 *               partner.
 * In args:      partner, comm
 *               loc_list:  block being sent
 *               count:  size of block being sent
 *               n:  size of rcv_buf
 * Out arg:      rcv_buf:  storage for received block
 *               rcv_buf_sz_p:  size of rcv_buf 
 *
 * Note:         The address in loc_list will, in general, be
 *               an offset into loc_list in the caller.
 */
void Send_recv(int loc_list[], int count, int rcv_buf[], 
      int* rcv_buf_sz_p, int n, int partner, MPI_Comm comm) {
   MPI_Status status;
   MPI_Request req;

   MPI_Isend(loc_list, count, MPI_INT, partner, 0, comm, &req);

   // Block until message is available
   MPI_Recv(rcv_buf, n, MPI_INT, partner, 0, comm, &status);

   // Wait on Isend
   MPI_Wait(&req, MPI_STATUS_IGNORE);
   MPI_Get_count(&status, MPI_INT, rcv_buf_sz_p);

}  /* Send_recv */


/*-------------------------------------------------------------------
 * Function:    Merge
 * Purpose:     Merge the contents of loc_list (starting at 
 *              loc_list + offset) and the contents of rcv_buf
 * In args:     loc_n_p:  current number of elements in loc_list
 *              offset
 *              rcv_buf
 *              rcv_buf_sz
 * In/out arg:  loc_list_p
 *              loc_n_p
 * Scratch:     tmp_buf_p
 *
 * Note:        loc_list and tmp_buf should both be large enough
 *              to store the contents of loc_list and rcv_buf
 */
void Merge(int** loc_list_p, int* loc_n_p, int offset, 
      int rcv_buf[], int rcv_buf_sz, int** tmp_buf_p) {
   int lli = 0, ri = 0, ti = 0;
   int* loc_list = *loc_list_p + offset;
   int loc_n = *loc_n_p;
   int *tmp_buf = *tmp_buf_p;
   int *t_p;
   int tmp_buf_sz = loc_n + rcv_buf_sz;
#  ifdef DEBUG
   int my_rank, p;
   MPI_Comm comm = MPI_COMM_WORLD; 
   MPI_Comm_rank(comm, &my_rank);
   MPI_Comm_size(comm, &p);
   printf("Proc %d > In Merge loc_n = %d, rcv_buf_sz = %d, tmp_buf_sz = %d\n",
         my_rank, loc_n, rcv_buf_sz, tmp_buf_sz);
   fflush(stdout);
   MPI_Barrier(comm);
   Print_loc_lists(my_rank, p, comm, "In Merge, loc_list =", loc_list, loc_n);
   fflush(stdout);
   MPI_Barrier(comm);
   Print_loc_lists(my_rank, p, comm, "In Merge, rcv_buf =", rcv_buf,
         rcv_buf_sz);
   fflush(stdout);
   MPI_Barrier(comm);
#  endif

   while (lli < loc_n && ri < rcv_buf_sz) 
      if (loc_list[lli] < rcv_buf[ri]) 
         tmp_buf[ti++] = loc_list[lli++];
      else
         tmp_buf[ti++] = rcv_buf[ri++];

   if (ri < rcv_buf_sz)
      for ( ; ri < rcv_buf_sz; ri++)
         tmp_buf[ti++] = rcv_buf[ri];
   else
      for ( ; lli < loc_n; lli++)
         tmp_buf[ti++] = loc_list[lli];

#  ifdef DEBUG
   Print_loc_lists(my_rank, p, comm, "After merge", tmp_buf, tmp_buf_sz);
#  endif

   t_p = *loc_list_p;
   *loc_list_p = *tmp_buf_p;
   *tmp_buf_p = t_p;

   *loc_n_p = tmp_buf_sz;
}  /* Merge */
