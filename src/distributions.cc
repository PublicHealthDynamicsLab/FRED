#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "distributions.h"

static unsigned long mt[L]; /* the array for the state vector  */
static int mti=L+1; /* mti==L+1 means mt[L] is not initialized */

int init_genrand(unsigned long s)
{
    mt[0]= s & 0xffffffffUL;
    for (mti=1; mti<L; mti++) {
        mt[mti] = 
	    (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti); 
        /* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
        /* In the previous versions, MSBs of the seed affect   */
        /* only MSBs of the array mt[].                        */
        /* 2002/01/09 modified by Makoto Matsumoto             */
        mt[mti] &= 0xffffffffUL;
        /* for >32 bit machines */
    }
  return 1000;    //mina added additionally to test if this is called!
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
/* slight change for C++, 2004/2/26 */
void init_by_array(unsigned long init_key[], int key_length)
{
    int i, j, k;
    init_genrand(19650218UL);
    i=1; j=0;
    k = (L>key_length ? L : key_length);
    for (; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))
          + init_key[j] + j; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++; j++;
        if (i>=L) { mt[0] = mt[L-1]; i=1; }
        if (j>=key_length) j=0;
    }
    for (k=L-1; k; k--) {
        mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))
          - i; /* non linear */
        mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
        i++;
        if (i>=L) { mt[0] = mt[L-1]; i=1; }
    }

    mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */ 
}

/* generates a random number on [0,0xffffffff]-interval */
unsigned long genrand_int32(void)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0UL, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= L) { /* generate N words at one time */
        int kk;

        if (mti == L+1)   /* if init_genrand() has not been called, */
            init_genrand(5489UL); /* a default initial seed is used */

        for (kk=0;kk<L-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<L-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-L)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (mt[L-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[L-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        mti = 0;
    }
  
    y = mt[mti++];

    /* Tempering */
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

	return y;
}


/* generates a random number on [0,1]-real-interval */
double genrand_real1(void)
{
	unsigned long test;
	test = genrand_int32();

    return test*(1.0/4294967295.0); 
    /* divided by 2^32-1 */ 
}

double uniform(long int *seed)		//from Law & Kelton p.430-431
{ 
	double test;
	test = genrand_real1();

//  fprintf(Global::mina_test, "\nuniform: %f", test);      //mina

    return test;//genrand_real1();//retval;
} 

double uniform_a_b(double low, double high, long int *seed)
{
	double unif, range, retval;
	
	unif = uniform(seed);
  
//  fprintf(Global::mina_test, "\nuniform_a_b: %f", unif);      //mina

	range = high - low;
	retval = low + unif*range;
	return retval;
}

/* returns a r.v. from exp distribution with mean = mean */
double exponential(double mean, long int *seed)
{
	double unif;

	unif = uniform(seed);
	return (-1*mean*log(unif));
}

//method obtained from Law and Kelton
double normal(double mean, double sd, long int *seed)
{
	double u1, u2, v1, v2, w=2, y, std_normal, reg_normal;  //w=2 just for starting while loop;

	while (w > 1)
	{
		u1 = uniform(seed);
		u2 = uniform(seed);
		v1 = 2*u1 - 1;
		v2 = 2*u2 - 1;
		w = pow(v1,2) + pow(v2,2);
	}

	y = sqrt((-2*log(w))/w);

	std_normal = v1*y;
	reg_normal = mean + std_normal*sd;
	//printf ("Called normal!\n");
	return reg_normal;
}

//from Law and Kelton
//generates a rv from a gamma(a1,1) distribution
double gamma_a1_1(double a1, long int *seed)
{
	double unif, b, y, p, u1, u2, a, q, d, theta, v, z, w;

	if (a1 == 1) //exponential distrib with mean = 1
	{
		return exponential(1,seed);
	}
	else if (a1 < 1)
	{
		b = (exp(1.0) + a1)/exp(1.0);
		//this will run until a value is returned
		while (1)
		{
			unif = uniform(seed);
			p = b*unif;
			if (p > 1)
			{
				y = -1*log((b-p)/a1);
				unif = uniform(seed);
				if (unif <= pow(y,a1-1))
					return y;
			}
			else
			{
				y = pow(p,(1/a1));
				unif = uniform(seed);
				if (unif <= exp(-y))
					return y;
			}
		}
		
		//should never get here
		printf("Error in gamma_a1_1 (a1 < 1)\n");
		exit(0);
	}
	else //a1 > 1
	{
		a = 1/sqrt(2*a1 - 1);
		b = a1 - log(4.0);
		q = a1 + (1/a);
		theta = 4.5;
		d = 1 + log(theta);

		while (1)
		{
			u1 = uniform(seed);
			u2 = uniform(seed);
			v = a*log(u1/(1-u1));
			y = a1*exp(v);
			z = pow(u1,2)*u2;
			w = b + q*v - y;
			if ((w + d - theta*z) >= 0)
				return y;
			else if (w >= log(z))
				return y;
		}
		//should never get here
		printf("Error in gamma_a1_1 (a1>1) \n");
		exit(0);
	}
}


double gamma_a1_a2(double a1, double a2, long int *seed)
{
	return (a2*gamma_a1_1(a1, seed));
}

//from Law and Kelton:
double beta(double a1, double a2, long int *seed)
{
	double gamma1, gamma2, retval;

	gamma1 = gamma_a1_1(a1, seed);
	gamma2 = gamma_a1_1(a2, seed);

	retval = gamma1/(gamma1 + gamma2);
	return retval;
}

/* The following cdf functions were taken from "A Short Note on the Numerical Approximation of the Standard Normal Cumulative Distribution and It's Inverse," by Chokri Dridi.  REAL 03-T-7, March 2003.*/

/* cdf function */
long double cdf(long double x){
	if(x>=0.){
		return (1.+GL(0,x/sqrt(2.)))/2.;
	}
	else {
		return (1.-GL(0,-x/sqrt(2.)))/2.;
	}
}

/* Integration on a closed interval */
long double GL(long double a, long double b)
{
	long double y1=0, y2=0, y3=0, y4=0, y5=0;

	long double x1=-sqrt(245.+14.*sqrt(70.))/21., x2=-sqrt(245.-14.*sqrt(70.))/21.;
	long double x3=0, x4=-x2, x5=-x1;
	long double w1=(322.-13.*sqrt(70.))/900., w2=(322.+13.*sqrt(70.))/900.;
	long double w3=128./225.,w4=w2,w5=w1;
	int n=4800;
	long double i=0, s=0, h=(b-a)/n;

	for (i=0;i<=n;i++){
		y1=h*x1/2.+(h+2.*(a+i*h))/2.;
		y2=h*x2/2.+(h+2.*(a+i*h))/2.;
		y3=h*x3/2.+(h+2.*(a+i*h))/2.;
		y4=h*x4/2.+(h+2.*(a+i*h))/2.;
		y5=h*x5/2.+(h+2.*(a+i*h))/2.;
		s=s+h*(w1*f(y1)+w2*f(y2)+w3*f(y3)+w4*f(y4)+w5*f(y5))/2.;
	}
	return s;
}

/* Function f, to integrate */
long double f(long double x){
	return (2./sqrt(22./7.))*exp(-pow(x,(long double)2.));
}



