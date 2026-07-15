/* File:     nbody_basic_cg.cu
 * Purpose:  Implement a 2-dimensional n-body solver that uses the 
 *           straightforward n^2 algorithm with CUDA.  This version 
 *           directly computes all the forces.  Each CUDA thread t
 *           is responsible for the calculation of all forces on
 *           particle t.  This version uses CUDA cooperative groups
 *           to implement barriers across all the threads in the
 *           grid.
 *
 * Compile:  nvcc -dc -gencode --gencode arch=compute_61,code=sm_61 \
 *              -c nbody_basic_cg.cu 
 *           nvcc -c stats.c 
 *           nvcc -c set_device.c
 *           nvcc -o nbody_basic_cg.o nbody_basic_cg.o stats.o set_device.o
 *           To turn off output except for timing results, define NO_OUTPUT
 *           To get verbose output, define DEBUG
 *           Default floating point type is double.  To get float, define
 *              FLOAT
 *           The COMPUTE_ENERGY macro is not available.
 *           Needs timer.h, stats.h, set_device.h
 * Run:      ./nbody_basic <block count> <threads per block> 
 *              <number of particles> <number of timesteps>  
 *              <size of timestep> <output frequency> <g|i>
 *              'g': generate initial conditions using a random number
 *                   generator
 *              'i': read initial conditions from stdin
 *           blk_ct*th_per_blk should be >= n
 *           A timestep of 0.01 seems to work reasonably well for
 *           the automatically generated data.
 *
 * Input:    If 'g' is specified on the command line, none.  
 *           If 'i', mass, initial position and initial velocity of 
 *              each particle
 * Output:   If the output frequency is k, then position and velocity of 
 *              each particle at every kth timestep
 *
 * Algorithm: Slightly modified version of algorithm in James Demmel, 
 *    "CS 267, Applications of Parallel Computers:  Hierarchical 
 *    Methods for the N-Body Problem",
 *    www.cs.berkeley.edu/~demmel/cs267_Spr09, April 20, 2009.
 *
 *    for each timestep t {
 *       for each particle i
 *          compute f(i), the force on i
 *       for each particle i
 *          update position and velocity of i using F = ma
 *       if (output step) Output new positions and velocities
 *    }
 *
 * Force:    The force on particle i due to particle k is given by
 *
 *    -G m_i m_k (s_i - s_k)/|s_i - s_k|^3
 *
 * Here, m_j is the mass of particle j, s_j is its position vector
 * (at time t), and G is the gravitational constant (see below).  
 *
 * Integration:  We use Euler's method:
 *
 *    v_i(t+1) = v_i(t) + h v'_i(t)
 *    s_i(t+1) = s_i(t) + h v_i(t)
 *
 * Here, v_i(u) is the velocity of the ith particle at time u and
 * s_i(u) is its position.
 *
 * Preprocessor macros
 *    FLOAT:     if defined use floats instead of doubles
 *    REAL:      float or double depending on macro FLOAT
 *    DEBUG:     Very verbose output
 *    NO_OUTPUT: Suppress all output except run-time 
 *    NO_SERIAL: Suppress cpu calculation of solution to nbody problem
 *
 * Environment variables
 *    DEVID:     Which CUDA device to use (an int).  Default 0
 *    ITERS:     How many iterations for stats on run times.  Default:
 *               1, if DEBUG is set always 1.  
 *
 * IPP2:    7.1.13 and Prog Assignment 7.6 (p. 393 and pp. 452 & ff.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <cooperative_groups.h>
extern "C" {
#include "timer.h"
#include "stats.h"
#include "set_device.h"
}

/* In C++ namespaces are used so that two libraries with the same */
/*    identifier can be used without conflict.                    */
/* Here we're saying that the identifiers from the namespace      */
/*    cooperative_groups will be used                             */
using namespace cooperative_groups;
/* We'll use cg as a shorthand for the cooperative_groups         */
/*    namespace                                                   */
namespace cg = cooperative_groups;

