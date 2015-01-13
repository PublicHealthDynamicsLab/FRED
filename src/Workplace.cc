/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Workplace.cc
//

#include "Workplace.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"
#include "Disease.h"
#include "Place_List.h"
#include "Office.h"
#include "Utils.h"

//Private static variables that will be set by parameter lookups
double * Workplace::Workplace_contacts_per_day;
double *** Workplace::Workplace_contact_prob;
int Workplace::Office_size = 50;
int Workplace::Small_workplace_size = 50;
int Workplace::Medium_workplace_size = 100;
int Workplace::Large_workplace_size = 500;

//Private static variable to assure we only lookup parameters once
bool Workplace::Workplace_parameters_set = false;

//Private static variables for population level statistics
int Workplace::workers_in_small_workplaces = 0;
int Workplace::workers_in_medium_workplaces = 0;
int Workplace::workers_in_large_workplaces = 0;
int Workplace::workers_in_xlarge_workplaces = 0;
int Workplace::total_workers = 0;

Workplace::Workplace(const char *lab, double lon, double lat, Place *container, Population *pop) {
  type = WORKPLACE;
  setup(lab, lon, lat, container, pop);
  get_parameters(Global::Diseases);
  offices.clear();
  next_office = 0;
}

void Workplace::get_parameters(int diseases) {
  char param_str[80];
  
  if (Workplace::Workplace_parameters_set) return;
  
  Workplace::Workplace_contacts_per_day = new double [ diseases ];
  Workplace::Workplace_contact_prob = new double** [ diseases ];
  
  // people per office
  Params::get_param_from_string("office_size", &Workplace::Office_size);

  // workplace size limits
  Params::get_param_from_string("small_workplace_size", &Workplace::Small_workplace_size);
  Params::get_param_from_string("medium_workplace_size", &Workplace::Medium_workplace_size);
  Params::get_param_from_string("large_workplace_size", &Workplace::Large_workplace_size);

  for (int s = 0; s < diseases; s++) {
    int n;
    sprintf(param_str, "workplace_contacts[%d]", s);
    Params::get_param((char *) param_str, &Workplace::Workplace_contacts_per_day[s]);
    
    sprintf(param_str, "workplace_prob[%d]", s);
    n = Params::get_param_matrix(param_str, &Workplace::Workplace_contact_prob[s]);
    if (Global::Verbose > 1) {
      printf("\nWorkplace_contact_prob:\n");
      for (int i  = 0; i < n; i++) {
        for (int j  = 0; j < n; j++) {
          printf("%f ", Workplace::Workplace_contact_prob[s][i][j]);
        }
        printf("\n");
      }
    }
  }
  Workplace::Workplace_parameters_set = true;
}

// this method is called after all workers are assigned to workplaces
void Workplace::prepare() {

  // update employment stats based on size of workplace
  if (N < Workplace::Small_workplace_size) {
    Workplace::workers_in_small_workplaces += N;
  }
  else if (N < Workplace::Medium_workplace_size) {
    Workplace::workers_in_medium_workplaces += N;
  }
  else if (N < Workplace::Large_workplace_size) {
    Workplace::workers_in_large_workplaces += N;
  }
  else {
    Workplace::workers_in_xlarge_workplaces += N;
  }
  Workplace::total_workers += N;

  // reserve memory for place-specific agent lists
  for (int s = 0; s < Global::Diseases; s++) {
    //susceptibles[s].reserve(N);
    infectious[s].reserve(N);
    total_cases[s] = total_deaths[s] = 0;
  }
  open_date = 0;
  close_date = INT_MAX;
  next_office = 0;
  update(0);

  if (Global::Verbose > 2) {
    printf("prepare place: %d\n", id);
    print(0);
    fflush(stdout);
  }

}

double Workplace::get_transmission_prob(int disease, Person * i, Person * s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease, i);
  int col = get_group(disease, s);
  double tr_pr = Workplace::Workplace_contact_prob[disease][row][col];
  return tr_pr;
}

double Workplace::get_contacts_per_day(int disease) {
  return Workplace::Workplace_contacts_per_day[disease];
}

int Workplace::get_number_of_rooms() {
  if (Workplace::Office_size == 0)
    return 0;
  int rooms = N / Workplace::Office_size;
  next_office = 0;
  if (N % Workplace::Office_size) rooms++;
  return rooms;
}

void Workplace::setup_offices( Allocator< Office > & office_allocator ) {
  int rooms = get_number_of_rooms();

  FRED_STATUS( 1, "workplace %d %s number %d rooms %d\n", id, label, N, rooms );
  
  for (int i = 0; i < rooms; i++) {
    char new_label[128];
    sprintf(new_label, "%s-%03d", this->get_label(), i);
    
    Place *p = new ( office_allocator.get_free() ) Office( new_label,
        this->get_longitude(),
        this->get_latitude(),
        this,
        this->get_population() );

    //Global::Places.add_place(p);
    offices.push_back(p);
    int id = p->get_id();

    FRED_STATUS( 1, "workplace %d %s added office %d %s %d\n",
        id, label,i,p->get_label(),p->get_id());
  }
}

Place * Workplace::assign_office(Person *per) {

  if (Workplace::Office_size == 0) {
    return NULL;
  }

  FRED_STATUS( 1, "assign office for person %d at workplace %d %s size %d == ",
     per->get_id(), id, label, N);

  // pick next office, round-robin
  int i = next_office;

  FRED_STATUS( 1, "office = %d %s %d\n",
      i, offices[i]->get_label(), offices[i]->get_id());

  // update next pick
  if (next_office < (int) offices.size()-1)
    next_office++;
  else
    next_office = 0;
  return offices[i];
}
