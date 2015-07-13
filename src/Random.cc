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
// File: Random.cc
//
#include "Random.h"
#include <stdio.h>

Random FRED_Random::Random_Number_Generator;

Random::Random() {
  thread_rng = new RNG [fred::omp_get_max_threads()];
}

void Random::initialize(unsigned long seed) {
  unsigned long new_seed = seed;
  for(int t = 0; t < fred::omp_get_max_threads(); ++t) {
    thread_rng[t].initialize(new_seed);
    new_seed = new_seed * 17;
  }
}


void RNG::initialize(unsigned long seed) {
  mt_engine.seed(seed);
}

int RNG::draw_from_distribution(int n, double* dist) {
  double r = random();
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

double RNG::exponential(double lambda) {
  double u = random();
  return (-log(u) / lambda);
}

double RNG::normal(double mu, double sigma) {
  return mu + sigma * normal_dist(mt_engine);
}

double RNG::lognormal(double mu, double sigma) {
  double z = normal(0.0,1.0);
  return exp(mu + sigma * z);
}


int RNG::draw_from_cdf(double* v, int size) {
  double r = random();
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

int RNG::draw_from_cdf_vector(const vector<double>& v) {
  int size = v.size();
  double r = random();
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

void RNG::build_binomial_cdf(double p, int n, std::vector<double> &cdf) {
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

void RNG::build_lognormal_cdf(double mu, double sigma, std::vector<double> &cdf) {
  int maxval = -1;
  int count[1000];
  for(int i = 0; i < 1000; i++) {
    count[i] = 0;
  }
  for(int i = 0; i < 1000; i++) {
    double x = lognormal(mu, sigma);
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

void RNG::sample_range_without_replacement(int N, int s, int* result) {
  std::vector<bool> selected(N, false);
  for(int n = 0; n < s; ++n) {
    int i = random_int(0, N - 1);
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


