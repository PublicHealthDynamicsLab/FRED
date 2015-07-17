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
// File: Perceptions.cc
//
#include "Global.h"
#include "Params.h"
#include "Perceptions.h"
#include "Random.h"
#include "Person.h"
#include "Disease.h"
#include "Disease_List.h"
#include "Household.h"

//Private static variables that will be set by parameter lookups
double Perceptions::memory_decay_distr[2] = { 0.0, 0.0 };
bool Perceptions::parameters_set = false;

Perceptions::Perceptions(Person *p) {
  this->self = p;

  if(Perceptions::parameters_set == false)
    get_parameters();

  // individual differences:
  this->memory_decay = Random::draw_normal(Perceptions::memory_decay_distr[0], Perceptions::memory_decay_distr[1]);
  if(this->memory_decay < 0.00001) {
    this->memory_decay = 0.00001;
  }

  this->perceived_susceptibility = new double[Global::Dis.get_number_of_diseases()];
  this->perceived_severity = new double[Global::Dis.get_number_of_diseases()];
  for(int b = 0; b < Behavior_index::NUM_BEHAVIORS; b++) {
    this->perceived_benefits[b] = new double[Global::Dis.get_number_of_diseases()];
    this->perceived_barriers[b] = new double[Global::Dis.get_number_of_diseases()];
  }

  for(int d = 0; d < Global::Dis.get_number_of_diseases(); d++) {
    this->perceived_susceptibility[d] = 0.0;
    this->perceived_severity[d] = 0.0;
    for(int b = 0; b < Behavior_index::NUM_BEHAVIORS; b++) {
      this->perceived_benefits[b][d] = 1.0;
      this->perceived_barriers[b][d] = 0.0;
    }
  }
}

void Perceptions::get_parameters() {
  char param_str[FRED_STRING_SIZE];
  sprintf(param_str, "memory_decay");
  int n = Params::get_param_vector(param_str, Perceptions::memory_decay_distr);
  if(n != 2) {
    Utils::fred_abort("bad %s\n", param_str);
  }
  Perceptions::parameters_set = true;
}

void Perceptions::update(int day) {
  update_perceived_severity(day);
  update_perceived_susceptibility(day);
  update_perceived_benefits(day);
  update_perceived_barriers(day);
}

void Perceptions::update_perceived_severity(int day) {
  for(int d = 0; d < Global::Dis.get_number_of_diseases(); d++) {
    this->perceived_severity[d] = 1.0;
  }
}

void Perceptions::update_perceived_susceptibility(int day) {
  for(int d = 0; d < Global::Dis.get_number_of_diseases(); d++) {
    this->epidemic = Global::Dis.get_disease(d)->get_epidemic();
    this->perceived_susceptibility[d] = 100.0 * this->epidemic->get_symptomatic_prevalence();
    // printf("update_per_sus: %f\n", perceived_susceptibility[d]);
  }
}

void Perceptions::update_perceived_benefits(int day) {
  return;
}

void Perceptions::update_perceived_barriers(int day) {
  return;
}
