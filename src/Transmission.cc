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
// File: Transmission.cc
//
#include <algorithm>

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
char Transmission::HOUSEHOLD = 'H';
char Transmission::NEIGHBORHOOD = 'N';
char Transmission::SCHOOL = 'S';
char Transmission::CLASSROOM = 'C';
char Transmission::WORKPLACE = 'W';
char Transmission::OFFICE = 'O';
char Transmission::HOSPITAL = 'M';
char Transmission::COMMUNITY = 'X';

bool Transmission::Enable_Neighborhood_Density_Transmission = false;
bool Transmission::Enable_Density_Transmission_Maximum_Infectees = false;
int Transmission::Density_Transmission_Maximum_Infectees = 10.0;

double Transmission::Seasonal_Reduction = 0.0;
double* Transmission::Seasonality_multiplier = NULL;


void Transmission::get_parameters() {
  int temp_int = 0;
  Params::get_param_from_string("enable_neighborhood_density_transmission", &temp_int);
  Transmission::Enable_Neighborhood_Density_Transmission = (temp_int == 1);
  Params::get_param_from_string("enable_density_transmission_maximum_infectees", &temp_int);
  Transmission::Enable_Density_Transmission_Maximum_Infectees = (temp_int == 1);
  Params::get_param_from_string("density_transmission_maximum_infectees",
				&Transmission::Density_Transmission_Maximum_Infectees);

  // all-disease seasonality reduction
  Params::get_param_from_string("seasonal_reduction", &Transmission::Seasonal_Reduction);
  // setup seasonal multipliers

  if (Transmission::Seasonal_Reduction > 0.0) {
    int seasonal_peak_day_of_year; // e.g. Jan 1
    Params::get_param_from_string("seasonal_peak_day_of_year", &seasonal_peak_day_of_year);

    // setup seasonal multipliers
    Transmission::Seasonality_multiplier = new double [367];
    for(int day = 1; day <= 366; ++day) {
      int days_from_peak_transmissibility = abs(seasonal_peak_day_of_year - day);
      Transmission::Seasonality_multiplier[day] = (1.0 - Transmission::Seasonal_Reduction) +
	Transmission::Seasonal_Reduction * 0.5 * (1.0 + cos(days_from_peak_transmissibility * (2 * PI / 365.0)));
      if(Transmission::Seasonality_multiplier[day] < 0.0) {
	Transmission::Seasonality_multiplier[day] = 0.0;
      }
      // printf("Seasonality_multiplier[%d] = %e %d\n", day, Transmission::Seasonality_multiplier[day], days_from_peak_transmissibility);
    }
  }
}

static bool compare_age (Person* p1, Person* p2) {
  return ((p1->get_age() == p2->get_age()) ? (p1->get_id() < p2->get_id()) : (p1->get_age() < p2->get_age()));
}

static bool compare_id (Person* p1, Person* p2) {
  return p1->get_id() < p2->get_id();
}

double** Transmission::prob_contact = NULL;

void Transmission::setup() {

  if(Transmission::prob_contact == NULL) {
    Transmission::prob_contact = new double * [101];
    
    // create a PolyMod type matrix;
    for(int i = 0; i <= 100; ++i) {
      Transmission::prob_contact[i] = new double [101];
      
      for(int j = 0; j <= 100; ++j) {
        Transmission::prob_contact[i][j] = 0.05;
      }
    }
    for(int i = 0; i <= 100; ++i) {
      for(int j = i - 4; j <= i+4; ++j) {
        if(j < 0 || j > 100) {
          continue;
        }
        Transmission::prob_contact[i][j] = 1.0 - 0.2 * abs(i - j);
      }
    }
  }
}

/////////////////////////////////////////
//
// PLACE-SPECIFIC TRANSMISSION MODELS
//
/////////////////////////////////////////


