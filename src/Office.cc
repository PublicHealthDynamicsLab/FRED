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

//Private static variables that will be set by parameter lookups
double * Office::Office_contacts_per_day;
double *** Office::Office_contact_prob;

//Private static variable to assure we only lookup parameters once
bool Office::Office_parameters_set = false;

Office::Office( const char *lab, fred::place_subtype _subtype, fred::geo lon, fred::geo lat, Place *container) {
  this->type = Place::OFFICE;
  this->subtype = _subtype;
  assert(container != NULL);
  setup( lab, lon, lat, container);
  get_parameters(Global::Diseases.get_number_of_diseases());
}

void Office::get_parameters(int diseases) {
  if (Office::Office_parameters_set) return;
  
  if (Global::Enable_Vector_Transmission == false) {
    Office::Office_contacts_per_day = new double [ diseases ];
    Office::Office_contact_prob = new double** [ diseases ];
  
    char param_str[80];
    for(int disease_id = 0; disease_id < diseases; disease_id++) {
      Disease * disease = Global::Diseases.get_disease(disease_id);
      sprintf(param_str, "%s_office_contacts", disease->get_disease_name());
      Params::get_param((char *) param_str, &Office::Office_contacts_per_day[disease_id]);
      if(Office::Office_contacts_per_day[disease_id] < 0) {
	Office::Office_contacts_per_day[disease_id] = (1.0 - Office::Office_contacts_per_day[disease_id])
          * this->container->get_contacts_per_day(disease_id);
      }

      sprintf(param_str, "%s_office_prob", disease->get_disease_name());
      int n = Params::get_param_matrix(param_str, &Office::Office_contact_prob[disease_id]);
      if(Global::Verbose > 1) {
	printf("\noffice_contact_prob:\n");
	for(int i  = 0; i < n; i++)  {
	  for(int j  = 0; j < n; j++) {
	    printf("%f ", Office::Office_contact_prob[disease_id][i][j]);
	  }
	  printf("\n");
	}
      }
    }
  }

  Office::Office_parameters_set = true;
}

int Office::get_group(int disease, Person * per) {
  return 0;
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
