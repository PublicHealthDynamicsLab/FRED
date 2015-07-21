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
  void set_seed(unsigned long seed);
  double random() {
    return unif_dist(mt_engine);
  }
  int random_int(int low, int high) {
    return low + (int) ((high - low + 1) * random());
  }
  double exponential(double lambda);
  int draw_from_distribution(int n, double *dist);
  double normal(double mu, double sigma);
  double lognormal(double mu, double sigma);
  int draw_from_cdf(double *v, int size);
  int draw_from_cdf_vector(const std::vector <double>& v);
  void build_binomial_cdf(double p, int n, std::vector<double> &cdf);
  void build_lognormal_cdf(double mu, double sigma, std::vector<double> &cdf);
  void sample_range_without_replacement(int N, int s, int* result);

private:
  std::mt19937_64 mt_engine;
  std::uniform_real_distribution<double> unif_dist;
  std::normal_distribution<double> normal_dist;
};


class Thread_RNG {
public:
  Thread_RNG();

  void set_seed(unsigned long seed);
  double get_random() {
    return thread_rng[fred::omp_get_thread_num()].random();
  }
  double get_random(double low, double high) {
    return low + (high-low)*thread_rng[fred::omp_get_thread_num()].random();
  }
  int get_random_int(int low, int high) {
    return thread_rng[fred::omp_get_thread_num()].random_int(low,high);
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
  double lognormal(double mu, double sigma) {
    return thread_rng[fred::omp_get_thread_num()].lognormal(mu, sigma);
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

class Random {
public:
  static void set_seed(unsigned long seed) { 
    Random_Number_Generator.set_seed(seed);
  }
  static double draw_random() { 
    return Random_Number_Generator.get_random();
  }
  static double draw_random(double low, double high) { 
    return Random_Number_Generator.get_random(low,high);
  }
  static int draw_random_int(int low, int high) { 
    return Random_Number_Generator.get_random_int(low,high);
  }
  static double draw_exponential(double lambda) { 
    return Random_Number_Generator.exponential(lambda);
  }
  static double draw_normal(double mu, double sigma) { 
    return Random_Number_Generator.normal(mu,sigma);
  }
  static double draw_lognormal(double mu, double sigma) { 
    return Random_Number_Generator.lognormal(mu,sigma);
  }
  static int draw_from_cdf(double *v, int size) { 
    return Random_Number_Generator.draw_from_cdf(v,size);
  }
  static int draw_from_cdf_vector(const std::vector <double>& vec) { 
    return Random_Number_Generator.draw_from_cdf_vector(vec);
  }
  static int draw_from_distribution(int n, double *dist) { 
    return Random_Number_Generator.draw_from_distribution(n,dist);
  }
  static void build_binomial_cdf(double p, int n, std::vector<double> &cdf) { 
    Random_Number_Generator.build_binomial_cdf(p,n,cdf);
  }
  static void sample_range_without_replacement(int N, int s, int *result) { 
    Random_Number_Generator.sample_range_without_replacement(N,s,result);
  }

private:
  static Thread_RNG Random_Number_Generator;
};


template <typename T> 
void FYShuffle( std::vector <T> &array){
  int m,randIndx;
  T tmp;
  unsigned int n = array.size();
  m=n;
  while (m > 0){
    randIndx = (int)(Random::draw_random()*n);
    m--;
    tmp = array[m];
    array[m] = array[randIndx];
    array[randIndx] = tmp;
  }
}


#endif // _FRED_RANDOM_H