bool Transmission::attempt_transmission(double transmission_prob, Person* infector, Person* infectee,
					int disease_id, int day, Place* place) {

  assert(infectee->is_susceptible(disease_id));
  FRED_STATUS(1, "infector %d -- infectee %d is susceptible\n", infector->get_id(), infectee->get_id());

  double susceptibility = infectee->get_susceptibility(disease_id);

  // reduce transmission probability due to infector's hygiene (face masks or hand washing)
  transmission_prob *= infector->get_transmission_modifier_due_to_hygiene(disease_id);

  // reduce susceptibility due to infectee's hygiene (face masks or hand washing)
  susceptibility *= infectee->get_susceptibility_modifier_due_to_hygiene(disease_id);
    
  if(Global::Enable_hh_income_based_susc_mod) {
    int hh_income = Household::get_max_hh_income(); //Default to max (no modifier)
    Household* hh = static_cast<Household*>(infectee->get_household());
    if(hh != NULL) {
      hh_income = hh->get_household_income();
    }
    susceptibility *= infectee->get_health()->get_susceptibility_modifier_due_to_household_income(hh_income);
    //Utils::fred_log("SUSC Modifier [%.4f] for HH Income [%i] modified suscepitibility to [%.4f]\n", hh_income, infectee->get_health()->get_susceptibility_modifier_due_to_household_income(hh_income), susceptibility);
  }

  FRED_VERBOSE(2, "susceptibility = %f\n", susceptibility);

  // reduce transmissibility due to seasonality
  if (Transmission::Seasonal_Reduction > 0.0) {
    int day_of_year = Date::get_day_of_year(day);
    transmission_prob *= Transmission::Seasonality_multiplier[day_of_year];
  }

  double r = Random::draw_random();
  double infection_prob = transmission_prob * susceptibility;

  if(r < infection_prob) {
    // successful transmission; create a new infection in infectee
    infector->infect(infectee, disease_id, place, day);

    FRED_VERBOSE(1, "transmission succeeded: r = %f  prob = %f\n", r, infection_prob);
    FRED_CONDITIONAL_VERBOSE(1, infector->get_exposure_date(disease_id) == 0,
			     "SEED infection day %i from %d to %d\n", day, infector->get_id(), infectee->get_id());
    FRED_CONDITIONAL_VERBOSE(1, infector->get_exposure_date(disease_id) != 0,
			     "infection day %i of disease %i from %d to %d\n", day, disease_id, infector->get_id(),
			     infectee->get_id());
    FRED_CONDITIONAL_VERBOSE(0, infection_prob > 1, "infection_prob exceeded unity!\n");

    // notify the epidemic
    Global::Diseases.get_disease(disease_id)->get_epidemic()->become_exposed(infectee, day);

    return true;
  } else {
    FRED_VERBOSE(1, "transmission failed: r = %f  prob = %f\n", r, infection_prob);
    return false;
  }
}

//////////////////////////////////////////////////////////
//
// PLACE SPECIFIC TRANSMISSION MODELS
//
//////////////////////////////////////////////////////////


void Transmission::spread_infection(int day, int disease_id, Place * place) {
  Disease* disease = Global::Diseases.get_disease(disease_id);
  double beta = disease->get_transmissibility();
  if(beta == 0.0) {
    reset_place_state(disease_id);
    return;
  }

  if(is_open(day) == false) {
    reset_place_state(disease_id);
    return;
  }
  if(should_be_open(day, disease_id) == false) {
    reset_place_state(disease_id);
    return;
  }

  if(this->first_day_infectious == -1) {
    this->first_day_infectious = day;
  }
  this->last_day_infectious = day;

  // get reference to susceptibles and infectious lists for this disease_id
  std::vector<Person*> susceptibles = this->enrollees;
  std::vector<Person*> infectious = this->infectious_people[disease_id];
  // printf("spread_infection place %d inf %d sus %d\n", id, (int)infectious.size(), (int)susceptibles.size());

  if(Global::Enable_Vector_Transmission) {
    vector_transmission(day, disease_id);
    reset_place_state(disease_id);
    return;
  }

  // need at least one susceptible
  if(susceptibles.size() == 0) {
    reset_place_state(disease_id);
    return;
  }

  if(this->is_household()) {
    pairwise_transmission_model(day, disease_id, &infectious, &susceptibles);
    reset_place_state(disease_id);
    return;
  }

  if(this->is_neighborhood() && Transmission::Enable_Neighborhood_Density_Transmission==true) {
    density_transmission_model(day, disease_id, &infectious, &susceptibles);
    reset_place_state(disease_id);
    return;
  }

  if(Global::Enable_New_Transmission_Model) {
    age_based_transmission_model(day, disease_id, &infectious, &susceptibles);
  } else {
    default_transmission_model(day, disease_id, &infectious, &susceptibles);
  }
  reset_place_state(disease_id);
  return;
}

