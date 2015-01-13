/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
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
#include "Disease.h"

//Private static variables that will be set by parameter lookups
double * Neighborhood::Neighborhood_contacts_per_day = NULL;
double *** Neighborhood::Neighborhood_contact_prob = NULL;
double * Neighborhood::Weekend_contact_rate = NULL;

//Private static variable to assure we only lookup parameters once
bool Neighborhood::Neighborhood_parameters_set = false;

Neighborhood::Neighborhood( const char *lab, double lon,
                           double lat, Place *container, Population *pop ) {
  type = NEIGHBORHOOD;
  setup( lab, lon, lat, container, pop );
  get_parameters(Global::Diseases);
}

void Neighborhood::get_parameters(int diseases) {
  char param_str[80];
  
  if (Neighborhood::Neighborhood_parameters_set) return;
  
  Neighborhood::Weekend_contact_rate = new double [ diseases ];
  Neighborhood::Neighborhood_contacts_per_day = new double [ diseases ];
  Neighborhood::Neighborhood_contact_prob = new double** [ diseases ];
  
  for (int s = 0; s < diseases; s++) {
    int n;
    sprintf(param_str, "weekend_contact_rate[%d]", s);
    Params::get_param((char *) param_str, &Neighborhood::Weekend_contact_rate[s]);
    sprintf(param_str, "neighborhood_contacts[%d]", s);
    Params::get_param((char *) param_str, &Neighborhood::Neighborhood_contacts_per_day[s]);
    
    sprintf(param_str, "neighborhood_prob[%d]", s);
    n = Params::get_param_matrix(param_str, &Neighborhood::Neighborhood_contact_prob[s]);
    if (Global::Verbose > 1) {
      printf("\nNeighborhood_contact_prob:\n");
      for (int i  = 0; i < n; i++)  {
        for (int j  = 0; j < n; j++) {
          printf("%f ", Neighborhood::Neighborhood_contact_prob[s][i][j]);
        }
        printf("\n");
      }
    }
  }

  Neighborhood::Neighborhood_parameters_set = true;
}

int Neighborhood::get_group(int disease, Person * per) {
  int age = per->get_age();
  if (age < Global::ADULT_AGE) { return 0; }
  else { return 1; }
}

double Neighborhood::get_transmission_prob(int disease, Person * i, Person * s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease, i);
  int col = get_group(disease, s);
  double tr_pr = Neighborhood::Neighborhood_contact_prob[disease][row][col];
  return tr_pr;
}

double Neighborhood::get_contacts_per_day(int disease) {
  return Neighborhood::Neighborhood_contacts_per_day[disease];
}
