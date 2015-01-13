/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
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



///////////// RANDOM NUMBER GENERATOR UTILITIES //////////////////////

/* Using Marseinne Twister MT19937 by T. Nishimura and M. Matsumoto */
/* See mt19937ar.c for acknowledgements */

//double genrand_real2();
//void init_genrand( );

//#define INIT_RANDOM(SEED)   init_gen_rand(SEED)
//#define RANDOM()        genrand_real2()

#define INIT_RANDOM(SEED)   RNG::init(SEED)
#define RANDOM()        RNG::random_double()

#define IRAND(LOW,HIGH) ( (int) ( (LOW) + (int) ( ( (HIGH)-(LOW)+1 ) * RANDOM() ) ) )
#define URAND(LOW,HIGH) ((double)((LOW)+(((HIGH)-(LOW))*RANDOM())))

#define IRAND_0_7() ( RNG::random_int_0_7() )




// BufferLengthChar must be multiple of 4

template< int BufferLengthDouble, int BufferLengthChar >
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
  dsfmt_t dsfmt_state_int;

  RNG_State() {
    buffer_index_dbl = BufferLengthDouble;
    buffer_index_char = BufferLengthChar;
    dsfmt_state_dbl = dsfmt_t();
    dsfmt_state_int = dsfmt_t();
  }
   
  void init( int seed ) {
    dsfmt_init_gen_rand( &dsfmt_state_int, seed );
    dsfmt_init_gen_rand( &dsfmt_state_dbl, seed );
  }

  double random_double() {
    if ( buffer_index_dbl == BufferLengthDouble ) {
      refresh_doubles_buffer(); 
    }
    return buffer_dbl[ ++buffer_index_dbl ];
  }

  unsigned char random_char() {
    if ( buffer_index_char == BufferLengthChar ) {
      refresh_chars_buffer(); 
    }
    return buffer_char[ ++buffer_index_char ];
  }

  void refresh_doubles_buffer() {
    dsfmt_fill_array_open_open( &dsfmt_state_dbl,
        buffer_dbl, BufferLengthDouble );
    buffer_index_dbl = -1;
  }

  void refresh_chars_buffer() {
    int buffer_start = BufferLengthChar - buffer_index_char;
    for ( int i = buffer_start; i < BufferLengthChar; i += 4 ) {
      union_int u;
      u.i = dsfmt_genrand_uint32( &dsfmt_state_int );
      buffer_char[ i + 0 ] = u.uc.c0;
      buffer_char[ i + 1 ] = u.uc.c1;
      buffer_char[ i + 2 ] = u.uc.c1;
      buffer_char[ i + 3 ] = u.uc.c1;
    }
    buffer_index_char = -1;
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


// non-member functions 

int draw_poisson(double lambda);
double draw_exponential(double lambda);
int draw_from_distribution(int n, double *dist);
double draw_standard_normal();
double draw_normal(double mu, double sigma);
int draw_from_cdf(double *v, int size);
int draw_from_cdf_vector(const std::vector <double>& v);

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
