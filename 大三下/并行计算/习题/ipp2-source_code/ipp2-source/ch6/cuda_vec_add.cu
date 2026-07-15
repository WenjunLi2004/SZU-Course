/* File:     cuda_vec_add.cu
 * Purpose:  Implement vector addition on a GPU using cuda.  This version
 *           uses unified memory in CUDA 6 or later.  So it must run
 *           on a system with compute capability >= 3.0
 *
 * Compile:  nvcc -arch=sm_30 -o cud_vec_add cuda_vec_add.cu 
 *              arch can be >= 30
 * Run:      ./cuda_vec_add <n> <blk_ct> <th_per_blk> <i|g>
 *              n is the vector length.  
 *              blk_ct is the number of thread blocks
 *              th_per_blk is the number of threads in each block
 *              'i':  indicates that the user will enter the two
 *                    vectors
 *              'g':  indicates that the program will use a random
 *                    number generator to generate the vectors
 *
 * Input:    If the command line has 'i' two n-component vectors.
 *           Otherwise there is no input.
 * Output:   If DEBUG is defined, the input and output vectors.
 *           Otherwise, just the two-norm of the difference between 
 *           the host- and device-computed results.
 *
 * Notes:
 * 1.  Define DEBUG if you want to see the input vectors and the
 *     the vectors computed by the device and the host.
 * 2.  The total number of threads, blk_ct*th_per_blk, should be
 *     greater than or equal to n
 *
 * IPP2:     6.8 (pp. 302 and ff.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cuda.h>

/* Host functions */
void Get_args(const int argc, char* argv[], int* n_p, int* blk_ct_p, 
      int* th_per_blk_p, char* i_g);
void Allocate_vectors(float** x_p, float** y_p, float** z_p, 
      float** cz_p, int n);
void Init_vectors(float x[], float y[], int n, char i_g);
void Read_vector(const char prompt[], float x[], const int n);
void Rand_vector(float x[], const int n);
void Print_vector(const char title[], const float x[], const int n);
void Serial_vec_add(const float x[], const float y[], float cz[], 
      const int n);
double Two_norm_diff(const float z[], const float cz[], const int n);
void Free_vectors(float* x, float* y, float* z, float* cz);

/*-------------------------------------------------------------------*/
/* Device code */
/* Kernel for vector addition */
__global__ void Vec_add(const float x[], const float y[], float z[], 
      const int n) {
   int my_elt = blockDim.x * blockIdx.x + threadIdx.x;

   /* block_count*threads_per_block may be >= n */
   if (my_elt < n) 
      z[my_elt] = x[my_elt] + y[my_elt];
}  /* Vec_add */


/*-------------------------------------------------------------------*/
/* Host code */
int main(int argc, char* argv[]) {
   int n, th_per_blk, blk_ct;
   char i_g;  /* Are x and y user input or random? */
   float *x, *y, *z, *cz;
   double diff_norm;

   Get_args(argc, argv, &n, &blk_ct, &th_per_blk, &i_g);

#  ifdef DEBUG
   printf("n = %d, blk_ct = %d, th_per_blk = %d, i_g = %c\n",
         n, blk_ct, th_per_blk, i_g);
#  endif

   Allocate_vectors(&x, &y, &z, &cz, n);
   Init_vectors(x, y, n, i_g);  

#  ifdef DEBUG
   Print_vector("x", x, n);
   Print_vector("y", y, n);
#  endif

   /* Invoke kernel using blk_ct blocks, each of which  */
   /* contains th_per_blk threads                       */
   Vec_add <<<blk_ct, th_per_blk>>>(x, y, z, n);

   /* Wait for the kernel to complete */
   cudaDeviceSynchronize();

   /* Check for correctness */
   Serial_vec_add(x, y, cz, n);
   diff_norm = Two_norm_diff(z, cz, n);
#  ifdef DEBUG
   Print_vector("z", z, n);
   Print_vector("cz", cz, n);
#  endif

   printf("Two-norm of difference between host and ");
   printf("device = %e\n", diff_norm);

   Free_vectors(x, y, z, cz);

   return 0;
}  /* main */


/*---------------------------------------------------------------------
 */
