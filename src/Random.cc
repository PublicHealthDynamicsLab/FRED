/*
 * This file is part of the FRED system.
 *
 * Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette, Shawn Brown, 
 * Roni Rosenfield, Alona Fyshe, David Galloway, Nathan Stone, Jay DePasse, 
 * Anuroop Sriram, and Donald Burke
 * All rights reserved.
 *
 * Copyright (c) 2013-2019, University of Pittsburgh, John Grefenstette, Robert Frankeny,
 * David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
 * All rights reserved.
 *
 * FRED is distributed on the condition that users fully understand and agree to all terms of the 
 * End User License Agreement.
 *
 * FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.
 *
 * See the file "LICENSE" for more information.
 */

//
//
// File: Random.cc
//
#include "Random.h"
#include <stdio.h>
#include <float.h>

Thread_RNG Random::Random_Number_Generator;

Thread_RNG::Thread_RNG() {
  thread_rng = new RNG [fred::omp_get_max_threads()];
}

void Thread_RNG::set_seed(unsigned long metaseed) {
  std::mt19937_64 seed_generator;
  seed_generator.seed(metaseed);
  for(int t = 0; t < fred::omp_get_max_threads(); ++t) {
    unsigned long new_seed = seed_generator();
    thread_rng[t].set_seed(new_seed);
  }
}


void RNG::set_seed(unsigned long seed) {
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
  assert(lambda > 0.0);
  double u = random();
  if (u > 0.0) {
    return (-log(u) / lambda);
  }
  else {
    return DBL_MAX;
  }
}

double RNG::normal(double mu, double sigma) {
  return mu + sigma * normal_dist(mt_engine);
}

double RNG::lognormal(double mu, double sigma) {
  // Notation as on https://en.wikipedia.org/wiki/Log-normal_distribution
  // mu = log(median)
  // sigma = log(dispersion)
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


