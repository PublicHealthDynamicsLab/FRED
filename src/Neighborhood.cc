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
// File: Neighborhood.cc
//

#include "Neighborhood.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"
#include "Condition.h"
#include "Condition_List.h"

//Private static variables that will be set by parameter lookups
double Neighborhood::contacts_per_day = 0.0;
double Neighborhood::same_age_bias = 0.0;
double** Neighborhood::prob_transmission_per_contact = NULL;
double Neighborhood::weekend_contact_rate = 0.0;

Neighborhood::Neighborhood(const char* lab, char _subtype, fred::geo lon, fred::geo lat) : Place(lab, lon, lat) {
  this->set_type(Place::TYPE_NEIGHBORHOOD);
  this->set_subtype(_subtype);
  this->intimacy = 0.0025;
}

void Neighborhood::get_parameters() {
  Params::get_param("neighborhood_contacts", &Neighborhood::contacts_per_day);
  Params::get_param("neighborhood_same_age_bias", &Neighborhood::same_age_bias);
  int n = Params::get_param_matrix((char *)"neighborhood_trans_per_contact", &Neighborhood::prob_transmission_per_contact);
  if(Global::Verbose > 1) {
    printf("\nNeighborhood_contact_prob:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      printf("%f ", Neighborhood::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
  }
  Params::get_param("weekend_contact_rate", &Neighborhood::weekend_contact_rate);

  if(Global::Verbose > 0) {
    printf("\nprob_transmission_per_contact before normalization:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      printf("%f ", Neighborhood::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
    printf("\ncontact rate: %f\n", Neighborhood::contacts_per_day);
  }

  // normalize contact parameters
  // find max contact prob
  double max_prob = 0.0;
  for(int i  = 0; i < n; ++i)  {
    for(int j  = 0; j < n; ++j) {
      if(Neighborhood::prob_transmission_per_contact[i][j] > max_prob) {
	      max_prob = Neighborhood::prob_transmission_per_contact[i][j];
      }
    }
  }

  // convert max contact prob to 1.0
  if(max_prob > 0) {
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      Neighborhood::prob_transmission_per_contact[i][j] /= max_prob;
      }
    }
    // compensate contact rate
    Neighborhood::contacts_per_day *= max_prob;
    // end normalization

    if(Global::Verbose > 0) {
      printf("\nprob_transmission_per_contact after normalization:\n");
      for(int i  = 0; i < n; ++i)  {
	      for(int j  = 0; j < n; ++j) {
	        printf("%f ", Neighborhood::prob_transmission_per_contact[i][j]);
	      }
	      printf("\n");
      }
      printf("\ncontact rate: %f\n", Neighborhood::contacts_per_day);
    }
    // end normalization
  }
}

int Neighborhood::get_group(int condition, Person* per) {
  int age = per->get_age();
  if(age < Global::ADULT_AGE) {
    return 0;
  } else {
    return 1;
  }
}

double Neighborhood::get_transmission_probability(int condition, Person* i, Person* s) {
  double age_i = i->get_real_age();
  double age_s = s->get_real_age();
  double diff = fabs(age_i - age_s);
  double prob = exp(-Neighborhood::same_age_bias * diff);
  return prob;
}

double Neighborhood::get_transmission_prob(int condition, Person* i, Person* s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(condition, i);
  int col = get_group(condition, s);
  double tr_pr = Neighborhood::prob_transmission_per_contact[row][col];
  return tr_pr;
}

double Neighborhood::get_contacts_per_day(int condition) {
  return Neighborhood::contacts_per_day;
}

