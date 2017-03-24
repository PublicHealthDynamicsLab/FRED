/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Classroom.cc
//
#include "Classroom.h"
#include "Condition.h"
#include "Condition_List.h"
#include "Global.h"
#include "Mixing_Group.h"
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
  this->set_type(Place::TYPE_CLASSROOM);
  this->set_subtype(Place::SUBTYPE_NONE);
  this->age_level = -1;
  this->school = NULL;
}

Classroom::Classroom(const char* lab, char _subtype, fred::geo lon, fred::geo lat) : Place(lab, lon, lat) {
  this->set_type(Place::TYPE_CLASSROOM);
  this->set_subtype(_subtype);
  this->age_level = -1;
  this->school = NULL;
}

void Classroom::get_parameters() {

  Params::get_param("classroom_contacts", &Classroom::contacts_per_day);
  int n = Params::get_param_matrix((char*)"classroom_trans_per_contact", &Classroom::prob_transmission_per_contact);
  if(Global::Verbose > 1) {
   FRED_STATUS(0, "\nClassroom_contact_prob:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
        FRED_STATUS(0, "%f ", Classroom::prob_transmission_per_contact[i][j]);
      }
      FRED_STATUS(0, "\n");
    }
  }

  // normalize contact parameters
  // find max contact prob
  double max_prob = 0.0;
  for(int i  = 0; i < n; ++i)  {
    for(int j  = 0; j < n; ++j) {
      if(Classroom::prob_transmission_per_contact[i][j] > max_prob) {
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
    FRED_STATUS(0, "\nClassroom_contact_prob after normalization:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
        FRED_STATUS(0, "%f ", Classroom::prob_transmission_per_contact[i][j]);
      }
      FRED_STATUS(0, "\n");
    }
    FRED_STATUS(0, "\ncontact rate: %f\n", Classroom::contacts_per_day);
  }
  // end normalization
}

double Classroom::get_contacts_per_day(int condition) {
  return Classroom::contacts_per_day;
}

int Classroom::get_group(int condition, Person* per) {
  return this->school->get_group(condition, per);
}

double Classroom::get_transmission_prob(int condition, Person* i, Person* s) {

  // i = infected agent
  // s = susceptible agent
  int row = get_group(condition, i);
  int col = get_group(condition, s);
  double tr_pr = Classroom::prob_transmission_per_contact[row][col];
  return tr_pr;
}

bool Classroom::is_open(int day) {
  bool open = this->school->is_open(day);
  if(!open) {
    FRED_VERBOSE(0, "Place %s is closed on day %d\n", this->get_label(), day);
  }
  return open;
}

bool Classroom::should_be_open(int day, int condition) {
  return this->school->should_be_open(day, condition);
}

int Classroom::get_container_size() {
  return this->school->get_size();
}

int Classroom::enroll(Person* person) {
  assert(person->is_teacher() == false);

  // call base class method:
  int return_value = Mixing_Group::enroll(person);

  int age = person->get_age();
  int grade = ((age < Global::GRADES) ? age : Global::GRADES-1);
  assert(grade > 0);

  FRED_VERBOSE(1, "Enrolled person %d age %d in classroom %d grade %d %s\n",
	       person->get_id(), person->get_age(), this->get_id(), grade, this->get_label());

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
  FRED_VERBOSE(1, "UNENROLL removed %d age %d grade %d, is_teacher %d from school %d %s size = %d\n",
	       removed->get_id(), removed->get_age(), grade, removed->is_teacher()?1:0, this->get_id(), this->get_label(), this->get_size());
  
  // call base class method
  Mixing_Group::unenroll(pos);
}

void Classroom::set_school(School* _school) {
  this->school = _school;
  set_census_tract_fips(this->school->get_census_tract_fips());
}

