/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Respiratory_Transmission.cc
//
#include <algorithm>

#include "Respiratory_Transmission.h"
#include "Date.h"
#include "Condition.h"
#include "Condition_List.h"
#include "Epidemic.h"
#include "Global.h"
#include "Household.h"
#include "Params.h"
#include "Person.h"
#include "Place.h"
#include "Random.h"
#include "Utils.h"


Respiratory_Transmission::Respiratory_Transmission() {
  this->enable_neighborhood_density_transmission = false;
  this->enable_density_transmission_maximum_infectees = false;
  this->density_transmission_maximum_infectees = 10.0;
  this->prob_contact = NULL;
}

Respiratory_Transmission::~Respiratory_Transmission() {
  if(this->prob_contact != NULL) {
    delete[] this->prob_contact;
  }
}

void Respiratory_Transmission::setup(Condition* condition) {
  int temp_int = 0;
  Params::get_param("enable_neighborhood_density_transmission", &temp_int);
  this->enable_neighborhood_density_transmission = (temp_int == 1);
  Params::get_param("enable_density_transmission_maximum_infectees", &temp_int);
  this->enable_density_transmission_maximum_infectees = (temp_int == 1);
  Params::get_param("density_transmission_maximum_infectees",
				&(this->density_transmission_maximum_infectees));

  /*
  Respiratory_Transmission::prob_contact = new double * [101];
  // create a PolyMod type matrix;
  for(int i = 0; i <= 100; ++i) {
    Respiratory_Transmission::prob_contact[i] = new double [101];
      
    for(int j = 0; j <= 100; ++j) {
      Respiratory_Transmission::prob_contact[i][j] = 0.05;
    }
  }
  for(int i = 0; i <= 100; ++i) {
    for(int j = i - 4; j <= i+4; ++j) {
      if(j < 0 || j > 100) {
	continue;
      }
      Respiratory_Transmission::prob_contact[i][j] = 1.0 - 0.2 * abs(i - j);
    }
  }
  */
}


/////////////////////////////////////////
//
// REQUIRED ENTRY POINT TO TRANSMISSION MODELS
//
/////////////////////////////////////////

void Respiratory_Transmission::spread_infection(int day, int condition_id, Mixing_Group* mixing_group) {
  if(dynamic_cast<Place*>(mixing_group) == NULL) {
    //Respiratory_Transmission must occur on a Place type
    return;
  } else {
    this->spread_infection(day, condition_id, dynamic_cast<Place*>(mixing_group));
  }
}

void Respiratory_Transmission::spread_infection(int day, int condition_id, Place* place) {

  FRED_VERBOSE(1, "spread_infection day %d condition %d place %d %c %s\n",
	       day, condition_id, place->get_id(), place->get_type(), place->get_label());

  // abort if transmissibility == 0 or if place is closed
  Condition* condition = Global::Conditions.get_condition(condition_id);
  double beta = condition->get_transmissibility();
  if(beta == 0.0 || place->is_open(day) == false || place->should_be_open(day, condition_id) == false) {
    place->reset_place_state(condition_id);
    return;
  }

  // have place record first and last day of infectiousness
  place->record_infectious_days(day);

  // need at least one susceptible
  if(place->get_size() == 0) {
    place->reset_place_state(condition_id);
    return;
  }

  if(place->is_household()) {
    pairwise_transmission_model(day, condition_id, place);
    FRED_VERBOSE(1, "spread_infection finished day %d condition %d place %d %c %s\n",
		 day, condition_id, place->get_id(), place->get_type(), place->get_label());
    return;
  }

  if(place->is_neighborhood() && this->enable_neighborhood_density_transmission == true) {
    density_transmission_model(day, condition_id, place);
  } else {
    default_transmission_model(day, condition_id, place);
  }

  /*
  if(Global::Enable_New_Transmission_Model) {
    age_based_transmission_model(day, condition_id, place);
  } else {
    default_transmission_model(day, condition_id, place);
  }
  */
  FRED_VERBOSE(1, "spread_infection finished day %d condition %d place %d %c %s\n",
	       day, condition_id, place->get_id(), place->get_type(), place->get_label());

  return;
}
/////////////////////////////////////////
//
// RESPIRATORY TRANSMISSION MODELS
//
/////////////////////////////////////////


