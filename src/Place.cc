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
#include "Neighborhood_Patch.h"
#include "Params.h"
#include "Person.h"
#include "Seasonality.h"
#include "Utils.h"
#include "Vector_Layer.h"

#define PI 3.14159265359

// static place type codes
char Place::HOUSEHOLD = 'H';
char Place::NEIGHBORHOOD = 'N';
char Place::SCHOOL = 'S';
char Place::CLASSROOM = 'C';
char Place::WORKPLACE = 'W';
char Place::OFFICE = 'O';
char Place::HOSPITAL = 'M';
char Place::COMMUNITY = 'X';


void Place::setup(const char* lab, fred::geo lon, fred::geo lat) {
  this->id = -1;		  // actual id assigned in Place_List::add_place
  strcpy(this->label, lab);
  this->longitude = lon;
  this->latitude = lat;
  this->enrollees.reserve(8); // initial slots for 8 people -- this is expanded in enroll()
  this->enrollees.clear();
  this->first_day_infectious = -1;
  this->last_day_infectious = -2;
  this->intimacy = 0.0;
  if(this->is_household()) {
    this->intimacy = 1.0;
  }
  if(this->is_neighborhood()) {
    this->intimacy = 0.0025;
  }
  if(this->is_school()) {
    this->intimacy = 0.025;
  }
  if(this->is_workplace()) {
    this->intimacy = 0.01;
  }
  this->staff_size = 0;

  int diseases = Global::Diseases.get_number_of_diseases();
  infectious_people = new std::vector<Person*> [diseases];

  new_infections = new int [diseases]; 
  current_infections = new int [diseases]; 
  total_infections = new int [diseases]; 
  new_symptomatic_infections = new int [diseases]; 
  current_symptomatic_infections = new int [diseases]; 
  total_symptomatic_infections = new int [diseases]; 
  current_infectious_visitors = new int [diseases]; 
  current_symptomatic_visitors = new int [diseases]; 

  // zero out all disease-specific counts
  for(int d = 0; d < diseases; ++d) {
    this->new_infections[d] = 0;
    this->current_infections[d] = 0;
    this->total_infections[d] = 0;
    this->new_symptomatic_infections[d] = 0;
    this->total_symptomatic_infections[d] = 0;
    this->current_infectious_visitors[d] = 0;
    this->current_symptomatic_visitors[d] = 0;
    this->infectious_people[d].clear();
  }
  this->county_index = -1;
  this->census_tract_index = -1;
}

void Place::prepare() {
  this->N_orig = this->enrollees.size();
  for(int d = 0; d < Global::Diseases.get_number_of_diseases(); ++d) {
    this->new_infections[d] = 0;
    this->current_infections[d] = 0;
    this->new_symptomatic_infections[d] = 0;
    this->current_infectious_visitors[d] = 0;
    this->current_symptomatic_visitors[d] = 0;
  }
  this->open_date = 0;
  this->close_date = INT_MAX;
  this->infectious_bitset.reset();
  this->human_infectious_bitset.reset();
  this->exposed_bitset.reset();

  if(Global::Enable_Vector_Transmission && !this->is_neighborhood()) {
    setup_vector_model();
  }

  FRED_VERBOSE(2, "Prepare place %d label %s type %c\n", this->id, this->label, this->type);
}


void Place::add_infectious_person(int disease_id, Person * person) {
  this->infectious_people[disease_id].push_back(person);
  /*
  FRED_VERBOSE(0, "AFTER ADD_INF_PERSON %d for disease_id %d to place %d %s => %d infectious people\n",
	       person->get_id(), disease_id, this->id, this->label, this->infectious_people[disease_id].size());
  */
}

void Place::update(int day) {
  // stub for future use.
}

/*
double Place::get_transmission_prob(double*** contact_prob_matrix, int disease_id, Person* infector, Person* infectee) {
  int row = get_group(disease_id, infector);
  int col = get_group(disease_id, infectee);
  double trans_prob = contact_prob[disease_id][row][col];
  return trans_prob;
}
*/

void Place::reset_visualization_data(int day) {
  for(int d = 0; d < Global::Diseases.get_number_of_diseases(); ++d) {
    this->new_infections[d] = 0;
    this->current_infections[d] = 0;
    this->new_symptomatic_infections[d] = 0;
    this->current_infectious_visitors[d] = 0;
    this->current_symptomatic_visitors[d] = 0;
  }
}

void Place::print(int disease_id) {
  printf("Place %d label %s type %c\n", this->id, this->label, this->type);
  fflush (stdout);
}

int Place::enroll(Person* per) {
  if (get_size() == enrollees.capacity()) {
    // double capacity if needed (to reduce future reallocations)
    enrollees.reserve(2*get_size());
  }
  this->enrollees.push_back(per);
  FRED_VERBOSE(1,"Enroll person %d age %d in place %d %s\n", per->get_id(), per->get_age(), this->id, this->label);
  return enrollees.size()-1;
}

