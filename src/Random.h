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

#include <math.h>
#include <vector>

#include "Global.h"

#define DSFMT_MEXP 19937

#include <dSFMT.h>
#include <dSFMT-params19937.h>

#include <random>

// non-member functions 

void INIT_RANDOM(unsigned long seed);
double RANDOM();
int draw_poisson(double lambda);
double draw_exponential(double lambda);
int draw_from_distribution(int n, double *dist);
double draw_standard_normal();
double draw_normal(double mu, double sigma);
double draw_lognormal(double mu, double sigma);
int draw_from_cdf(double *v, int size);
int draw_from_cdf_vector(const std::vector <double>& v);

#define IRAND(LOW,HIGH) ( (int) ( (LOW) + (int) ( ( (HIGH)-(LOW)+1 ) * RANDOM() ) ) )
#define URAND(LOW,HIGH) ((double)((LOW)+(((HIGH)-(LOW))*RANDOM())))
#define IRAND_0_7() ( IRAND(0,7) )
#define draw_uniform(low,high) URAND(low,high)


// BufferLengthChar must be multiple of 4

template< int BufferLengthDouble, int BufferLengthChar, int BufferLengthInt, int MaxBufferedInt >
struct RNG_State {

  double buffer_dbl[ BufferLengthDouble ];
  int buffer_index_dbl;
  dsfmt_t dsfmt_state_dbl;

  union union_int {
    struct union_char {
      unsigned char c0, c1, c2, c3;
    } uc;
    unsigned int i;
  };

  char buffer_char[ BufferLengthChar ];
  int buffer_index_char;
  dsfmt_t dsfmt_state_char;

  RNG_State() {
    buffer_index_dbl = BufferLengthDouble;
    buffer_index_char = BufferLengthChar;
    dsfmt_state_dbl = dsfmt_t();
    dsfmt_state_char = dsfmt_t();
  }
   
  void init( int seed ) {
    dsfmt_init_gen_rand( &dsfmt_state_char, seed );
    dsfmt_init_gen_rand( &dsfmt_state_dbl, seed );
  }

  double random_double() {
    if ( buffer_index_dbl == BufferLengthDouble ) {
      refresh_doubles_buffer(); 
    }
    return buffer_dbl[ buffer_index_dbl++ ];
  }

  unsigned char random_char() {
    if ( buffer_index_char == BufferLengthChar ) {
      refresh_chars_buffer(); 
    }
    return buffer_char[ buffer_index_char++ ];
  }

  void refresh_doubles_buffer() {
    dsfmt_fill_array_open_open( &dsfmt_state_dbl,
        buffer_dbl, BufferLengthDouble );
    buffer_index_dbl = 0;
  }

  void refresh_chars_buffer() {
    int buffer_start = BufferLengthChar - buffer_index_char;
    for ( int i = buffer_start; i < BufferLengthChar; i += 4 ) {
      union_int u;
      u.i = dsfmt_genrand_uint32( &dsfmt_state_char );
      buffer_char[ i + 0 ] = u.uc.c0;
      buffer_char[ i + 1 ] = u.uc.c1;
      buffer_char[ i + 2 ] = u.uc.c1;
      buffer_char[ i + 3 ] = u.uc.c1;
    }
    buffer_index_char = 0;
  }

  void refresh_all_buffers() {
    refresh_doubles_buffer();
    refresh_chars_buffer();
  }

};



struct RNG {
  static void init( int seed );
  static double random_double();
  static unsigned char random_char();
  static int random_int_0_7();
  static void refresh_all_buffers();
};


double binomial_coefficient( int n, int k );
void build_binomial_cdf( double p, int n, std::vector< double > & cdf );
void build_lognormal_cdf( double mu, double sigma, std::vector< double > & cdf );
void sample_range_without_replacement( int n, int s, int * result );

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
