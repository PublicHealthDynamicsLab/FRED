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
// File: Place.cc
//
#include <algorithm>
#include <climits>

#include "Place.h"

#include "Date.h"
#include "Disease.h"
#include "Disease_List.h"
#include "Global.h"
#include "Household.h"
#include "Infection.h"
#include "Neighborhood.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include "Params.h"
#include "Person.h"
#include "Random.h"
#include "Seasonality.h"
#include "Utils.h"
#include "Vector_Layer.h"

#define PI 3.14159265359

// static place type codes
char Place::TYPE_HOUSEHOLD = 'H';
char Place::TYPE_NEIGHBORHOOD = 'N';
char Place::TYPE_SCHOOL = 'S';
char Place::TYPE_CLASSROOM = 'C';
char Place::TYPE_WORKPLACE = 'W';
char Place::TYPE_OFFICE = 'O';
char Place::TYPE_HOSPITAL = 'M';
char Place::TYPE_COMMUNITY = 'X';
char Place::TYPE_UNSET = 'U';

// static place subtype codes
char Place::SUBTYPE_NONE = 'X';
char Place::SUBTYPE_COLLEGE = 'C';
char Place::SUBTYPE_PRISON = 'P';
char Place::SUBTYPE_MILITARY_BASE = 'M';
char Place::SUBTYPE_NURSING_HOME = 'N';
char Place::SUBTYPE_HEALTHCARE_CLINIC = 'I';
char Place::SUBTYPE_MOBILE_HEALTHCARE_CLINIC = 'Z';

Place::Place() : Mixing_Group("BLANK") {
  this->set_id(-1);      // actual id assigned in Place_List::add_place
  this->set_type(Place::TYPE_UNSET);
  this->set_subtype(Place::SUBTYPE_NONE);
  this->index = -1;
  this->staff_size = 0;
  this->household_fips = -1;
  this->open_date = 0;
  this->close_date = INT_MAX;
  this->intimacy = 0.0;
  this->enrollees.reserve(8); // initial slots for 8 people -- this is expanded in enroll()
  this->enrollees.clear();
  this->first_day_infectious = -1;
  this->last_day_infectious = -2;
  this->county_index = -1;
  this->census_tract_index = -1;
  this->intimacy = 0.0;
  this->patch = NULL;

  fred::geo undefined = -1.0;
  this->longitude = undefined;
  this->latitude = undefined;

  int diseases = Global::Diseases.get_number_of_diseases();
  this->infectious_people = new std::vector<Person*>[diseases];

  this->new_infections = new int[diseases];
  this->current_infections = new int[diseases];
  this->total_infections = new int[diseases];
  this->new_symptomatic_infections = new int[diseases];
  this->current_symptomatic_infections = new int[diseases];
  this->total_symptomatic_infections = new int[diseases];

  // zero out all disease-specific counts
  for(int d = 0; d < diseases; ++d) {
    this->new_infections[d] = 0;
    this->current_infections[d] = 0;
    this->total_infections[d] = 0;
    this->new_symptomatic_infections[d] = 0;
    this->total_symptomatic_infections[d] = 0;
    this->infectious_people[d].clear();
  }

  this->vector_disease_data = NULL;
  this->vectors_have_been_infected_today = false;
  this->vector_control_status = false;
}

Place::Place(const char* lab, fred::geo lon, fred::geo lat) : Mixing_Group(lab) {
  this->set_id(-1);      // actual id assigned in Place_List::add_place
  this->set_type(Place::TYPE_UNSET);
  this->set_subtype(Place::SUBTYPE_NONE);
  this->index = -1;
  this->staff_size = 0;
  this->household_fips = -1;
  this->longitude = lon;
  this->latitude = lat;
  this->open_date = 0;
  this->close_date = INT_MAX;
  this->intimacy = 0.0;
  this->enrollees.reserve(8); // initial slots for 8 people -- this is expanded in enroll()
  this->enrollees.clear();
  this->first_day_infectious = -1;
  this->last_day_infectious = -2;
  this->county_index = -1;
  this->census_tract_index = -1;
  this->intimacy = 0.0;
  this->patch = NULL;

  int diseases = Global::Diseases.get_number_of_diseases();
  this->infectious_people = new std::vector<Person*>[diseases];

  this->new_infections = new int[diseases];
  this->current_infections = new int[diseases];
  this->total_infections = new int[diseases];
  this->new_symptomatic_infections = new int[diseases];
  this->current_symptomatic_infections = new int[diseases];
  this->total_symptomatic_infections = new int[diseases];

  // zero out all disease-specific counts
  for(int d = 0; d < diseases; ++d) {
    this->new_infections[d] = 0;
    this->current_infections[d] = 0;
    this->total_infections[d] = 0;
    this->new_symptomatic_infections[d] = 0;
    this->total_symptomatic_infections[d] = 0;
    this->infectious_people[d].clear();
  }

  this->vector_disease_data = NULL;
  this->vectors_have_been_infected_today = false;
  this->vector_control_status = false;
}

