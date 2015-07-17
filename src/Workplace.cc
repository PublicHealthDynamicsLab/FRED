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
// File: Workplace.cc
//

#include "Workplace.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"
#include "Disease.h"
#include "Disease_List.h"
#include "Place_List.h"
#include "Office.h"
#include "Utils.h"

//Private static variables that will be set by parameter lookups
double* Workplace::Workplace_contacts_per_day;
double*** Workplace::Workplace_contact_prob;
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

Workplace::Workplace(const char *lab, fred::place_subtype _subtype, fred::geo lon, fred::geo lat, Place *container) {
  this->type = WORKPLACE;
  this->subtype = _subtype;
  setup(lab, lon, lat, container);
  get_parameters(Global::Dis.get_number_of_diseases());
  this->offices.clear();
  this->next_office = 0;
}

void Workplace::get_parameters(int diseases) {
  if(Workplace::Workplace_parameters_set) {
    return;
  }
  
  // people per office
  Params::get_param_from_string("office_size", &Workplace::Office_size);

  // workplace size limits
  Params::get_param_from_string("small_workplace_size", &Workplace::Small_workplace_size);
  Params::get_param_from_string("medium_workplace_size", &Workplace::Medium_workplace_size);
  Params::get_param_from_string("large_workplace_size", &Workplace::Large_workplace_size);

  if(Global::Enable_Vector_Transmission == false) {
    Workplace::Workplace_contacts_per_day = new double[diseases];
    Workplace::Workplace_contact_prob = new double**[diseases];
  
    char param_str[80];
    for(int disease_id = 0; disease_id < diseases; ++disease_id) {
      Disease * disease = Global::Dis.get_disease(disease_id);
      sprintf(param_str, "%s_workplace_contacts", disease->get_disease_name());
      Params::get_param((char *) param_str, &Workplace::Workplace_contacts_per_day[disease_id]);
      sprintf(param_str, "%s_workplace_prob", disease->get_disease_name());
      int n = Params::get_param_matrix(param_str, &Workplace::Workplace_contact_prob[disease_id]);
      if(Global::Verbose > 1) {
	      printf("\nWorkplace_contact_prob:\n");
	      for(int i  = 0; i < n; ++i)  {
	        for(int j  = 0; j < n; ++j) {
	          printf("%f ", Workplace::Workplace_contact_prob[disease_id][i][j]);
	        }
	        printf("\n");
	      }
      }
    }
  }

  Workplace::Workplace_parameters_set = true;
}

// this method is called after all workers are assigned to workplaces
void Workplace::prepare() {

  assert(Global::Pop.is_load_completed());
  // update employment stats based on size of workplace
  if(this->N < Workplace::Small_workplace_size) {
    Workplace::workers_in_small_workplaces += this->N;
  } else if(this->N < Workplace::Medium_workplace_size) {
    Workplace::workers_in_medium_workplaces += this->N;
  } else if(this->N < Workplace::Large_workplace_size) {
    Workplace::workers_in_large_workplaces += this->N;
  } else {
    Workplace::workers_in_xlarge_workplaces += this->N;
  }
  Workplace::total_workers += this->N;

  // now call base class function to perform preparations common to all Places 
  Place::prepare();
}

double Workplace::get_transmission_prob(int disease, Person* i, Person* s) {
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
  if(Workplace::Office_size == 0) {
    return 0;
  }
  int rooms = this->N / Workplace::Office_size;
  this->next_office = 0;
  if(this->N % Workplace::Office_size) {
    rooms++;
  }
  if(rooms == 0) {
    ++rooms;
  }
  return rooms;
}

void Workplace::setup_offices(Allocator<Office> &office_allocator) {
  int rooms = get_number_of_rooms();

  FRED_STATUS(1, "workplace %d %s number %d rooms %d\n", id, label, N, rooms );
  
  for(int i = 0; i < rooms; ++i) {
    char new_label[128];
    sprintf(new_label, "%s-%03d", this->get_label(), i);
    
    Place* p = new (office_allocator.get_free())Office(new_label,
							   fred::PLACE_SUBTYPE_NONE,
							   this->get_longitude(),
							   this->get_latitude(),
							   this);

    this->offices.push_back(p);
    int id = p->get_id();

    FRED_STATUS(1, "workplace %d %s added office %d %s %d\n",
        id, label,i,p->get_label(),p->get_id());
  }
}

Place* Workplace::assign_office(Person* per) {

  if(Workplace::Office_size == 0) {
    return NULL;
  }

  FRED_STATUS( 1, "assign office for person %d at workplace %d %s size %d == ",
     per->get_id(), id, label, N);

  // pick next office, round-robin
  int i = this->next_office;

  assert(this->offices.size() > i);

  FRED_STATUS(1, "office = %d %s %d\n",
      i, offices[i]->get_label(), offices[i]->get_id());

  // update next pick
  if(this->next_office < (int)this->offices.size() - 1) {
    this->next_office++;
  } else {
    this->next_office = 0;
  }
  return this->offices[i];
}
