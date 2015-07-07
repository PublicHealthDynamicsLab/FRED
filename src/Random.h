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

#define INIT_RANDOM(SEED) (Global::Random_Number_Generator.initialize(SEED))
#define RANDOM() Global::Random_Number_Generator.get_random()
#define DRAW_FROM_CDF(v,size) Global::Random_Number_Generator.draw_from_cdf(v,size)
#define DRAW_FROM_CDF_VECTOR(VEC) Global::Random_Number_Generator.draw_from_cdf_vector(VEC)
#define DRAW_FROM_DISTRIBUTION(n, dist) Global::Random_Number_Generator.draw_from_distribution(n,dist)

#define DRAW_EXPONENTIAL(lambda) Global::Random_Number_Generator.exponential(lambda)
#define DRAW_NORMAL(mu,sigma) Global::Random_Number_Generator.normal(mu,sigma)

#define BUILD_BINOMIAL_CDF(p,n,cdf) Global::Random_Number_Generator.build_binomial_cdf(p,n,cdf)
#define SAMPLE_RANGE_WITHOUT_REPLACEMENT(N,s,result) Global::Random_Number_Generator.sample_range_without_replacement(N,s,result)

#define IRAND(LOW,HIGH) ( (int) ( (LOW) + (int) ( ( (HIGH)-(LOW)+1 ) * RANDOM() ) ) )
#define IRAND_0_7() ( IRAND(0,7) )
#define URAND(LOW,HIGH) ((double)((LOW)+(((HIGH)-(LOW))*RANDOM())))
#define DRAW_UNIFORM(low,high) URAND(low,high)

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
