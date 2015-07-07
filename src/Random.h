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
#include "Global.h"
#include "RNG.h"

using namespace std;

class Random {
 public:
  Random();

  void initialize(unsigned long seed);

  double get_random() {
    return rng_list[fred::omp_get_thread_num()].draw_u01();
  }

  int draw_poisson(double lambda) {
    return rng_list[fred::omp_get_thread_num()].rng_draw_poisson(lambda);
  }

  double draw_exponential(double lambda) {
    return rng_list[fred::omp_get_thread_num()].rng_draw_exponential(lambda);
  }

  int draw_from_distribution(int n, double *dist) {
    return rng_list[fred::omp_get_thread_num()].rng_draw_from_distribution(n, dist);
  }

  double draw_standard_normal() {
    return rng_list[fred::omp_get_thread_num()].rng_draw_standard_normal();
  }

  double draw_normal(double mu, double sigma) {
    return rng_list[fred::omp_get_thread_num()].rng_draw_normal(mu, sigma);
  }

  double  draw_lognormal(double mu, double sigma) {
    return rng_list[fred::omp_get_thread_num()].rng_draw_lognormal(mu, sigma);
  }

  int draw_from_cdf(double *v, int size) {
    return rng_list[fred::omp_get_thread_num()].rng_draw_from_cdf(v, size);
  }

  int draw_from_cdf_vector(const std::vector <double>& v) {
    return rng_list[fred::omp_get_thread_num()].rng_draw_from_cdf_vector(v);
  }

  void build_binomial_cdf(double p, int n, std::vector<double> &cdf) {
    rng_list[fred::omp_get_thread_num()].rng_build_binomial_cdf(p, n, cdf);
  }

  void build_lognormal_cdf(double mu, double sigma, std::vector<double> &cdf) {
    rng_list[fred::omp_get_thread_num()].rng_build_lognormal_cdf(mu, sigma, cdf);
  }

  void sample_range_without_replacement(int N, int s, int* result) {
    rng_list[fred::omp_get_thread_num()].rng_sample_range_without_replacement(N, s, result);
  }

 private:
  RNG * rng_list;
};

#define INIT_RANDOM(SEED) (Global::Random_Number_Generator.initialize(SEED))
#define RANDOM() Global::Random_Number_Generator.get_random()
#define draw_from_cdf_vector(VEC) Global::Random_Number_Generator.draw_from_cdf_vector(VEC)
#define draw_from_distribution(n, dist) Global::Random_Number_Generator.draw_from_distribution(n,dist)
#define draw_poisson(lambda) Global::Random_Number_Generator.draw_poisson(lambda)
#define draw_exponential(lambda) Global::Random_Number_Generator.draw_exponential(lambda)
#define draw_standard_normal() Global::Random_Number_Generator.draw_standard_normal()
#define draw_normal(mu,sigma) Global::Random_Number_Generator.draw_normal(mu,sigma)
#define draw_lognormal(mu,sigma) Global::Random_Number_Generator.draw_lognormal(mu,sigma)
#define draw_from_cdf(v,size) Global::Random_Number_Generator.draw_from_cdf(v,size)
#define build_binomial_cdf(p,n,cdf) Global::Random_Number_Generator.build_binomial_cdf(p,n,cdf)
#define build_lognormal_cdf(mu,sigma,cdf) Global::Random_Number_Generator.build_lognormal_cdf(mu,sigma,cdf)
#define sample_range_without_replacement(N,s,result) Global::Random_Number_Generator.sample_range_without_replacement(N,s,result)


#define IRAND(LOW,HIGH) ( (int) ( (LOW) + (int) ( ( (HIGH)-(LOW)+1 ) * RANDOM() ) ) )
#define IRAND_0_7() ( IRAND(0,7) )
#define URAND(LOW,HIGH) ((double)((LOW)+(((HIGH)-(LOW))*RANDOM())))
#define draw_uniform(low,high) URAND(low,high)
#define REFRESH_RNG() 1

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