void Transmission::default_transmission_model(int day, int disease_id, Place * place) {

  Disease* disease = Global::Diseases.get_disease(disease_id);
  int N = this->get_size();

  FRED_VERBOSE(1, "default_transmission DAY %d PLACE %s N %d susc %d inf %d\n",
	       day, this->get_label(), N, (int) susceptibles->size(), (int) infectious->size());

  // the number of possible infectees per infector is max of (N-1) and S[s]
  // where N is the capacity of this place and S[s] is the number of current susceptibles
  // visiting this place. S[s] might exceed N if we have some ad hoc visitors,
  // since N is estimated only at startup.
  int number_targets = (N - 1 > susceptibles->size() ? N - 1 : susceptibles->size());

  // contact_rate is contacts_per_day with weeked and seasonality modulation (if applicable)
  double contact_rate = get_contact_rate(day, disease_id);

  // randomize the order of the infectious list
  FYShuffle<Person*>(*infectious);
  // printf("shuffled infectious size %d\n", (int)infectious->size());

  for(int infector_pos = 0; infector_pos < infectious->size(); ++infector_pos) {
    // infectious visitor
    Person* infector = (*infectious)[infector_pos];
    // printf("infector id %d\n", infector->get_id());
    if(infector->is_infectious(disease_id) == false) {
      // printf("infector id %d not infectious!\n", infector->get_id());
      continue;
    }

    // get the actual number of contacts to attempt to infect
    int contact_count = get_contact_count(infector, disease_id, day, contact_rate);

    std::map<int, int> sampling_map;
    // get a susceptible target for each contact resulting in infection
    for(int c = 0; c < contact_count; ++c) {
      // select a target infectee from among susceptibles with replacement
      int pos = Random::draw_random_int(0, number_targets - 1);
      if(pos < susceptibles->size()) {
        if(infector == (*susceptibles)[pos]) {
          if(susceptibles->size() > 1) {
            --(c); // redo
            continue;
          } else {
            break; // give up
          }
        }
        sampling_map[pos]++;
      }
    }

    std::map<int, int>::iterator i;
    for(i = sampling_map.begin(); i != sampling_map.end(); ++i) {
      int pos = (*i).first;
      int times_drawn = (*i).second;
      Person* infectee = (*susceptibles)[pos];
      assert (infector != infectee);
      infectee->update_schedule(day);
      if (!infectee->is_present(day, this)) {
	continue;
      }
      // get the transmission probs for this infector/infectee pair
      double transmission_prob = get_transmission_prob(disease_id, infector, infectee);
      for(int draw = 0; draw < times_drawn; ++draw) {
        // only proceed if person is susceptible
        if(infectee->is_susceptible(disease_id)) {
          attempt_transmission(transmission_prob, infector, infectee, disease_id, day);
        }
      }
    } // end contact loop
  } // end infectious list loop
}