#define DIM 2  /* Two-dimensional system */
#define X 0    /* x-coordinate subscript */
#define Y 1    /* y-coordinate subscript */

#ifdef FLOAT
#  define REAL float
#else
#  define REAL double
#endif


const REAL G = 6.673e-11;  /* Gravitational constant. */
                           /* Units are m^3/(kg*s^2)  */
// const REAL G = 0.1;     /* Gravitational constant. */
                           /* Units are m^3/(kg*s^2)  */

/* Max allowable two-norm of difference between 
 * host state vectors and device state vectors
 */
#ifndef NO_SERIAL
const double tol = 1.0e-3;
#endif

typedef REAL vect_t[DIM];  /* Vector type for position, etc. */

struct particle_s {
   REAL m;    /* Mass     */
   vect_t s;  /* Position */
   vect_t v;  /* Velocity */
};

/* Host code */
void Usage(char* prog_name);
void Get_args(int argc, char* argv[], int* blk_ct_p, int* th_per_blk_p,
      int* n_p, int* n_steps_p, REAL* delta_t_p, int* output_freq_p, 
      char* g_i_p);
void Cuda_setup(int argc, char** argv, int blk_ct, int th_per_blk,
      int device);
void Get_init_cond(struct particle_s curr[], int n);
void Gen_init_cond(struct particle_s curr[], int n);
void Host_sim(vect_t forces[], struct particle_s curr[], REAL delta_t,
      int n, int n_steps, int output_freq);
void Dev_sim_driver(vect_t forces[], struct particle_s curr[], 
      REAL delta_t, int n, int n_steps, int output_freq, 
      int blk_ct, int th_per_blk);
void Compute_force(int part, vect_t forces[], struct particle_s curr[], 
      int n);
void Update_part(int part, vect_t forces[], struct particle_s curr[], 
      int n, REAL delta_t);
double Norm2_diff(vect_t x, vect_t y);
double Max_norm(vect_t x, vect_t y);
void Check_state(struct particle_s h_curr[], struct particle_s d_curr[],
      int n, double tol);

/* Kernel */
__global__ void Dev_sim(vect_t forces[], struct particle_s curr[],
      int n, REAL delta_t, int n_steps, int output_freq);


/* Device code */
__device__ void Dev_compute_force(vect_t forces[], 
      struct particle_s curr[], int n, int my_part);
__device__ void Dev_update_part(vect_t forces[], 
      struct particle_s curr[], int n, REAL delta_t, int my_part);

/* Host and device code */
__host__ __device__ void Output_state(const char title[], REAL time, 
      struct particle_s curr[], int n);

/*--------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
   int n;                       /* Number of particles        */
   int n_steps;                 /* Number of timesteps        */
   REAL delta_t;                /* Size of timestep           */
   struct particle_s *d_curr,
        *start_state;           /* State of system            */
   vect_t *d_forces;            /* Forces on each particle    */
#  ifndef NO_SERIAL
   struct particle_s *h_curr;   /* State of system on host    */
   vect_t *h_forces;            /* Forces on each particle on host */
#  endif
   char g_i;                   /*_G_en or _i_nput init conds */
   double start, finish;       /* For timings  */
   int blk_ct, th_per_blk, iter;
   int output_freq, iters;
   int device;

   Get_args(argc, argv, &blk_ct, &th_per_blk, &n, &n_steps, &delta_t, 
         &output_freq, &g_i);
   device = Set_device();
   Cuda_setup(argc, argv, blk_ct, th_per_blk, device);
   start_state = (struct particle_s*) malloc(n*sizeof(struct particle_s));
#  ifndef NO_SERIAL
   h_curr = (struct particle_s*) malloc(n*sizeof(struct particle_s));
   h_forces = (vect_t*) malloc(n*sizeof(vect_t));
