/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
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

//Private static variables that will be set by parameter lookups
double * Office::Office_contacts_per_day;
double *** Office::Office_contact_prob;

//Private static variable to assure we only lookup parameters once
bool Office::Office_parameters_set = false;

Office::Office( const char *lab, double lon, double lat, Place *container, Population *pop ) {
  type = OFFICE;
  assert(container != NULL);
  setup( lab, lon, lat, container, pop );
  get_parameters(Global::Diseases);
}

void Office::get_parameters(int diseases) {
  char param_str[80];
  
  if (Office::Office_parameters_set) return;
  
  Office::Office_contacts_per_day = new double [ diseases ];
  Office::Office_contact_prob = new double** [ diseases ];
  
  for (int s = 0; s < diseases; s++) {
    int n;
    sprintf(param_str, "office_contacts[%d]", s);
    Params::get_param((char *) param_str, &Office::Office_contacts_per_day[s]);
    if (Office::Office_contacts_per_day[s] == -1) {
      Office::Office_contacts_per_day[s] = 2.0 * container->get_contacts_per_day(s);
    }
    
    sprintf(param_str, "office_prob[%d]", s);
    n =  Params::get_param_matrix(param_str, &Office::Office_contact_prob[s]);
    if (Global::Verbose > 1) {
      printf("\nOffice_contact_prob:\n");
      for (int i  = 0; i < n; i++)  {
        for (int j  = 0; j < n; j++) {
          printf("%f ", Office::Office_contact_prob[s][i][j]);
        }
        printf("\n");
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
