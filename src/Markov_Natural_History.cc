/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "Markov_Natural_History.h"
#include "Disease.h"
#include "Params.h"
#include "Person.h"
#include "Population.h"
#include "Random.h"
#include "Utils.h"

Markov_Natural_History::Markov_Natural_History() {
}

Markov_Natural_History::~Markov_Natural_History() {
}

void Markov_Natural_History::setup(Disease * _disease) {
  Natural_History::setup(_disease);
}

void Markov_Natural_History::get_parameters() {

  FRED_VERBOSE(0, "Markov::Natural_History::get_parameters\n");

  Natural_History::get_parameters();

  // read in the disease-specific parameters
  char state_transition_file[256];
  char paramstr[256];
  char disease_name[20];
  int n;

  strcpy(disease_name, this->disease->get_disease_name());

  Params::get_indexed_param(disease_name,"state_transition_file", state_transition_file);
  FILE *fp = NULL;
  fp = Utils::fred_open_file(state_transition_file);
  if (fp == NULL) {
    fprintf(Global::Statusfp, "State transition file %s not found\n", state_transition_file);
    exit(1);
  }
  fscanf(fp, " Number of states = %d ", &(this->number_of_states));
  this->transition_matrix = new double * [this->number_of_states];
  this->state_name.reserve(this->number_of_states);
    
  for (int i = 0; i < this->number_of_states; i++) {
    int j;
    char str[80];
    fscanf(fp, " state %d = %s", &j, str);
    assert(i==j);
    this->state_name.push_back(str);
  }
  for (int i = 0; i < this->number_of_states; i++) {
    this->transition_matrix[i] = new double [number_of_states];
    for (int j = 0; j < this->number_of_states; j++) {
      this->transition_matrix[i][j] = 0.0;
    }
  }
  fscanf(fp, " state transition probabilities: ");
  int i, j;
  double p;
  while (fscanf(fp, " %d %d %lf ", &i, &j, &p) == 3) {
    this->transition_matrix[i][j] = p;
    // printf("read %d to %d: %f\n",i,j,p);
  }
  // gaurantee probability distribution by making same-state transition the default
  for (int i = 0; i < this->number_of_states; i++) {
    double sum = 0;
    for (int j = 0; j < this->number_of_states; j++) {
      if (i != j) {
	sum += this->transition_matrix[i][j];
      }
    }
    assert(sum <= 1.0);
    this->transition_matrix[i][i] = 1.0 - sum;
  }
  fclose(fp);
  // print();
}


void Markov_Natural_History::print() {
  for (int i = 0; i < this->number_of_states; i++) {
    printf("State %d = %s\n", i, this->state_name[i].c_str());
  }
  for (int i = 0; i < this->number_of_states; i++) {
    for (int j = 0; j < this->number_of_states; j++) {
      printf("%d to %d: %f\n",i,j,this->transition_matrix[i][j]);
    }
  }
  exit(0);
}


double Markov_Natural_History::get_probability_of_symptoms(int age) {
  return 1.0;
}

int Markov_Natural_History::get_latent_period(Person* host) {
  return -1;
}

int Markov_Natural_History::get_duration_of_infectiousness(Person* host) {
  // infectious forever
  return -1;
}

int Markov_Natural_History::get_duration_of_immunity(Person* host) {
  // immune forever
  return -1;
}

int Markov_Natural_History::get_incubation_period(Person* host) {
  return -1;
}

int Markov_Natural_History::get_duration_of_symptoms(Person* host) {
  // symptoms last forever
  return -1;
}


bool Markov_Natural_History::is_fatal(double real_age, double symptoms, int days_symptomatic) {
  return false;
}

bool Markov_Natural_History::is_fatal(Person* per, double symptoms, int days_symptomatic) {
  return false;
}


void Markov_Natural_History::update_infection(int day, Person* host, Infection *infection) {
  // put daily updates to host here.
}

