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
// File: Vector_Patch.cc
//

#include "Vector_Patch.h"
#include "Vector_Layer.h"
#include "Disease.h"
#include "Disease_List.h"
#include "Params.h"
#include "Utils.h"
#include "Person.h"
#include <vector>
using namespace std;

void Vector_Patch::setup(int i, int j, double patch_size, double grid_min_x, double grid_min_y, double trans_efficiency, double infec_efficiency) {
  row = i;
  col = j;
  min_x = grid_min_x + (col)*patch_size;
  min_y = grid_min_y + (row)*patch_size;
  max_x = grid_min_x + (col+1)*patch_size;
  max_y = grid_min_y + (row+1)*patch_size;
  center_y = (min_y+max_y)/2.0;
  center_x = (min_x+max_x)/2.0;

  // local state variables
  N_vectors = 0;
  N_hosts = 0;
  S_vectors = 0;
  for (int i = 0; i < DISEASE_TYPES; i++) {
    E_vectors[i] = 0;
    I_vectors[i] = 0;
    S_hosts[i].clear();
    E_hosts[i] = 0;
    I_hosts[i] = 0;
    R_hosts[i] = 0;
    seeds[i] = 0.0;
    day_start_seed[i] = 0;
    day_end_seed[i] = 0;
  }	
  death_rate = 1.0/18.0;
  birth_rate = 1.0/18.0;
  bite_rate = 0.76;
  incubation_rate = 1.0/11.0;
  infection_efficiency = infec_efficiency;
  transmission_efficiency = trans_efficiency;
  suitability = 1.0;
  temperature = 0;
  vectors_per_host = 0; // depends on the number of pupa and temperature
  pupae_per_host = 1.02; //Armenia average 
  life_span = 18.0; // From Chao and longini
  sucess_rate = 0.83; // Focks
  female_ratio = 0.5;
  development_time = 1.0;
}

double Vector_Patch::distance_to_patch(Vector_Patch *p2) {
  double x1 = center_x;
  double y1 = center_y;
  double x2 = p2->get_center_x();
  double y2 = p2->get_center_y();
  return sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}
void Vector_Patch::set_temperature(double patch_temperature){
	//temperatures vs development times..FOCKS2000: DENGUE TRANSMISSION THRESHOLDS
	double temps[8] = {8.49,3.11,4.06,3.3,2.66,2.04,1.46,0.92}; //temperatures
	double dev_times[8] = {15.0,20.0,22.0,24.0,26.0,28.0,30.0,32.0}; //development times
	temperature = patch_temperature;
	if (temperature > 32) temperature = 32;
	if (temperature <=18)
	{
	  vectors_per_host = 0;
	}else {
		for (int i=0;i<8;i++)
		{
			if (temperature<=temps[i]) {
				//obtain the development time using linear interpolation
				development_time = dev_times[i-1] + (dev_times[i]-dev_times[i-1])/(temps[i] - temps[i-1])*(temperature - temps[i-1]);
			}
		}
	   vectors_per_host = pupae_per_host*female_ratio*sucess_rate*life_span/development_time;	
	}
	FRED_VERBOSE(1, "SET TEMP: Patch %d %d temp %f vectors_per_host %f\n", row, col, patch_temperature, vectors_per_host);

}

double Vector_Patch::get_seeds(int dis, int day) {
  if((day<day_start_seed[dis]) ||( day>day_end_seed[dis])){
    return 0.0;
  }
  else {
    return seeds[dis];
  }
}

void Vector_Patch::set_vector_seeds(int dis,int day_on, int day_off,double seeds_){
  seeds[dis] = seeds_;
  day_start_seed[dis] = day_on;
  day_end_seed[dis] = day_off;
  FRED_VERBOSE(1, "SET_VECTOR_SEEDS: Patch %d %d proportion of susceptible for disease [%d]: %f. start: %d end: %d\n", row, col, dis,seeds[dis],day_on,day_off);
}
void Vector_Patch::print() {
  FRED_VERBOSE(0, "Vector_patch: %d %d N_vectors %d\n",
	       row, col, N_vectors);
}