void Get_args(const int argc, char* argv[], int* n_p, int* blk_ct_p, 
      int* th_per_blk_p, char* i_g) {
   if (argc != 5) {
      fprintf(stderr, "usage: %s <n> <blk_ct> <th_per_blk> <i|g>\n",
            argv[0]);
      fprintf(stderr, "   n is the vector length\n");  
      fprintf(stderr, "   blk_ct is the number of thread blocks\n");
      fprintf(stderr, "   th_per_blk is the number of threads in each block\n");
      fprintf(stderr, "   'i':  indicates that the user will enter the two\n");
      fprintf(stderr, "      vectors\n");
      fprintf(stderr, "   'g':  indicates that the program will use a\n");
      fprintf(stderr, "   number generator to generate the vectors\n");
      exit(0);
   }
   *n_p = strtol(argv[1], NULL, 10);
   *blk_ct_p = strtol(argv[2], NULL, 10);
   *th_per_blk_p = strtol(argv[3], NULL, 10);
   *i_g = argv[4][0];

   if (*n_p > (*blk_ct_p)*(*th_per_blk_p)) {
      fprintf(stderr, "n should be <= blk_ct*th_per_blk\n");
      exit(0);
   }
}  /* Get_args */


/*---------------------------------------------------------------------
 * Function:  Allocate_vectors
 * Purpose:   Allocate storage for the vectors used in the program
 */
void Allocate_vectors(float** x_p, float** y_p, float** z_p, 
      float** cz_p, int n) {

   /* x, y, and z are used on host and device */
   cudaMallocManaged(x_p, n*sizeof(float));
   cudaMallocManaged(y_p, n*sizeof(float));
   cudaMallocManaged(z_p, n*sizeof(float));

   /* cz is only used on host */
   *cz_p = (float*) malloc(n*sizeof(float));
}  /* Allocate_vectors */


/*---------------------------------------------------------------------
 * Function:  Init_vectors
 * Purpose:   Initialize vectors x and y either by reading from stdin
 *            (i_g = 'i') or by using the C library random function
 *            (i_g = 'g')
 */
void Init_vectors(float x[], float y[], int n, char i_g) {

   if (i_g == 'i') {
      Read_vector("x", x, n);
      Read_vector("y", y, n);
   } else {
      srandom(1);
      Rand_vector(x, n);
      Rand_vector(y, n);
   }
}  /* Init_vectors */


/*---------------------------------------------------------------------
 * Function:  Read_vector
 * Purpose:   Read a vector from stdin
 */
void Read_vector(const char prompt[], float x[], const int n) {
   
   printf("Enter %s\n", prompt);
   for (int i = 0; i < n; i++)
      scanf("%f", &x[i]);
}  /* Read_vector */


/*---------------------------------------------------------------------
 * Function:  Rand_vector
 * Purpose:   Initialize a vector using the C library random function
 * Note:      Assumes srandom has been called before this function is
 *            called
 */
void Rand_vector(float x[], const int n) {
   for (int i = 0; i < n; i++)
      x[i] = random()/((double) RAND_MAX);
}  /* Rand_vector */


/*---------------------------------------------------------------------
 * Function:  Print_vector
 * Purpose:   Print a vector to stdout
 */
void Print_vector(const char title[], const float x[], const int n) {

   printf("%s = ", title);
   for (int i = 0; i < n; i++)
      printf("%.2f ", x[i]);
   printf("\n");
}  /* Print_vector */


/*---------------------------------------------------------------------
 * Function:  Serial_vec_add
 * Purpose:   Adds two vectors on host
 */
void Serial_vec_add(const float x[], const float y[], float cz[], 
      const int n) {
   for (int i = 0; i < n; i++)
      cz[i] = x[i] + y[i];
}  /* Serial_vec_add */


/*---------------------------------------------------------------------
 * Function:  Two_norm_diff
 * Purpose:   Find the two-norm of the difference between two vectors
 */
double Two_norm_diff(const float z[], const float cz[], const int n) {
   double diff, sum = 0.0;
   for (int i = 0; i < n; i++) {
      diff = z[i] - cz[i];
      sum += diff*diff;
   }
   return sqrt(sum);
}  /* Two_norm_diff */


/*---------------------------------------------------------------------
 * Function:  Free_vectors
 * Purpose:   Find the two-norm of the difference between two vectors
 */
void Free_vectors(float* x, float* y, float* z, float* cz) {
   cudaFree(x);
   cudaFree(y);
   cudaFree(z);
   free(cz);
}  /* Free_vectors */