void Transmission::age_based_transmission_model(int day, int disease_id, Place * place) {
  Disease* disease = Global::Diseases.get_disease(disease_id);

  int N = this->get_size();
  std::vector<double> infectivity_of_agent((int) infectious->size());
  int n[101];
  int start[101];
  int s[101];
  int size[101];
  double p[101][101];
  double infectivity_of_group[101];
  int infectee_count[101];
  double beta = disease->get_transmissibility();
  double force = beta * this->intimacy;

  // sort the infectious list by age
  std::sort(infectious->begin(), infectious->end(), compare_age);

  // get number of infectious and susceptible persons and infectivity of each age group
  for (int i = 0; i <=100; ++i) {
    n[i] = 0;
    s[i] = 0;
    size[i] = 0;
    start[i] = -1;
    infectivity_of_group[i] = 0.0;
  }
  for(int inf_pos = 0; inf_pos < infectious->size(); ++inf_pos) {
    Person* person = (*infectious)[inf_pos];
    int age = person->get_age();
    if (age > 100) age = 100;
    n[age]++;
    if (start[age] == -1) { start[age] = inf_pos; }
    double infectivity = person->get_infectivity(disease_id, day); 
    infectivity_of_group[age] += infectivity;
    infectivity_of_agent[inf_pos] = infectivity;
    size[age]++;
  }
  for(int sus_pos = 0; sus_pos < susceptibles->size(); ++sus_pos) {
    int age = (*susceptibles)[sus_pos]->get_age();
    if (age > 100) age = 100;
    s[age]++;
    size[age]++;
  }

  // compute p[i][j] = prob of an individual in group i 
  // getting infected by someone in group j
  for(int i = 0; i <=100; ++i) {
    for (int j = 0; j <=100; ++j) {
      if (s[i] == 0 || size[j] == 0) {
        p[i][j] = 0.0;
      } else {
	p[i][j] = force * Transmission::prob_contact[i][j] * infectivity_of_group[j] / size[j];
      }
    }
  }    

  // get number of infections for each age group i
  for(int i = 0; i <= 100; ++i) {
    infectee_count[i] = 0;
    if(s[i] > 0) {
      double product = 1.0;
      for(int j = 0; j <= 100; ++j) {
        if(n[j] == 0) {
          continue;
        }
        if(p[i][j] == 0.0) {
          continue;
        }
        product *= pow((1 - p[i][j]), n[j]);
	if(0 && this->is_household()) {
	  printf("SUS_AGE %d INF_AGE %d INF_COUNT %d p[i][j] %e PRODUCT %e\n",
		 i, j, n[j], p[i][j], product);
        }
      }
      double prob_infection = 1.0 - product;
      double expected_infections = s[i] * prob_infection;
	
      // convert to integer
      infectee_count[i] = (int) expected_infections;
      double r = Random::draw_random();
      if(r < expected_infections - infectee_count[i]) {
        infectee_count[i]++;
      }
      if(1 || this->is_household()) {
        printf("PLACE TYPE %c AGE GROUP %d SIZE %d PROB_INF %0.6f EXPECTED %0.2f INFECTEE COUNT %d\n",
	       this->type, i, s[i], prob_infection, expected_infections, infectee_count[i]);
      }
    }
  }
    
  // turn rows in p[i][j] into a cdf
  for(int i = 0; i <= 100; ++i) {
    double total = 0.0;
    for(int j = 0; j <= 100; ++j) {
      p[i][j] += total;
      total = p[i][j];
    }
  }  
  printf("cdf done\n"); fflush(stdout);

  // randomize the order of the susceptibles list
  FYShuffle<Person*>(*susceptibles);

  // infect susceptibles according to the infectee_counts
  for(int sus_pos = 0; sus_pos < susceptibles->size(); ++sus_pos) {
    // susceptible visitor
    Person* infectee = (*susceptibles)[sus_pos];
    infectee->update_schedule(day);
    if (!infectee->is_present(day, this)) {
      continue;
    }
    int age = infectee->get_age();
    if(age > 100) {
      age = 100;
    }
    if(infectee_count[age] > 0) {

      // is this person susceptible?
      double r = Random::draw_random();
      if(r < infectee->get_susceptibility(disease_id)) {

	printf("INFECTING pos %d age %d \n", sus_pos, age); fflush(stdout);

	// pick an age group of infector based on cdf in row p[i]
	r = Random::draw_random(0,p[age][100]);
	int j = 0;
	for(j = 0; j <= 100; ++j) {
	  printf("r %e p[%d][%d] %e\n",r,age,j,p[age][j]);
	  if(r < p[age][j]) {
	    break;
	  }
	}
	if(j > 100) {
	  printf("age_based_transmission: ERROR IN FINDING INFECTING AGE GROUP\n");
	  abort();
	}
	printf("PICK INFECTOR age %d size %d start pos %d infectivity %e\n", j, size[j], start[j], infectivity_of_group[j]); fflush(stdout);

	// pick a infectious person from group j based on infectivity
	r = Random::draw_random(0, infectivity_of_group[j]);
	int pos = start[j];
	while(infectivity_of_agent[pos] < r) {
	  r -= infectivity_of_agent[pos];
	  pos++;
	}
	Person* infector = (*infectious)[pos];
	printf("PICKED INFECTOR pos %d with infectivity %e\n", pos, infectivity_of_agent[pos]); fflush(stdout);

	// successful transmission; create a new infection in infectee
	infector->infect(infectee, disease_id, this, day);

	FRED_CONDITIONAL_VERBOSE(1, infector->get_exposure_date(disease_id) == 0,
				 "SEED infection day %i from %d to %d\n",
				 day, infector->get_id(), infectee->get_id());
	FRED_CONDITIONAL_VERBOSE(1, infector->get_exposure_date(disease_id) != 0,
				 "infection day %i of disease %i from %d to %d\n",
				 day, disease_id, infector->get_id(),
				 infectee->get_id());
	
	// notify the epidemic
	Global::Diseases.get_disease(disease_id)->get_epidemic()->become_exposed(infectee, day);

      }
      // decrement counter (even if transmission fails)
      infectee_count[age]++;
    }
  }
  return;
}


