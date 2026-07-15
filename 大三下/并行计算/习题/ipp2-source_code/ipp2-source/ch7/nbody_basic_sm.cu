/* File:     nbody_basic_sm.cu
 * Purpose:  Implement a 2-dimensional n-body solver that uses the 
 *           straightforward n^2 algorithm with CUDA.  This version 
 *           directly computes all the forces.  Each CUDA thread t
 *           is responsible for the calculation of all forces on
 *           particle t.  This version reads positions and velocities
 *           into shared memory for calculation of the forces.
 *
 * Compile:  nvcc -o nbody_basic_sm nbody_basic_sm.cu stats.c set_device.c
 *           If COMPUTE_ENERGY is defined, the program will print 
 *              total potential energy, total kinetic energy and total
 *              energy of the system at each time step.
 *           To turn off output except for timing results, define NO_OUTPUT
 *           To get verbose output, define DEBUG
 *           Default floating point type is REAL = double.  To get
 *              float, define FLOAT
 *           Needs timer.h, stats.h, set_device.h
 * Run:      ./nbody_basic_sm <block count> <threads per block> 
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
 *    NO_OUTPUT: Suppress all output except run-time and check of 
 *       host vs device final states 
 *    COMPUTE_ENERGY:  Compute kinetic energy, potential energy, total energy
 *    NO_SERIAL: Suppress cpu calculation of solution to nbody problem
 *
 * Environment variables
 *    DEVID:     Which CUDA device to use (an int).  Default 0
 *    ITERS:     How many iterations for stats on run times.  Default:
 *               1, if DEBUG is set always 1.  
 *
 * Timing info for this version nbody_basic_sm.cu:
 *    Compiled with -DFLOAT, -DNO_OUTPUT and -arch=sm_61
 *       -DNO_SERIAL added for 64 or more blocks
 *    Run with ITERS=1 (all inputs run at least twice)
 *    Run on CS Pascal system cabra, device 1
 *    delta_t = 0.01, timesteps = 2
 *    Timings taken 12/16/2019
 *    Program was run a minimum of three times for each input, 
 *       and minimum runtimes are reported.
 *    This data is OK.
 *
 *    blks  thds/blk   parts   runtime
 *    ----  --------   -----   -------
 *      1     1024      1024   1.59e-3 
 *                             7.01e-2 serial
 *      2     1024      2048   2.17e-3
 *                             2.82e-1 serial
 *     32     1024    32,768   4.17e-2
 *                             5.07e+1 serial
 *     64     1024    65,536   1.98e-1
 *                             
 *    256     1024   262,144   1.96e+0 
 *                             
 *    1024    1024 1,048,576   2.91e+1
 *
 * IPP2:  7.1.16 (pp. 395 and ff.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
extern "C" {
#include "timer.h"
#include "stats.h"
#include "set_device.h"
}

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

#define MAX_BLKSZ 1024

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
void Get_init_cond(struct particle_s curr[], int n);
void Gen_init_cond(struct particle_s curr[], int n);
void Output_state(const char title[], REAL time, struct particle_s curr[], 
      int n);
void Host_sim(vect_t forces[], struct particle_s curr[], REAL delta_t,
      int n, int n_steps, int output_freq);
void Dev_sim_driver(vect_t forces[], struct particle_s curr[],
      REAL delta_t, int n, int n_steps, int output_freq, int blk_ct,
      int th_per_blk);
void Compute_force(int part, vect_t forces[], struct particle_s curr[],
      int n);
void Update_part(int part, vect_t forces[], struct particle_s curr[],
      int n, REAL delta_t);
void Compute_energy(struct particle_s curr[],
      int n, REAL* kin_en_p, REAL* pot_en_p);
void Check_state(struct particle_s h_curr[], struct particle_s d_curr[],
      int n, double tol);

/* Kernels */
__global__ void Dev_compute_force(vect_t forces[], 
      struct particle_s curr[], int n);
__global__ void Dev_update_part(vect_t forces[], 
      struct particle_s curr[], int n, REAL delta_t);

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
   char g_i;                    /*_G_en or _i_nput init conds */
   double start, finish;        /* For timings                */
   int blk_ct, th_per_blk, iter;
   int output_freq, iters;

   Get_args(argc, argv, &blk_ct, &th_per_blk, &n, &n_steps, &delta_t, 
         &output_freq, &g_i);
   Set_device();
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
 * Function:  Host_sim
 * Purpose:   Run serial n-body simulation on host
 * In args:   n, n_steps, output_freq
 * Out arg:   curr:  state of system
 * Scratch:   forces
 */
