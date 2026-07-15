/* File:     mpi_oe.c
 * Purpose:  Implements an odd-even transposition sort of a list of
 *           ints using MPI.  It can be linked into an existing MPI 
 *           program.
 *
 * Requirements:
 *    p = number of MPI processes
 *    n = number of elements in global 
 *    p must evenly divide n
 *    MPI program should call OE_sort with the following arguments:
 *
 *       - int** local_A_p = pointer to array containing calling
 *            process' sublist of n/p ints
 *       - int* temp_B = scratch storage containing enough space
 *            for n/p ints
 *       - int** temp_C_p = pointer to scratch storage containing
 *            space for n/p ints.
 *       - int local_n = n/p 
 *       - int my_rank = rank of calling process in communicator
 *            comm
 *       - int p = number of processes in communicator comm 
 *       - MPI_Comm comm = communicator containing the p
 *            processes
 *
 * Note:  The arrays referred to by local_A_p and local_B_p
 *    should be allocated on the heap.
 *
 * Note: Optional -DDEBUG compile flag for verbose output
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "mpi-oe.h"

/* Local functions */
void OE_print_list(int local_A[], int local_n, int rank);
void Merge_split_low(int** local_A_p, int temp_B[], int** temp_C_p, 
         int local_n);
void Merge_split_high(int** local_A_p, int temp_B[], int** temp_C_p, 
        int local_n);
int  OE_compare(const void* a_p, const void* b_p);
void Debug_print_list(char* title, int list[], int local_n, int my_rank);

/* Functions involving communication */
void OE_sort(int** local_A_p, int* temp_B, int** temp_C_p, int local_n, 
         int my_rank, int p, MPI_Comm comm);
void Odd_even_iter(int** local_A_p, int* temp_B, int** temp_C_p,
         int local_n, int phase, int even_partner, int odd_partner,
         int my_rank, int p, MPI_Comm comm);
void OE_print_local_lists(char* title, int local_A[], int local_n, 
         int my_rank, int p, MPI_Comm comm);
void OE_print_global_list(int local_A[], int local_n, int my_rank,
         int p, MPI_Comm comm);


/*-------------------------------------------------------------------
 * Function:   OE_print_global_list
 * Purpose:    OE_print the contents of the global list A
 * Input args:  
 *    n, the number of elements 
 *    A, the list
 * Note:       Purely local, called only by process 0
 */
void OE_print_global_list(int local_A[], int local_n, int my_rank, int p, 
      MPI_Comm comm) {
   int* A = NULL;
   int i, n;

   if (my_rank == 0) {
      n = p*local_n;
      A = (int*) malloc(n*sizeof(int));
      MPI_Gather(local_A, local_n, MPI_INT, A, local_n, MPI_INT, 0,
            comm);
      printf("Global list:\n");
      for (i = 0; i < n; i++)
         printf("%d ", A[i]);
      printf("\n\n");
      free(A);
   } else {
      MPI_Gather(local_A, local_n, MPI_INT, A, local_n, MPI_INT, 0,
            comm);
   }

}  /* OE_print_global_list */


/*-------------------------------------------------------------------
 * Function:    OE_compare
 * Purpose:     Compare 2 ints, return -1, 0, or 1, respectively, when
 *              the first int is less than, equal, or greater than
 *              the second.  Used by qsort.
 */
int OE_compare(const void* a_p, const void* b_p) {
   int a = *((int*)a_p);
   int b = *((int*)b_p);

   if (a < b)
      return -1;
   else if (a == b)
      return 0;
   else /* a > b */
      return 1;
}  /* OE_compare */


/*-------------------------------------------------------------------
 * Function:    OE_sort
 * Purpose:     Sort local list, use odd-even sort to sort
 *              global list.
 * Input args:  local_n, my_rank, p, comm
 * In/out args: local_A 
 */
