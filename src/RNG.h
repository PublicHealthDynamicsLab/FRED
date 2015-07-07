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
// File: RNG.h
//

#ifndef _FRED_RNG_H
#define _FRED_RNG_H

#include <random>
using namespace std;

class RNG {

 public:
  void initialize(unsigned long seed);
  double draw_u01();
  int rng_draw_poisson(double lambda);
  double rng_draw_exponential(double lambda);
  int rng_draw_from_distribution(int n, double *dist);
  double rng_draw_standard_normal();
  double rng_draw_normal(double mu, double sigma);
  double rng_draw_lognormal(double mu, double sigma);
  int rng_draw_from_cdf(double *v, int size);
  int rng_draw_from_cdf_vector(const std::vector <double>& v);
  void rng_build_binomial_cdf(double p, int n, std::vector<double> &cdf);
  void rng_build_lognormal_cdf(double mu, double sigma, std::vector<double> &cdf);
  void rng_sample_range_without_replacement(int N, int s, int* result);

 private:
  std::mt19937 mt_engine;
  std::uniform_real_distribution<double> u01;
};



#endif // _FRED_RNG_H
