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

Random::Random() {
  rng_list = new RNG [fred::omp_get_max_threads()];
}

void Random::initialize(unsigned long seed) {
  unsigned long new_seed = seed;
  for(int t = 0; t < fred::omp_get_max_threads(); ++t) {
    rng_list[t].initialize(new_seed);
    new_seed = new_seed * 17;
  }
}

#include "RNG.cc"


