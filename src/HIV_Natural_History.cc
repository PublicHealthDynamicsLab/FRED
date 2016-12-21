/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "HIV_Natural_History.h"
#include "Condition.h"
#include "Params.h"
#include "Person.h"
#include "Random.h"
#include "Utils.h"

HIV_Natural_History::HIV_Natural_History() {
}

HIV_Natural_History::~HIV_Natural_History() {
}

void HIV_Natural_History::setup(Condition * _condition) {
  Natural_History::setup(_condition);
}

void HIV_Natural_History::get_parameters() {

  FRED_VERBOSE(0, "HIV::Natural_History::get_parameters\n");

  Natural_History::get_parameters();

  // Read in any file having to do with HIV natural history, such as
  // mortality rates

  // If you read in a value X here that should be accessible to
  // HIV_Infection, you should define a method in HIV_Natural_History.h
  // called get_X() that returns the value of X.  Then HIV_Infection can
  // access the value of X as follows:

  // int my_x = this->condition->get_natural_history()->get_X();

}



double HIV_Natural_History::get_probability_of_symptoms(int age) {
  return 1.0;
}

int HIV_Natural_History::get_latent_period(Person* host) {
  return 14;
}

int HIV_Natural_History::get_duration_of_infectiousness(Person* host) {
  // infectious forever
  return -1;
}

int HIV_Natural_History::get_duration_of_immunity(Person* host) {
  // immune forever
  return -1;
}

int HIV_Natural_History::get_incubation_period(Person* host) {
  return -1;
}

int HIV_Natural_History::get_duration_of_symptoms(Person* host) {
  // symptoms last forever
  return -1;
}


bool HIV_Natural_History::is_fatal(double real_age, double symptoms, int days_symptomatic) {
  return false;
}

bool HIV_Natural_History::is_fatal(Person* per, double symptoms, int days_symptomatic) {
  return false;
}


void HIV_Natural_History::update_infection(int day, Person* host, Infection *infection) {
  // put daily updates to host here.
}

