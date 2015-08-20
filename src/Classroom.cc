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
#include "Disease.h"
#include "Disease_List.h"
#include "Global.h"
#include "Params.h"
#include "Person.h"
#include "Random.h"
#include "School.h"

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

Classroom::Classroom(const char *lab, fred::place_subtype _subtype, fred::geo lon, fred::geo lat) {
  this->type = Place::CLASSROOM;
  this->subtype = _subtype;
  setup(lab, lon, lat);
  get_parameters(Global::Diseases.get_number_of_diseases());
  this->age_level = -1;
}

void Classroom::get_parameters(int diseases) {
  if(Classroom::Classroom_parameters_set)
    return;

  Classroom::Classroom_contacts_per_day = new double[diseases];
  Classroom::Classroom_contact_prob = new double**[diseases];

  char param_str[80];
  for(int disease_id = 0; disease_id < diseases; disease_id++) {
    Disease * disease = Global::Diseases.get_disease(disease_id);
    sprintf(param_str, "%s_classroom_contacts", disease->get_disease_name());
    Params::get_param((char *) param_str, &Classroom::Classroom_contacts_per_day[disease_id]);
    if(Classroom::Classroom_contacts_per_day[disease_id] < 0) {
      Classroom::Classroom_contacts_per_day[disease_id] = (1.0 - Classroom::Classroom_contacts_per_day[disease_id])
	* School::get_school_contacts_per_day(disease_id);
    }

    sprintf(param_str, "%s_classroom_prob", disease->get_disease_name());
    int n = Params::get_param_matrix(param_str, &Classroom::Classroom_contact_prob[disease_id]);

    if(Global::Verbose > 0) {
      printf("\nClassroom_contact_prob before normalization:\n");
      for(int i  = 0; i < n; i++)  {
	for(int j  = 0; j < n; j++) {
	  printf("%f ", Classroom::Classroom_contact_prob[disease_id][i][j]);
	}
	printf("\n");
      }
      printf("\ncontact rate: %f\n", Classroom::Classroom_contacts_per_day[disease_id]);
    }

    // normalize contact parameters
    // find max contact prob
    double max_prob = 0.0;
    for(int i  = 0; i < n; i++)  {
      for(int j  = 0; j < n; j++) {
	if (Classroom::Classroom_contact_prob[disease_id][i][j] > max_prob) {
	  max_prob = Classroom::Classroom_contact_prob[disease_id][i][j];
	}
      }
    }

    // convert max contact prob to 1.0
    if (max_prob > 0) {
      for(int i  = 0; i < n; i++)  {
	for(int j  = 0; j < n; j++) {
	  Classroom::Classroom_contact_prob[disease_id][i][j] /= max_prob;
	}
      }
      // compensate contact rate
      Classroom::Classroom_contacts_per_day[disease_id] *= max_prob;
    }

    if(Global::Verbose > 0) {
      printf("\nClassroom_contact_prob after normalization:\n");
      for(int i  = 0; i < n; i++)  {
	for(int j  = 0; j < n; j++) {
	  printf("%f ", Classroom::Classroom_contact_prob[disease_id][i][j]);
	}
	printf("\n");
      }
      printf("\ncontact rate: %f\n", Classroom::Classroom_contacts_per_day[disease_id]);
    }
    // end normalization
  }
  Classroom::Classroom_parameters_set = true;
}

double Classroom::get_contacts_per_day(int disease) {
  return Classroom::Classroom_contacts_per_day[disease];
}

int Classroom::get_group(int disease, Person * per) {
  return this->school->get_group(disease, per);
}

double Classroom::get_transmission_prob(int disease, Person * i, Person * s) {

  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease, i);
  int col = get_group(disease, s);
  double tr_pr = Classroom::Classroom_contact_prob[disease][row][col];
  return tr_pr;
}

bool Classroom::is_open(int day) {
  bool open = this->school->is_open(day);
  if (!open) {
    FRED_VERBOSE(0,"Place %s is closed on day %d\n", this->label, day);
  }
  return open;
}

bool Classroom::should_be_open(int day, int disease) {
  return this->school->should_be_open(day, disease);
}

int Classroom::get_container_size() {
  return this->school->get_size();
}

int Classroom::enroll(Person* person) {
  assert(person->is_teacher() == false);

  // call base class method:
  int return_value = Place::enroll(person);

  int age = person->get_age();
  int grade = ((age < GRADES) ? age : GRADES - 1);
  assert(grade > 0);

  FRED_VERBOSE(1,"Enrolled person %d age %d in classroom %d grade %d %s\n",
	       person->get_id(), person->get_age(), this->id, this->age_level, this->label);
  if (this->age_level == -1) { this->age_level = age; }
  assert(grade == this->age_level);

  return return_value;
}

void Classroom::unenroll(int pos) {
  int size = enrollees.size();
  assert(0 <= pos && pos < size);
  Person *removed = enrollees[pos];
  assert(removed->is_teacher() == false);
  int grade = removed->get_grade();
  FRED_VERBOSE(0,"UNENROLL removed %d age %d grade %d, is_teacher %d from school %d %s size = %d\n",
	       removed->get_id(), removed->get_age(), grade, removed->is_teacher()?1:0, this->id, this->label, get_size());
  
  // call base class method
  Place::unenroll(pos);
}