void Transmission::pairwise_transmission_model(int day, int disease_id, Place * place) {
  double contact_prob = get_contact_rate(day, disease_id);
  
  FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s N %d\n",
	       day, this->get_label(), this->get_size());
  
  for(int infector_pos = 0; infector_pos < infectious->size(); ++infector_pos) {
    Person* infector = (*infectious)[infector_pos];      // infectious individual
    FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infector %d is %d\n",
		 day, this->get_label(), infector_pos, infector->get_id());
    
    if(infector->is_infectious(disease_id) == false) {
      FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infector %d is not infectious!\n",
		   day, this->get_label(), infector->get_id());
      continue;
    }
    
    int sus_size = susceptibles->size();
    for(int pos = 0; pos < sus_size; ++pos) {
      Person* infectee = (*susceptibles)[pos];
      if (infector == infectee) {
	continue;
      }
      FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infectee is %d\n",
		   day, this->get_label(), infectee->get_id());
      
      if (infectee->is_infectious(disease_id) == false) {
	FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infectee %d is not infectious -- updating schedule\n",
		     day, this->get_label(), infectee->get_id());
	infectee->update_schedule(day);
      }
      else {
	FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infectee %d is infectious\n",
		     day, this->get_label(), infectee->get_id());
      }
      if (!infectee->is_present(day, this)) {
	FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infectee %d is not present today\n",
		     day, this->get_label(), infectee->get_id());
	continue;
      }
      // only proceed if person is susceptible
      if(infectee->is_susceptible(disease_id)) {
	// get the transmission probs for this infector/infectee pair
	double transmission_prob = get_transmission_prob(disease_id, infector, infectee);
	double infectivity = infector->get_infectivity(disease_id, day);
	// scale transmission prob by infectivity and contact prob
	transmission_prob *= infectivity * contact_prob;     
	attempt_transmission(transmission_prob, infector, infectee, disease_id, day);
      }
      else {
	FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infectee %d is not susceptible\n",
		     day, this->get_label(), infectee->get_id());
      }
    } // end susceptibles loop
  }
}


