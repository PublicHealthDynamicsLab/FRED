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
  char state_description_file[256];
  char paramstr[256];
  char disease_name[20];
  int n;

  strcpy(disease_name, this->disease->get_disease_name());

  Params::get_indexed_param(disease_name,"state_description_file", state_description_file);
  FILE *fp = NULL;
  fp = Utils::fred_open_file(state_description_file);
  if (fp == NULL) {
    fprintf(Global::Statusfp, "State description file %s not found\n", state_description_file);
    exit(1);
  }
  fscanf(fp, " Number of states = %d ", &(this->number_of_states));
  this->transition_matrix = new double * [this->number_of_states];
  this->state_name.reserve(this->number_of_states);
  this->state_infectivity.reserve(this->number_of_states);
  this->state_symptoms.reserve(this->number_of_states);
    
  double initial_total = 0.0;
  for (int i = 0; i < this->number_of_states; i++) {
    int j;
    char str[80];
    double inf, symp, init;
    int fatal;
    fscanf(fp, " state %d: ", &j);
    assert(i==j);
    assert (fscanf(fp, " name = %s ", str) == 1);
    assert (fscanf(fp, " infectivity = %lf ", &inf) == 1);
    assert (fscanf(fp, " symptoms = %lf ", &symp) == 1);
    assert (fscanf(fp, " fatality = %d ", &fatal) == 1);
    assert (fscanf(fp, " initial_percent = %lf ", &init) == 1);
    this->state_name.push_back(str);
    this->state_infectivity.push_back(inf);
    this->state_symptoms.push_back(symp);
    this->state_fatality.push_back(fatal);
    this->state_initial_percent.push_back(init);
    initial_total += init;
  }
  assert(initial_total == 100.0);
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
  print();
}


void Markov_Natural_History::print() {
  for (int i = 0; i < this->number_of_states; i++) {
    printf("MARKOV State %d: name = %s\n", i, this->state_name[i].c_str());
    printf("MARKOV State %d %s: infectivity = %f\n", i, this->state_name[i].c_str(), this->state_infectivity[i]);
    printf("MARKOV State %d %s: symptoms = %f\n", i, this->state_name[i].c_str(), this->state_symptoms[i]);
    printf("MARKOV State %d %s: fatality = %d\n", i, this->state_name[i].c_str(), this->state_fatality[i]);
    printf("MARKOV State %d %s: initial_percent = %f\n", i, this->state_name[i].c_str(), this->state_initial_percent[i]);
  }
  for (int i = 0; i < this->number_of_states; i++) {
    for (int j = 0; j < this->number_of_states; j++) {
      printf("MARKOV prob %d to %d: %f\n",i,j,this->transition_matrix[i][j]);
    }
  }
}

int Markov_Natural_History::get_initial_state() {
  double r = 100.0 * Random::draw_random();
  double sum = 0.0;
  for (int i = 0; i < this->number_of_states; i++) {
    sum += this->state_initial_percent[i];
    if (r < sum) {
      return i;
    }
  }  
  assert(r < sum);
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