void Host_sim(vect_t forces[], struct particle_s curr[], REAL delta_t,
      int n, int n_steps, int output_freq) {
   int step, part;
   REAL t;

#  ifdef COMPUTE_ENERGY
   REAL kinetic_energy, potential_energy;
   Compute_energy(curr, n, &kinetic_energy, &potential_energy);
   printf("Host:   PE = %e, KE = %e, Total Energy = %e\n",
         potential_energy, kinetic_energy, kinetic_energy+potential_energy);
#  endif
#  ifndef NO_OUTPUT
   Output_state("   Host", 0, curr, n);
#  endif
   for (step = 1; step <= n_steps; step++) {
      t = step*delta_t;
//    memset(forces, 0, n*sizeof(vect_t));
      for (part = 0; part < n; part++)
         Compute_force(part, forces, curr, n);
      for (part = 0; part < n; part++)
         Update_part(part, forces, curr, n, delta_t);
#     ifdef COMPUTE_ENERGY
      Compute_energy(curr, n, &kinetic_energy, &potential_energy);
      printf("Host:   PE = %e, KE = %e, Total Energy = %e\n",
            potential_energy, kinetic_energy, 
            kinetic_energy+potential_energy);
#     endif
#     ifndef NO_OUTPUT
      if (step % output_freq == 0)
         Output_state("   Host", t, curr, n);
#     endif
   } /* for step */
}  /* Host_sim */


/*---------------------------------------------------------------------
 * Function:   Dev_sim_driver
 * Purpose:    Run n-body simulation on device
 * In args:    delta_t, n, n_steps, output_freq, blk_ct, th_per_blk
 * In/out arg: curr:  state of system
 * Scratch:    forces
 */
__host__ void Dev_sim_driver(vect_t forces[], struct particle_s curr[], 
      REAL delta_t, int n, int n_steps, int output_freq, 
      int blk_ct, int th_per_blk) {
   int step;
   REAL t;

#  ifdef COMPUTE_ENERGY
   REAL kinetic_energy, potential_energy;
   Compute_energy(curr, n, &kinetic_energy, &potential_energy);
   printf("Device: PE = %e, KE = %e, Total Energy = %e\n",
         potential_energy, kinetic_energy, kinetic_energy+potential_energy);
#  endif
#  ifndef NO_OUTPUT
   Output_state("Device", 0, curr, n);
#  endif
   for (step = 1; step <= n_steps; step++) {
      t = step*delta_t;
//    memset(forces, 0, n*sizeof(vect_t));
      Dev_compute_force <<<blk_ct, th_per_blk>>> (forces, curr, n);
      Dev_update_part <<<blk_ct, th_per_blk>>> (forces, curr, n, delta_t);

#     if defined(COMPUTE_ENERGY) || !defined(NO_OUTPUT)
      cudaDeviceSynchronize();
#     endif
#     ifdef COMPUTE_ENERGY
      Compute_energy(curr, n, &kinetic_energy, &potential_energy);
      printf("Device: PE = %e, KE = %e, Total Energy = %e\n",
            potential_energy, kinetic_energy, 
            kinetic_energy+potential_energy);
#     endif
#     ifndef NO_OUTPUT
      if (step % output_freq == 0)
         Output_state("Device", t, curr, n);
#     endif
   } /* for step */
   // Without this, the final Dev_update_part may not complete
   cudaDeviceSynchronize();
}  /* Dev_sim_driver */


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
 * Purpose:   Compute the total force on particle part for the
 *            current timestep
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
   #     ifdef DDEBUG
         printf("Prt %d <- prt %d diff = (%.3e, %.3e)\n",
               part, k, f_part_k[X], f_part_k[Y]);
         printf("Prt %d <- prt %d: dist = %3e, dist^3 = %.3e\n",
               part, k, len, len_3);
   #     endif
         gm1 = -G*curr[part].m;
         m2l3 = curr[k].m/len_3;
         fact = gm1*m2l3;
   #     ifdef DDEBUG
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
 * Purpose:   Compute the total force on each particle for the current
 *            timestep
 * In args:   curr, n
 * Out arg:   forces
 *
 * Note:      The order in which the arithmetic operations are carried
 *            out has been changed from the original serial, double precision
 *            code in an effort to avoid overflow
 */
