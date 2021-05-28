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
// File: Admin_Division.cc
//

#include "Admin_Division.h"
#include "Place.h"

Admin_Division::~Admin_Division() {
}

Admin_Division::Admin_Division(long long int _admin_code) {
  this->admin_code = _admin_code;
  this->households.clear();
  this->subdivisions.clear();
  this->higher = NULL;
}

void Admin_Division::setup() {
}

void Admin_Division::add_household(Place* hh) {
  this->households.push_back(hh);
  if (this->higher != NULL) {
    this->higher->add_household(hh);
  }
}
  

Place* Admin_Division::get_household(int i) {
  assert(0 <= i && i < this->households.size());
  return this->households[i];
}

int Admin_Division::get_population_size() {
  int size = this->households.size();
  int popsize = 0;
  for (int i = 0; i < size; i++) {
    popsize += this->households[i]->get_size();
  }
  return popsize;
}