void Transmission::density_transmission_model(int day, int disease_id, Place * place) {
  Disease* disease = Global::Diseases.get_disease(disease_id);

  int N = get_size();

  // printf("DAY %d PLACE %s N %d susc %d inf %d\n",
  // day, this->get_label(), N, (int) susceptibles->size(), (int) infectious->size());

  double contact_prob = get_contact_rate(day, disease_id);
  int sus_hosts = susceptibles->size();
  int inf_hosts = infectious->size();
  int exposed = 0;
      
  // each host's probability of infection
  double prob_infection = 1.0 - pow((1.0 - contact_prob), inf_hosts);
  
  // select a number of hosts to be infected
  double expected_infections = sus_hosts * prob_infection;
  exposed = floor(expected_infections);
  double remainder = expected_infections - exposed;
  if(Random::draw_random() < remainder) {
    exposed++;
  }

  int infectee_count [inf_hosts];
  for (int i = 0; i < inf_hosts; ++i) {
    infectee_count[i] = 0;
  }
    
  int reached_max_infectees_count = 0;
  int number_infectious_hosts = inf_hosts;

  FRED_VERBOSE(1, "density_transmission: exposed = %d\n", exposed);
  // randomize the order of the susceptibles list
  FYShuffle<Person *>(*susceptibles);

  for(int j = 0; j < exposed && j < sus_hosts && 0 < inf_hosts; ++j) {
    Person * infectee = (*susceptibles)[j];
    infectee->update_schedule(day);
    if (!infectee->is_present(day, this)) {
      continue;
    }
    FRED_VERBOSE(1,"selected host %d age %d\n", infectee->get_id(), infectee->get_age());

    // only proceed if person is susceptible
    if(infectee->is_susceptible(disease_id)) {
      // select a random infector
      int infector_pos = Random::draw_random_int(0,inf_hosts-1);
      Person * infector = (*infectious)[infector_pos];
      assert(infector->get_health()->is_infectious(disease_id));

      // get the transmission probs for this infector  pair
      double transmission_prob = infector->get_infectivity(disease_id, day);
      if(attempt_transmission(transmission_prob, infector, infectee, disease_id, day)) {
	// successful transmission
	infectee_count[infector_pos]++;
	// if the infector has reached the max allowed, remove from infector list
	if(Enable_Density_Transmission_Maximum_Infectees &&
	   Transmission::Density_Transmission_Maximum_Infectees <= infectee_count[infector_pos]) {
	  // effectively remove the infector from infector list
	  (*infectious)[infector_pos] = (*infectious)[inf_hosts-1];
	  int tmp = infectee_count[infector_pos];
	  infectee_count[infector_pos] = infectee_count[inf_hosts-1];
	  infectee_count[inf_hosts-1] = tmp;
	  inf_hosts--;
	  reached_max_infectees_count++;
        }
      }
    } else {
      FRED_VERBOSE(1,"host %d not susceptible\n", infectee->get_id());
    }
  }
  if(reached_max_infectees_count) {
    FRED_VERBOSE(1, "day %d DENSITY TRANSMISSION place %s: %d with %d infectees out of %d infectious hosts\n",
		 day, this->label, reached_max_infectees_count,
		 Transmission::Density_Transmission_Maximum_Infectees, number_infectious_hosts);
  }
}

//////////////////////////////////////////////////////////
//
// PLACE SPECIFIC VECTOR TRANSMISSION MODEL
//
//////////////////////////////////////////////////////////

void Transmission::vector_transmission(int day, int disease_id, Place * place) {

  // N_vectors is updated in update_vector_population()

  // N_hosts includes all visitors: infectious, susceptible, or neither
  this->N_hosts = this->enrollees.size();

  // infections of vectors by hosts
  if(this->vectors_not_infected_yet) {
    this->infect_vectors(day);
  }

  // transmission from vectors to hosts
  this->vectors_transmit_to_hosts(day, disease_id);
}


void Transmission::infect_vectors(int day, Place * place) {

  // skip if there are no susceptible vectors
  if(this->S_vectors == 0) {
    return;
  }

  // find the percent distribution of infectious hosts
  int* infectious_hosts = new int [Global::Diseases.get_number_of_diseases()];
  for(int disease_id = 0; disease_id < Global::Diseases.get_number_of_diseases(); ++disease_id) {
    infectious_hosts[disease_id] = 0;
  }
  int total_infectious_hosts = 0;
  for(int disease_id = 0; disease_id < Global::Diseases.get_number_of_diseases(); ++disease_id) {
    std::vector<Person*> * infectious = &(infectious_people[disease_id]);

    infectious_hosts[disease_id] = infectious->size();
    total_infectious_hosts += infectious_hosts[disease_id];
  }

  // return if there are no infectious hosts
  if(total_infectious_hosts == 0) {
    return;
  }

  FRED_VERBOSE(1, "infect_vectors entered S_vectors %d N_inf_hosts %d\n", this->S_vectors, total_infectious_hosts);

  // the vector infection model of Chao and Longini

  // decide on total number of vectors infected by any infectious host

  // each vectors's probability of infection
  double prob_infection = 1.0 - pow((1.0 - this->infection_efficiency), (this->bite_rate * total_infectious_hosts) / this->N_hosts);

  // select a number of vectors to be infected
  int total_infections = prob_infection * this->S_vectors;
  FRED_VERBOSE(1, "total infections %d\n", total_infections);

  // assign strain based on distribution of infectious hosts
  int newly_infected = 0;
  for(int disease_id = 0; disease_id < Global::Diseases.get_number_of_diseases(); ++disease_id) {
    int strain_infections = total_infections *((double)infectious_hosts[disease_id] / (double)total_infectious_hosts);
    this->E_vectors[disease_id] += strain_infections;
    this->S_vectors -= strain_infections;
    newly_infected += strain_infections;
  }
  this->vectors_not_infected_yet = false;
  FRED_VERBOSE(1, "newly_infected_vectors %d S %d \n", newly_infected,this-> S_vectors);
}

