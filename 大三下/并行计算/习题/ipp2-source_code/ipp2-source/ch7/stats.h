#ifndef __STATS_H__
#define __STATS_H__

extern double s_start, s_finish;
extern double s_dmin, s_dmax, s_dtotal;
extern double s_hmin, s_hmax, s_htotal;

int  Setup_stats(void);
void Update_stats(double start, double finish, 
      double* min_p, double* max_p, double* total_p);
void Print_stats(const char title[], double min, double max, 
      double total, int iters);
#endif