void Vector_Patch::update(int day) {
  if(day==0){
    S_vectors = N_vectors;
  }
  FRED_VERBOSE(1, "update Vector_patch: %d %d S = %d N = %d N_host = %d Temp = %f\n",row, col, S_vectors, N_vectors, N_hosts, temperature);
  // infection of vectors by hosts
  this->infect_vectors(day);

  // transmission from vectors to hosts
  this->transmit_to_hosts(day);

  // update vector population size
  this->update_vector_population(day);

  // prepare for next next day:
  for (int i = 0; i < DISEASE_TYPES; i++) {
    N_hosts = 0;
    S_hosts[i].clear();
    I_hosts[i] = 0;
    R_hosts[i] = 0;
  }
}


void Vector_Patch::infect_vectors(int day) {
  if (S_vectors == 0 || N_hosts == 0) return;
  FRED_VERBOSE(1, "infect_vectors entered S_vectors %d N_hosts %d\n", S_vectors, N_hosts);
  // the vector infection model of Chao and Longini

  // decide on total number of vectors infected by any infectious host
  int total_infectious_hosts = 0;
  for (int disease_id = 0; disease_id < DISEASE_TYPES; disease_id++) {
    total_infectious_hosts += I_hosts[disease_id];
  }
  FRED_VERBOSE(1, "total_infectious_hosts %d\n", total_infectious_hosts);
  // each vectors's probability of infection
  if (total_infectious_hosts == 0) return;
  double prob_infection = 1.0 - pow((1.0-infection_efficiency), (bite_rate*total_infectious_hosts)/N_hosts);   
  // select a number of vectors to be infected
  int total_infections = prob_infection * S_vectors;
  FRED_VERBOSE(1, "total infections %d\n", total_infections);
  // assign strain based on distribution of infectious hosts
  for (int disease_id = 0; disease_id < DISEASE_TYPES; disease_id++) {
    int strain_infections = ((1.0*I_hosts[disease_id])/total_infectious_hosts)*total_infections;
    E_vectors[disease_id] += strain_infections;
    S_vectors -= strain_infections;
  }
  FRED_VERBOSE(1, "newly_infected_vectors %d S %d \n", get_newly_infected_vectors(), S_vectors);
}


void Vector_Patch::transmit_to_hosts(int day) {
  if (N_hosts == 0)
    return;
  for (int disease_id = 0; disease_id < DISEASE_TYPES; disease_id++) {
    E_hosts[disease_id] = 0;
    int s_hosts = S_hosts[disease_id].size();
    if (s_hosts == 0 || I_vectors[disease_id] == 0 || transmission_efficiency == 0.0)
      continue;

    // each host's probability of infection
    double prob_infection = 1.0 - pow((1.0-transmission_efficiency), (bite_rate*I_vectors[disease_id]/N_hosts));
  
    // select a number of hosts to be infected
    double expected_infections = s_hosts * prob_infection;
    E_hosts[disease_id] = floor(expected_infections);
    double remainder = expected_infections - E_hosts[disease_id];
    if (Random::draw_random() < remainder) E_hosts[disease_id]++;
    FRED_VERBOSE(1, "transmit_to_hosts: E_hosts[%d] = %d\n", disease_id, E_hosts[disease_id]);
    // randomize the order of the susceptibles list
    FYShuffle<Person *>(S_hosts[disease_id]);
    FRED_VERBOSE(1,"Shuffle finished S_hosts size = %d\n", S_hosts[disease_id].size());
    // get the disease object   
    Disease * disease = Global::Diseases.get_disease(disease_id);

    for (int j = 0; j < E_hosts[disease_id] && j < S_hosts[disease_id].size(); j++) {
      Person * infectee = S_hosts[disease_id][j];
      FRED_VERBOSE(1,"selected host %d age %d\n", infectee->get_id(), infectee->get_age());
      // NOTE: need to check if infectee already infected
      if (infectee->is_susceptible(disease_id)) {
        // create a new infection in infectee
	FRED_VERBOSE(1,"transmitting to host %d\n", infectee->get_id());
        Transmission transmission = Transmission(NULL, NULL, day);
        transmission.set_initial_loads(disease->get_primary_loads(day));
        infectee->become_exposed(disease, transmission);

        // become unsusceptible to other diseases(?)
        for (int d = 0; d < DISEASE_TYPES; d++) {
          if (d != disease_id) {
            Disease * other_disease = Global::Diseases.get_disease(d);
            infectee->become_unsusceptible(other_disease);
          }
        }
      }else{
	FRED_VERBOSE(1,"host %d not susceptible\n", infectee->get_id());
      }
    }
  }
}