#  endif
   cudaMallocManaged(&d_curr, n*sizeof(struct particle_s));
   cudaMallocManaged(&d_forces, n*sizeof(vect_t));
   if (g_i == 'i')
      Get_init_cond(start_state, n);
   else
      Gen_init_cond(start_state, n);

   iters = Setup_stats();
   for (iter = 0; iter < iters; iter++) {
      memcpy(d_curr, start_state, n*sizeof(struct particle_s));
#     ifdef DEBUG
      printf("Device Start\n");
#     endif
      GET_TIME(start);
      Dev_sim_driver(d_forces, d_curr, delta_t, n, n_steps, output_freq,
            blk_ct, th_per_blk);
      GET_TIME(finish);
      /* s_dmin, s_dmax, s_dtotal defined in stats.c */
      Update_stats(start, finish, &s_dmin, &s_dmax, &s_dtotal);

#     ifdef DEBUG
      printf("Device finish\n\n");
      printf("Host Start\n");
#     endif

#     ifndef NO_SERIAL
      memcpy(h_curr, start_state, n*sizeof(struct particle_s));
      GET_TIME(start);
      Host_sim(h_forces, h_curr, delta_t, n, n_steps, output_freq);
      GET_TIME(finish);
      /* s_hmin, s_hmax, s_htotal defined in stats.c */
      Update_stats(start, finish, &s_hmin, &s_hmax, &s_htotal);
#     endif

      /* Make sure device is ready for next iteration */
      cudaDeviceSynchronize();
   } /* for iter */

#  ifndef NO_SERIAL
   Check_state(h_curr, d_curr, n, tol);
#  endif

   Print_stats("Device", s_dmin, s_dmax, s_dtotal, iters);
#  ifndef NO_SERIAL
   Print_stats("  Host", s_hmin, s_hmax, s_htotal, iters);
#  endif

   cudaFree(d_curr);
   cudaFree(d_forces);
#  ifndef NO_SERIAL
   free(h_curr);
   free(h_forces);
#  endif
   free(start_state);

   return 0;
}  /* main */


/*---------------------------------------------------------------------
 * Function: Usage
 * Purpose:  Print instructions for command-line and exit
 * In arg:   
 *    prog_name:  the name of the program as typed on the command-line
 */
void Usage(char* prog_name) {
   fprintf(stderr, "usage: %s <blk_ct> <th_per_blk>\n", prog_name);
   fprintf(stderr, "   <number of particles> <number of timesteps>\n");
   fprintf(stderr, "   <size of timestep> <output frequency>\n");
   fprintf(stderr, "   <g|i>\n");
   fprintf(stderr, "   'g': program should generate init conds\n");
   fprintf(stderr, "   'i': program should get init conds from stdin\n");
   fprintf(stderr, "   blk_ct*th_per_blk should be >= n\n");
    
   exit(0);
}  /* Usage */


/*---------------------------------------------------------------------
 * Function:  Get_args
 * Purpose:   Get command line args
 * In args:
 *    argc:            number of command line args
 *    argv:            command line args
 * Out args:
 *    blk_ct_p:        pointer to blk_ct, number of thread blocks
 *    th_per_blk_p:    pointer to th_per_blk, number of threads in each block
 *    n_p:             pointer to n, the number of particles
 *    n_steps_p:       pointer to n_steps, the number of timesteps
 *    delta_t_p:       pointer to delta_t, the size of each timestep
 *    output_freq:     the number of timesteps between steps whose output 
 *                     is printed
 *    g_i_p:           pointer to char which is 'g' if the init conds
 *                     should be generated by the program and 'i' if
 *                     they should be read from stdin
 */
