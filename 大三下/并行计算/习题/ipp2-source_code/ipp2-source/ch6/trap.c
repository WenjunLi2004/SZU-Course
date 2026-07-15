/* File:    trap.c
 * Purpose: Calculate definite integral using trapezoidal 
 *          rule.
 *
 * Input:   a, b, n
 * Output:  Estimate of integral from a to b of f(x)
 *          using n trapezoids.
 *
 * Compile: gcc -g -Wall -o trap trap.c
 * Usage:   ./trap
 *
 * Notes:    
 *    1.  The function f(x) is hardwired.
 *    2.  This is very similar to the trap.c program
 *        in Chapters 3 and 5,   However, it uses floats
 *        instead of floats.
 *
 * IPP2:  6.10.1 (pp. 314 and ff.)
 */

#include <stdio.h>

float f(const float x);    /* Function we're integrating */
float Trap(const float a, const float b, const int n);

int main(void) {
   float  integral;   /* Store result in integral   */
   float  a, b;       /* Left and right endpoints   */
   int    n;          /* Number of trapezoids       */

   printf("Enter a, b, and n\n");
   scanf("%f", &a);
   scanf("%f", &b);
   scanf("%d", &n);

   integral = Trap(a, b, n);
   
   printf("With n = %d trapezoids, our estimate\n", n);
   printf("of the integral from %f to %f = %f\n",
      a, b, integral);

   return 0;
}  /* main */

/*------------------------------------------------------------------
 * Function:    Trap
 * Purpose:     Estimate integral from a to b of f using trap rule and
 *              n trapezoids
 * Input args:  a, b, n
 * Return val:  Estimate of the integral 
 */
float Trap(const float a, const float b, const int n) {
   float x, h = (b-a)/n;
   float trap = 0.5*(f(a) + f(b));

   for (int i = 1; i <= n-1; i++) {
      x = a + i*h;
      trap += f(x);
   }
   trap = trap*h;

   return trap;
}  /* Trap */

/*------------------------------------------------------------------
 * Function:    f
 * Purpose:     Compute value of function to be integrated
 * Input args:  x
 */
float f(const float x) {
   float return_val;

   return_val = x*x;
   return return_val;
}  /* f */
