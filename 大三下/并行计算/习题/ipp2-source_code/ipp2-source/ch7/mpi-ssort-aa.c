/* File:      mpi-ssort-aa.c
 *               - aa = alltoall
 * Purpose:   Implement sample sort of a list of ints using MPI and 
 *               - random number generator to generate sample, 
 *               - process 0 to sort sample and choose splitters,
 *               - MPI_Alltoallv to redistribute list
 *
 * Compile:   mpicc -g -Wall -o mpi-ssort-aa mpi-ssort-aa.c stats.c
 *            note:  stats.h header file should be in current directory
 * Run:       mpiexec -n <p> ./mpi-ssort-aa <n> <s> [mod]
 *               p:  number of MPI processes
 *               n:  number of elements in the list
 *               s:  size of the sample
 *             mod:  if present, modulus for calls to random()
 *                   used in generating the list
 *            note:  p should evenly divide both n and s, and
 *                   s should evenly divide n.
 *
 * Input:    If mod is on the command line, none.
 *           Otherwise, the n elements of the list
 * Output:   Run-time of the sample sort and run time of qsort
 *           on process 0.  Whether list was correctly sorted.
 *           If PRINT_LIST is defined, initial list and sorted
 *              list
 *           If DEBUG is defined, verbose output on progress of
 *              program.
 * Note:     ITERS is a macro used to specify the number of times
 *           the program will be run.  Currently ITERS = 1 if
 *           DEBUG is set.  Otherwise ITERS = 10.
 *
 * IPP2:   7.2.8 (pp. 418 and ff.)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>


/* stats.c defines variables
 *   s_dmin, s_dmax, s_dtotal  // MPI runtimes 
 *   s_hmin, s_hmax, s_htotal  // Qsort runtimes
 * and functions
 *   Setup_stats (not used), Update_stats and Print_stats
 */
#include "stats.h"


#ifdef DEBUG
#define ITERS 1
#else
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
void Count_elts_going_to_procs(int loc_list[], int loc_n, int splitters[], 
      int p, int my_to_counts[]);
void Excl_prefix_sums(int in[], int out[], int m);


/*--------------------Functions Involving Communication---------------*/
void Get_args(int argc, char* argv[], int my_rank, int p, MPI_Comm comm,
      int* n_p, int* s_p, int* mod_p);
void Ssort(int my_rank, int p, MPI_Comm comm, int tlist[], int n, 
      int loc_list[], int loc_n, int s);
void Print_loc_lists(int my_rank, int p, MPI_Comm comm, 
      const char title_in[], int list[], int n);
void Get_list(int my_rank, int p, MPI_Comm comm, int list[], int n, int mod,
      int loc_list[], int loc_n);


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

   Get_args(argc, argv, my_rank, p, comm, &n, &s, &mod);
   if (my_rank == 0) {
      list = malloc(n*sizeof(int));
      tlist2 = malloc(n*sizeof(int));
   }
   tlist1 = malloc(n*sizeof(int));
   loc_n = n/p;
   loc_list = malloc(loc_n*sizeof(int));

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
   free(loc_list);

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
 *               - process 0 to sort sample and choose splitters,
 *               - MPI_Alltoall to redistribute list
 * In args:    my_rank:  calling process' rank in communicator
 *             p:  number of processes in communicator
 *             comm:  communicator used by all processes
 *             n:  size of global list
 *             loc_n:  number of elements in calling process'
 *                 sublist
 *             s:  size of sample
 *
 * In/out:     loc_list:  calling process' sublist
 *            
 * Scratch/out:tlist: temporary storage on each process.
 *             Can store up to n elements.  Sorted global 
 *             list on process 0 on return
 */
void Ssort(int my_rank, int p, MPI_Comm comm, int tlist[], int n, 
      int loc_list[], int loc_n, int s) {
   int *loc_samp, loc_s = s/p;
   int splitters[p+1];
   int *sample = tlist;
   int my_to_counts[p];
   int my_to_offsets[p];
   int my_fr_counts[p];
   int my_fr_offsets[p];
   int bkt_counts[p];
   int bkt_offsets[p];
   int my_new_count;

#  ifdef DEBUG
   Print_loc_lists(my_rank, p, comm, "Start of Ssort", loc_list, loc_n);
#  endif
   
   /* Generate the sample and find the splitters */
   loc_samp = malloc(loc_s*sizeof(int));
   Gen_sample(my_rank, loc_list, loc_n, loc_samp, loc_s);
   MPI_Gather(loc_samp, loc_s, MPI_INT, sample, loc_s, MPI_INT,
         0, comm);
   if (my_rank == 0) {
      Find_splitters(sample, s, splitters, p);
#     ifdef DEBUG
      Print_list("Sample", sample, s);
      Print_list("Splitters", splitters, p+1);
#     endif
   }
   free(loc_samp);
   MPI_Bcast(splitters, p+1, MPI_INT, 0, comm);

   /* Determine counts of elements going to/from processes */
   qsort(loc_list, loc_n, sizeof(int), Compare);
   Count_elts_going_to_procs(loc_list, loc_n, splitters, p, 
         my_to_counts);
   MPI_Alltoall(my_to_counts, 1, MPI_INT, my_fr_counts, 1, MPI_INT, comm);

   /* Find offset of first element to/from each process */
   Excl_prefix_sums(my_to_counts, my_to_offsets, p);
   Excl_prefix_sums(my_fr_counts, my_fr_offsets, p);

   /* Send each process the elements it should receive */
   MPI_Alltoallv(loc_list, my_to_counts, my_to_offsets, MPI_INT,
         tlist, my_fr_counts, my_fr_offsets, MPI_INT, comm);
   my_new_count = my_fr_offsets[p-1] + my_fr_counts[p-1];

   /* Sort the new local list */
   qsort(tlist, my_new_count, sizeof(int), Compare);

   /* Gather buckets to Process 0 */
   MPI_Gather(&my_new_count, 1, MPI_INT, bkt_counts, 1, MPI_INT, 
         0, comm);
   if (my_rank == 0)
      Excl_prefix_sums(bkt_counts, bkt_offsets, p);
   if (my_rank == 0)
      MPI_Gatherv(MPI_IN_PLACE, my_new_count, MPI_INT, tlist, bkt_counts,
            bkt_offsets, MPI_INT, 0, comm);
   else
      MPI_Gatherv(tlist, my_new_count, MPI_INT, tlist, bkt_counts,
            bkt_offsets, MPI_INT, 0, comm);
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
 * Function:   Count_elts_going_to_procs
 * Purpose:    Count the number of elements going from this process
 *             to every process.  
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
   int dest, elt;

   memset(my_to_counts, 0, p*sizeof(int));
   for (dest = 1, elt = 0; dest <= p && elt < loc_n; dest++)
      while (elt < loc_n && loc_list[elt] < splitters[dest]) {
         my_to_counts[dest-1]++;
         elt++;
      }
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