void Get_args(int argc, char* argv[], int* blk_ct_p, int* th_per_blk_p,
      int* n_p, int* n_steps_p, 
      REAL* delta_t_p, int* output_freq_p, char* g_i_p) {
   if (argc != 8) Usage(argv[0]);
   *blk_ct_p = strtol(argv[1], NULL, 10);
   *th_per_blk_p = strtol(argv[2], NULL, 10);
   *n_p = strtol(argv[3], NULL, 10);
   *n_steps_p = strtol(argv[4], NULL, 10);
   *delta_t_p = strtod(argv[5], NULL);
   *output_freq_p = strtol(argv[6], NULL, 10);
   *g_i_p = argv[7][0];

   if (*n_p <= 0 || *n_steps_p < 0 || *delta_t_p <= 0) Usage(argv[0]);
   if (*g_i_p != 'g' && *g_i_p != 'i') Usage(argv[0]);
   if ((*blk_ct_p)*(*th_per_blk_p) < *n_p) Usage(argv[0]);

#  ifdef DEBUG
   printf("blk_ct = %d, th_per_blk = %d\n", *blk_ct_p, *th_per_blk_p);
   printf("n = %d\n", *n_p);
   printf("n_steps = %d\n", *n_steps_p);
   printf("delta_t = %e\n", *delta_t_p);
   printf("output_freq = %d\n", *output_freq_p);
   printf("g_i = %c\n", *g_i_p);
#  endif
}  /* Get_args */

/*---------------------------------------------------------------------
 * Function:   Cuda_setup
 * Purpose:    Set device, check whether proposed parameters are OK.
 * In args:    blk_ct, th_per_blk
 * Note:       Much of this is copied from the CUDA 10 sample
 *             conjugateGradientMultiBlockCG.cu
 * Note:       This was motivated by the total failure of the code
 *             in going from 16 blocks and 1024 threads per block to
 *             32 blocks and 1024 threads per block
 */
void Cuda_setup(int argc, char* argv[], int blk_ct, int th_per_blk,
      int device) {

   cudaDeviceProp deviceProp;
   cudaGetDeviceProperties(&deviceProp, device);
 
   if (!deviceProp.managedMemory) {
      // This program requires being run on a device that supports Unified 
      // Memory
      fprintf(stderr, "Unified Memory not supported on this device\n");
      exit(-1);
   }
 
   // This program requires being run on a device that supports 
   // Cooperative Kernel Launch
   if (!deviceProp.cooperativeLaunch) {
      fprintf(stderr, "Device does not support Cooperative Kernel Launch\n");
      exit(-1);
   }

   // Statistics about the GPU device
   printf("GPU device has %d Multi-Processors, SM %d.%d compute capabilities\n",
      deviceProp.multiProcessorCount, deviceProp.major, deviceProp.minor);

   // This can be 0
   int sMemSize = 0;
   int numBlocksPerSm = 0;
   int numThreads = th_per_blk;

   cudaOccupancyMaxActiveBlocksPerMultiprocessor(
      &numBlocksPerSm, Dev_sim, numThreads, sMemSize);

   int numSms = deviceProp.multiProcessorCount;
   int maxActiveBlks = numSms*numBlocksPerSm;

   printf("Max active blks = %d\n", maxActiveBlks);
   printf("Requested active blks = %d\n", blk_ct);
   if (maxActiveBlks < blk_ct) {
      fprintf(stderr, "Can't run this many blocks\n");
      exit(-1);
   }

}  /* Cuda_setup */

/*---------------------------------------------------------------------
 * Function:  Get_init_cond
 * Purpose:   Read in initial conditions:  mass, position and velocity
 *            for each particle
 * In args:  
 *    n:      number of particles
 * Out args:
 *    curr:   array of n structs, each struct stores the mass (scalar),
 *            position (vector), and velocity (vector) of a particle
 */
void Get_init_cond(struct particle_s curr[], int n) {
   int part;

   printf("For each particle, enter (in order):\n");
   printf("   its mass, its x-coord, its y-coord, ");
   printf("its x-velocity, its y-velocity\n");
   for (part = 0; part < n; part++) {
#     ifdef FLOAT
      scanf("%f", &curr[part].m);
      scanf("%f", &curr[part].s[X]);
      scanf("%f", &curr[part].s[Y]);
      scanf("%f", &curr[part].v[X]);
      scanf("%f", &curr[part].v[Y]);
#     else
      scanf("%lf", &curr[part].m);
      scanf("%lf", &curr[part].s[X]);
      scanf("%lf", &curr[part].s[Y]);
      scanf("%lf", &curr[part].v[X]);
      scanf("%lf", &curr[part].v[Y]);
#     endif
   }
}  /* Get_init_cond */