void OE_sort(int** local_A_p, int* temp_B, int** temp_C_p, int local_n, 
      int my_rank, int p, MPI_Comm comm) {
   int phase;
   int even_partner;  /* phase is even or left-looking */
   int odd_partner;   /* phase is odd or right-looking */
#  ifdef DEBUG
   char title[100];
#  endif

   /* Find partners:  negative rank => do nothing during phase */
   if (my_rank % 2 != 0) {
      even_partner = my_rank - 1;
      odd_partner = my_rank + 1;
      if (odd_partner == p) odd_partner = -1;  // Idle during odd phase
   } else {
      even_partner = my_rank + 1;
      if (even_partner == p) even_partner = -1;  // Idle during even phase
      odd_partner = my_rank-1;  
   }

   /* Sort local list using built-in quick sort */
   qsort(*local_A_p, local_n, sizeof(int), OE_compare);

#  ifdef DEBUG
   printf("Proc %d > before loop in sort\n", my_rank);
   fflush(stdout);
#  endif

   for (phase = 0; phase < p; phase++) {
      Odd_even_iter(local_A_p, temp_B, temp_C_p, local_n, phase, 
             even_partner, odd_partner, my_rank, p, comm);
#     ifdef DEBUG
      sprintf(title, "After phase %d", phase);
      OE_print_local_lists(title, *local_A_p, local_n, my_rank, p, comm);
      fflush(stdout);
#     endif
   }

}  /* Sort */


/*-------------------------------------------------------------------
 * Function:    Odd_even_iter
 * Purpose:     One iteration of Odd-even transposition sort
 * In args:     local_n, phase, my_rank, p, comm
 * In/out args: local_A
 * Scratch:     temp_B, temp_C
 */
void Odd_even_iter(int** local_A_p, int* temp_B, int** temp_C_p,
        int local_n, int phase, int even_partner, int odd_partner,
        int my_rank, int p, MPI_Comm comm) {
   MPI_Status status;
   int* local_A = *local_A_p;

#  ifdef DEBUG
   char title[100];
   printf("Proc %d > phase = %d, even_partner = %d, odd_partner = %d\n",
         my_rank, phase, even_partner, odd_partner);
   printf("Proc %d > phase = %d, local_n = %d\n", my_rank, phase, local_n);
#  endif

   if (phase % 2 == 0) {
      if (even_partner >= 0) {
#        ifdef DEBUG
         printf("Proc %d > phase = %d, exchanging with %d\n",
               my_rank, phase, even_partner);
#        endif
         MPI_Sendrecv(local_A, local_n, MPI_INT, even_partner, 0, 
            temp_B, local_n, MPI_INT, even_partner, 0, comm,
            &status);
#        ifdef DEBUG
         sprintf(title, "phase %d, received", phase);
         Debug_print_list(title, temp_B, local_n, my_rank);
#        endif
         if (my_rank % 2 != 0)
            Merge_split_high(local_A_p, temp_B, temp_C_p, local_n);
         else
            Merge_split_low(local_A_p, temp_B, temp_C_p, local_n);
#        ifdef DEBUG
         sprintf(title, "phase %d, kept", phase);
         Debug_print_list(title, local_A, local_n, my_rank);
#        endif
      }
   } else { /* odd phase */
      if (odd_partner >= 0) {
#        ifdef DEBUG
         printf("Proc %d > phase = %d, exchanging with %d\n",
               my_rank, phase, even_partner);
#        endif
         MPI_Sendrecv(local_A, local_n, MPI_INT, odd_partner, 0, 
            temp_B, local_n, MPI_INT, odd_partner, 0, comm,
            &status);
#        ifdef DEBUG
         sprintf(title, "phase %d, received", phase);
         Debug_print_list(title, temp_B, local_n, my_rank);
#        endif
         if (my_rank % 2 != 0)
            Merge_split_low(local_A_p, temp_B, temp_C_p, local_n);
         else
            Merge_split_high(local_A_p, temp_B, temp_C_p, local_n);
#        ifdef DEBUG
         sprintf(title, "phase %d, kept", phase);
         Debug_print_list(title, local_A, local_n, my_rank);
#        endif
      }
   }
}  /* Odd_even_iter */