void Place::prepare() {
  this->N_orig = this->enrollees.size();
  for(int d = 0; d < Global::Diseases.get_number_of_diseases(); ++d) {
    this->new_infections[d] = 0;
    this->current_infections[d] = 0;
    this->new_symptomatic_infections[d] = 0;
  }
  this->open_date = 0;
  this->close_date = INT_MAX;
  this->infectious_bitset.reset();
  this->human_infectious_bitset.reset();
  this->exposed_bitset.reset();

  if(Global::Enable_Vector_Transmission) {
    setup_vector_model();
  }

  Global::Neighborhoods->register_place(this);

  FRED_VERBOSE(2, "Prepare place %d label %s type %c\n", this->get_id(), this->get_label(), this->get_type());
}

void Place::update(int sim_day) {
  // stub for future use.
}

void Place::print(int disease_id) {
  FRED_STATUS(0, "Place %d label %s type %c\n", this->get_id(), this->get_label(), this->get_type());
  fflush(stdout);
}

void Place::turn_workers_into_teachers(Place* school) {
  std::vector <Person*> workers;
  workers.reserve(static_cast<int>(this->enrollees.size()));
  workers.clear();
  for(int i = 0; i < static_cast<int>(this->enrollees.size()); ++i) {
    workers.push_back(this->enrollees[i]);
  }
  FRED_VERBOSE(0, "turn_workers_into_teachers: place %d %s has %d workers\n", this->get_id(), this->get_label(), this->enrollees.size());
  int new_teachers = 0;
  for(int i = 0; i < static_cast<int>(workers.size()); ++i) {
    Person* person = workers[i];
    assert(person != NULL);
    FRED_VERBOSE(0, "Potential teacher %d age %d\n", person->get_id(), person->get_age());
    if(person->become_a_teacher(school)) {
      new_teachers++;
      FRED_VERBOSE(0, "new teacher %d age %d moved from workplace %d %s to school %d %s\n",
		   person->get_id(), person->get_age(), this->get_id(), this->get_label(), school->get_id(), school->get_label());
    }
  }
  FRED_VERBOSE(0, "%d new teachers reassigned from workplace %s to school %s\n", new_teachers,
	       this->get_label(), school->get_label());
}

void Place::reassign_workers(Place* new_place) {
  std::vector<Person*> workers;
  workers.reserve((int)this->enrollees.size());
  workers.clear();
  for(int i = 0; i < static_cast<int>(this->enrollees.size()); ++i) {
    workers.push_back(this->enrollees[i]);
  }
  int reassigned_workers = 0;
  for(int i = 0; i < static_cast<int>(workers.size()); ++i) {
    workers[i]->change_workplace(new_place, 0);
    // printf("worker %d age %d moving from workplace %s to place %s\n",
    //   workers[i]->get_id(), workers[i]->get_age(), label, new_place->get_label());
    reassigned_workers++;
  }
  FRED_VERBOSE(1, "%d workers reassigned from workplace %s to place %s\n", reassigned_workers,
	       this->get_label(), new_place->get_label());
}

int Place::get_visualization_counter(int day, int disease_id, int output_code) {
  switch(output_code) {
    case Global::OUTPUT_I:
      return this->get_number_of_infectious_people(disease_id);
      break;
    case Global::OUTPUT_Is:
      return this->get_current_symptomatic_infections(day, disease_id);
      break;
    case Global::OUTPUT_C:
      return this->get_new_infections(day, disease_id);
      break;
    case Global::OUTPUT_Cs:
      return this->get_new_symptomatic_infections(day, disease_id);
      break;
    case Global::OUTPUT_P:
      return this->get_current_infections(day, disease_id);
      break;
    case Global::OUTPUT_R:
      return this->get_recovereds(disease_id);
      break;
    case Global::OUTPUT_N:
      return this->get_size();
      break;
    case Global::OUTPUT_HC_DEFICIT:
      if(this->get_type() == Place::TYPE_HOUSEHOLD) {
        Household* hh = static_cast<Household*>(this);
        return hh->get_count_hc_accept_ins_unav();
      } else {
        return 0;
      }
      break;
  }
  return 0;
}