void Place::unenroll(int pos) {
  int size = enrollees.size();
  if (!(0 <= pos && pos < size)) {
    printf("place %d %s pos = %d size = %d\n", this->id, this->label, pos, size);
  }
  assert(0 <= pos && pos < size);
  Person* removed = enrollees[pos];
  if (pos < size-1) {
    Person* moved = enrollees[size-1];
    FRED_VERBOSE(1,"UNENROLL place %d %s pos = %d size = %d removed %d moved %d\n",
		 this->id, this->label, pos, size, removed->get_id(), moved->get_id());
    enrollees[pos] = moved;
    moved->update_enrollee_index(this,pos);
  }
  else {
    FRED_VERBOSE(1,"UNENROLL place %d %s pos = %d size = %d removed %d moved NONE\n",
		 this->id, this->label, pos, size, removed->get_id());
  }
  enrollees.pop_back();
  FRED_VERBOSE(1,"UNENROLL place %d %s size = %d\n", this->id, this->label, this->enrollees.size());
}

int Place::get_children() {
  int children = 0;
  for(int i = 0; i < this->enrollees.size(); ++i) {
    children += (this->enrollees[i]->get_age() < Global::ADULT_AGE);
  }
  return children;
}

int Place::get_adults() {
  return (this->enrollees.size() - this->get_children());
}


void Place::print_infectious(int disease_id) {
  printf("INFECTIOUS in place %d Disease %d: ", id, disease_id);
  int size = infectious_people[disease_id].size();
  for (int i = 0; i < size; i++) {
    printf(" %d", infectious_people[disease_id][i]->get_id());
  }
  printf("\n");
}

void Place::turn_workers_into_teachers(Place* school) {
  std::vector <Person *> workers;
  workers.reserve((int)this->enrollees.size());
  workers.clear();
  for(int i = 0; i < (int)this->enrollees.size(); ++i) {
    workers.push_back(this->enrollees[i]);
  }
  FRED_VERBOSE(0,"turn_workers_into_teachers: place %d %s has %d workers\n", this->id, this->get_label(), this->enrollees.size());
  int new_teachers = 0;
  for(int i = 0; i < (int)workers.size(); ++i) {
    Person* person = workers[i];
    assert(person != NULL);
    FRED_VERBOSE(0,"Potential teacher %d age %d\n", person->get_id(), person->get_age());
    if(person->become_a_teacher(school)) {
      new_teachers++;
      FRED_VERBOSE(0,"new teacher %d age %d moved from workplace %d %s to school %d %s\n",
		   person->get_id(), person->get_age(), this->id, this->label, school->get_id(), school->get_label());
    }
  }
  FRED_VERBOSE(0, "%d new teachers reassigned from workplace %s to school %s\n", new_teachers,
	       label, school->get_label());
}

void Place::reassign_workers(Place* new_place) {
  std::vector<Person*> workers;
  workers.reserve((int)this->enrollees.size());
  workers.clear();
  for(int i = 0; i < (int)this->enrollees.size(); ++i) {
    workers.push_back(this->enrollees[i]);
  }
  int reassigned_workers = 0;
  for(int i = 0; i < (int)workers.size(); ++i) {
    workers[i]->change_workplace(new_place,0);
    // printf("worker %d age %d moving from workplace %s to place %s\n",
    //   workers[i]->get_id(), workers[i]->get_age(), label, new_place->get_label());
    reassigned_workers++;
  }
  FRED_VERBOSE(1, "%d workers reassigned from workplace %s to place %s\n", reassigned_workers,
	       label, new_place->get_label());
}

int Place::get_output_count(int disease_id, int output_code) {
  switch(output_code) {
    case Global::OUTPUT_I:
      return get_current_infectious_visitors(disease_id);
      break;
    case Global::OUTPUT_Is:
      return get_current_symptomatic_visitors(disease_id);
      break;
    case Global::OUTPUT_C:
      return get_new_infections(disease_id);
      break;
    case Global::OUTPUT_Cs:
      return get_new_symptomatic_infections(disease_id);
      break;
    case Global::OUTPUT_P:
      return get_current_infections(disease_id);
      break;
    case Global::OUTPUT_R:
      return get_recovereds(disease_id);
      break;
    case Global::OUTPUT_N:
      return get_size();
      break;
    case Global::OUTPUT_HC_DEFICIT:
      if(this->type == Place::HOUSEHOLD) {
        Household* hh = static_cast<Household*>(this);
        return hh->get_count_hc_accept_ins_unav();
      } else {
        return 0;
      }
      break;
  }
  return 0;
}