/*---------------------------------------------------------------------
 * Function:  Gen_init_cond
 * Purpose:   Generate initial conditions:  mass, position and velocity
 *            for each particle
 * In args:  
 *    n:      number of particles
 * Out args:
 *    curr:   array of n structs, each struct stores the mass (scalar),
 *            position (vector), and velocity (vector) of a particle
 *
 * Note:      The initial conditions place all particles at
 *            equal intervals on the nonnegative x-axis with 
 *            identical masses, and identical initial speeds
 *            parallel to the y-axis.  However, some of the
 *            velocities are in the positive y-direction and
 *            some are negative.
 */
void Gen_init_cond(struct particle_s curr[], int n) {
   int part;
   REAL mass = 5.0e24;
   REAL gap = 1.0e5;
   REAL speed = 3.0e4;

   srandom(1);
   for (part = 0; part < n; part++) {
      curr[part].m = mass;
      curr[part].s[X] = part*gap;
      curr[part].s[Y] = 0.0;
      curr[part].v[X] = 0.0;
//    if (random()/((REAL) RAND_MAX) >= 0.5)
      if (part % 2 == 0)
         curr[part].v[Y] = speed;
      else
         curr[part].v[Y] = -speed;
   }
}  /* Gen_init_cond */


/*---------------------------------------------------------------------
 * Function:     Host_sim
 * Purpose:      Run serial n-body simulation on host
 * In args:      n, n_steps, output_freq
 * In/out arg:   curr:  state of system
 * Scratch:      forces
 */
void Host_sim(vect_t forces[], struct particle_s curr[], REAL delta_t,
      int n, int n_steps, int output_freq) {
   int step, part;
   REAL t;

#  ifndef NO_OUTPUT
   // Output_state("   Host", 0, curr, n);
#  endif
   for (step = 1; step <= n_steps; step++) {
      t = step*delta_t;
//    memset(forces, 0, n*sizeof(vect_t));
      for (part = 0; part < n; part++)
         Compute_force(part, forces, curr, n);
      for (part = 0; part < n; part++)
         Update_part(part, forces, curr, n, delta_t);
#     ifndef NO_OUTPUT
      if (step % output_freq == 0)
         Output_state("   Host", t, curr, n);
#     endif
   } /* for step */
}  /* Host_sim */


/*---------------------------------------------------------------------
 * Function:     Dev_sim_driver
 * Purpose:      Run n-body simulation on device
 * In args:      n, n_steps, output_freq, blk_ct, th_per_blk
 * In/out arg:   curr:  state of system
 * Scratch:      forces
 */
__host__ void Dev_sim_driver(vect_t forces[], struct particle_s curr[], 
      REAL delta_t, int n, int n_steps, int output_freq, 
      int blk_ct, int th_per_blk) {

#  ifndef NO_OUTPUT
// Output_state("Device", 0, curr, n);
#  endif

   // Set up for call to Dev_sim
   // __host__ cudaError_t cudaLaunchCooperativeKernel ( const void* func, 
   //      dim3 blk_ct, dim3 th_per_blk, 
   //      void** args, size_t shared_mem_sz, cudaStream_t stream ) 
   // int shared_mem_sz = 0;
   // int th_per_blk = ...;
   void *dev_sim_args[] = {(void*) &forces, (void*) &curr, (void*) &n,
                           (void*) &delta_t, (void*) &n_steps,
                           (void*) &output_freq};
   cudaLaunchCooperativeKernel((void*) Dev_sim,
         blk_ct, th_per_blk, 
         dev_sim_args, 0, NULL);

   // This seems to be needed
   cudaDeviceSynchronize();
}  /* Dev_sim_driver */


