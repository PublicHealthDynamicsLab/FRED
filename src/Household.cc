/*
   Copyright 2009 by the University of Pittsburgh
   Licensed under the Academic Free License version 3.0
   See the file "LICENSE" for more information
   */

//
//
// File: Household.cc
//

#include "Household.h"
#include "Global.h"
#include "Params.h"
#include "Person.h"
#include "Cell.h"
#include "Grid.h"
#include "Utils.h"
#include "Random.h"
#include "Transmission.h"
#include "Large_Cell.h"
#include "Large_Grid.h"

//Private static variables that will be set by parameter lookups
double * Household::Household_contacts_per_day;
double *** Household::Household_contact_prob;

//Private static variable to assure we only lookup parameters once
bool Household::Household_parameters_set = false;

Household::Household( const  char *lab, fred::geo lon,
    fred::geo lat, Place *container, Population* pop ) {
  type = HOUSEHOLD;
  setup(lab, lon, lat, container, pop);
  get_parameters(Global::Diseases);
  housemate.clear();
  adults = children = 0;
  N = 0; 
}

void Household::get_parameters(int diseases) {
  char param_str[80];

  if (Household::Household_parameters_set) return;
  Household::Household_contacts_per_day = new double [ diseases ];
  Household::Household_contact_prob = new double** [ diseases ];
  for (int s = 0; s < diseases; s++) {
    int n;
    sprintf(param_str, "household_contacts[%d]", s);
    Params::get_param((char *) param_str, &Household::Household_contacts_per_day[s]);
    sprintf(param_str, "household_prob[%d]", s);
    n = Params::get_param_matrix(param_str, &Household::Household_contact_prob[s]);
    if (Global::Verbose > 1) {
      printf("\nHousehold_contact_prob:\n");
      for (int i  = 0; i < n; i++)  {
        for (int j  = 0; j < n; j++) {
          printf("%f ", Household::Household_contact_prob[s][i][j]);
        }
        printf("\n");
      }
    }
  }
  Household::Household_parameters_set = true;
}

int Household::get_group(int disease, Person * per) {
  int age = per->get_age();
  if (age < Global::ADULT_AGE) { return 0; }
  else { return 1; }
}

double Household::get_transmission_prob(int disease, Person * i, Person * s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease, i);
  int col = get_group(disease, s);
  double tr_pr = Household::Household_contact_prob[disease][row][col];
  return tr_pr;
}

double Household::get_contacts_per_day(int disease) {
  return Household::Household_contacts_per_day[disease];
}

void Household::enroll(Person * per) {
  int age = per->get_age();
  N++;
  if (age < Global::ADULT_AGE)
    children++;
  else {
    adults++;
  }
  if (Global::Verbose>1) {
    printf("Add person %d to household %d\n", per->get_id(), get_id()); fflush(stdout);
  }
  housemate.push_back(per);
  // for (int i = 0; i < N; i++)
  // printf("%d ", housemate[i]->get_id()); 
  // printf("\n"); fflush(stdout);
  if (N == 1) {
    grid_cell->add_occupied_house();
  }
}

void Household::unenroll(Person * per) {
  int age = per->get_age();
  if (Global::Verbose>1) {
    printf("Removing person %d age %d from household %d\n",
        per->get_id(), age, get_id()); fflush(stdout);
    for (int i = 0; i < N; i++)
      printf("%d ", housemate[i]->get_id()); 
    printf("\n"); fflush(stdout);
    fflush(stdout);
  }

  // erase from housemates
  vector <Person *>::iterator it;
  for (it = housemate.begin(); *it != per && it != housemate.end(); it++);
  if (*it == per) {
    housemate.erase(it);
    N--;
    if (Global::Verbose>1) {
      printf("Removed person %d from household %d\n", per->get_id(), get_id());
      for (int i = 0; i < N; i++)
        printf("%d ", housemate[i]->get_id()); 
      printf("\n\n"); fflush(stdout);
    }
    if (N == 0) { 
      Global::Cells->add_vacant_house(this);
      grid_cell->subtract_occupied_house();
    }
  }
  else {
    printf("Removing person %d age %d from household %d\n",
        per->get_id(), age, get_id()); fflush(stdout);
    printf("Household::unenroll -- Help! unenrolled person not found in housemate list\n");
    for (int i = 0; i < N; i++)
      printf("%d ", housemate[i]->get_id()); 
    printf("\n"); fflush(stdout);
    Utils::fred_abort("");
  }

  // unenroll from cell as well
  if(Global::Enable_Large_Grid) {
    Large_Cell *large_cell = Global::Large_Cells->get_grid_cell_from_lat_lon(latitude, longitude);
    if(large_cell != NULL) large_cell->unenroll(per);
  }
}

void Household::record_profile() {
  // record the ages in sorted order
  ages.clear();
  for (int i = 0; i < N; i++)
    ages.push_back(housemate[i]->get_age()); 
  sort(ages.begin(), ages.end());

  // record the id's of the original members of the household
  ids.clear();
  for (int i = 0; i < N; i++)
    ids.push_back(housemate[i]->get_id()); 
}


void Household::spread_infection(int day, int disease_id) {

  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp,"spread_infection: Disease %d day %d place %d type %c\n",
        disease_id, day, id, type);
    fflush(Global::Statusfp);
  }

  vector<Person *>::iterator itr;
  double contact_prob = get_contact_rate(day,disease_id);

  // randomize the order of the infectious list
  FYShuffle<Person *>(infectious[disease_id]);

  for (itr = infectious[disease_id].begin(); itr != infectious[disease_id].end(); itr++) {

    Person * infector = *itr;      // infectious indiv
    assert(infector->get_health()->is_infectious(disease_id));

    for (int pos = 0; pos < S[disease_id]; pos++) {
      Person * infectee = susceptibles[disease_id][pos];

      if (Global::Verbose > 1) {
        fprintf(Global::Statusfp,"possible infectee = %d  pos = %d  S[%d] = %d\n",
            infectee->get_id(), pos, disease_id, S[disease_id]);
        fflush(Global::Statusfp);
      }

      // is the target still susceptible?
      if (infectee->is_susceptible(disease_id)) {

        // get the transmission probs for this infector/infectee pair
        double transmission_prob = get_transmission_prob(disease_id, infector, infectee);

        if (Global::Verbose > 1) {
          fprintf(Global::Statusfp,"infectee is susceptible\n");
          fprintf(Global::Statusfp,"trans_prob = %f\n", transmission_prob);
          fflush(Global::Statusfp);
        }

        double infectivity = infector->get_infectivity(disease_id,day);

        // scale transmission prob by infectivity and contact prob
        transmission_prob *= infectivity * contact_prob;     

        attempt_transmission(transmission_prob, infector, infectee, disease_id, day);

      } // end of susceptible infectee
    } // end contact loop
  } // end infectious list loop
}