int Place::get_recovereds(int disease_id) {
  int count = 0;
  int size = enrollees.size();
  for(int i = 0; i < size; ++i) {
    Person * perso = get_enrollee(i);
    count += perso->is_recovered(disease_id);
  }
  return count;
}



/////////////////////////////////////////
//
// PLACE-SPECIFIC TRANSMISSION DATA
//
/////////////////////////////////////////

double Place::get_contact_rate(int day, int disease_id) {

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

int Place::get_contact_count(Person* infector, int disease_id, int day, double contact_rate) {
  // reduce number of infective contacts by infector's infectivity
  double infectivity = infector->get_infectivity(disease_id, day);
  double infector_contacts = contact_rate * infectivity;

  FRED_VERBOSE(1, "infectivity = %f, so ", infectivity);
  FRED_VERBOSE(1, "infector's effective contacts = %f\n", infector_contacts);

  // randomly round off the expected value of the contact counts
  int contact_count = (int) infector_contacts;
  double r = Random::draw_random();
  if(r < infector_contacts - contact_count) {
    contact_count++;
  }

  FRED_VERBOSE(1, "infector contact_count = %d  r = %f\n", contact_count, r);

  return contact_count;
}


void Place::record_infectious_days(int day) {
  if(this->first_day_infectious == -1) {
    this->first_day_infectious = day;
  }
  this->last_day_infectious = day;
}


//////////////////////////////////////////////////////////
//
// PLACE SPECIFIC VECTOR DATA
//
//////////////////////////////////////////////////////////


void Place::setup_vector_model() {
  this->N_vectors = 0;
  this->S_vectors = 0;
  for(int i = 0; i < DISEASE_TYPES; ++i) {
    this->E_vectors[i] = 0;
    this->I_vectors[i] = 0;
    this->place_seeds[i] = Global::Vectors->get_seeds(this,i);
    this->day_start_seed[i] = Global::Vectors->get_day_start_seed(this,i);
    this->day_end_seed[i] = Global::Vectors->get_day_end_seed(this,i);
  }	
  this->death_rate = 1.0/18.0;
  this->birth_rate = 1.0/18.0;
  this->bite_rate = 0.76;
  this->incubation_rate = 1.0/11.0;
  this->infection_efficiency = Global::Vectors->get_infection_efficiency();
  this->transmission_efficiency = Global::Vectors->get_transmission_efficiency();
  this->suitability = 1.0;
  this->temperature = 0;
  this->vectors_per_host = 0; // depends on the number of pupa and temperature
  this->pupae_per_host = 1.02; //Armenia average
  this->life_span = 18.0; // From Chao and longini
  this->sucess_rate = 0.83; // Focks
  this->female_ratio = 0.5;
  this->development_time = 1.0;
  set_temperature();
  this->N_vectors = this->N_orig * this->vectors_per_host;
  this->S_vectors = this->N_vectors;
  FRED_VERBOSE(1, "VECTOR_MODEL_SETUP: House %s temp %f vectors_per_host %f N_vectors %d N_orig %d\n", this->label, this->temperature, this->vectors_per_host, this->N_vectors, this->N_orig);
}

double Place::get_seeds(int dis, int day) {
  if((day < this->day_start_seed[dis]) ||( day > this->day_end_seed[dis])){
    return 0.0;
  } else {
    return this->place_seeds[dis];
  }
}

void Place::set_temperature(){
  //temperatures vs development times..FOCKS2000: DENGUE TRANSMISSION THRESHOLDS
  double temps[8] = {8.49,3.11,4.06,3.3,2.66,2.04,1.46,0.92}; //temperatures
  double dev_times[8] = {15.0,20.0,22.0,24.0,26.0,28.0,30.0,32.0}; //development times
  this->temperature = Global::Vectors->get_temperature(this);
  if(this->temperature > 32) {
    this->temperature = 32;
  }
  if(this->temperature <= 18) {
    this->vectors_per_host = 0;
  } else {
    for(int i = 0; i < 8; ++i) {
      if(this->temperature <= temps[i]) {
        //obtain the development time using linear interpolation
        this->development_time = dev_times[i - 1] + (dev_times[i] - dev_times[i - 1]) / (temps[i] - temps[i-1]) * (this->temperature - temps[i - 1]);
      }
    }
    this->vectors_per_host = this->pupae_per_host * this->female_ratio * this->sucess_rate * this->life_span / this->development_time;
  }
  FRED_VERBOSE(1, "SET TEMP: House %s temp %f vectors_per_host %f N_vectors %d N_orig %d\n", this->label, this->temperature, this->vectors_per_host, this->N_vectors, this->N_orig);
}


void Place::update_vector_population(int day) {
  this->vectors_have_been_infected_today = false;
  
  if(this->is_neighborhood()){
    return;
  }

  if(this->N_vectors <= 0) {
    return;
  }
  FRED_VERBOSE(1,"update vector pop: day %d place %s initially: S %d, N: %d\n",day,this->label,S_vectors,N_vectors);

  int born_infectious[DISEASE_TYPES];
  int total_born_infectious=0;

  // new vectors are born susceptible
  this->S_vectors += floor(this->birth_rate * this->N_vectors - this->death_rate * S_vectors);
  FRED_VERBOSE(1,"vector_update_population:: S_vector: %d, N_vectors: %d\n", this->S_vectors, this->N_vectors);
  // but some are infected
  for(int d=0;d<DISEASE_TYPES;d++){
    double seeds = this->get_seeds(d, day);
    born_infectious[d] = ceil(this->S_vectors * seeds);
    total_born_infectious += born_infectious[d];
    if(born_infectious[d] > 0) {
      FRED_VERBOSE(1,"vector_update_population:: Vector born infectious disease[%d] = %d \n",d, born_infectious[d]);
      FRED_VERBOSE(1,"Total Vector born infectious: %d \n", total_born_infectious);// 
    }
  }
  // printf("PLACE %s SEEDS %d S_vectors %d DAY %d\n",this->label,total_born_infectious,S_vectors,day);
  this->S_vectors -= total_born_infectious;
  FRED_VERBOSE(1,"vector_update_population - seeds:: S_vector: %d, N_vectors: %d\n", this->S_vectors, this->N_vectors);
  // print this 
  if(total_born_infectious > 0){
    FRED_VERBOSE(1,"Total Vector born infectious: %d \n", total_born_infectious);// 
  }
  if(this->S_vectors < 0) {
    this->S_vectors = 0;
  }
    
  // accumulate total number of vectors
  this->N_vectors = this->S_vectors;
  // we assume vectors can have at most one infection, if not susceptible
  for(int i = 0; i < DISEASE_TYPES; ++i) {
    // some die
    FRED_VERBOSE(1,"vector_update_population:: E_vectors[%d] = %d \n",i, this->E_vectors[i]);
    if(this->E_vectors[i] < 10){
      for(int k = 0; k < this->E_vectors[i]; ++k) {
	double r = Random::draw_random(0,1);
	if(r < this->death_rate){
	  this->E_vectors[i]--;
	}
      }
    } else {
      this->E_vectors[i] -= floor(this->death_rate * this->E_vectors[i]);
    } 
    // some become infectious
    int become_infectious = 0;
    if(this->E_vectors[i] < 10) {
      for(int k = 0; k < this->E_vectors[i]; ++k) {
	double r = Random::draw_random(0,1);
	if(r < this->incubation_rate) {
	  become_infectious++;
	}
      }
    } else {
      become_infectious = floor(this->incubation_rate * this->E_vectors[i]);
    }
      
    // int become_infectious = floor(incubation_rate * E_vectors[i]);
    FRED_VERBOSE(1,"vector_update_population:: become infectious [%d] = %d, incubation rate: %f,E_vectors[%d] %d \n", i,
		 become_infectious, this->incubation_rate, i, this->E_vectors[i]);
    this->E_vectors[i] -= become_infectious;
    if(this->E_vectors[i] < 0) this->E_vectors[i] = 0;
    // some die
    FRED_VERBOSE(1,"vector_update_population:: I_Vectors[%d] = %d \n", i, this->I_vectors[i]);
    this->I_vectors[i] -= floor(this->death_rate * this->I_vectors[i]);
    FRED_VERBOSE(1,"vector_update_population:: I_Vectors[%d] = %d \n", i, this->I_vectors[i]);
    // some become infectious
    this->I_vectors[i] += become_infectious;
    FRED_VERBOSE(1,"vector_update_population:: I_Vectors[%d] = %d \n", i, this->I_vectors[i]);
    // some were born infectious
    this->I_vectors[i] += born_infectious[i];
    FRED_VERBOSE(1,"vector_update_population::+= born infectious I_Vectors[%d] = %d,born infectious[%d] = %d \n", i,
		 this->I_vectors[i], i, born_infectious[i]);
    if(this->I_vectors[i] < 0) {
      this->I_vectors[i] = 0;
    }

    // add to the total
    this->N_vectors += (this->E_vectors[i] + this->I_vectors[i]);
    FRED_VERBOSE(1, "update_vector_population entered S_vectors %d E_Vectors[%d] %d  I_Vectors[%d] %d N_Vectors %d\n",
		 this->S_vectors, i, this->E_vectors[i], i, this->I_vectors[i], this->N_vectors);

    // register if any infectious vectors
    if(this->I_vectors[i]) {
      // this->register_as_an_infectious_place(i);
    }
  }
}

char* Place::get_place_label(Place* p) {
  return ((p == NULL) ? (char*) "-1" : p->get_label());
}