/*---------------------------------------------------------------------
 * Function:     Dev_sim
 * Purpose:      Run n-body simulation on device
 * In args:      n, n_steps, output_freq
 * In/out arg:   curr:  state of system
 * Scratch:      forces
 */
__global__ void Dev_sim(vect_t forces[], struct particle_s curr[],
      int n, REAL delta_t, int n_steps, int output_freq) {
   int step;
   REAL t;
   int my_part = blockDim.x*blockIdx.x + threadIdx.x;
   cg::grid_group grid = cg::this_grid();
   cg::sync(grid);

   for (step = 1; step <= n_steps; step++) {
      t = step*delta_t;
//    memset(forces, 0, n*sizeof(vect_t));
      Dev_compute_force(forces, curr, n, my_part);
      cg::sync(grid);
      Dev_update_part(forces, curr, n, delta_t, my_part);
      cg::sync(grid);

#     ifndef NO_OUTPUT
      if (step % output_freq == 0)
         if (my_part == 0) // my_part == my_rank
            Output_state("Device", t, curr, n);
      cg::sync(grid);
#     endif
   } /* for step */
}  /* Dev_sim */

/*---------------------------------------------------------------------
 * Function:  Output_state
 * Purpose:   Print the current state of the system
 * In args:
 *    curr:   array with n elements, curr[i] stores the state (mass,
 *            position and velocity) of the ith particle
 *    n:      number of particles
 */
void Output_state(const char title[], REAL time, struct particle_s curr[], 
      int n) {
   int part;
   printf("%s: %.2f\n", title, time);
   for (part = 0; part < n; part++) {
//    printf("%.3f ", curr[part].m);
      printf("%3d %10.3e ", part, curr[part].s[X]);
      printf("  %10.3e ", curr[part].s[Y]);
      printf("  %10.3e ", curr[part].v[X]);
      printf("  %10.3e\n", curr[part].v[Y]);
   }
   printf("\n");
}  /* Output_state */


/*---------------------------------------------------------------------
 * Function:  Compute_force
 * Purpose:   Compute the total force on particle part.  
 * In args:   
 *    part:   the particle on which we're computing the total force
 *    curr:   current state of the system:  curr[i] stores the mass,
 *            position and velocity of the ith particle
 *    n:      number of particles
 * Out arg:
 *    force:  force stores the total force on particle part
 *
 * Note: This function uses the force due to gravitation.  So 
 * the force on particle i due to particle k is given by
 *
 *    -G_m m_i m_k (s_k - s_i)/|s_k - s_i|^2
 *
 * Here, m_j is the mass of particle j and s_k is its position vector
 * (at time t). 
 *
 * Note:      The order in which the arithmetic operations are carried
 *            out has been changed from the original serial, double precision
 *            code in an effort to avoid overflow
 */
void Compute_force(int part, vect_t forces[], struct particle_s curr[], 
      int n) {
   int k;
   REAL gm1, m2l3; 
   vect_t f_part_k;
   REAL len, len_3, fact;

   forces[part][X] = forces[part][Y] = 0.0;
   for (k = 0; k < n; k++) {
      if (k != part) {
         /* Compute force on part due to k */
         f_part_k[X] = curr[part].s[X] - curr[k].s[X];
         f_part_k[Y] = curr[part].s[Y] - curr[k].s[Y];
         len = sqrt(f_part_k[X]*f_part_k[X] + f_part_k[Y]*f_part_k[Y]);
         len_3 = len*len*len;
   #     ifdef DEBUG
         printf("Prt %d <- prt %d diff = (%.3e, %.3e)\n",
               part, k, f_part_k[X], f_part_k[Y]);
         printf("Prt %d <- prt %d: dist = %3e, dist^3 = %.3e\n",
               part, k, len, len_3);
   #     endif
         gm1 = -G*curr[part].m;
         m2l3 = curr[k].m/len_3;
         fact = gm1*m2l3;
   #     ifdef DEBUG
         printf("Prt %d <- prt %d: mp = %.3e, mk = %.3e, gm1 = %.3e, m2l3 = %.3e, fact = %.3e\n",
               part, k, curr[part].m, curr[k].m, gm1, m2l3, fact);
   #     endif
         f_part_k[X] *= fact;
         f_part_k[Y] *= fact;
   #     ifdef DEBUG
         printf("Force on particle %d due to particle %d = (%.3e, %.3e)\n",
               part, k, f_part_k[X], f_part_k[Y]);
   #     endif
   
         /* Add force in to total forces */
         forces[part][X] += f_part_k[X];
         forces[part][Y] += f_part_k[Y];
      }
   }
}  /* Compute_force */