/*-------------------------------------------------------------------
 * Function:    Merge_split_low
 * Purpose:     Merge the smallest local_n elements in local_A 
 *              and temp_B into temp_C.  Then swap temp_C_p
 *              and local_A_p.
 * In args:     local_n, temp_B
 * In/out args: local_A_p
 * Scratch:     temp_C
 */
void Merge_split_low(int** local_A_p, int temp_B[], int** temp_C_p, 
        int local_n) {
   int ai, bi, ci;
   int* temp;
   int* local_A = *local_A_p;
   int* temp_C = *temp_C_p;
   
   ai = bi = ci = 0;
   while (ci < local_n) {
      if (local_A[ai] <= temp_B[bi]) {
         temp_C[ci] = local_A[ai];
         ci++; ai++;
      } else {
         temp_C[ci] = temp_B[bi];
         ci++; bi++;
      }
   }

   temp = *local_A_p;
   *local_A_p = *temp_C_p;
   *temp_C_p = temp;
}  /* Merge_split_low */


/*-------------------------------------------------------------------
 * Function:    Merge_split_high
 * Purpose:     Merge the largest local_n elements in local_A 
 *              and temp_B into temp_C.  Then swap temp_C_p
 *              and local_A_p.
 * In args:     local_n, temp_B
 * In/out args: local_A_p
 * Scratch:     temp_C
 */
void Merge_split_high(int** local_A_p, int temp_B[], int** temp_C_p, 
        int local_n) {
   int ai, bi, ci;
   int* temp;
   int* local_A = *local_A_p;
   int* temp_C = *temp_C_p;
   
   ai = bi = ci = local_n-1;
   while (ci >= 0) {
      if (local_A[ai] >= temp_B[bi]) {
         temp_C[ci] = local_A[ai];
         ci--; ai--;
      } else {
         temp_C[ci] = temp_B[bi];
         ci--; bi--;
      }
   }

   temp = *local_A_p;
   *local_A_p = *temp_C_p;
   *temp_C_p = temp;

}  /* Merge_split_high */


/*-------------------------------------------------------------------
 * Only called by process 0
 */
void OE_print_list(int local_A[], int local_n, int rank) {
   int i;
   printf("%d: ", rank);
   for (i = 0; i < local_n; i++)
      printf("%d ", local_A[i]);
   printf("\n");
}  /* OE_print_list */


/*-------------------------------------------------------------------
 * Function:   OE_print_local_lists
 * Purpose:    OE_print each process' current list contents
 * Input args: all
 * Notes:
 * 1.  Assumes all participating processes are contributing local_n 
 *     elements
 */
void OE_print_local_lists(char* title, int local_A[], int local_n, 
         int my_rank, int p, MPI_Comm comm) {
   int*       A;
   int        q;
   MPI_Status status;

   if (my_rank == 0) {
      printf("%s\n", title);
      A = (int*) malloc(local_n*sizeof(int));
      OE_print_list(local_A, local_n, my_rank);
      for (q = 1; q < p; q++) {
         MPI_Recv(A, local_n, MPI_INT, q, 0, comm, &status);
         OE_print_list(A, local_n, q);
      }
      printf("\n");
      free(A);
   } else {
      MPI_Send(local_A, local_n, MPI_INT, 0, 0, comm);
   }
}  /* OE_print_local_lists */


/*-------------------------------------------------------------------
 * Function:  Debug_print_list
 * Purpose:   When multiple processes are accessing stdout, output of
 *            a string is less likely to be interrupted than output of
 *            an array of ints.  So store array of ints as a string
 *            and print the string
 */
void Debug_print_list(char* title, int list[], int local_n, int my_rank) {
   char char_list[10000];
   char* p = char_list;
   int i;

   for (i = 0; i < local_n; i++) {
      sprintf(p, "%d ", list[i]);
      p = char_list + strlen(char_list);
   }

   printf("Proc %d > %s %s\n", my_rank, title, char_list);
}  /* Debug_print */