__global__ void Dev_compute_force(vect_t forces[], 
      struct particle_s curr[], int n) {

   __shared__ vect_t shmem_locs[MAX_BLKSZ];
   __shared__ REAL shmem_masses[MAX_BLKSZ];

   int part = blockDim.x*blockIdx.x + threadIdx.x;
   int my_loc_rk = threadIdx.x;
   int blk_sz = blockDim.x;
// int my_blk = blockIdx.x;

   if (part < n) {
      int tile, my_part, loc_part, glb_part;
      REAL my_mass, gm1, m2l3; 
      vect_t f_part_k, my_loc;
      REAL len, len_3, fact, fx, fy;
   
      fx = fy = 0.0;
      my_mass = curr[part].m;
      my_loc[X] = curr[part].s[X];
      my_loc[Y] = curr[part].s[Y];
      for (tile = 0; tile < n/blk_sz; tile++) {
         // Each thread in the block loads the pos and mass of one
         // of the particles in the current tile
         my_part = tile*blk_sz +  my_loc_rk;
         shmem_locs[my_loc_rk][X] = curr[my_part].s[X];
         shmem_locs[my_loc_rk][Y] = curr[my_part].s[Y];
         shmem_masses[my_loc_rk] = curr[my_part].m;
         __syncthreads();

         for (loc_part = 0; loc_part < blk_sz; loc_part++) {
            glb_part = tile*blk_sz + loc_part;
            if (glb_part != part) {
               /* Compute force on part due to loc_part */
               f_part_k[X] = my_loc[X] - shmem_locs[loc_part][X];
               f_part_k[Y] = my_loc[Y] - shmem_locs[loc_part][Y];
               len = sqrt(f_part_k[X]*f_part_k[X] + f_part_k[Y]*f_part_k[Y]);
               len_3 = len*len*len;
               gm1 = -G*my_mass;
               m2l3 = shmem_masses[loc_part]/len_3;
               fact = gm1*m2l3;
               f_part_k[X] *= fact;
               f_part_k[Y] *= fact;
 #             ifdef DEBUG
               printf("Force on particle %d due to particle %d = (%.3e, %.3e)\n",
                     part, glb_part, f_part_k[X], f_part_k[Y]);
#              endif
      
               /* Add force in to total forces */
               fx += f_part_k[X];
               fy += f_part_k[Y];
            }
         }
         __syncthreads();
      }
      forces[part][X] = fx;
      forces[part][Y] = fy;
   }  /* part < n */
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
   printf("   delta_t = %.3e, mass = %.3e, fact = %.3e\n", 
            delta_t, curr[part].m, fact); 
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
 *    force:   force stores the total force on the current particle
 *
 * In/out arg:
 *    curr:    curr stores the mass, position and velocity of the
 *                current particle
 *
 * Note:  This version uses Euler's method to update both the velocity
 *    and the position.
 */
__global__ void Dev_update_part(vect_t forces[], 
      struct particle_s curr[], int n, REAL delta_t) {
   int part = blockDim.x*blockIdx.x + threadIdx.x;

   if (part < n) {
      REAL fact = delta_t/curr[part].m;

#     ifdef DEBUG
      printf("Before update of %d:\n", part);
      printf("   delta_t = %.3e, mass = %.3e, fact = %.3e\n", 
            delta_t, curr[part].m, fact); 
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
 * Function:  Compute_energy
 * Purpose:   Compute the kinetic and potential energy in the system
 * In args:
 *    curr:   current state of the system, curr[i] stores the mass,
 *            position and velocity of the ith particle
 *    n:      number of particles
 * Out args:
 *    kin_en_p: pointer to kinetic energy of system
 *    pot_en_p: pointer to potential energy of system
 */
void Compute_energy(struct particle_s curr[], int n, REAL* kin_en_p,
      REAL* pot_en_p) {
   int i, j;
   vect_t diff;
   REAL pe = 0.0, ke = 0.0;
   REAL dist, speed_sqr;

   for (i = 0; i < n; i++) {
      speed_sqr = curr[i].v[X]*curr[i].v[X] + curr[i].v[Y]*curr[i].v[Y];
      ke += curr[i].m*speed_sqr;
   }
   ke *= 0.5;

   for (i = 0; i < n-1; i++) {
      for (j = i+1; j < n; j++) {
         diff[X] = curr[i].s[X] - curr[j].s[X];
         diff[Y] = curr[i].s[Y] - curr[j].s[Y];
         dist = sqrt(diff[X]*diff[X] + diff[Y]*diff[Y]);
         pe += -G*curr[i].m*curr[j].m/dist;
      }
   }

   *kin_en_p = ke;
   *pot_en_p = pe;
}  /* Compute_energy */


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
 * Function:  Check_state
 * Purpose:   Compare the current state as computed by the host and
 *            the current state as computed by the device
 * In args:   All
 */
void Check_state(struct particle_s h_curr[], struct particle_s d_curr[],
      int n, double tol) {
   int part; 
   double n2diff;

   for (part = 0; part < n; part++) {
      n2diff = Norm2_diff(h_curr[part].s, d_curr[part].s);
      if (n2diff > tol)
        printf("Part %d, pos > H = (%e, %e), D = (%e, %e), ND = %e\n", 
              part, 
              h_curr[part].s[X], h_curr[part].s[X], 
              d_curr[part].s[X], d_curr[part].s[Y],  
              n2diff);
      n2diff = Norm2_diff(h_curr[part].v, d_curr[part].v);
      if (n2diff > tol)
        printf("Part %d, vel > H = (%e, %e), D = (%e, %e), ND = %e\n", 
              part, 
              h_curr[part].v[X], h_curr[part].v[Y], 
              d_curr[part].v[X], d_curr[part].v[Y],  
              n2diff);
   }
}  /* Check_state */