/*---------------------------------------------------------------------
 * Function:  Dev_compute_force
 * Purpose:   Compute the total force on each particle
 * In args:   curr, n, my_part
 * Out arg:   forces
 *
 * Note:      The order in which the arithmetic operations are carried
 *            out has been changed from the original serial, double precision
 *            code in an effort to avoid overflow
 */
__device__ void Dev_compute_force(vect_t forces[], 
      struct particle_s curr[], int n, int my_part) {

   if (my_part < n) {
      int k;
      REAL gm1, m2l3; 
      vect_t f_part_k;
      REAL len, len_3, fact, fx, fy;
   
      fx = fy = 0.0;
      for (k = 0; k < n; k++) {
         if (k != my_part) {
            /* Compute force on my_part due to k */
            f_part_k[X] = curr[my_part].s[X] - curr[k].s[X];
            f_part_k[Y] = curr[my_part].s[Y] - curr[k].s[Y];
            len = sqrt(f_part_k[X]*f_part_k[X] + f_part_k[Y]*f_part_k[Y]);
            len_3 = len*len*len;
            gm1 = -G*curr[my_part].m;
            m2l3 = curr[k].m/len_3;
            fact = gm1*m2l3;
            f_part_k[X] *= fact;
            f_part_k[Y] *= fact;
 #          ifdef DEBUG
            printf("Force on particle %d due to particle %d = (%.3e, %.3e)\n",
                  my_part, k, f_part_k[X], f_part_k[Y]);
#           endif
      
            /* Add force in to total forces */
            fx += f_part_k[X];
            fy += f_part_k[Y];
         }
      }
      forces[my_part][X] = fx;
      forces[my_part][Y] = fy;
   }  /* my_part < n */
}  /* Dev_compute_force */


/*---------------------------------------------------------------------
 * Function:  Update_part
 * Purpose:   Update the velocity and position for particle part
 * In args:
 *    part:    the particle we're updating
 *    forces:  forces[i] stores the total force on the ith particle
 *    n:       number of particles
 *
 * In/out arg:
 *    curr:    curr[i] stores the mass, position and velocity of the
 *             ith particle
 *
 * Note:  This version uses Euler's method to update both the velocity
 *    and the position.
 */
void Update_part(int part, vect_t forces[], struct particle_s curr[], 
      int n, REAL delta_t) {
   REAL fact = delta_t/curr[part].m;

#  ifdef DEBUG
   printf("Before update of %d:\n", part);
   printf("   Position  = (%.3e, %.3e)\n", curr[part].s[X], curr[part].s[Y]);
   printf("   Velocity  = (%.3e, %.3e)\n", curr[part].v[X], curr[part].v[Y]);
   printf("   Net force = (%.3e, %.3e)\n", forces[part][X], forces[part][Y]);
#  endif
   curr[part].s[X] += delta_t * curr[part].v[X];
   curr[part].s[Y] += delta_t * curr[part].v[Y];
   curr[part].v[X] += fact * forces[part][X];
   curr[part].v[Y] += fact * forces[part][Y];
#  ifdef DEBUG
   printf("Position of %d = (%.3e, %.3e), Velocity = (%.3e,%.3e)\n",
         part, curr[part].s[X], curr[part].s[Y],
               curr[part].v[X], curr[part].v[Y]);
#  endif
}  /* Update_part */


