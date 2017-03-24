/* Coherent noise function over 1, 2 or 3 dimensions */
/* (copyright Ken Perlin) */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "perlin.h"
#include "distributions.h"


//extern long int seed;


static int p[B + B + 2];
static double g3[B + B + 2][3];
static double g2[B + B + 2][2];
static double g1[B + B + 2];
static int start = 1;

double noise1(double arg)
{
   int bx0, bx1;
   double rx0, rx1, sx, t, u, v, vec[1];

   vec[0] = arg;
   if (start) {
      start = 0;
      init();
   }

   setup_perlin(0,bx0,bx1,rx0,rx1);

   sx = s_curve(rx0);
   u = rx0 * g1[ p[ bx0 ] ];
   v = rx1 * g1[ p[ bx1 ] ];

   return(lerp(sx, u, v));
}



void normalize2(double v[2])
{
   double s;

   s = sqrt(v[0] * v[0] + v[1] * v[1]);
   v[0] = v[0] / s;
   v[1] = v[1] / s;
}

void normalize3(double v[3])
{
   double s;

   s = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
   v[0] = v[0] / s;
   v[1] = v[1] / s;
   v[2] = v[2] / s;
}

void init(void)
{
   int i, j, k;

   for (i = 0 ; i < B ; i++) {
      p[i] = i;
	  g1[i] = (double)((newRand() % (B + B)) - B) / B;

      for (j = 0 ; j < 2 ; j++)
         g2[i][j] = (double)((newRand() % (B + B)) - B) / B;
      normalize2(g2[i]);

      for (j = 0 ; j < 3 ; j++)
         g3[i][j] = (double)((newRand() % (B + B)) - B) / B;
      normalize3(g3[i]);
   }

   while (--i) {
      k = p[i];
      p[i] = p[j = newRand() % B];
      p[j] = k;
   }

   for (i = 0 ; i < B + 2 ; i++) {
      p[B + i] = p[i];
      g1[B + i] = g1[i];
      for (j = 0 ; j < 2 ; j++)
         g2[B + i][j] = g2[i][j];
      for (j = 0 ; j < 3 ; j++)
         g3[B + i][j] = g3[i][j];
   }
}

/* --- My harmonic summing functions - PDB --------------------------*/

/*
   In what follows "alpha" is the weight when the sum is formed.
   Typically it is 2, As this approaches 1 the function is noisier.
   "beta" is the harmonic scaling/spacing, typically 2.
*/

double PerlinNoise1D(double x,double alpha,double beta,int n)
{
   int i;
   double val,sum = 0;
   double p,scale = 1;

   p = x;
   for (i=0;i<n;i++) {
      val = noise1(p);
      sum += val / scale;
      scale *= alpha;
      p *= beta;
   }
   return(sum);
}

//Added by Kim to replace the math.h rand() function which gives different results on different operating systems
int newRand() 
{
  return (int)uniform_a_b(0, 32767, &HIV_Natural_History::seed);
}