bool Respiratory_Transmission::attempt_transmission(double transmission_prob, Person* infector, Person* infectee,
					int condition_id, int day, Place* place) {

  assert(infectee->is_susceptible(condition_id));
  FRED_STATUS(1, "infector %d -- infectee %d is susceptible\n", infector->get_id(), infectee->get_id());

  double susceptibility = infectee->get_susceptibility(condition_id);

  // reduce transmission probability due to infector's other conditions
  transmission_prob *= infector->get_transmission_modifier(condition_id);

  // reduce susceptibility due to infectee's other conditions
  susceptibility *= infectee->get_susceptibility_modifier(condition_id);
    
  FRED_VERBOSE(2, "susceptibility = %f\n", susceptibility);

  // reduce transmissibility due to seasonality
  if(Transmission::Seasonal_Reduction > 0.0) {
    int day_of_year = Date::get_day_of_year(day);
    transmission_prob *= Transmission::Seasonality_multiplier[day_of_year];
  }

  double r = Random::draw_random();
  double infection_prob = transmission_prob * susceptibility;

  if(r < infection_prob) {
    // successful transmission; create a new infection in infectee
    infector->infect(infectee, condition_id, place, day);

    FRED_VERBOSE(1, "transmission succeeded: r = %f  prob = %f\n", r, infection_prob);
    FRED_CONDITIONAL_VERBOSE(1, infector->get_exposure_date(condition_id) == 0,
			     "SEED infection day %i from %d to %d\n", day, infector->get_id(), infectee->get_id());
    FRED_CONDITIONAL_VERBOSE(1, infector->get_exposure_date(condition_id) != 0,
			     "infection day %i of condition %i from %d to %d\n", day, condition_id, infector->get_id(),
			     infectee->get_id());
    FRED_CONDITIONAL_VERBOSE(0, infection_prob > 1, "infection_prob exceeded unity!\n");

    // notify the epidemic
    Global::Conditions.get_condition(condition_id)->get_epidemic()->become_exposed(infectee, day);

    return true;
  } else {
    FRED_VERBOSE(1, "transmission failed: r = %f  prob = %f\n", r, infection_prob);
    return false;
  }
}

void Respiratory_Transmission::default_transmission_model(int day, int condition_id, Place * place) {
  Condition* condition = Global::Conditions.get_condition(condition_id);
  int N = place->get_size();
  int new_infections = 0;

  person_vec_t* infectious = place->get_infectious_people(condition_id);
  person_vec_t* susceptibles = place->get_enrollees();

  FRED_VERBOSE(1, "default_transmission DAY %d PLACE %s N %d susc %d inf %d\n",
	             day, place->get_label(), N, (int) susceptibles->size(), (int) infectious->size());

  // the number of possible infectees per infector is max of (N-1) and S[s]
  // where N is the capacity of this place and S[s] is the number of current susceptibles
  // visiting this place. S[s] might exceed N if we have some ad hoc visitors,
  // since N is estimated only at startup.
  int number_targets = (N - 1 > susceptibles->size() ? N - 1 : susceptibles->size());

  // contact_rate is contacts_per_day with weeked and seasonality modulation (if applicable)
  double contact_rate = place->get_contact_rate(day, condition_id);

  // randomize the order of processing the infectious list
  std::vector<int> shuffle_index;
  int number_of_infectious = infectious->size();
  shuffle_index.reserve(number_of_infectious);
  for(int i = 0; i < number_of_infectious; ++i) {
    shuffle_index[i] = i;
  }
  FYShuffle<int>(shuffle_index);

  for(int n = 0; n < number_of_infectious; ++n) {
    int infector_pos = shuffle_index[n];
    // infectious visitor
    Person* infector = (*infectious)[infector_pos];
    // printf("infector id %d\n", infector->get_id());
    if(infector->is_infectious(condition_id) == false) {
      // printf("infector id %d not infectious!\n", infector->get_id());
      continue;
    }

    // get the actual number of contacts to attempt to infect
    int contact_count = place->get_contact_count(infector, condition_id, day, contact_rate);

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
      if(!infectee->is_present(day, place)) {
	      continue;
      }
      // get the transmission probs for given infector/infectee pair
      double transmission_prob = 1.0;
      if(Global::Enable_Transmission_Bias) {
	      transmission_prob = place->get_transmission_probability(condition_id, infector, infectee);
      } else {
	      transmission_prob = place->get_transmission_prob(condition_id, infector, infectee);
      }
      for(int draw = 0; draw < times_drawn; ++draw) {
        // only proceed if person is susceptible
        if(infectee->is_susceptible(condition_id)) {
          if (attempt_transmission(transmission_prob, infector, infectee, condition_id, day, place)) {
	    new_infections++;
	  }
        }
      }
    } // end contact loop
  } // end infectious list loop
  place->reset_place_state(condition_id);
  if (0 && new_infections > 0) {
    FRED_VERBOSE(1, "default_transmission DAY %d PLACE %s gives %d new_infections\n",
		 day, place->get_label(), new_infections);
  }
}


