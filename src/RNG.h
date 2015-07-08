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

  double random();
  int irandom(int low, int high);

  double normal();
  double normal(double mu, double sigma);

  int draw_from_cdf(double *v, int size);
  int draw_from_cdf_vector(const std::vector <double>& v);
  int draw_from_distribution(int n, double *dist);
  double exponential(double lambda);
  double lognormal(double mu, double sigma);
  void build_binomial_cdf(double p, int n, std::vector<double> &cdf);
  void build_lognormal_cdf(double mu, double sigma, std::vector<double> &cdf);
  void sample_range_without_replacement(int N, int s, int* result);

 private:
  std::mt19937 mt_engine;
  std::uniform_real_distribution<double> unif_dist;
  std::normal_distribution<double> norm_dist;
};



#endif // _FRED_RNG_H
