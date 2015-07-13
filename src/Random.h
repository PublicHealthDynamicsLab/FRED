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
// File: Random.h
//

#ifndef _FRED_RANDOM_H
#define _FRED_RANDOM_H

#include <vector>
#include <random>
#include "Global.h"
using namespace std;

class RNG {

 public:
  void initialize(unsigned long seed);
  double random();
  double exponential(double lambda);
  int draw_from_distribution(int n, double *dist);
  double standard_normal();
  double normal(double mu, double sigma);
  double lognormal(double mu, double sigma);
  int draw_from_cdf(double *v, int size);
  int draw_from_cdf_vector(const std::vector <double>& v);
  void build_binomial_cdf(double p, int n, std::vector<double> &cdf);
  void build_lognormal_cdf(double mu, double sigma, std::vector<double> &cdf);
  void sample_range_without_replacement(int N, int s, int* result);

 private:
  std::mt19937 mt_engine;
  std::uniform_real_distribution<double> u01;
};


class Random {
 public:
  Random();

  void initialize(unsigned long seed);

  double get_random() {
    return thread_rng[fred::omp_get_thread_num()].random();
  }

  int draw_from_cdf(double *v, int size) {
    return thread_rng[fred::omp_get_thread_num()].draw_from_cdf(v, size);
  }

  int draw_from_cdf_vector(const std::vector <double>& v) {
    return thread_rng[fred::omp_get_thread_num()].draw_from_cdf_vector(v);
  }

  int draw_from_distribution(int n, double *dist) {
    return thread_rng[fred::omp_get_thread_num()].draw_from_distribution(n, dist);
  }

  double exponential(double lambda) {
    return thread_rng[fred::omp_get_thread_num()].exponential(lambda);
  }

  double normal(double mu, double sigma) {
    return thread_rng[fred::omp_get_thread_num()].normal(mu, sigma);
  }

  void build_binomial_cdf(double p, int n, std::vector<double> &cdf) {
    thread_rng[fred::omp_get_thread_num()].build_binomial_cdf(p, n, cdf);
  }

  void sample_range_without_replacement(int N, int s, int* result) {
    thread_rng[fred::omp_get_thread_num()].sample_range_without_replacement(N, s, result);
  }

 private:
  RNG * thread_rng;
};

class FRED_Random {
 public:
  static Random Random_Number_Generator;
};

#define INIT_RANDOM(SEED) (FRED_Random::Random_Number_Generator.initialize(SEED))
#define RANDOM() FRED_Random::Random_Number_Generator.get_random()
#define IRAND(LOW,HIGH) ( (int) ( (LOW) + (int) ( ( (HIGH)-(LOW)+1 ) * RANDOM() ) ) )
#define IRAND_0_7() ( IRAND(0,7) )
#define URAND(LOW,HIGH) ((double)((LOW)+(((HIGH)-(LOW))*RANDOM())))
#define DRAW_UNIFORM(low,high) URAND(low,high)

#define DRAW_FROM_CDF_VECTOR(VEC) FRED_Random::Random_Number_Generator.draw_from_cdf_vector(VEC)
#define DRAW_FROM_DISTRIBUTION(n, dist) FRED_Random::Random_Number_Generator.draw_from_distribution(n,dist)
#define DRAW_EXPONENTIAL(lambda) FRED_Random::Random_Number_Generator.exponential(lambda)
#define DRAW_STANDARD_NORMAL() FRED_Random::Random_Number_Generator.standard_normal()
#define DRAW_NORMAL(mu,sigma) FRED_Random::Random_Number_Generator.normal(mu,sigma)
#define DRAW_LOGNORMAL(mu,sigma) FRED_Random::Random_Number_Generator.lognormal(mu,sigma)
#define DRAW_FROM_CDF(v,size) FRED_Random::Random_Number_Generator.draw_from_cdf(v,size)
#define BUILD_BINOMIAL_CDF(p,n,cdf) FRED_Random::Random_Number_Generator.build_binomial_cdf(p,n,cdf)
#define SAMPLE_RANGE_WITHOUT_REPLACEMENT(N,s,result) FRED_Random::Random_Number_Generator.sample_range_without_replacement(N,s,result)

template <typename T> 
void FYShuffle( std::vector <T> &array){
  int m,randIndx;
  T tmp;
  unsigned int n = array.size();
  m=n;
  while (m > 0){
    randIndx = (int)(RANDOM()*n);
    m--;
    tmp = array[m];
    array[m] = array[randIndx];
    array[randIndx] = tmp;
  }
}


#endif // _FRED_RANDOM_H
