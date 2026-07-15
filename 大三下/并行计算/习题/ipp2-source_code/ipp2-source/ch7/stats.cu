/* File:     stats.c
 * Purpose:  Keep track of statistics on host and device run-times
 *
 * Compile:  gcc -g -Wall -o prog prog.c stats.c
 * Run:      ./prog ...
 *
 * Usage:    Assumes a loop of repeated executions of the code of
 *           interest.  Then performance stats printed.
 *
 *              ITERS = Setup_stats();
 *              for (iter = 0; iter < ITERS; iter++) {
 *                 GET_TIME(start);
 *                 Execute device code;
 *                 GET_TIME(finish);
 *                 Update_stats(start, finish, &s_dmin, &s_dmax,
 *                       &s_dtotal);
 *
 *                 GET_TIME(start);
 *                 Execute host code;
 *                 GET_TIME(finish);
 *                 Update_stats(start, finish, &s_hmin, &s_hmax,
 *                       &s_htotal);
 *              }
 *
 *              Print_stats("Device:", s_dmin, s_dmax, s_dtotal);
 *              Print_stats("  Host:", s_hmin, s_hmax, s_htotal);
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>  /* For getenv */
#include "stats.h"

double s_start, s_finish;
double host_time = 0.0;
double s_dmin = 0.0, s_dmax = 0.0, s_dtotal = -1;
double s_hmin = 0.0, s_hmax = 0.0, s_htotal = -1;

/*---------------------------------------------------------------------
 * Function:  Setup_stats
 * Purpose:   Setup timing stats
 *
 * Ret_val:   Number of iterations
 */
int Setup_stats(void) {
#  ifndef DEBUG
   char* iters_str = NULL;
#  endif
   int iters;

#  ifndef DEBUG
   iters_str = getenv("ITERS");
   if (iters_str == NULL || strlen(iters_str) == 0) {
      fprintf(stderr, "ITERS not set.  Setting ITERS = 1\n");
      iters = 1;
   } else {
      iters = strtol(iters_str, NULL, 10);
      fprintf(stderr, "Using ITERS = %d\n", iters);
   }
#  else
   iters = 1;
#  endif
   return iters;
}  /* Setup_stats */


/*---------------------------------------------------------------------
 * Function:  Update_stats
 * Purpose:   Update timing stats
 *
 * Note:      Negative value of *total_p, indicates function
 *            is being called for the first time.
 */
void Update_stats(double start, double finish, 
                  double start2, double finish2, 
      double* min_p, double* max_p, double* total_p) {
   double elapsed = finish - start;
   if (*total_p < 0) {
      *min_p = *max_p = *total_p = elapsed;
   } else {
      if (elapsed < *min_p) {
         *min_p = elapsed;
         host_time = finish2-start2;
      }
      if (elapsed > *max_p) *max_p = elapsed;
      *total_p += elapsed;
   }
}  /* Update_stats */


/*---------------------------------------------------------------------
 * Function:  Print
 * Purpose:   Print timing stats
 *
 */
void Print_stats(const char title[], double min, double max, double total,
      int iters) {
   printf("%s:  %4.2e  %4.2e  %4.2e\n", title,
         min, max, total/iters);
   printf("Host code: %4.2e\n", host_time);
// printf("%s:  min = %e, max = %e, avg = %e\n", title,
//       min, max, total/iters);
}
