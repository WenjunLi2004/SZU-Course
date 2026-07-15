/* File:    serial_bitonic1.c
 *
 * Purpose: Implement serial bitonic sort so that each element of
 *          the list can "decide what to do" on the basis of
 *             - it's subscript
 *             - the current length of the sublists
 *          Note that n should be a power of 2.
 *
 * Compile: gcc -g -Wall -o sb1 serial_bitonic1.c
 * Usage:   ./sb1 <n> [mod]
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
 * 1.  In order to see input and output lists, PRINT_LIST 
 *     at compile time
 * 2.  Very verbose output is enables with the compiler macro
 *     DEBUG
 */
#include <stdio.h>
#include <stdlib.h>
#include "timer.h"

#define INC 0
#define DEC 1
#define SWAP(x,y) {int z = x; x = y; y = z;}
#define MAX_TITLE 1000
#define ALL_ONES (~0)
#define U_SZ (8*sizeof(unsigned))


void     Usage(char* prog_name);
void     Get_args(int argc, char* argv[], int* n_p, int* mod_p);
void     Generate_list(int a[], int n, int mod);
void     Print_list(int a[], int n, char* title);
void     Read_list(int a[], int n);

unsigned Get_width(unsigned val);
void     Print_unsigned(unsigned val, unsigned field_width);
unsigned Insert_zero(unsigned val, unsigned j);
void     Compare_swap(int a[], unsigned elt, unsigned partner,
               unsigned inc_dec);
unsigned Which_bit(unsigned part_mask);
unsigned Log_2(unsigned x);
void     Get_elts(unsigned th, unsigned stage, unsigned which_bit, 
               unsigned* my_elt1_p, unsigned* my_elt2_p);
void     Bitonic_sort(int a[], int n);
void     Swap(int* x_p, int* y_p);
int      Check_sort(int a[], int n);

/*-----------------------------------------------------------------*/
int main(int argc, char* argv[]) {
   int  n, mod;
   int *a;
   double start, finish;

   Get_args(argc, argv, &n, &mod);
   a = (int*) malloc(n*sizeof(int));
   if (mod != 0)
      Generate_list(a, n, mod);
   else
      Read_list(a, n);

#  ifdef PRINT_LIST
   printf("n = %d, mod = %d\n", n, mod);
   Print_list(a, n, "Before sort");
#  endif

   GET_TIME(start);
   Bitonic_sort(a, n);
   GET_TIME(finish);
   printf("Elapsed time to sort %d ints using bitonic sort = %e seconds\n", 
         n, finish-start);

#  ifdef PRINT_LIST
   Print_list(a, n, "After sort");
#  endif

   if (Check_sort(a, n) != 0) 
      printf("List is sorted\n");
   else
      printf("List is not sorted\n");
   
   free(a);
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
void Print_list(int a[], int n, char* title) {
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
   char bin_str[field_width+1];

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
 *                 inc_dec = DEC => pair should decrease
 *             
 * In args:     elt, partner:  subscripts of elements of a
 *                 elt should always be < partner
 *              inc_dec:   whether pair should increase (0)
 *                 or decrease (1)
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
   } else {  /* inc_dec == DEC */
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
 * Function:     Log_2
 * Purpose:      Find floor(log_2(x))
 * In arg:       x
 * Ret val:      floor(log_2(x))
 *
 */
unsigned Log_2(unsigned x) {
   unsigned rv = 0;
   while (x > 1) {
      x = x >> 1;
      rv++;
   }
   return rv;
}  /* Log_2 */


/*-----------------------------------------------------------------
 * Function:    Get_elts
 * Purpose:     Given a ``thread rank'' th, and which_bit should
 *              be inserted, determine the subscripts of the two
 *              elements that this thread should compare-swap
 * In args:     th, stage, which_bit
 * Out args:    my_elt1_p, my_elt2_p
 */
void Get_elts(unsigned th, unsigned stage, unsigned which_bit, 
      unsigned* my_elt1_p, unsigned* my_elt2_p) {
   *my_elt1_p = Insert_zero(th, which_bit);
   *my_elt2_p = *my_elt1_p ^ stage;
}  /* Get_elts */

/*-----------------------------------------------------------------
 * Function:     Bitonic_sort
 * Purpose:      Sort list of n elements using bitonic sort
 * In args:      n
 * In/out args:  a
 *
 * Note:         n should be a power of 2
 */
void Bitonic_sort(int a[], int n) {
   unsigned bf_sz, stage, my_elt1, my_elt2, which_bit, th;
#  ifdef DEBUG
   char title[MAX_TITLE];
#  endif

   for (bf_sz = 2; bf_sz <= n; bf_sz = bf_sz << 1) {
#     ifdef DEBUG
      printf("bf_sz = %u\n", bf_sz);
#     endif 
      for (stage = bf_sz >> 1; stage > 0; stage = stage >> 1) {
         which_bit = Which_bit(stage);
#        ifdef DEBUG
         printf("   stage = %u, which_bit = %u\n", stage, which_bit);
#        endif 
         for (th = 0; th < n/2; th++) {
            Get_elts(th, stage, which_bit, &my_elt1, &my_elt2);
#           ifdef DEBUG
            printf("      th = %u = ", th);
            Print_unsigned(th, Get_width(n)); 
            printf(", my_elt1 = %u = ", my_elt1);
            Print_unsigned(my_elt1, 1 + Get_width(n)); 
            printf(", my_elt2 = %u = ", my_elt2);
            Print_unsigned(my_elt2, 1 + Get_width(n)); 
            printf("\n");
            printf("      Before swap: my_elt1 = %u, a[%u] = %d, my_elt2 = %u, a[%u] = %d\n", 
                                my_elt1, my_elt1, a[my_elt1], 
                                my_elt2, my_elt2, a[my_elt2]);
#           endif 
            /* my_elt1 & bf_sz = INC or DEC */
            Compare_swap(a, my_elt1, my_elt2, my_elt1 & bf_sz);
#           ifdef DEBUG
            printf("      After swap:  my_elt1 = %u, a[%u] = %d, my_elt2 = %u, a[%u] = %d\n", 
                                my_elt1, my_elt1, a[my_elt1], 
                                my_elt2, my_elt2, a[my_elt2]);
#           endif 
         }
      }
#     ifdef DEBUG
      sprintf(title, "bf_sz = %d, after bitonic", bf_sz);
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
      if (a[i] > a[i+1]) {
         printf("a[%d] = %d > %d = a[%d]\n",
               i, a[i], a[i+1], i+1);
         return 0;
      }
   return 1; 
}  /* Check_sort */
