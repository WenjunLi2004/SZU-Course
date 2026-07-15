/* File:     mpi_oe.h
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
 * Note: The arrays referred to by local_A_p and local_B_p
 *    should be allocated on the heap.
 *
 * Note: Optional -DDEBUG compile flag for verbose output
 */
#ifndef __MPI_OE_H__
#define __MPI_OE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

/* Local functions: no communication */
void Merge_split_low(int** local_A_p, int temp_B[], int** temp_C_p, 
         int local_n);
void Merge_split_high(int** local_A_p, int temp_B[], int** temp_C_p, 
        int local_n);
int  OE_compare(const void* a_p, const void* b_p);
void OE_print_list(int local_A[], int local_n, int rank);
void Debug_print_list(char* title, int list[], int local_n, int my_rank);

/* Functions involving communication */
void OE_sort(int** local_A_p, int* temp_B, int** temp_C_p, int local_n, 
         int my_rank, int p, MPI_Comm comm);
void Odd_even_iter(int** local_A_p, int* temp_B, int** temp_C_p,
         int local_n, int phase, int even_partner, int odd_partner,
         int my_rank, int p, MPI_Comm comm);
void OE_print_local_lists(char* title, int local_A[], int local_n, 
         int my_rank, int p, MPI_Comm comm);
void OE_Print_global_list(int local_A[], int local_n, int my_rank,
         int p, MPI_Comm comm);

#endif