/////////////////////////////////////////
//
// PLACE-SPECIFIC TRANSMISSION DATA
//
/////////////////////////////////////////

double Place::get_contact_rate(int sim_day, int disease_id) {

  Disease* disease = Global::Diseases.get_disease(disease_id);
  // expected number of susceptible contacts for each infectious person
  double contacts = get_contacts_per_day(disease_id) * disease->get_transmissibility();
  if(Global::Enable_Seasonality) {

    double m = Global::Clim->get_seasonality_multiplier_by_lat_lon(this->latitude, this->longitude, disease_id);
    //cout << "SEASONALITY: " << day << " " << m << endl;
    contacts *= m;
  }

  // increase neighborhood contacts on weekends
  if(this->is_neighborhood()) {
    int day_of_week = Date::get_day_of_week();
    if(day_of_week == 0 || day_of_week == 6) {
      contacts = Neighborhood::get_weekend_contact_rate(disease_id) * contacts;
    }
  }
  // FRED_VERBOSE(1,"Disease %d, expected contacts = %f\n", disease_id, contacts);
  return contacts;
}

int Place::get_contact_count(Person* infector, int disease_id, int sim_day, double contact_rate) {
  // reduce number of infective contacts by infector's infectivity
  double infectivity = infector->get_infectivity(disease_id, sim_day);
  double infector_contacts = contact_rate * infectivity;

  FRED_VERBOSE(1, "infectivity = %f, so ", infectivity);
  FRED_VERBOSE(1, "infector's effective contacts = %f\n", infector_contacts);

  // randomly round off the expected value of the contact counts
  int contact_count = static_cast<int>(infector_contacts);
  double r = Random::draw_random();
  if(r < infector_contacts - contact_count) {
    contact_count++;
  }

  FRED_VERBOSE(1, "infector contact_count = %d  r = %f\n", contact_count, r);

  return contact_count;
}

//////////////////////////////////////////////////////////
//
// PLACE SPECIFIC VECTOR DATA
//
//////////////////////////////////////////////////////////


void Place::setup_vector_model() {

  this->vector_disease_data = new vector_disease_data_t;

  // initial vector counts
  if(this->is_neighborhood()) {
    // no vectors in neighborhoods (outdoors)
    this->vector_disease_data->vectors_per_host = 0.0;
  }
  else {
    this->vector_disease_data->vectors_per_host = 0.0;
    this->vector_disease_data->vectors_per_host = Global::Vectors->get_vectors_per_host(this);
  }
  this->vector_disease_data->N_vectors = this->N_orig * this->vector_disease_data->vectors_per_host;
  this->vector_disease_data->S_vectors = this->vector_disease_data->N_vectors;
  for(int i = 0; i < VECTOR_DISEASE_TYPES; ++i) {
    this->vector_disease_data->E_vectors[i] = 0;
    this->vector_disease_data->I_vectors[i] = 0;
  }

  // initial vector seed counts
  for(int i = 0; i < VECTOR_DISEASE_TYPES; ++i) {
    if (this->is_neighborhood()) {
      // no vectors in neighborhoods (outdoors)
      this->vector_disease_data->place_seeds[i] = 0;
      this->vector_disease_data->day_start_seed[i] = 0;
      this->vector_disease_data->day_end_seed[i] = 1;
    }
    else {
      this->vector_disease_data->place_seeds[i] = Global::Vectors->get_seeds(this,i);
      this->vector_disease_data->day_start_seed[i] = Global::Vectors->get_day_start_seed(this,i);
      this->vector_disease_data->day_end_seed[i] = Global::Vectors->get_day_end_seed(this,i);
    }
  }
  FRED_VERBOSE(1, "setup_vector_model: place %s vectors_per_host %f N_vectors %d N_orig %d\n",
         this->get_label(), this->vector_disease_data->vectors_per_host,
         this->vector_disease_data->N_vectors, this->N_orig);
}

double Place::get_seeds(int dis, int sim_day) {
  if((sim_day) || (sim_day > this->vector_disease_data->day_end_seed[dis])){
    return 0.0;
  } else {
    return this->vector_disease_data->place_seeds[dis];
  }
}

void Place::update_vector_population(int day) {
  if(this->is_neighborhood() == false) {
    *(this->vector_disease_data) = Global::Vectors->update_vector_population(day, this);
  }
}

char* Place::get_place_label(Place* p) {
  return ((p == NULL) ? (char*) "-1" : p->get_label());
}

    