void Vector_Patch::update_vector_population(int day) {
  int born_infectious[DISEASE_TYPES];
  int total_born_infectious=0;
  FRED_VERBOSE(1,"vector_update_population:: S_vector: %d, N_vectors: %d\n",S_vectors,N_vectors);
  // new vectors are born susceptible
  S_vectors += floor(birth_rate*N_vectors-death_rate*S_vectors);
  FRED_VERBOSE(1,"vector_update_population:: S_vector: %d, N_vectors: %d\n",S_vectors,N_vectors);
  // but some are infected
  for (int d=0;d<DISEASE_TYPES;d++){
    if((day<day_start_seed[d]) ||( day>day_end_seed[d])){
      born_infectious[d] = 0;
      FRED_VERBOSE(1,"vector_update_population:: Vector born infectious disease[%d] = 0 \n",d);
    }else{
      born_infectious[d] = S_vectors*seeds[d];
    }
    total_born_infectious += born_infectious[d];
    if (born_infectious[d]>0){
         FRED_VERBOSE(1,"vector_update_population:: Vector born infectious disease[%d] = %d \n",d, born_infectious[d]);
    }
  }
  S_vectors -= total_born_infectious;
  FRED_VERBOSE(1,"vector_update_population - seeds:: S_vector: %d, N_vectors: %d\n",S_vectors,N_vectors);
  // print this 
  if (total_born_infectious>0){
    FRED_VERBOSE(1,"Total Vector born infectious: %d \n", total_born_infectious);// 
  }
  if (S_vectors < 0) S_vectors = 0;
  // add to the total
  N_vectors = S_vectors;
  // we assume vectors can have at most one infection, if not susceptible
  for (int i = 0; i < DISEASE_TYPES; i++) {
    // some die
    FRED_VERBOSE(1,"vector_update_population:: E_vectors[%d] = %d \n",i, E_vectors[i]);
    E_vectors[i] -= floor(death_rate*E_vectors[i]);
    // some become infectious
    int become_infectious = floor(incubation_rate * E_vectors[i]);
    FRED_VERBOSE(1,"vector_update_population:: become infectious [%d] = %d, incubation rate: %f,E_vectors[%d] %d \n",i, become_infectious,incubation_rate,i,E_vectors[i]);
    E_vectors[i] -= become_infectious;
    if (E_vectors[i] < 0) E_vectors[i] = 0;
    // some die
    FRED_VERBOSE(1,"vector_update_population:: I_Vectors[%d] = %d \n",i, I_vectors[i]);
    I_vectors[i] -= floor(death_rate*I_vectors[i]);
    FRED_VERBOSE(1,"vector_update_population:: I_Vectors[%d] = %d \n",i, I_vectors[i]);
    // some become infectious
    I_vectors[i] += become_infectious;
    FRED_VERBOSE(1,"vector_update_population:: I_Vectors[%d] = %d \n",i, I_vectors[i]);
    // some were born infectious
    I_vectors[i] += born_infectious[i];
    FRED_VERBOSE(1,"vector_update_population::+= born infectious I_Vectors[%d] = %d,born infectious[%d] = %d \n",i, I_vectors[i],i,born_infectious[i]);
    if (I_vectors[i] < 0) I_vectors[i] = 0;

    // add to the total
    N_vectors += E_vectors[i] + I_vectors[i];
    FRED_VERBOSE(1, "update_vector_population entered S_vectors %d E_Vectors[%d] %d  I_Vectors[%d] %d N_Vectors %d\n", S_vectors,i,E_vectors[i],i,I_vectors[i],N_vectors);
  }

}


void Vector_Patch::add_host(Person *person) {
  for (int disease_id = 0; disease_id < DISEASE_TYPES; disease_id++) {
    if (person->is_susceptible(disease_id)) {
      S_hosts[disease_id].push_back(person);
    }
    else if (person->is_infectious(disease_id)) {
      I_hosts[disease_id]++; 
    }
    else {
      R_hosts[disease_id]++; 
    }
  }
  N_hosts++;
  // FRED_VERBOSE(0, "add_host entered: N_host: %d \n",N_hosts);
}
