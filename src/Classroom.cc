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
double Classroom::contacts_per_day;
double** Classroom::prob_transmission_per_contact;
char Classroom::Classroom_closure_policy[80];
int Classroom::Classroom_closure_day = 0;
double Classroom::Classroom_closure_threshold = 0.0;
int Classroom::Classroom_closure_period = 0;
int Classroom::Classroom_closure_delay = 0;

Classroom::Classroom() : Place() {
  this->type = Place::CLASSROOM;
  this->subtype = fred::PLACE_SUBTYPE_NONE;
  this->age_level = -1;
  this->school = NULL;
}

Classroom::Classroom(const char* lab, fred::place_subtype _subtype, fred::geo lon, fred::geo lat) : Place(lab, lon, lat) {
  this->type = Place::CLASSROOM;
  this->subtype = _subtype;
  this->age_level = -1;
  this->school = NULL;
}

void Classroom::get_parameters() {

  Params::get_param_from_string("classroom_contacts", &Classroom::contacts_per_day);
  int n = Params::get_param_matrix((char *)"classroom_trans_per_contact", &Classroom::prob_transmission_per_contact);
  if(Global::Verbose > 1) {
    printf("\nClassroom_contact_prob:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      printf("%f ", Classroom::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
  }

  // normalize contact parameters
  // find max contact prob
  double max_prob = 0.0;
  for(int i  = 0; i < n; ++i)  {
    for(int j  = 0; j < n; ++j) {
      if (Classroom::prob_transmission_per_contact[i][j] > max_prob) {
	      max_prob = Classroom::prob_transmission_per_contact[i][j];
      }
    }
  }

  // convert max contact prob to 1.0
  if (max_prob > 0) {
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      Classroom::prob_transmission_per_contact[i][j] /= max_prob;
      }
    }
    // compensate contact rate
    Classroom::contacts_per_day *= max_prob;
  }

  if(Global::Verbose > 0) {
    printf("\nClassroom_contact_prob after normalization:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      printf("%f ", Classroom::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
    printf("\ncontact rate: %f\n", Classroom::contacts_per_day);
  }
  // end normalization
}

double Classroom::get_contacts_per_day(int disease) {
  return Classroom::contacts_per_day;
}

int Classroom::get_group(int disease, Person* per) {
  return this->school->get_group(disease, per);
}

double Classroom::get_transmission_prob(int disease, Person* i, Person* s) {

  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease, i);
  int col = get_group(disease, s);
  double tr_pr = Classroom::prob_transmission_per_contact[row][col];
  return tr_pr;
}

bool Classroom::is_open(int day) {
  bool open = this->school->is_open(day);
  if(!open) {
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
  if(this->age_level == -1) {
    this->age_level = age;
  }
  assert(grade == this->age_level);

  return return_value;
}

void Classroom::unenroll(int pos) {
  int size = this->enrollees.size();
  assert(0 <= pos && pos < size);
  Person* removed = this->enrollees[pos];
  assert(removed->is_teacher() == false);
  int grade = removed->get_grade();
  FRED_VERBOSE(0,"UNENROLL removed %d age %d grade %d, is_teacher %d from school %d %s size = %d\n",
	       removed->get_id(), removed->get_age(), grade, removed->is_teacher()?1:0, this->id, this->label, get_size());
  
  // call base class method
  Place::unenroll(pos);
}

