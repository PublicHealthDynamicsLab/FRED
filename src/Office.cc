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
double * Office::Office_contacts_per_day;
double *** Office::Office_contact_prob;

Office::Office( const char *lab, fred::place_subtype _subtype, fred::geo lon, fred::geo lat) {
  this->type = Place::OFFICE;
  this->subtype = _subtype;
  setup( lab, lon, lat);
}

void Office::get_parameters() {

  int diseases = Global::Diseases.get_number_of_diseases();
  Office::Office_contacts_per_day = new double [ diseases ];
  Office::Office_contact_prob = new double** [ diseases ];
  
  char param_str[80];
  for(int disease_id = 0; disease_id < diseases; disease_id++) {
    Disease * disease = Global::Diseases.get_disease(disease_id);
    if (strcmp("respiratory",disease->get_transmission_mode())==0) {
      sprintf(param_str, "%s_office_contacts", disease->get_disease_name());
      Params::get_param((char *) param_str, &Office::Office_contacts_per_day[disease_id]);
      if(Office::Office_contacts_per_day[disease_id] < 0) {
	Office::Office_contacts_per_day[disease_id] = (1.0 - Office::Office_contacts_per_day[disease_id])
	  * Workplace::get_workplace_contacts_per_day(disease_id);
      }

      sprintf(param_str, "%s_office_prob", disease->get_disease_name());
      int n = Params::get_param_matrix(param_str, &Office::Office_contact_prob[disease_id]);

      if(Global::Verbose > 0) {
	printf("\nOffice_contact_prob before normalization:\n");
	for(int i  = 0; i < n; i++)  {
	  for(int j  = 0; j < n; j++) {
	    printf("%f ", Office::Office_contact_prob[disease_id][i][j]);
	  }
	  printf("\n");
	}
	printf("\ncontact rate: %f\n", Office::Office_contacts_per_day[disease_id]);
      }

      // normalize contact parameters
      // find max contact prob
      double max_prob = 0.0;
      for(int i  = 0; i < n; i++)  {
	for(int j  = 0; j < n; j++) {
	  if (Office::Office_contact_prob[disease_id][i][j] > max_prob) {
	    max_prob = Office::Office_contact_prob[disease_id][i][j];
	  }
	}
      }

      // convert max contact prob to 1.0
      if (max_prob > 0) {
	for(int i  = 0; i < n; i++)  {
	  for(int j  = 0; j < n; j++) {
	    Office::Office_contact_prob[disease_id][i][j] /= max_prob;
	  }
	}
	// compensate contact rate
	Office::Office_contacts_per_day[disease_id] *= max_prob;
      }

      if(Global::Verbose > 0) {
	printf("\nOffice_contact_prob after normalization:\n");
	for(int i  = 0; i < n; i++)  {
	  for(int j  = 0; j < n; j++) {
	    printf("%f ", Office::Office_contact_prob[disease_id][i][j]);
	  }
	  printf("\n");
	}
	printf("\ncontact rate: %f\n", Office::Office_contacts_per_day[disease_id]);
      }
      // end normalization
    }
  }
}

int Office::get_container_size() {
  return this->workplace->get_size();
}

double Office::get_transmission_prob(int disease, Person * i, Person * s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease, i);
  int col = get_group(disease, s);
  double tr_pr = Office::Office_contact_prob[disease][row][col];
  return tr_pr;
}

double Office::get_contacts_per_day(int disease) {
  return Office::Office_contacts_per_day[disease];
}
