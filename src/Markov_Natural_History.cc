/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "Disease.h"
#include "Global.h"
#include "Markov_Natural_History.h"
#include "Params.h"
#include "Utils.h"

class Person;


Markov_Natural_History::Markov_Natural_History() {
}


Markov_Natural_History::~Markov_Natural_History() {
}


void Markov_Natural_History::setup(Disease * _disease) {
  Natural_History::setup(_disease);
  Markov_Model::setup(this->disease->get_disease_name());
}


void Markov_Natural_History::get_parameters() {

  FRED_VERBOSE(0, "Markov_Natural_History::get_parameters\n");

  Natural_History::get_parameters();
  Markov_Model::get_parameters();

  this->state_infectivity.reserve(this->number_of_states);
  this->state_symptoms.reserve(this->number_of_states);
  this->state_fatality.reserve(this->number_of_states);
    
  this->state_infectivity.clear();
  this->state_symptoms.clear();
  this->state_fatality.clear();
    
  char paramstr[256];
  int fatal;
  double inf, symp;
  for (int i = 0; i < this->number_of_states; i++) {
    sprintf(paramstr, "%s[%d].infectivity", this->name, i);
    Params::get_param(paramstr, &inf);
    sprintf(paramstr, "%s[%d].symptoms", this->name, i);
    Params::get_param(paramstr, &symp);
    sprintf(paramstr, "%s[%d].fatality", this->name, i);
    Params::get_param(paramstr, &fatal);
    this->state_infectivity.push_back(inf);
    this->state_symptoms.push_back(symp);
    this->state_fatality.push_back(fatal);
  }

  // print
  for (int i = 0; i < this->number_of_states; i++) {
    printf("MARKOV MODEL %s[%d].infectivity = %f\n",
	   this->name, i, this->state_infectivity[i]);
    printf("MARKOV MODEL %s[%d].symptoms = %f\n",
	   this->name, i, this->state_symptoms[i]);
    printf("MARKOV MODEL %s[%d].fatality = %d\n",
	   this->name, i, this->state_fatality[i]);
  }
}


void Markov_Natural_History::prepare() {
  Markov_Model::prepare();
}


int Markov_Natural_History::get_initial_state() {
  return Markov_Model::get_initial_state();
}


void Markov_Natural_History::update_infection(int day, Person* host, Infection *infection) {
  // put daily updates to host here.
}