void Respiratory_Transmission::pairwise_transmission_model(int day, int condition_id, Place* place) {

  person_vec_t* infectious = place->get_infectious_people(condition_id);
  person_vec_t* susceptibles = place->get_enrollees();

  double contact_prob = place->get_contact_rate(day, condition_id);
  int new_infections = 0;
  
  FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s N %d\n",
	       day, place->get_label(), place->get_size());
  
  for(int infector_pos = 0; infector_pos < infectious->size(); ++infector_pos) {
    Person* infector = (*infectious)[infector_pos];      // infectious individual
    // FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infector %d is %d\n", day, place->get_label(), infector_pos, infector->get_id());
    
    if(infector->is_infectious(condition_id) == false) {
      FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infector %d is not infectious!\n",
		   day, place->get_label(), infector->get_id());
      continue;
    }
    
    int sus_size = susceptibles->size();
    for(int pos = 0; pos < sus_size; ++pos) {
      Person* infectee = (*susceptibles)[pos];
      if(infector == infectee) {
	      continue;
      }
      int infectee_id = infectee->get_id();
      char* label = place->get_label();

      FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infectee is %d\n",
		   day, label, infectee_id);
      
      if(infectee->is_infectious(condition_id) == false) {
	      FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infectee %d is not infectious -- updating schedule\n",
		                 day, label, infectee_id);
	      infectee->update_schedule(day);
      } else {
	      FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infectee %d is infectious\n",
		                 day, label, infectee_id);
      }
      if(!infectee->is_present(day, place)) {
	      FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infectee %d is not present today\n",
		                 day, label, infectee_id);
	      continue;
      }
      // only proceed if person is susceptible
      if(infectee->is_susceptible(condition_id)) {
	      FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infectee %d is present and susceptible\n",
		                 day, label, infectee_id);
	      // get the transmission probs for infector/infectee pair
	      double transmission_prob = 1.0;
	      if(Global::Enable_Transmission_Bias) {
	        transmission_prob = place->get_transmission_probability(condition_id, infector, infectee);
	      } else {
	        transmission_prob = place->get_transmission_prob(condition_id, infector, infectee);
	      }
	      double infectivity = infector->get_infectivity(condition_id);
	      // scale transmission prob by infectivity and contact prob
	      transmission_prob *= infectivity * contact_prob;
	      if (attempt_transmission(transmission_prob, infector, infectee, condition_id, day, place)) {
		new_infections++;
	      };
      } else {
	      FRED_VERBOSE(1, "pairwise_transmission DAY %d PLACE %s infectee %d is not susceptible\n",
		                 day, label, infectee_id);
      }
    } // end susceptibles loop
  }
  place->reset_place_state(condition_id);
  if (0 && new_infections > 0) {
    FRED_VERBOSE(0, "default_transmission DAY %d PLACE %s gives %d new_infections\n",
		 day, place->get_label(), new_infections);
  }
}

