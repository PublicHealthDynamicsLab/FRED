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
// File: Network.cc
//

#include "Network.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"
#include "Disease.h"
#include "Disease_List.h"

//Private static variables that will be set by parameter lookups
double Network::contacts_per_day;
double** Network::prob_transmission_per_contact;

Network::Network( const char *lab, fred::place_subtype _subtype, fred::geo lon, fred::geo lat) {
  this->type = Place::NETWORK;
  this->subtype = _subtype;
  setup( lab, lon, lat);
}

void Network::get_parameters() {

  Params::get_param_from_string("network_contacts", &Network::contacts_per_day);
  int n = Params::get_param_matrix((char *)"network_trans_per_contact", &Network::prob_transmission_per_contact);
  if(Global::Verbose > 1) {
    printf("\nNetwork_contact_prob:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	printf("%f ", Network::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
  }

  // normalize contact parameters
  // find max contact prob
  double max_prob = 0.0;
  for(int i  = 0; i < n; i++)  {
    for(int j  = 0; j < n; j++) {
      if (Network::prob_transmission_per_contact[i][j] > max_prob) {
	max_prob = Network::prob_transmission_per_contact[i][j];
      }
    }
  }

  // convert max contact prob to 1.0
  if (max_prob > 0) {
    for(int i  = 0; i < n; i++)  {
      for(int j  = 0; j < n; j++) {
	Network::prob_transmission_per_contact[i][j] /= max_prob;
      }
    }
    // compensate contact rate
    Network::contacts_per_day *= max_prob;
  }

  if(Global::Verbose > 0) {
    printf("\nNetwork_contact_prob after normalization:\n");
    for(int i  = 0; i < n; i++)  {
      for(int j  = 0; j < n; j++) {
	printf("%f ", Network::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
    printf("\ncontact rate: %f\n", Network::contacts_per_day);
  }
  // end normalization
}


double Network::get_transmission_prob(int disease, Person * i, Person * s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease, i);
  int col = get_group(disease, s);
  double tr_pr = Network::prob_transmission_per_contact[row][col];
  return tr_pr;
}

double Network::get_contacts_per_day(int disease) {
  return Network::contacts_per_day;
}

void Network::print() {
  char filename[64];
  sprintf(filename, "%s/%s.txt", Global::Simulation_directory, get_label());
  FILE *link_fileptr = fopen(filename,"w");
  sprintf(filename, "%s/%s-people.txt", Global::Simulation_directory, get_label());
  FILE *people_fileptr = fopen(filename,"w");
  int size = get_size();
  for (int i = 0; i < size; i++) {
    Person * person = get_enrollee(i);
    person->print_transmission_network(link_fileptr);
    person->print(people_fileptr,0);
  }
  fclose(link_fileptr);
  fclose(people_fileptr);
}
