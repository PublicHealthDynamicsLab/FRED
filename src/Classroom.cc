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
// File: Classroom.cc
//
#include "Classroom.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"
#include "Disease.h"
#include "Disease_List.h"

//Private static variables that will be set by parameter lookups
double * Classroom::Classroom_contacts_per_day;
double *** Classroom::Classroom_contact_prob;
char Classroom::Classroom_closure_policy[80];
int Classroom::Classroom_closure_day = 0;
double Classroom::Classroom_closure_threshold = 0.0;
int Classroom::Classroom_closure_period = 0;
int Classroom::Classroom_closure_delay = 0;

//Private static variable to assure we only lookup parameters once
bool Classroom::Classroom_parameters_set = false;

Classroom::Classroom(const char *lab, fred::place_subtype _subtype, fred::geo lon, fred::geo lat, Place *container) {
  this->type = Place::CLASSROOM;
  this->subtype = _subtype;
  setup(lab, lon, lat, container);
  get_parameters(Global::Diseases.get_number_of_diseases());
  this->age_level = -1;
}

void Classroom::get_parameters(int diseases) {
  if(Classroom::Classroom_parameters_set)
    return;

  if (Global::Enable_Vector_Transmission == false) {
    Classroom::Classroom_contacts_per_day = new double[diseases];
    Classroom::Classroom_contact_prob = new double**[diseases];

    char param_str[80];
    for(int disease_id = 0; disease_id < diseases; disease_id++) {
      Disease * disease = Global::Diseases.get_disease(disease_id);
      sprintf(param_str, "%s_classroom_contacts", disease->get_disease_name());
      Params::get_param((char *) param_str, &Classroom::Classroom_contacts_per_day[disease_id]);
      if(Classroom::Classroom_contacts_per_day[disease_id] < 0) {
	Classroom::Classroom_contacts_per_day[disease_id] = (1.0 - Classroom::Classroom_contacts_per_day[disease_id])
          * this->container->get_contacts_per_day(disease_id);
      }

      sprintf(param_str, "%s_classroom_prob", disease->get_disease_name());
      int n = Params::get_param_matrix(param_str, &Classroom::Classroom_contact_prob[disease_id]);
      if(Global::Verbose > 1) {
	printf("\nclassroom_contact_prob:\n");
	for(int i  = 0; i < n; i++)  {
	  for(int j  = 0; j < n; j++) {
	    printf("%f ", Classroom::Classroom_contact_prob[disease_id][i][j]);
	  }
	  printf("\n");
	}
      }
    }
  }

  Classroom::Classroom_parameters_set = true;
}

int Classroom::get_group(int disease, Person * per) {
  return this->container->get_group(disease, per);
}

double Classroom::get_transmission_prob(int disease, Person * i, Person * s) {

  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease, i);
  int col = get_group(disease, s);
  double tr_pr = Classroom::Classroom_contact_prob[disease][row][col];
  return tr_pr;
}

bool Classroom::should_be_open(int day, int disease) {
  return this->container->should_be_open(day, disease);
}

double Classroom::get_contacts_per_day(int disease) {
  return Classroom::Classroom_contacts_per_day[disease];
}

// Only student get enrolled in a classroom. Teachers are only enrolled in the school.
void Classroom::enroll(Person * per) {
  int age = per->get_age();
  if (age >= GRADES) age = GRADES - 1;
  assert(per->is_teacher() == false);
  if (get_enrollee_index(per) == -1) {
    if (N == enrollees.size()) {
      // double capacity if needed (to reduce future reallocations)
      enrollees.reserve(2*N);
    }
    this->enrollees.push_back(per);
    this->N++;
    FRED_VERBOSE(1,"Enroll person %d age %d in classroom %d age_level %d %s\n", per->get_id(), per->get_age(), this->id, this->age_level, this->label);
    if (this->age_level == -1) { this->age_level = age; }
    assert(age == this->age_level);
  }
  else {
    FRED_VERBOSE(1,"Enroll EC_WARNING person %d already in classroom %d %s\n", per->get_id(), this->id, this->label);
  }
}

void Classroom::unenroll(Person * per) {
  assert(per->is_teacher() == false);
  FRED_VERBOSE(1,"Unenroll person %d age %d from classroom %d %s size = %d\n",
	       per->get_id(), per->get_age(), this->id, this->label, N);
  int i = get_enrollee_index(per);
  if (i == -1) {
    FRED_VERBOSE(0,"Unenroll WARNING person %d age %d not found in classroom %d %s size = %d\n",
		 per->get_id(), per->get_age(), this->id, this->label, N);
    assert(i != -1);
  }
  else {
    enrollees.erase(enrollees.begin()+i);
    this->N--;
    FRED_VERBOSE(1,"Unenrolled. Size = %d\n", N);
  }
}