void Respiratory_Transmission::density_transmission_model(int day, int condition_id, Place* place) {

  person_vec_t* infectious = place->get_infectious_people(condition_id);
  person_vec_t* susceptibles = place->get_enrollees();

  Condition* condition = Global::Conditions.get_condition(condition_id);
  int N = place->get_size();
  int new_infections = 0;

  // printf("DAY %d PLACE %s N %d susc %d inf %d\n",
  // day, place->get_label(), N, (int) susceptibles->size(), (int) infectious->size());

  double contact_prob = place->get_contact_rate(day, condition_id);
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

  int infectee_count[inf_hosts];
  for(int i = 0; i < inf_hosts; ++i) {
    infectee_count[i] = 0;
  }
    
  int reached_max_infectees_count = 0;
  int number_infectious_hosts = inf_hosts;

  // randomize the order of processing the susceptible list
  std::vector<int> shuffle_index;
  int number_of_susceptibles = susceptibles->size();
  shuffle_index.reserve(number_of_susceptibles);
  for(int i = 0; i < number_of_susceptibles; ++i) {
    shuffle_index[i] = i;
  }
  FYShuffle<int>(shuffle_index);

  for(int j = 0; j < exposed && j < sus_hosts && 0 < inf_hosts; ++j) {
    Person* infectee = (*susceptibles)[shuffle_index[j]];
    infectee->update_schedule(day);
    if(!infectee->is_present(day, place)) {
      continue;
    }
    FRED_VERBOSE(1,"selected host %d age %d\n", infectee->get_id(), infectee->get_age());

    // only proceed if person is susceptible
    if(infectee->is_susceptible(condition_id)) {
      // select a random infector
      int infector_pos = Random::draw_random_int(0,inf_hosts-1);
      Person* infector = (*infectious)[infector_pos];
      assert(infector->get_health()->is_infectious(condition_id));

      // get the transmission probs for  infectee/infector  pair
      double transmission_prob = infector->get_infectivity(condition_id);
      if(attempt_transmission(transmission_prob, infector, infectee, condition_id, day, place)) {
        // successful transmission
	new_infections++;
        infectee_count[infector_pos]++;
        // if the infector has reached the max allowed, remove from infector list
        if(this->enable_density_transmission_maximum_infectees &&
           this->density_transmission_maximum_infectees <= infectee_count[infector_pos]) {
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
                day, place->get_label(), reached_max_infectees_count,
                this->density_transmission_maximum_infectees, number_infectious_hosts);
  }
  place->reset_place_state(condition_id);
  if (0 && new_infections > 0) {
    FRED_VERBOSE(0, "density_transmission DAY %d PLACE %s gives %d new_infections\n",
		 day, place->get_label(), new_infections);
  }
}


/*
static bool compare_age (Person* p1, Person* p2) {
  return ((p1->get_age() == p2->get_age()) ? (p1->get_id() < p2->get_id()) : (p1->get_age() < p2->get_age()));
}

static bool compare_id (Person* p1, Person* p2) {
  return p1->get_id() < p2->get_id();
}

void Respiratory_Transmission::age_based_transmission_model(int day, int condition_id, Place * place) {
  person_vec_t * infectious = place->get_infectious_people(condition_id);
  person_vec_t * susceptibles = place->get_enrollees();

  Condition* condition = Global::Conditions.get_condition(condition_id);

  int N = place->get_size();
  std::vector<double> infectivity_of_agent((int) infectious->size());
  int n[101];
  int start[101];
  int s[101];
  int size[101];
  double p[101][101];
  double infectivity_of_group[101];
  int infectee_count[101];
  double beta = condition->get_transmissibility();
  double force = beta * place->intimacy;

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
    double infectivity = person->get_infectivity(condition_id); 
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
	p[i][j] = force * Respiratory_Transmission::prob_contact[i][j] * infectivity_of_group[j] / size[j];
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
	if(0 && place->is_household()) {
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
      if(1 || place->is_household()) {
        printf("PLACE TYPE %c AGE GROUP %d SIZE %d PROB_INF %0.6f EXPECTED %0.2f INFECTEE COUNT %d\n",
	       place->type, i, s[i], prob_infection, expected_infections, infectee_count[i]);
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

  // randomize the order of processing the susceptible list
  std::vector<int> shuffle_index;
  int number_of_susceptibles = susceptibles->size();
  shuffle_index.reserve(number_of_susceptibles);
  for (int i = 0; i < number_of_susceptibles; i++) {
    shuffle_index[i] = i;
  }
  FYShuffle<int>(shuffle_index);

  // infect susceptibles according to the infectee_counts
  for(int i = 0; i < number_of_susceptibles; ++i) {
    int sus_pos = shuffle_index[i];
    // susceptible visitor
    Person* infectee = (*susceptibles)[sus_pos];
    infectee->update_schedule(day);
    if (!infectee->is_present(day, place)) {
      continue;
    }
    int age = infectee->get_age();
    if(age > 100) {
      age = 100;
    }
    if(infectee_count[age] > 0) {

      // is infectee susceptible?
      double r = Random::draw_random();
      if(r < infectee->get_susceptibility(condition_id)) {

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
	infector->infect(infectee, condition_id, place, day);

	FRED_CONDITIONAL_VERBOSE(1, infector->get_exposure_date(condition_id) == 0,
				 "SEED infection day %i from %d to %d\n",
				 day, infector->get_id(), infectee->get_id());
	FRED_CONDITIONAL_VERBOSE(1, infector->get_exposure_date(condition_id) != 0,
				 "infection day %i of condition %i from %d to %d\n",
				 day, condition_id, infector->get_id(),
				 infectee->get_id());
	
	// notify the epidemic
	Global::Conditions.get_condition(condition_id)->get_epidemic()->become_exposed(infectee, day);

      }
      // decrement counter (even if transmission fails)
      infectee_count[age]++;
    }
  }
  place->reset_place_state(condition_id);
}
  */

