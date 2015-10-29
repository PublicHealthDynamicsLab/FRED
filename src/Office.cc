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
// File: Office.cc
//

#include "Office.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"
#include "Disease.h"
#include "Disease_List.h"
#include "Workplace.h"

//Private static variables that will be set by parameter lookups
double Office::contacts_per_day;
double** Office::prob_transmission_per_contact;

Office::Office() : Place() {
  this->set_type(Place::TYPE_OFFICE);
  this->set_subtype(Place::SUBTYPE_NONE);
  this->workplace = NULL;
}

Office::Office(const char* lab, char _subtype, fred::geo lon, fred::geo lat) : Place(lab, lon, lat) {
  this->set_type(Place::TYPE_OFFICE);
  this->set_subtype(_subtype);
  this->workplace = NULL;
}

void Office::get_parameters() {

  Params::get_param_from_string("office_contacts", &Office::contacts_per_day);
  int n = Params::get_param_matrix((char*)"office_trans_per_contact", &Office::prob_transmission_per_contact);
  if(Global::Verbose > 1) {
    printf("\nOffice_contact_prob:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      printf("%f ", Office::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
  }

  // normalize contact parameters
  // find max contact prob
  double max_prob = 0.0;
  for(int i  = 0; i < n; ++i)  {
    for(int j  = 0; j < n; ++j) {
      if (Office::prob_transmission_per_contact[i][j] > max_prob) {
	      max_prob = Office::prob_transmission_per_contact[i][j];
      }
    }
  }

  // convert max contact prob to 1.0
  if(max_prob > 0) {
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      Office::prob_transmission_per_contact[i][j] /= max_prob;
      }
    }
    // compensate contact rate
    Office::contacts_per_day *= max_prob;
  }

  if(Global::Verbose > 0) {
    printf("\nOffice_contact_prob after normalization:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      printf("%f ", Office::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
    printf("\ncontact rate: %f\n", Office::contacts_per_day);
  }
  // end normalization
}

int Office::get_container_size() {
  return this->workplace->get_size();
}

double Office::get_transmission_prob(int disease, Person* i, Person* s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease, i);
  int col = get_group(disease, s);
  double tr_pr = Office::prob_transmission_per_contact[row][col];
  return tr_pr;
}

double Office::get_contacts_per_day(int disease) {
  return Office::contacts_per_day;
}
