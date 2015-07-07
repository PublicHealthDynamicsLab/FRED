/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: RNG.cc
//
#include "Random.h"
#include "RNG.h"
#include <stdio.h>

void RNG::initialize(unsigned long seed) {
  mt_engine.seed(seed);
}

double RNG::draw_u01() {
  return u01(mt_engine);
}

int RNG::rng_draw_from_distribution(int n, double* dist) {
  double r = draw_u01();
  int i = 0;
  while(i <= n && dist[i] < r) {
    i++;
  }
  if(i <= n) {
    return i;
  } else {
    printf("Help! draw from distribution failed.\n");
    printf("Is distribution properly formed? (should end with 1.0)\n");
    for(int i = 0; i <= n; i++) {
      printf("%f ", dist[i]);
    }
    printf("\n");
    return -1;
  }
}

double RNG::rng_draw_exponential(double lambda) {
  double u = draw_u01();
  return (-log(u) / lambda);
}

#define TWOPI (2.0*3.141592653589)

double RNG::rng_draw_standard_normal() {
  // Box-Muller method
  double U = draw_u01();
  double V = draw_u01();
  return (sqrt(-2.0 * log(U)) * cos(TWOPI * V));
}

double RNG::rng_draw_normal(double mu, double sigma) {
  return mu + sigma * draw_standard_normal();
}


double RNG::rng_draw_lognormal(double mu, double sigma) {
  double z = draw_standard_normal();
  return exp(mu + sigma * z);
}


int RNG::rng_draw_from_cdf(double* v, int size) {
  double r = draw_u01();
  int top = size - 1;
  int bottom = 0;
  int s = top / 2;
  while(bottom <= top) {
    if(r <= v[s]) {
      if(s == 0 || r > v[s - 1]) {
        return s;
      } else {
        top = s - 1;
      }
    } else { // r > v[s]
      if(s == size - 1) {
        return s;
      }
      if(r < v[s + 1]) {
        return s + 1;
      } else {
        bottom = s + 1;
      }
    }
    s = bottom + (top - bottom)/2;
  }
  // assert(bottom <= top);
  return -1;
}

int RNG::rng_draw_from_cdf_vector(const vector<double>& v) {
  int size = v.size();
  double r = draw_u01();
  int top = size - 1;
  int bottom = 0;
  int s = top / 2;
  while(bottom <= top) {
    if(r <= v[s]) {
      if(s == 0 || r > v[s - 1]) {
        return s;
      } else {
        top = s - 1;
      }
    } else { // r > v[s]
      if(s == size - 1) {
        return s;
      }
      if(r < v[s + 1]) {
        return s + 1;
      } else {
        bottom = s + 1;
      }
    }
    s = bottom + (top - bottom) / 2;
  }
  return -1;
}

/*
   algorithm poisson random number (Knuth):
init:
Let L = exp(−lambda), k = 0 and p = 1.
do:
k = k + 1.
Generate uniform random number u in [0,1] and let p = p × u.
while p > L.
return k − 1.
*/

int RNG::rng_draw_poisson(double lambda) {
  if(lambda <= 0.0) {
    return 0;
  } else {
    double L = exp(-lambda);
    int k = 0;
    double p = 1.0;
    do {
      p *= draw_u01();
      k++;
    } while(p > L);
    return k - 1;
  }
}

double binomial_coefficient(int n, int k) {
  if(k < 0 ||  k > n) {
    return 0;
  }
  if(k > n - k)  {
    k = n - k;
  }
  double c = 1.0;
  for(int i = 0; i < k; ++i) {
    c = c * (n - (k - (i + 1)));
    c = c / (i + 1);
  }
  return c;
}

void RNG::rng_build_binomial_cdf(double p, int n, std::vector<double> &cdf) {
  for(int i = 0; i <= n; ++i) {
    double prob = 0.0;
    for(int j = 0; j <= i; ++j) {
      prob += binomial_coefficient(n, i)
        * pow(10, ((i * log10(p)) + ((n - 1) * log10(1 - p))));
    }
    if(i > 0) {
      prob += cdf.back();
    }
    if(prob < 1) {
      cdf.push_back(prob);
    } else {
      cdf.push_back(1.0);
      break;
    }
  }
  cdf.back() = 1.0;
}

void RNG::rng_build_lognormal_cdf(double mu, double sigma, std::vector<double> &cdf) {
  int maxval = -1;
  int count[1000];
  for(int i = 0; i < 1000; i++) {
    count[i] = 0;
  }
  for(int i = 0; i < 1000; i++) {
    double x = draw_lognormal(mu, sigma);
    int j = (int) x + 0.5;
    if(j > 999) {
      j = 999;
    }
    count[j]++;
    if(j > maxval) {
      maxval = j;
    }
  }
  for(int i = 0; i <= maxval; ++i) {
    double prob = (double) count[i] / 1000.0;
    if(i > 0) {
      prob += cdf.back();
    }
    if(prob < 1.0) {
      cdf.push_back( prob );
    } else {
      cdf.push_back(1.0);
      break;
    }
  }
  cdf.back() = 1.0;
}

void RNG::rng_sample_range_without_replacement(int N, int s, int* result) {
  std::vector<bool> selected(N, false);
  for(int n = 0; n < s; ++n) {
    int i = IRAND(0, N - 1);
    if(selected[i]) {
      if(i < N - 1 && !(selected[i + 1])) {
        ++i;
      } else if(i > 0 && !(selected[i - 1])) {
        --i;
      } else {
        --n;
        continue;
      }
    }
    selected[i] = true;
    result[n] = i;
  }
}