void Transmission::vectors_transmit_to_hosts(int day, int disease_id, Place * place) {

  int e_hosts = 0;

  // get reference to susceptibles list for this disease_id
  // this->place_state_merge = Place_State_Merge();
  // this->place_state[disease_id].apply(this->place_state_merge);
  // std::vector<Person*> & susceptibles = this->place_state_merge.get_susceptible_vector();
  std::vector<Person*> * susceptibles = &(this->enrollees);

  int s_hosts = susceptibles->size();

  if(Global::Verbose > 1) {
    int R_hosts = get_recovereds(disease_id);
    FRED_VERBOSE(1, "day %d vector_update: %s S_vectors %d E_vectors %d I_vectors %d  N_vectors = %d N_host = %d S_hosts %d R_hosts %d Temp = %f dis %d\n",
		 day, this->label, S_vectors, E_vectors[disease_id], I_vectors[disease_id], N_vectors, 
		 N_hosts,susceptibles->size(),R_hosts,temperature,disease_id);
  }

  if(s_hosts == 0 || this->I_vectors[disease_id] == 0 || this->transmission_efficiency == 0.0) {
    return;
  }

  // each host's probability of infection
  double prob_infection = 1.0 - pow((1.0 - this->transmission_efficiency), (this->bite_rate * this->I_vectors[disease_id] / this->N_hosts));
  
  // select a number of hosts to be infected
  double expected_infections = s_hosts * prob_infection;
  e_hosts = floor(expected_infections);
  double remainder = expected_infections - e_hosts;
  if(Random::draw_random() < remainder) {
    e_hosts++;
  }
  FRED_VERBOSE(1, "transmit_to_hosts: E_hosts[%d] = %d\n", disease_id, e_hosts);
  // randomize the order of the susceptibles list
  FYShuffle<Person *>(*susceptibles);
  FRED_VERBOSE(1,"Shuffle finished S_hosts size = %d\n", susceptibles->size());
  // get the disease object   
  Disease* disease = Global::Diseases.get_disease(disease_id);

  for(int j = 0; j < e_hosts && j < susceptibles->size(); ++j) {
    Person* infectee = (*susceptibles)[j];
    infectee->update_schedule(day);
    if (!infectee->is_present(day, this)) {
      continue;
    }
    FRED_VERBOSE(1,"selected host %d age %d\n", infectee->get_id(), infectee->get_age());
    // NOTE: need to check if infectee already infected
    if(infectee->is_susceptible(disease_id)) {
      // create a new infection in infectee
      FRED_VERBOSE(1,"transmitting to host %d\n", infectee->get_id());
      infectee->become_exposed(disease_id, NULL, this, day);

      // become unsusceptible to other diseases(?)
      for(int d = 0; d < DISEASE_TYPES; d++) {
        if(d != disease_id) {
          Disease* other_disease = Global::Diseases.get_disease(d);
          infectee->become_unsusceptible(other_disease);
        }
      }
    } else {
      FRED_VERBOSE(1,"host %d not susceptible\n", infectee->get_id());
    }
  }
}

void Transmission::update_vector_population(int day, Place * place) {
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



