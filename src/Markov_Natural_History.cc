/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "Condition.h"
#include "Global.h"
#include "Markov_Natural_History.h"
#include "Markov_Chain.h"
#include "Params.h"
#include "Utils.h"

class Person;


Markov_Natural_History::Markov_Natural_History() {
}


Markov_Natural_History::~Markov_Natural_History() {
}


void Markov_Natural_History::setup(Condition * _condition) {
  Natural_History::setup(_condition);
  this->markov_chain = new Markov_Chain;
  markov_chain->setup(this->condition->get_condition_name());
}


void Markov_Natural_History::get_parameters() {
  char paramstr[256];

  FRED_VERBOSE(0, "Markov_Natural_History::get_parameters\n");

  // Note: we skip get_parameters() in base class.

  // get the state definitions and transition probabilities
  markov_chain->get_parameters();

  this->state_infectivity.reserve(get_number_of_states());
  this->state_symptoms.reserve(get_number_of_states());
  this->state_fatality.reserve(get_number_of_states());
    
  this->state_infectivity.clear();
  this->state_symptoms.clear();
  this->state_fatality.clear();
    
  int fatal;
  double inf, symp;
  for (int i = 0; i < get_number_of_states(); i++) {

    sprintf(paramstr, "%s[%d].infectivity", get_name(), i);
    Params::get_param(paramstr, &inf);
    this->state_infectivity.push_back(inf);

    sprintf(paramstr, "%s[%d].symptoms", get_name(), i);
    Params::get_param(paramstr, &symp);
    this->state_symptoms.push_back(symp);

    sprintf(paramstr, "%s[%d].fatality", get_name(), i);
    Params::get_param(paramstr, &fatal);
    this->state_fatality.push_back(fatal);

  }
  // enable case_fatality, as specified by each state
  this->enable_case_fatality = 1;

}

char* Markov_Natural_History::get_name() {
  return markov_chain->get_name();
}

int Markov_Natural_History::get_number_of_states() {
  return markov_chain->get_number_of_states();
}

std::string Markov_Natural_History::get_state_name(int i) {
  return markov_chain->get_state_name(i);
}

void Markov_Natural_History::print() {
  markov_chain->print();
  for (int i = 0; i < get_number_of_states(); i++) {
    printf("MARKOV MODEL %s[%d].infectivity = %f\n",
	   get_name(), i, this->state_infectivity[i]);
    printf("MARKOV MODEL %s[%d].symptoms = %f\n",
	   get_name(), i, this->state_symptoms[i]);
    printf("MARKOV MODEL %s[%d].fatality = %d\n",
	   get_name(), i, this->state_fatality[i]);
  }
}

