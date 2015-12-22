/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

// File Markov_Infection.cc

#include "Markov_Infection.h"
#include "Markov_Natural_History.h"
#include "Disease.h"
#include "Global.h"
#include "Person.h"
#include "Mixing_Group.h"
#include "Utils.h"

Markov_Infection::Markov_Infection(Disease* _disease, Person* _infector, Person* _host, Mixing_Group* _mixing_group, int day) :
  Infection(_disease, _infector, _host, _mixing_group, day) {
  this->infectious_start_date = -1;
  this->infectious_end_date = -1;
  this->symptoms_start_date = -1;
  this->symptoms_end_date = -1;
  this->immunity_end_date = -1;
  this->infection_is_fatal_today = false;
  this->will_develop_symptoms = false;
}

void Markov_Infection::setup() {

  // Infection::setup();

    FRED_VERBOSE(1, "Markov_Infection::setup entered\n");

  // initialize Markov specific-variables here:

    this->state = disease->get_natural_history()->get_initial_state();
    printf("MARKOV INIT state %d\n", this->state);
    if (this->get_infectivity(this->exposure_date) > 0.0) {
      this->infectious_start_date = this->exposure_date;
      this->infectious_end_date = 99999;
    }
    if (this->get_symptoms(this->exposure_date) > 0.0) {
      this->symptoms_start_date = this->exposure_date;
      this->symptoms_end_date = 99999;
    }
}

double Markov_Infection::get_infectivity(int day) {
  return (disease->get_natural_history()->get_infectivity(this->state));
}

double Markov_Infection::get_symptoms(int day) {
  return (disease->get_natural_history()->get_symptoms(this->state));
}

bool Markov_Infection::is_fatal(int day) {
  return (disease->get_natural_history()->is_fatal(this->state));
}

void Markov_Infection::update(int day) {

  FRED_VERBOSE(1, "update Markov INFECTION on day %d for host %d\n", day, host->get_id());

  // put daily update here
}