/*---------------------------------------------------------------------
 * Function:   Dev_update_part
 * Purpose:    Update the velocity and position for particle part
 * In arg:
 *    forces:  forces stores the total force on the current particle
 *    part:    calling thread's particle (= its global rank)
 *
 * In/out arg:
 *    curr:    curr stores the mass, position and velocity of the
 *                current particle
 *
 * Note:  This version uses Euler's method to update both the velocity
 *    and the position.
 */
__device__ void Dev_update_part(vect_t forces[], 
      struct particle_s curr[], int n, REAL delta_t, int part) {

   if (part < n) {
      REAL fact = delta_t/curr[part].m;

#     ifdef DEBUG
      printf("Before update of %d:\n", part);
      printf("   Position  = (%.3e, %.3e)\n", curr[part].s[X], curr[part].s[Y]);
      printf("   Velocity  = (%.3e, %.3e)\n", curr[part].v[X], curr[part].v[Y]);
      printf("   Net force = (%.3e, %.3e)\n", forces[part][X], forces[part][Y]);
#     endif
      curr[part].s[X] += delta_t * curr[part].v[X];
      curr[part].s[Y] += delta_t * curr[part].v[Y];
      curr[part].v[X] += fact * forces[part][X];
      curr[part].v[Y] += fact * forces[part][Y];
#     ifdef DEBUG
      printf("Position of %d = (%.3e, %.3e), Velocity = (%.3e,%.3e)\n",
            part, curr[part].s[X], curr[part].s[Y],
               curr[part].v[X], curr[part].v[Y]);
#     endif
   }  /* part < n */
}  /* Dev_update_part */


/*---------------------------------------------------------------------
 * Function:  Norm2_diff
 * Purpose:   Find the two-norm of the DIM-dimensional vectors x and y
 * In args:   x, y
 * Ret val:   ||x-y||_2
 */
double Norm2_diff(vect_t x, vect_t y) {
   double diff, norm = 0;
   int i;

   for (i = 0; i < DIM; i++) {
      diff = x[i]-y[i];
      norm += diff*diff;
   }
   return sqrt(norm);
}  /* Norm2_diff */


/*---------------------------------------------------------------------
 * Function:  Max_norm
 * Purpose:   Find the max-norm of the DIM-dimensional vectors x and y
 * In args:   x, y
 * Ret val:   max{|x_i-y_i|: i = 0, 1, ... DIM-1}
 */
double Max_norm(vect_t x, vect_t y) {
   double diff, norm = 0;
   int i;

   for (i = 0; i < DIM; i++) {
      diff = fabs(x[i]-y[i]);
      if (diff > norm) norm = diff;
   }
   return norm;
}  /* Max_norm */


/*---------------------------------------------------------------------
 * Function:  Check_state
 * Purpose:   Compare the current state as computed by the host and
 *            the current state as computed by the device
 * In args:   All
 */
void Check_state(struct particle_s h_curr[], struct particle_s d_curr[],
      int n, double tol) {
   int part; 
   double norm;

   for (part = 0; part < n; part++) {
      norm = Max_norm(h_curr[part].s, d_curr[part].s);
      if (norm > tol)
        printf("Part %d, pos > H = (%e, %e), D = (%e, %e), ND = %e\n", 
              part, 
              h_curr[part].s[X], h_curr[part].s[X], 
              d_curr[part].s[X], d_curr[part].s[Y],  
              norm);
      norm = Max_norm(h_curr[part].v, d_curr[part].v);
      if (norm > tol)
        printf("Part %d, vel > H = (%e, %e), D = (%e, %e), ND = %e\n", 
              part, 
              h_curr[part].v[X], h_curr[part].v[Y], 
              d_curr[part].v[X], d_curr[part].v[Y],  
              norm);
   }
}  /* Check_state */
