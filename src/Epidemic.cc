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
// File: Epidemic.cc
//
#include <stdlib.h>
#include <stdio.h>
#include <new>
#include <iostream>
#include <vector>
#include <map>
#include <set>

using namespace std;

#include "Date.h"
#include "Condition.h"
#include "Epidemic.h"
#include "Events.h"
#include "Geo.h"
#include "Global.h"
#include "Household.h"
#include "Infection.h"
#include "Natural_History.h"
#include "Neighborhood_Layer.h"
#include "Params.h"
#include "Person.h"
#include "Place.h"
#include "Place_List.h"
#include "Population.h"
#include "Random.h"
#include "School.h"
#include "Sexual_Transmission_Network.h"
#include "Tracker.h"
#include "Transmission.h"
#include "Utils.h"
#include "Vector_Layer.h"
#include "Workplace.h"

#include "Markov_Epidemic.h"
#include "HIV_Epidemic.h"

/**
 * This static factory method is used to get an instance of a specific
 * Epidemic Model.  Depending on the model parameter, it will create a
 * specific Epidemic Model and return a pointer to it.
 *
 * @param a string containing the requested Epidemic model type
 * @return a pointer to a Epidemic model
 */

Epidemic* Epidemic::get_epidemic(Condition* condition) {
  if(strcmp(condition->get_condition_name(), "drug_use") == 0) {
    return new Markov_Epidemic(condition);
  }
  else if(strcmp(condition->get_condition_name(), "hiv") == 0) {
    return new HIV_Epidemic(condition);
  } else {
    return new Epidemic(condition);
  }
}

//////////////////////////////////////////////////////
//
// CONSTRUCTOR
//
//

Epidemic::Epidemic(Condition* dis) {
  this->condition = dis;
  this->id = condition->get_id();
  this->causes_infection = condition->causes_infection();
  this->susceptible_people = 0;
  this->exposed_people = 0;
  this->people_becoming_infected_today = 0;
  this->infectious_people = 0;
  this->people_becoming_symptomatic_today = 0;
  this->people_with_current_symptoms = 0;
  this->removed_people = 0;
  this->immune_people = 0;
  this->vaccinated_people = 0;
  this->report_generation_time = false;
  this->report_transmission_by_age = false;
  this->population_infection_counts.tot_ppl_evr_inf = 0;
  this->population_infection_counts.tot_ppl_evr_sympt = 0;
  this->population_infection_counts.tot_days_sympt = 0;
  this->attack_rate = 0.0;
  this->symptomatic_attack_rate = 0.0;
  this->total_serial_interval = 0.0;
  this->total_secondary_cases = 0;
  this->N_init = 0;
  this->N = 0;
  this->prevalence_count = 0;
  this->incidence = 0;
  this->symptomatic_incidence = 0;
  this->prevalence = 0.0;
  this->RR = 0.0;
  this->daily_case_fatality_count = 0;
  this->total_case_fatality_count = 0;
  this->case_fatality_incidence = 0;
  this->counties = 0;
  this->county_incidence = NULL;
  this->census_tract_incidence = NULL;
  this->census_tract_symp_incidence = NULL;
  this->census_tracts = 0;
  this->fraction_seeds_infectious = 0.0;

  if (this->causes_infection == false) {
    //no need for remaining data
    return;
  }

  this->daily_cohort_size = new int[Global::Days];
  this->number_infected_by_cohort = new int[Global::Days];
  for(int i = 0; i < Global::Days; ++i) {
    this->daily_cohort_size[i] = 0;
    this->number_infected_by_cohort[i] = 0;
  }

  if(Global::Report_Mean_Household_Stats_Per_Income_Category) {
    //Values for household income based stratification
    for(int i = 0; i < Household_income_level_code::UNCLASSIFIED; ++i) {
      this->household_income_infection_counts_map[i].tot_ppl_evr_inf = 0;
      this->household_income_infection_counts_map[i].tot_ppl_evr_sympt = 0;
      this->household_income_infection_counts_map[i].tot_chldrn_evr_inf = 0;
      this->household_income_infection_counts_map[i].tot_chldrn_evr_sympt = 0;
    }
  }

  if(Global::Report_Epidemic_Data_By_County) {
    int n = Global::Places.get_number_of_counties();
    for(int i = 0; i < n; ++i) {
      int fips = Global::Places.get_fips_of_county_with_index(i);
      this->county_infection_counts_map[fips].current_infected = 0;
      this->county_infection_counts_map[fips].current_symptomatic = 0;
      this->county_infection_counts_map[fips].current_case_fatalities = 0;
    }
  }

  if(Global::Report_Epidemic_Data_By_Census_Tract) {
    //Values for household census_tract based stratification
    for(std::set<long int>::iterator census_tract_itr = Household::census_tract_set.begin();
	      census_tract_itr != Household::census_tract_set.end(); ++census_tract_itr) {
      this->census_tract_infection_counts_map[*census_tract_itr].current_infected = 0;
      this->census_tract_infection_counts_map[*census_tract_itr].current_symptomatic = 0;
      this->census_tract_infection_counts_map[*census_tract_itr].tot_ppl_evr_inf = 0;
      this->census_tract_infection_counts_map[*census_tract_itr].tot_ppl_evr_sympt = 0;
      this->census_tract_infection_counts_map[*census_tract_itr].tot_chldrn_evr_inf = 0;
      this->census_tract_infection_counts_map[*census_tract_itr].tot_chldrn_evr_sympt = 0;
    }
  }

  if(Global::Report_Presenteeism) {
    //Values for childhood school income-based stratification
    for(int i = 0; i < Workplace::get_workplace_size_group_count(); ++i) {
      this->workplace_size_infection_counts_map[i].tot_ppl_evr_sympt = 0;
    }
  }

  if(Global::Report_Childhood_Presenteeism) {
    //Values for childhood school income-based stratification
    for(int i = 0; i < 4; ++i) {
      this->school_income_infection_counts_map[i].tot_chldrn_evr_inf = 0;
      this->school_income_infection_counts_map[i].tot_sch_age_chldrn_evr_inf = 0;
      this->school_income_infection_counts_map[i].tot_sch_age_chldrn_w_home_adlt_crgvr_evr_inf = 0;
      this->school_income_infection_counts_map[i].tot_chldrn_evr_sympt = 0;
      this->school_income_infection_counts_map[i].tot_sch_age_chldrn_evr_sympt = 0;
      this->school_income_infection_counts_map[i].tot_sch_age_chldrn_w_home_adlt_crgvr_evr_sympt = 0;
    }
  }

  this->daily_infections_list.reserve(Global::Pop.get_population_size());
  this->daily_infections_list.clear();

  this->daily_symptomatic_list.reserve(Global::Pop.get_population_size());
  this->daily_symptomatic_list.clear();

  this->imported_cases_map.clear();
  this->import_by_age = false;
  this->import_age_lower_bound = 0;
  this->import_age_upper_bound = Demographics::MAX_AGE;
  this->seeding_type = SEED_EXPOSED;

  this->infectious_start_event_queue = new Events;
  this->infectious_end_event_queue = new Events;
  this->symptoms_start_event_queue = new Events;
  this->symptoms_end_event_queue = new Events;
  this->immunity_start_event_queue = new Events;
  this->immunity_end_event_queue = new Events;

  this->infected_people.clear();
  this->potentially_infectious_people.clear();
  this->new_infected_people.clear();
  this->new_symptomatic_people.clear();
  this->new_infectious_people.clear();
  this->recovered_people.clear();

  this->actually_infectious_people.reserve(Global::Pop.get_population_size());
  this->actually_infectious_people.clear();
}

Epidemic::~Epidemic() {
}

//////////////////////////////////////////////////////
//
// SETUP
//
//

void Epidemic::setup() {
  using namespace Utils;
  char paramstr[FRED_STRING_SIZE];
  char map_file_name[FRED_STRING_SIZE];
  int temp;

  // read time_step_map
  Params::get_param_from_string("primary_cases_file", map_file_name);
  // If this parameter is "none", then there is no map
  if(strncmp(map_file_name, "none", 4) != 0){
    Utils::get_fred_file_name(map_file_name);
    ifstream* ts_input = new ifstream(map_file_name);
    if(!ts_input->is_open()) {
      Utils::fred_abort("Help!  Can't read %s Timestep Map\n", map_file_name);
      abort();
    }
    string line;
    while(getline(*ts_input,line)){
      if(line[0] == '\n' || line[0] == '#') { // empty line or comment
	      continue;
      }
      char cstr[FRED_STRING_SIZE];
      std::strcpy(cstr, line.c_str());
      Time_Step_Map* tmap = new Time_Step_Map;
      int n = sscanf(cstr,
		     "%d %d %d %d %lf %d %lf %lf %lf",
		     &tmap->sim_day_start, &tmap->sim_day_end, &tmap->num_seeding_attempts,
		     &tmap->condition_id, &tmap->seeding_attempt_prob, &tmap->min_num_successful,
		     &tmap->lat, &tmap->lon, &tmap->radius);
      if(n < 3) {
	      Utils::fred_abort("Need to specify at least SimulationDayStart, SimulationDayEnd and NumSeedingAttempts for Time_Step_Map. ");
      }
      if(n < 9) {
        tmap->lat = 0.0;
        tmap->lon = 0.0;
        tmap->radius = -1;
      }
      if(n < 6) {
        tmap->min_num_successful = 0;
      }
      if(n < 5) {
        tmap->seeding_attempt_prob = 1;
      }
      if(n < 4) {
        tmap->condition_id = 0;
      }
      this->imported_cases_map.push_back(tmap);
    }
    ts_input->close();
  }
  if(Global::Verbose > 1) {
    for(int i = 0; i < this->imported_cases_map.size(); ++i) {
      string ss = this->imported_cases_map[i]->to_string();
      printf("%s\n", ss.c_str());
    }
  }

  Params::get_param_from_string("seed_by_age", &temp);
  this->import_by_age = (temp == 0 ? false : true);
  Params::get_param_from_string("seed_age_lower_bound", &import_age_lower_bound);
  Params::get_param_from_string("seed_age_upper_bound", &import_age_upper_bound);
  Params::get_param_from_string("report_generation_time", &temp);
  this->report_generation_time = (temp > 0);
  Params::get_param_from_string("report_transmission_by_age", &temp);
  this->report_transmission_by_age = (temp > 0);
  Params::get_param_from_string("advanced_seeding", this->seeding_type_name);
  if(!strcmp(this->seeding_type_name, "random")) {
    this->seeding_type = SEED_RANDOM;
  } else if(!strcmp(this->seeding_type_name, "exposed")) {
    this->seeding_type = SEED_EXPOSED;
  } else if(!strcmp(this->seeding_type_name, "infectious")) {
    this->fraction_seeds_infectious = 1.0;
    this->seeding_type = SEED_INFECTIOUS;
  } else if(strchr(this->seeding_type_name, ';') != NULL) {
    // format is exposed:0.0000; infectious:0.0000
    Tokens tokens = split_by_delim(this->seeding_type_name, ';', false);
    assert(strchr(tokens[0], ':') != NULL);
    assert(strchr(tokens[1], ':') != NULL);
    Tokens t1 = split_by_delim(tokens[0], ':', false);
    Tokens t2 = split_by_delim(tokens[1], ':', false);
    assert(strcmp(t1[0], "exposed") == 0);
    assert(strcmp(t2[0], "infectious") == 0);
    double fraction_exposed, fraction_infectious;
    fraction_exposed = atof(t1[1]);
    fraction_infectious = atof(t2[1]);
    assert(fraction_exposed + fraction_infectious <= 1.01);
    assert(fraction_exposed + fraction_infectious >= 0.99);
    this->fraction_seeds_infectious = fraction_infectious;
  } else {
    Utils::fred_abort("Invalid advance_seeding parameter: %s!\n", this->seeding_type_name);
  }
}

//////////////////////////////////////////////////////
//
// FINAL PREP FOR THE RUN
//
//

void Epidemic::prepare() {
  create_visualization_data_directories();
}


///////////////////////////////////////////////////////
//
// DAILY UPDATES
//
//

void Epidemic::update(int day) {

  FRED_VERBOSE(0, "epidemic update for condition %d day %d\n", id, day);
  Utils::fred_start_epidemic_timer();

  if(Global::Enable_Visualization_Layer) {
    this->new_infected_people.clear();
    this->new_symptomatic_people.clear();
    this->new_infectious_people.clear();
    this->recovered_people.clear();
  }

  if(this->causes_infection) {
    // import infections from unknown sources
    get_imported_infections(day);
    // Utils::fred_print_epidemic_timer("imported infections");
  }

  // update markov transitions
  markov_updates(day);

  if(this->causes_infection == false) {
    FRED_VERBOSE(0, "epidemic update finished for condition %d day %d\n", id, day);
    return;
  }

  // transition to infectious
  process_infectious_start_events(day);

  // transition to noninfectious
  process_infectious_end_events(day);

  // transition to symptomatic
  process_symptoms_start_events(day);

  // transition to asymptomatic
  process_symptoms_end_events(day);

  // transition to immune
  process_immunity_start_events(day);

  // transition to susceptible
  process_immunity_end_events(day);

  // Utils::fred_print_epidemic_timer("transition events");

  // update list of infected people
  for(std::set<Person*>::iterator it = this->infected_people.begin(); it != this->infected_people.end(); ) {
    Person* person = (*it);
    FRED_VERBOSE(1, "update_infection for person %d day %d\n", person->get_id(), day);
    person->update_infection(day, this->id);

    // handle case fatality
    if(person->is_case_fatality(this->id)) {
      // update epidemic fatality counters
      this->daily_case_fatality_count++;
      this->total_case_fatality_count++;
      // record removed person
      this->removed_people++;
      if(Global::Enable_Visualization_Layer) {
	      print_visualization_data_for_case_fatality(day, person);
      }
      if(Global::Report_Epidemic_Data_By_County) {
	      int fips = Global::Places.get_county_for_place(person->get_household());
	      this->county_infection_counts_map[fips].current_infected--;
	      this->county_infection_counts_map[fips].current_case_fatalities++;
      }
    }

    // note: case fatalities will be uninfected at this point
    if(person->is_infected(this->id) == false) {
      FRED_VERBOSE(1, "update_infection for person %d day %d - deleting from infected_people list\n", person->get_id(), day);
      // delete from infected list
      this->infected_people.erase(it++);
    } else {
      // update person's mixing group infection counters
      person->update_household_counts(day, this->id);
      person->update_school_counts(day, this->id);
      // move on the next infected person    
      ++it;
    }
  }

  // get list of actually infectious people
  this->actually_infectious_people.clear();
  for(std::set<Person*>::iterator it = this->potentially_infectious_people.begin(); it != this->potentially_infectious_people.end(); ++it) {
    Person* person = (*it);
    if(person->is_infectious(this->id)) {
      this->actually_infectious_people.push_back(person);
      FRED_VERBOSE(1, "ACTUALLY INF day %d person %d\n", day, person->get_id());
    }
  }
  this->infectious_people = this->actually_infectious_people.size();
  // Utils::fred_print_epidemic_timer("identifying actually infections people");

  // update the daily activities of infectious people
  for(int i = 0; i < this->infectious_people; ++i) {
    Person* person = this->actually_infectious_people[i];

    if(strcmp("sexual", this->condition->get_transmission_mode()) == 0) {
      FRED_VERBOSE(1, "ADDING_ACTUALLY INF person %d\n", person->get_id());
      // this will insert the infectious person onto the infectious list in sexual partner network
      Sexual_Transmission_Network* st_network = Global::Sexual_Partner_Network;
      st_network->add_infectious_person(this->id, person);
    } else {
      FRED_VERBOSE(1, "updating activities of infectious person %d -- %d out of %d\n", person->get_id(), i, this->infectious_people);
      person->update_activities_of_infectious_person(day);
      // note: infectious person will be added to the daily places in find_active_places_of_type()
    }
  }
  Utils::fred_print_epidemic_timer("scheduled updated");

  if(strcmp("sexual", this->condition->get_transmission_mode()) == 0) {
    Sexual_Transmission_Network* st_network = Global::Sexual_Partner_Network;
    this->condition->get_transmission()->spread_infection(day, this->id, st_network);
    st_network->clear_infectious_people(this->id);
  } else {
    // spread infection in places attended by actually infectious people
    for(int type = 0; type < 7; ++type) {
      find_active_places_of_type(day, type);
      spread_infection_in_active_places(day);
      char msg[80];
      sprintf(msg, "spread_infection for type %d", type);
      Utils::fred_print_epidemic_timer(msg);
    }
  }

  FRED_VERBOSE(0, "epidemic update finished for condition %d day %d\n", id, day);
  return;
}


void Epidemic::get_imported_infections(int day) {

  FRED_VERBOSE(0, "GET_IMPORTED_INFECTIONS %d map_size %d\n",  day, this->imported_cases_map.size());

  this->N = Global::Pop.get_population_size();

  for(int i = 0; i < this->imported_cases_map.size(); ++i) {
    Time_Step_Map* tmap = this->imported_cases_map[i];
    if(tmap->sim_day_start <= day && day <= tmap->sim_day_end) {
      FRED_VERBOSE(0,"IMPORT MST:\n"); // tmap->print();
      int imported_cases_requested = tmap->num_seeding_attempts;
      int imported_cases = 0;
      fred::geo lat = tmap->lat;
      fred::geo lon = tmap->lon;
      double radius = tmap->radius;
      // list of susceptible people that qualify by distance and age
      std::vector<Person*> people;
      if(this->import_by_age) {
	      FRED_VERBOSE(0, "IMPORT import by age %0.2f %0.2f\n", this->import_age_lower_bound, this->import_age_upper_bound);
      }

      int searches_within_given_location = 1;
      while(searches_within_given_location <= 10) {
	      FRED_VERBOSE(0,"IMPORT search number %d ", searches_within_given_location);
	
	      // clear the list of candidates
	      people.clear();
	
	      // find households that qualify by distance
	      int hsize = Global::Places.get_number_of_households();
	      // printf("IMPORT: houses  %d\n", hsize); fflush(stdout);
	      for(int i = 0; i < hsize; ++i) {
	        Household* house = Global::Places.get_household_ptr(i);
	        double dist = 0.0;
	        if(radius > 0) {
	          dist = Geo::xy_distance(lat,lon,house->get_latitude(),house->get_longitude());
	          if(radius < dist) {
	            continue;
	          }
	        }
	        // this household qualifies by distance.
	        // find all susceptible housemates who qualify by age.
	        int size = house->get_size();
	        // printf("IMPORT: house %s size %d\n", house->get_label(), size); fflush(stdout);
	        for(int j = 0; j < size; ++j) {
	          Person* person = house->get_enrollee(j);
	          if(person->get_health()->is_susceptible(this->id)) {
	            double age = person->get_real_age();
	            if(this->import_age_lower_bound <= age && age <= this->import_age_upper_bound) {
		            people.push_back(person);
	            }
	          }
	        }
	      }

	      int imported_cases_remaining = imported_cases_requested - imported_cases;
        FRED_VERBOSE(0, "IMPORT: seeking %d candidates, found %d\n", imported_cases_remaining, (int) people.size());

	      if(imported_cases_remaining <= people.size()) {
	        // we have at least the minimum number of candidates.
	        for(int n = 0; n < imported_cases_remaining; ++n) {
	          FRED_VERBOSE(0, "IMPORT candidate %d people.size %d\n", n, (int)people.size());

	          // pick a candidate without replacement
	          int pos = Random::draw_random_int(0,people.size()-1);
	          Person* infectee = people[pos];
	          people[pos] = people[people.size() - 1];
	          people.pop_back();

	          // infect the candidate
	          FRED_VERBOSE(0, "IMPORT infecting candidate %d id %d\n", n, infectee->get_id());
		        transition_person(infectee, day, 1);
	          imported_cases++;
	        }
	        FRED_VERBOSE(0, "IMPORT SUCCESS: %d imported cases\n", imported_cases);
	        return; // success!
	      } else {
	        // infect all the candidates
	        for(int n = 0; n < people.size(); ++n) {
	          Person* infectee = people[n];
		        transition_person(infectee, day, 1);
	          imported_cases++;
	        }
	      }

	      if(radius > 0) {
	        // expand the distance and try again
	        radius = 2 * radius;
	        FRED_VERBOSE(0, "IMPORT: increasing radius to %f\n", radius);
	        searches_within_given_location++;
	      }	else {
	        // return with a warning
	        FRED_VERBOSE(0, "IMPORT FAILURE: only %d imported cases out of %d\n", imported_cases, imported_cases_requested);
	        return;
	      }
      } //End while(searches_within_given_location <= 10)
      // after 10 tries, return with a warning
      FRED_VERBOSE(0, "IMPORT FAILURE: only %d imported cases out of %d\n", imported_cases, imported_cases_requested);
      return;
    }
  }
}

void Epidemic::advance_seed_infection(Person* person) {
  // if advanced_seeding is infectious or random
  int d = this->condition->get_id();
  int advance = 0;
  int duration = person->get_infectious_end_date(d) - person->get_exposure_date(d);
  assert(duration > 0);
  if(this->seeding_type == SEED_RANDOM) {
    advance = Random::draw_random_int(0, duration);
  } else if(Random::draw_random() < this->fraction_seeds_infectious ) {
    advance = person->get_infectious_start_date(d) - person->get_exposure_date(d);
  }
  assert(advance <= duration);
  FRED_VERBOSE(0, "%s %s %s %d %s %d %s\n", "advanced_seeding:", seeding_type_name,
	       "=> advance infection trajectory of duration", duration, "by", advance, "days");
  person->advance_seed_infection(d, advance);
}


void Epidemic::find_active_places_of_type(int day, int place_type) {

  FRED_VERBOSE(1, "find_active_places_of_type %d\n", place_type);
  this->active_places.clear();
  FRED_VERBOSE(1, "find_active_places_of_type %d actual %d\n", place_type, this->infectious_people);
  for(int i = 0; i < this->infectious_people; i++) {
    Person* person =  this->actually_infectious_people[i];
    assert(person!=NULL);
    Place* place = NULL;
    switch(place_type) {
    case 0:
      place = person->get_household();
      break;
    case 1:
      place = person->get_neighborhood();
      break;
    case 2:
      place = person->get_school();
      break;
    case 3:
      place = person->get_classroom();
      break;
    case 4:
      place = person->get_workplace();
      break;
    case 5:
      place = person->get_office();
      break;
    case 6:
      place = person->get_hospital();
      break;
    }
    FRED_VERBOSE(1, "find_active_places_of_type %d person %d place %s\n", place_type, person->get_id(), place? place->get_label() : "NULL");
    if(place != NULL && person->is_present(day, place) && person->is_infectious(this->id)) {
      FRED_VERBOSE(1, "add_infection_person %d place %s\n", person->get_id(), place->get_label());
      place->add_infectious_person(this->id, person);
      this->active_places.insert(place);
    }
  }
  
  // vector transmission mode (for dengue and chikungunya)
  if(strcmp("vector",this->condition->get_transmission_mode()) == 0) {

    // add all places that have any infectious vectors
    int size = 0;
    switch (place_type) {
    case 0:
      // add households
      size = Global::Places.get_number_of_households();
      for(int i = 0; i < size; ++i) {
	      Place* place = Global::Places.get_household(i);
	      if(place->get_infectious_vectors(this->id) > 0) {
	        this->active_places.insert(place);
	      }
      }
      break;
    case 2:
      // add schools
      size = Global::Places.get_number_of_schools();
      for(int i = 0; i < size; ++i) {
	      Place* place = Global::Places.get_school(i);
	      if(place->get_infectious_vectors(this->id) > 0) {
	        this->active_places.insert(place);
	      }
      }
      break;
    case 4:
      // add workplaces
      size = Global::Places.get_number_of_workplaces();
      for(int i = 0; i < size; ++i) {
	      Place* place = Global::Places.get_workplace(i);
	      if(place->get_infectious_vectors(this->id) > 0) {
	        this->active_places.insert(place);
	      }
      }
      break;
    }
  }

  FRED_VERBOSE(1, "find_active_places_of_type %d found %d\n", place_type, this->active_places.size());

  // convert active set to vector
  this->active_place_vec.clear();
  for(std::set<Place*>::iterator it = active_places.begin(); it != this->active_places.end(); ++it) {
    this->active_place_vec.push_back(*it);
  }
  FRED_VERBOSE(0, "find_active_places_of_type %d day %d found %d\n", place_type, day,  this->active_place_vec.size());

}
  
void Epidemic::spread_infection_in_active_places(int day) {
  FRED_VERBOSE(0, "spread_infection__active_places day %d\n", day);
  for(int i = 0; i < this->active_place_vec.size(); ++i) {
    Place* place = this->active_place_vec[i];
    this->condition->get_transmission()->spread_infection(day, this->id, place);
    place->clear_infectious_people(this->id);
  }
  return;
}


//////////////////////////////////////////////////////////////////
//
//  HANDLING EVENT QUEUES
//
//

void Epidemic::process_symptoms_start_events(int day) {
  int size = this->symptoms_start_event_queue->get_size(day);
  FRED_VERBOSE(1, "SYMP_START_EVENT_QUEUE day %d size %d\n", day, size);

  for(int i = 0; i < size; ++i) {
    Person* person =  this->symptoms_start_event_queue->get_event(day, i);

    become_symptomatic(person, day);

    // update next event list
    int symptoms_end_date = person->get_symptoms_end_date(this->id);
    if(Global::Report_Presenteeism) {
      this->population_infection_counts.tot_days_sympt += (symptoms_end_date - day);
    }
    this->symptoms_end_event_queue->add_event(symptoms_end_date, person);
  }

  this->symptoms_start_event_queue->clear_events(day);
}

void Epidemic::process_symptoms_end_events(int day) {
  int size = symptoms_end_event_queue->get_size(day);
  FRED_VERBOSE(1, "SYMP_END_EVENT_QUEUE day %d size %d\n", day, size);

  for(int i = 0; i < size; ++i) {
    Person* person = this->symptoms_end_event_queue->get_event(day, i);
    become_asymptomatic(person, day);

    // check to see if person has fully recovered:
    int infectious_start_date = person->get_infectious_start_date(this->id);
    int infectious_end_date = person->get_infectious_end_date(this->id);
    if((infectious_start_date == -1) || (-1 < infectious_end_date && infectious_end_date <= day)) {
      recover(person, day);
    }
  }
  this->symptoms_end_event_queue->clear_events(day);
}

void Epidemic::process_immunity_start_events(int day) {
  int size = immunity_start_event_queue->get_size(day);
  FRED_VERBOSE(1, "IMMUNITY_START_EVENT_QUEUE day %d size %d\n", day, size);

  for(int i = 0; i < size; ++i) {
    Person* person = this->immunity_start_event_queue->get_event(day, i);

    // update epidemic counters
    this->immune_people++;

    // update person's health record
    // person->become_immune(this->id);
  }
  this->immunity_start_event_queue->clear_events(day);
}

void Epidemic::process_immunity_end_events(int day) {
  int size = immunity_end_event_queue->get_size(day);
  FRED_VERBOSE(1, "IMMUNITY_END_EVENT_QUEUE day %d size %d\n", day, size);

  for(int i = 0; i < size; ++i) {
    Person* person = this->immunity_end_event_queue->get_event(day, i);

    // update epidemic counters
    this->immune_people--;
    
    // update epidemic counters
    this->removed_people--;
    
    // update person's health record
    person->become_susceptible(this->id);
  }
  this->immunity_end_event_queue->clear_events(day);
}

void Epidemic::cancel_symptoms_start(int day, Person* person) {
  this->symptoms_start_event_queue->delete_event(day, person);
}

void Epidemic::cancel_symptoms_end(int day, Person* person) {
  this->symptoms_end_event_queue->delete_event(day, person);
}

void Epidemic::cancel_infectious_start(int day, Person* person) {
  this->infectious_start_event_queue->delete_event(day, person);
}

void Epidemic::cancel_infectious_end(int day, Person* person) {
  this->infectious_end_event_queue->delete_event(day, person);
}

void Epidemic::cancel_immunity_start(int day, Person* person) {
  this->immunity_start_event_queue->delete_event(day, person);
}

void Epidemic::cancel_immunity_end(int day, Person* person) {
  this->immunity_end_event_queue->delete_event(day, person);
}



//////////////////////////////////////////////////////////////
//
// HANDLING CHAGES TO AN INDIVIDUAL'S STATUS
//
//

void Epidemic::become_exposed(Person* person, int day) {

  // FRED_VERBOSE(0, "become exposed day %d person %d\n", day, person->get_id());

  this->infected_people.insert(person);
  if(Global::Enable_Visualization_Layer) {
    this->new_infected_people.insert(person);
  }

  // update next event list
  int infectious_start_date = -1;
  if(this->condition->get_transmissibility() > 0.0) {
    infectious_start_date = person->get_infectious_start_date(this->id);
    if(0 <= infectious_start_date && infectious_start_date <= day) {
      FRED_VERBOSE(0, "TIME WARP day %d inf %d\n", day, infectious_start_date);
      infectious_start_date = day + 1;
    }
    this->infectious_start_event_queue->add_event(infectious_start_date, person);
  } else {
    // This condition is not transmissible, therefore, no one ever becomes
    // infectious.  Consequently, spread_infection is never called. So
    // no transmission model is even generated (see Condition::setup()).

    // This is how FRED supports non-communicable condition. Just use the parameter:
    // <condition_name>_transmissibility = 0
    //
  }

  int symptoms_start_date = person->get_symptoms_start_date(this->id);
  if(0 <= symptoms_start_date && symptoms_start_date <= day) {
    FRED_VERBOSE(0, "TIME WARP day %d symp %d\n", day, symptoms_start_date);
    symptoms_start_date = day + 1;
  }
  this->symptoms_start_event_queue->add_event(symptoms_start_date, person);

  // update epidemic counters
  this->exposed_people++;
  this->people_becoming_infected_today++;

  if(Global::Report_Mean_Household_Stats_Per_Income_Category) {
    if(person->get_household() != NULL) {
      Household* hh = static_cast<Household*>(person->get_household());
      int income_level = hh->get_household_income_code();
      if(income_level >= Household_income_level_code::CAT_I &&
         income_level < Household_income_level_code::UNCLASSIFIED) {
        this->household_income_infection_counts_map[income_level].tot_ppl_evr_inf++;
      }
    }
  }

  if(Global::Report_Epidemic_Data_By_County) {
    int fips = Global::Places.get_county_for_place(person->get_household());
    this->county_infection_counts_map[fips].current_infected++;
  }

  if(Global::Report_Epidemic_Data_By_Census_Tract) {
    if(person->get_household() != NULL) {
      Household* hh = static_cast<Household*>(person->get_household());
      long int census_tract = hh->get_census_tract_fips();
      if(Household::census_tract_set.find(census_tract) != Household::census_tract_set.end()) {
        this->census_tract_infection_counts_map[census_tract].tot_ppl_evr_inf++;
        if(person->is_child()) {
          this->census_tract_infection_counts_map[census_tract].tot_chldrn_evr_inf++;
        }
      }
    }
  }

  if(Global::Report_Childhood_Presenteeism) {
    if(person->is_student() &&
       person->get_school() != NULL &&
       person->get_household() != NULL) {
      School* schl = static_cast<School*>(person->get_school());
      Household* hh = static_cast<Household*>(person->get_household());
      int income_quartile = schl->get_income_quartile();

      if(person->is_child()) { //Already know person is student
        this->school_income_infection_counts_map[income_quartile].tot_chldrn_evr_inf++;
        this->school_income_infection_counts_map[income_quartile].tot_sch_age_chldrn_evr_inf++;
      }

      if(hh->has_school_aged_child_and_unemployed_adult()) {
        this->school_income_infection_counts_map[income_quartile].tot_sch_age_chldrn_w_home_adlt_crgvr_evr_inf++;
      }
    }
  }

  this->daily_infections_list.push_back(person);
}



void Epidemic::become_infectious(Person* person, int day) {

  // FRED_VERBOSE(0, "become infectious day %d person %d\n", day, person->get_id());

  // add to active people list
  this->potentially_infectious_people.insert(person);
  if(Global::Enable_Visualization_Layer) {
    this->new_infectious_people.insert(person);
  }

  // update epidemic counters
  this->exposed_people--;

  // update person's health record
  person->become_infectious(this->condition);
}


void Epidemic::become_noninfectious(Person* person, int day) {

  // FRED_VERBOSE(0, "become noninfectious day %d person %d\n", day, person->get_id());

  // update person's health record
  person->become_noninfectious(this->condition);
  
}


void Epidemic::become_symptomatic(Person* person, int day) {

  // FRED_VERBOSE(0, "become symptomatic day %d person %d\n", day, person->get_id());

  // update epidemic counters
  this->people_with_current_symptoms++;
  this->people_becoming_symptomatic_today++;
  if(Global::Enable_Visualization_Layer) {
    this->new_symptomatic_people.insert(person);
  }

  if(Global::Report_Epidemic_Data_By_County) {
    int fips = Global::Places.get_county_for_place(person->get_household());
    this->county_infection_counts_map[fips].current_symptomatic++;
    // FRED_VERBOSE(0, "SYMP_START_EVENT day %d person %d  fips %d count %d\n",
    // day, person->get_id(), fips, this->county_infection_counts_map[fips].current_symptomatic);
  }

  if(Global::Report_Mean_Household_Stats_Per_Income_Category) {
    if(person->get_household() != NULL) {
      int income_level = static_cast<Household*>(person->get_household())->get_household_income_code();
      if(income_level >= Household_income_level_code::CAT_I &&
	      income_level < Household_income_level_code::UNCLASSIFIED) {
	      this->household_income_infection_counts_map[income_level].tot_ppl_evr_sympt++;
	      if(person->is_child()) {
	        this->household_income_infection_counts_map[income_level].tot_chldrn_evr_sympt++;
	      }
      }
    }
  }

  if(Global::Report_Epidemic_Data_By_Census_Tract) {
    if(person->get_household() != NULL) {
      Household* hh = static_cast<Household*>(person->get_household());
      long int census_tract = hh->get_census_tract_fips();
      if(Household::census_tract_set.find(census_tract) != Household::census_tract_set.end()) {
	      this->census_tract_infection_counts_map[census_tract].tot_ppl_evr_sympt++;
	      if(person->is_child()) {
	        this->census_tract_infection_counts_map[census_tract].tot_chldrn_evr_sympt++;
	      }
      }
    }
  }

  if(Global::Report_Presenteeism) {
    char wp_symp[30];
    Workplace* wp = static_cast<Workplace*>(person->get_workplace());
    int index = Activities::get_index_of_sick_leave_dist(person);
    if(wp != NULL && index != -1) {
      this->workplace_size_infection_counts_map[index].tot_ppl_evr_sympt++;
    }
  }

  if(Global::Report_Childhood_Presenteeism) {
    if(person->get_household() != NULL &&
       person->is_student() &&
       person->get_school() != NULL) {
      Household* hh = static_cast<Household*>(person->get_household());
      School* schl = static_cast<School*>(person->get_school());
      int income_quartile = schl->get_income_quartile();
      if(person->is_child()) { //Already know person is student
        this->school_income_infection_counts_map[income_quartile].tot_chldrn_evr_sympt++;
        this->school_income_infection_counts_map[income_quartile].tot_sch_age_chldrn_evr_sympt++;
      }

      if(hh->has_school_aged_child_and_unemployed_adult()) {
        this->school_income_infection_counts_map[income_quartile].tot_sch_age_chldrn_w_home_adlt_crgvr_evr_sympt++;
      }
    }
  }

  // update person's health record
  person->become_symptomatic(this->condition);
}

void Epidemic::become_asymptomatic(Person* person, int day) {

  // FRED_VERBOSE(0, "become asymptomatic day %d person %d\n", day, person->get_id());

  // update epidemic counters
  this->people_with_current_symptoms--;

  // update person's health record
  person->resolve_symptoms(this->condition);
        
  if(Global::Report_Epidemic_Data_By_County) {
    int fips = Global::Places.get_county_for_place(person->get_household());
    this->county_infection_counts_map[fips].current_symptomatic--;
  }

}

void Epidemic::process_infectious_start_events(int day) {
  int size = this->infectious_start_event_queue->get_size(day);
  FRED_VERBOSE(1, "INF_START_EVENT_QUEUE day %d size %d\n", day, size);

  for(int i = 0; i < size; ++i) {
    Person* person =  this->infectious_start_event_queue->get_event(day, i);

    FRED_VERBOSE(1,"infectious_start_event day %d person %d\n",
		 day, person->get_id());

    become_infectious(person, day);

    // update next event list
    int infectious_end_date = person->get_infectious_end_date(this->id);
    this->infectious_end_event_queue->add_event(infectious_end_date, person);

  }
  this->infectious_start_event_queue->clear_events(day);
}

void Epidemic::process_infectious_end_events(int day) {
  int size =  this->infectious_end_event_queue->get_size(day);
  FRED_VERBOSE(1, "INF_END_EVENT_QUEUE day %d size %d\n", day, size);

  for(int i = 0; i < size; ++i) {
    Person* person =  this->infectious_end_event_queue->get_event(day, i);

    become_noninfectious(person, day);

    // check to see if person has fully recovered:
    int symptoms_start_date = person->get_symptoms_start_date(this->id);
    int symptoms_end_date = person->get_symptoms_end_date(this->id);
    if((symptoms_start_date == -1) || (-1 < symptoms_end_date && symptoms_end_date < day)) {
      recover(person, day);
    }

  }
  this->infectious_end_event_queue->clear_events(day);
}

void Epidemic::recover(Person* person, int day) {

  // FRED_VERBOSE(0, "recover day %d person %d\n", day, person->get_id());

  // remove from active list
  this->potentially_infectious_people.erase(person);
  
  this->removed_people++;

  if(Global::Report_Epidemic_Data_By_County) {
    int fips = Global::Places.get_county_for_place(person->get_household());
    this->county_infection_counts_map[fips].current_infected--;
  }

  if (Global::Enable_Visualization_Layer) {
    this->recovered_people.insert(person);
  }

  // update person's health record
  person->recover(day, this->condition);
}


void Epidemic::terminate_person(Person* person, int day) {

  FRED_VERBOSE(0, "EPIDEMIC TERMINATE person %d day %d  symptoms %d inf %d\n",
	       person->get_id(), day, person->get_symptoms_start_date(this->id), person->get_infectious_start_date(this->id));

  // cancel any events for this person
  int date = person->get_symptoms_start_date(this->id);
  if(date > day) {
    FRED_VERBOSE(0, "EPIDEMIC CANCEL symptoms_start_date %d %d\n", date, day);
    cancel_symptoms_start(date, person);
  }
  else if (date > -1) {
    date = person->get_symptoms_end_date(this->id);
    if(date > day) {
      FRED_VERBOSE(0, "EPIDEMIC CANCEL symptoms_end_date %d %d\n", date, day);
      cancel_symptoms_end(date, person);
      if(Global::Report_Epidemic_Data_By_County) {
	int fips = Global::Places.get_county_for_place(person->get_household());
	this->county_infection_counts_map[fips].current_symptomatic--;
      }
    }
  }

  date = person->get_infectious_start_date(this->id);
  if(date > day) {
    FRED_VERBOSE(0, "EPIDEMIC CANCEL infectious_start_date %d %d\n", date, day);
    cancel_infectious_start(date, person);
  }
  else if (date > -1) {
    date = person->get_infectious_end_date(this->id);
    if(date > day) {
      FRED_VERBOSE(0, "EPIDEMIC CANCEL infectious_end_date %d %d\n", date, day);
      cancel_infectious_end(date, person);
    }
  }

  date = person->get_immunity_end_date(this->id);
  if(date > day) {
    FRED_VERBOSE(0, "EPIDEMIC CANCEL immunity_end_date %d %d\n", date, day);
    cancel_immunity_end(date, person);
  }

  if(Global::Report_Epidemic_Data_By_County) {
    int fips = Global::Places.get_county_for_place(person->get_household());
    this->county_infection_counts_map[fips].current_infected--;
  }

  FRED_VERBOSE(0, "EPIDEMIC TERMINATE finished\n");
}


//////////////////////////////////////////////////////////////
//
// REPORTING
//
//


void Epidemic::report(int day) {
  print_stats(day);
  if(Global::Enable_Visualization_Layer) {
    print_visualization_data_for_active_infections(day);
  }
}


void Epidemic::print_stats(int day) {
  FRED_VERBOSE(1, "epidemic print stats for condition %d day %d\n", id, day);

  // set population size, and remember original pop size
  if(day == 0) {
    this->N_init = this->N = Global::Pop.get_population_size();
  } else {
    this->N = Global::Pop.get_population_size();
  }

  FRED_VERBOSE(1, "report condition specific stats\n");
  report_condition_specific_stats(day);

  if (this->causes_infection == false) {
    // nothing else to report to output file
    return;
  }

  // get reproductive rate for the cohort exposed RR_delay days ago
  // unless RR_delay == 0
  this->daily_cohort_size[day] = this->people_becoming_infected_today;
  this->RR = 0.0;         // reproductive rate for a fixed cohort of infectors
  if(0 < Global::RR_delay && Global::RR_delay <= day) {
    int cohort_day = day - Global::RR_delay;    // exposure day for cohort
    int cohort_size = this->daily_cohort_size[cohort_day];        // size of cohort
    if(cohort_size > 0) {
      // compute reproductive rate for this cohort
      this->RR = static_cast<double>(this->number_infected_by_cohort[cohort_day]) / static_cast<double>(cohort_size);
    }
  }

  this->population_infection_counts.tot_ppl_evr_inf += this->people_becoming_infected_today;
  this->population_infection_counts.tot_ppl_evr_sympt += this->people_becoming_symptomatic_today;

  this->attack_rate = (100.0 * this->population_infection_counts.tot_ppl_evr_inf) / static_cast<double>(this->N_init);
  this->symptomatic_attack_rate = (100.0 * this->population_infection_counts.tot_ppl_evr_sympt) / static_cast<double>(this->N_init);

  // preserve these quantities for use during the next day
  this->incidence = this->people_becoming_infected_today;
  this->symptomatic_incidence = this->people_becoming_symptomatic_today;
  // this->prevalence_count = this->exposed_people + this->infectious_people;
  this->prevalence_count = this->infected_people.size();
  this->prevalence = static_cast<double>(this->prevalence_count) / static_cast<double>(this->N);
  this->case_fatality_incidence = this->daily_case_fatality_count;
  double case_fatality_rate = 0.0;
  if(this->population_infection_counts.tot_ppl_evr_sympt > 0) {
    case_fatality_rate = 100000.0 * static_cast<double>(this->total_case_fatality_count)
      / static_cast<double>(this->population_infection_counts.tot_ppl_evr_sympt);
  }

  if(this->id == 0) {
    Global::Daily_Tracker->set_index_key_pair(day,"Date", Date::get_date_string());
    Global::Daily_Tracker->set_index_key_pair(day,"WkDay", Date::get_day_of_week_string());
    Global::Daily_Tracker->set_index_key_pair(day,"Year", Date::get_epi_year());
    Global::Daily_Tracker->set_index_key_pair(day,"Week", Date::get_epi_week());
    Global::Daily_Tracker->set_index_key_pair(day,"N", this->N);
  }

  this->susceptible_people = this->N - this->exposed_people - this->infectious_people - this->removed_people;
  track_value(day, (char*)"S", this->susceptible_people);
  track_value(day, (char*)"E", this->exposed_people);
  track_value(day, (char*)"I", this->infectious_people);
  track_value(day, (char*)"Is", this->people_with_current_symptoms);
  track_value(day, (char*)"R", this->removed_people);
  track_value(day, (char*)"CF", this->daily_case_fatality_count);
  track_value(day, (char*)"CFR", case_fatality_rate);
  track_value(day, (char*)"M", this->immune_people);
  track_value(day, (char*)"P",this->prevalence_count);
  track_value(day, (char*)"C", this->incidence);
  track_value(day, (char*)"Cs", this->symptomatic_incidence);
  track_value(day, (char*)"AR", this->attack_rate);
  track_value(day, (char*)"ARs", this->symptomatic_attack_rate);
  track_value(day, (char*)"RR", this->RR);

  if(Global::Enable_Vector_Layer && Global::Report_Vector_Population) {
    Global::Vectors->report(day, this);
  }

  if(this->report_transmission_by_age) {
    report_transmission_by_age_group(day);
  }

  if(Global::Report_Presenteeism) {
    report_presenteeism(day);
  }

  if(Global::Report_Childhood_Presenteeism) {
    report_school_attack_rates_by_income_level(day);
  }

  if(Global::Report_Place_Of_Infection) {
    FRED_VERBOSE(0, "report place if infection\n");
    report_place_of_infection(day);
  }

  if(Global::Report_Age_Of_Infection) {
    report_age_of_infection(day);
  }

  if(Global::Report_Distance_Of_Infection) {
    report_distance_of_infection(day);
  }

  if(Global::Report_Incidence_By_County) {
    FRED_VERBOSE(0, "report incidence by county\n");
    report_incidence_by_county(day);
  }

  if(Global::Report_Epidemic_Data_By_County) {
    FRED_VERBOSE(0, "report epidemic by county\n");
    report_by_county(day);
  }

  if(Global::Report_Incidence_By_Census_Tract) {
    report_incidence_by_census_tract(day);
  }
  if(Global::Report_Symptomatic_Incidence_By_Census_Tract) {
    report_symptomatic_incidence_by_census_tract(day);
  }
  if(this->report_generation_time || Global::Report_Serial_Interval) {
    FRED_VERBOSE(0, "report serial interval\n");
    report_serial_interval(day);
  }
  
  //Only report AR and ARs on last day
  if(Global::Report_Mean_Household_Stats_Per_Income_Category && day == (Global::Days - 1)) {
    report_household_income_stratified_results(day);
  }

  if(Global::Enable_Household_Shelter) {
    Global::Places.report_shelter_stats(day);
  }

  //Only report AR and ARs on last day
  if(Global::Report_Epidemic_Data_By_Census_Tract && day == (Global::Days - 1)) {
    report_census_tract_stratified_results(day);
  }

  if(Global::Enable_Group_Quarters) {
    report_group_quarters_incidence(day);
  }

  if(Global::Verbose) {
    fprintf(Global::Statusfp, "\n");
    fflush(Global::Statusfp);
  }

  // prepare for next day
  this->people_becoming_infected_today = 0;
  this->people_becoming_symptomatic_today = 0;
  this->daily_case_fatality_count = 0;
  this->daily_infections_list.clear();
  this->daily_symptomatic_list.clear();
}

void Epidemic::report_age_of_infection(int day) {
  int infants = 0;
  int toddlers = 0;
  int pre_school = 0;
  int elementary = 0;
  int high_school = 0;
  int young_adults = 0;
  int adults = 0;
  int elderly = 0;
  int age_count[Demographics::MAX_AGE + 1];				// age group counts
  double mean_age = 0.0;
  int count_infections = 0;
  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    age_count[i] = 0;
  }

  for(int i = 0; i < this->people_becoming_infected_today; ++i) {
    Person* infectee = this->daily_infections_list[i];
    int age = infectee->get_age();
    mean_age += age;
    count_infections++;
    int age_group = age / 5;
    if(age_group > 20) {
      age_group = 20;
    }
    if(Global::Report_Age_Of_Infection > 3) {
      age_group = age;
      if(age_group > Demographics::MAX_AGE) {
        age_group = Demographics::MAX_AGE;
      }
    }

    age_count[age_group]++;
    double real_age = infectee->get_real_age();
    if(Global::Report_Age_Of_Infection == 1) {
      if(real_age < 0.5) {
        infants++;
      } else if(real_age < 2.0) {
        toddlers++;
      } else if(real_age < 6.0) {
        pre_school++;
      } else if(real_age < 12.0) {
        elementary++;
      } else if(real_age < 18.0) {
        high_school++;
      } else if(real_age < 21.0) {
        young_adults++;
      } else if(real_age < 65.0) {
        adults++;
      } else if(65 <= real_age) {
        elderly++;
      }
    } else if (Global::Report_Age_Of_Infection == 2) {
      if(real_age < 19.0/12.0) {
        infants++;
      } else if(real_age < 3.0) {
        toddlers++;
      } else if(real_age < 5.0) {
        pre_school++;
      } else if(real_age < 12.0) {
        elementary++;
      } else if(real_age < 18.0) {
        high_school++;
      } else if(real_age < 21.0) {
        young_adults++;
      } else if(real_age < 65.0) {
        adults++;
      } else if(65 <= real_age) {
        elderly++;
      }
    } else if(Global::Report_Age_Of_Infection == 3) {
      if(real_age < 5.0) {
        pre_school++;
      } else if(real_age < 18.0) {
        high_school++;
      } else if(real_age < 50.0) {
        young_adults++;
      } else if(real_age < 65.0) {
        adults++;
      } else if(65 <= real_age) {
        elderly++;
      }
    }
  }    
  if(count_infections > 0) {
    mean_age /= count_infections;
  }
  //Write to log file
  Utils::fred_log("\nday %d INF_AGE: ", day);
  Utils::fred_log("\nAge_at_infection %f", mean_age);

  //Store for daily output file
  track_value(day, (char*)"Age_at_infection", mean_age);

  switch(Global::Report_Age_Of_Infection) {
  case 1:
    track_value(day, (char*)"Infants", infants);
    track_value(day, (char*)"Toddlers", toddlers);
    track_value(day, (char*)"Preschool", pre_school);
    track_value(day, (char*)"Students", elementary+high_school);
    track_value(day, (char*)"Elementary", elementary);
    track_value(day, (char*)"Highschool", high_school);
    track_value(day, (char*)"Young_adults", young_adults);
    track_value(day, (char*)"Adults", adults);
    track_value(day, (char*)"Elderly", elderly);
    break;
  case 2:
    track_value(day, (char*)"Infants", infants);
    track_value(day, (char*)"Toddlers", toddlers);
    track_value(day, (char*)"Pre-k", pre_school);
    track_value(day, (char*)"Elementary", elementary);
    track_value(day, (char*)"Highschool", high_school);
    track_value(day, (char*)"Young_adults", young_adults);
    track_value(day, (char*)"Adults", adults);
    track_value(day, (char*)"Elderly", elderly);
    break;
  case 3:
    track_value(day, (char*)"0_4", pre_school);
    track_value(day, (char*)"5_17", high_school);
    track_value(day, (char*)"18_49", young_adults);
    track_value(day, (char*)"50_64", adults);
    track_value(day, (char*)"65_up", elderly);
    break;
  case 4:
    for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
      char temp_str[10];
      sprintf(temp_str, "A%d", i);
      track_value(day, temp_str, age_count[i]);
      sprintf(temp_str, "Age%d", i);
      track_value(day, temp_str,
		  Global::Popsize_by_age[i] ?
		  (100000.0 * age_count[i] / static_cast<double>(Global::Popsize_by_age[i])) : 0.0);
    }
    break;
  default:
    if(Global::Age_Of_Infection_Log_Level >= Global::LOG_LEVEL_LOW) {
      report_transmission_by_age_group_to_file(day);
    }
    if(Global::Age_Of_Infection_Log_Level >= Global::LOG_LEVEL_MED) {
      for(int i = 0; i <= 20; ++i) {
	      char temp_str[10];
	      //Write to log file
	      sprintf(temp_str, "A%d", i * 5);
	      //Store for daily output file
	      track_value(day, temp_str, age_count[i]);
	      Utils::fred_log(" A%d_%d %d", i * 5, age_count[i], this->id);
      }
    }
    break;
  }
  Utils::fred_log("\n");
}

void Epidemic::report_by_county(int day) {
  char str[FRED_STRING_SIZE];
  int n = Global::Places.get_number_of_counties();
  for (int i = 0; i < n; i++) {
    int fips = Global::Places.get_fips_of_county_with_index(i);
    sprintf(str, "P:%d", fips);
    track_value(day, str, this->county_infection_counts_map[fips].current_infected);

    sprintf(str, "Ps:%d", fips);
    track_value(day, str, this->county_infection_counts_map[fips].current_symptomatic);

    sprintf(str, "Pa:%d", fips);
    track_value(day, str, this->county_infection_counts_map[fips].current_infected - this->county_infection_counts_map[fips].current_symptomatic);

    sprintf(str, "N:%d", fips);
    track_value(day, str, Global::Places.get_population_of_county_with_index(i));

    sprintf(str, "CF:%d", fips);
    track_value(day, str, this->county_infection_counts_map[fips].current_case_fatalities);

    this->county_infection_counts_map[fips].current_case_fatalities = 0;
  }
}

void Epidemic::report_serial_interval(int day) {

  for(int i = 0; i < this->people_becoming_infected_today; ++i) {
    Person* infectee = this->daily_infections_list[i];
    Person* infector = infectee->get_infector(id);
    if(infector != NULL) {
      int serial_interval = infectee->get_exposure_date(this->id)
	                           - infector->get_exposure_date(this->id);
      this->total_serial_interval += static_cast<double>(serial_interval);
      this->total_secondary_cases++;
    }
  }

  double mean_serial_interval = 0.0;
  if(this->total_secondary_cases > 0) {
    mean_serial_interval = this->total_serial_interval / static_cast<double>(this->total_secondary_cases);
  }

  if(Global::Report_Serial_Interval) {
    //Write to log file
    Utils::fred_log("\nday %d SERIAL_INTERVAL:", day);
    Utils::fred_log("\n ser_int %.2f\n", mean_serial_interval);
    
    //Store for daily output file
    Global::Daily_Tracker->set_index_key_pair(day,"ser_int", mean_serial_interval);
  }

  track_value(day, (char*)"Tg", mean_serial_interval);
}

int Epidemic::get_age_group(int age) {
  if(age < 5) {
    return 0;
  }
  if(age < 19) {
    return 1;
  }
  if(age < 65) {
    return 2;
  }
  return 3;
}

void Epidemic::report_transmission_by_age_group(int day) {
  int groups = 4;
  int age_count[groups][groups];    // age group counts
  for(int i = 0; i < groups; ++i) {
    for(int j = 0; j < groups; ++j) {
      age_count[i][j] = 0;
    }
  }
  for(int i = 0; i < this->people_becoming_infected_today; ++i) {
    Person* infectee = this->daily_infections_list[i];
    Person* infector = this->daily_infections_list[i]->get_infector(id);
    if(infector == NULL) {
      continue;
    }
    int g1 = get_age_group(infector->get_age());
    int g2 = get_age_group(infectee->get_age());
    age_count[g1][g2]++;
  }
  
  //Store for daily output file
  track_value(day,(char*)"T_4_to_4", age_count[0][0]);
  track_value(day,(char*)"T_4_to_18", age_count[0][1]);
  track_value(day,(char*)"T_4_to_64", age_count[0][2]);
  track_value(day,(char*)"T_4_to_99", age_count[0][3]);
  track_value(day,(char*)"T_18_to_4", age_count[1][0]);
  track_value(day,(char*)"T_18_to_18", age_count[1][1]);
  track_value(day,(char*)"T_18_to_64", age_count[1][2]);
  track_value(day,(char*)"T_18_to_99", age_count[1][3]);
  track_value(day,(char*)"T_64_to_4", age_count[2][0]);
  track_value(day,(char*)"T_64_to_18", age_count[2][1]);
  track_value(day,(char*)"T_64_to_64", age_count[2][2]);
  track_value(day,(char*)"T_64_to_99", age_count[2][3]);
  track_value(day,(char*)"T_99_to_4", age_count[3][0]);
  track_value(day,(char*)"T_99_to_18", age_count[3][1]);
  track_value(day,(char*)"T_99_to_64", age_count[3][2]);
  track_value(day,(char*)"T_99_to_99", age_count[3][3]);
} 

void Epidemic::report_transmission_by_age_group_to_file(int day) {
  FILE* fp;
  char file[1024];
  sprintf(file, "%s/AGE.%d", Global::Output_directory, day);
  fp = fopen(file, "w");
  if(fp == NULL) {
    Utils::fred_abort("Can't open file to report age transmission matrix\n");
  }
  int age_count[100][100];				// age group counts
  for(int i = 0; i < 100; ++i) {
    for(int j = 0; j < 100; ++j) {
      age_count[i][j] = 0;
    }
  }
  int group = 1;
  int groups = 100 / group;
  for(int i = 0; i < this->people_becoming_infected_today; ++i) {
    Person* infectee = this->daily_infections_list[i];
    Person* infector = this->daily_infections_list[i]->get_infector(id);
    if(infector == NULL) {
      continue;
    }
    int a1 = infector->get_age();
    int a2 = infectee->get_age();
    if(a1 > 99) {
      a1 = 99;
    }
    if(a2 > 99) {
      a2 = 99;
    }
    a1 = a1 / group;
    a2 = a2 / group;
    age_count[a1][a2]++;
  }
  for(int i = 0; i < groups; ++i) {
    for(int j = 0; j < groups; ++j) {
      fprintf(fp, " %d", age_count[j][i]);
    }
    fprintf(fp, "\n");
  }
  fclose(fp);
}

void Epidemic::report_incidence_by_county(int day) {
  if(day == 0) {
    // set up county counts
    this->counties = Global::Places.get_number_of_counties();
    this->county_incidence = new int[this->counties];
    for(int i = 0; i < this->counties; ++i) {
      this->county_incidence[i] = 0;
    }
  }
  // FRED_VERBOSE(0, "county incidence day %d\n", day);
  int infected = this->people_becoming_infected_today;
  for(int i = 0; i < infected; ++i) {
    Person* infectee = this->daily_infections_list[i];
    // FRED_VERBOSE(0, "person %d is %d out of %d\n", infectee->get_id(), i, infected);
    Household* hh = static_cast<Household*>(infectee->get_household());
    if(hh == NULL) {
      if(Global::Enable_Hospitals && infectee->is_hospitalized() && infectee->get_permanent_household() != NULL) {
        hh = static_cast<Household*>(infectee->get_permanent_household());;
      }
    }

    int fips = hh->get_county_fips();
    int c = Global::Places.get_index_of_county_with_fips(fips);
    assert(0 <= c && c < this->counties);
    this->county_incidence[c]++;
    // FRED_VERBOSE(0, "county %d incidence %d %d out of %d person %d \n", c, this->county_incidence[c], i, infected, infectee->get_id());
  }
  FRED_VERBOSE(1, "county incidence day %d\n", day);
  for(int c = 0; c < this->counties; ++c) {
    char name[80];
    sprintf(name, "County_%d", Global::Places.get_fips_of_county_with_index(c));
    track_value(day,name, this->county_incidence[c]);
    sprintf(name, "N_%d", Global::Places.get_fips_of_county_with_index(c));
    track_value(day,name, Global::Places.get_population_of_county_with_index(c));
    // prepare for next day
    this->county_incidence[c] = 0;
  }
  FRED_VERBOSE(1, "county incidence day %d done\n", day);
}


void Epidemic::report_symptomatic_incidence_by_census_tract(int day) {
  if(day == 0) {
    // set up census_tract counts
    this->census_tracts = Global::Places.get_number_of_census_tracts();
    this->census_tract_symp_incidence = new int[this->census_tracts];
  }
  for(int t = 0; t < this->census_tracts; t++) {
    this->census_tract_symp_incidence[t] = 0;
  }
  for(int i = 0; i < this->people_becoming_symptomatic_today; i++) {
    Person * infectee = this->daily_symptomatic_list[i];
    Household * h = (Household *) infectee->get_household();
    int t = Global::Places.get_index_of_census_tract_with_fips(h->get_census_tract_fips());
    assert(0 <= t && t < this->census_tracts);
    this->census_tract_symp_incidence[t]++;
  }
  for(int t = 0; t < this->census_tracts; t++) {
    char name[80];
    sprintf(name, "Tract_Cs_%ld", Global::Places.get_census_tract_with_index(t));
    track_value(day,name,  this->census_tract_symp_incidence[t]);
  }
}

int Epidemic::get_symptomatic_incidence_by_tract_index(int index_){
  this->census_tracts = Global::Places.get_number_of_census_tracts();
  if(index_ >= 0 && index_ < this->census_tracts){
    FRED_VERBOSE(0,"Census_tracts %d index %d\n",this->census_tracts, index_);
    return this->census_tract_symp_incidence[index_];
  }else{
    return -1;
  }
}

void Epidemic::report_incidence_by_census_tract(int day) {
  if(day == 0) {
    // set up census_tract counts
    this->census_tracts = Global::Places.get_number_of_census_tracts();
    this->census_tract_incidence = new int[this->census_tracts];
    for(int i = 0; i < this->census_tracts; ++i) {
      this->census_tract_incidence[i] = 0;
    }
  }
  for(int i = 0; i < this->people_becoming_infected_today; ++i) {
    Person* infectee = this->daily_infections_list[i];
    Household* hh = static_cast<Household*>(infectee->get_household());
    if(hh == NULL) {
      if(Global::Enable_Hospitals && infectee->is_hospitalized() && infectee->get_permanent_household() != NULL) {
        hh = static_cast<Household*>(infectee->get_permanent_household());;
      }
    }
    int t = Global::Places.get_index_of_census_tract_with_fips(hh->get_census_tract_fips());
    assert(0 <= t && t < this->census_tracts);
    this->census_tract_incidence[t]++;
  }
  for(int t = 0; t < this->census_tracts; ++t) {
    char name[80];
    sprintf(name, "Tract_%ld", Global::Places.get_census_tract_with_index(t));
    track_value(day,name, this->census_tract_incidence[t]);
    // prepare for next day
    this->census_tract_incidence[t] = 0;
  }
}

void Epidemic::report_group_quarters_incidence(int day) {
  // group quarters incidence counts
  int G = 0;
  int D = 0;
  int J = 0;
  int L = 0;
  int B = 0;

  for(int i = 0; i < this->people_becoming_infected_today; ++i) {
    Person* infectee = this->daily_infections_list[i];
    // record infections occurring in group quarters
    Mixing_Group* mixing_group = infectee->get_infected_mixing_group(this->id);
    if(dynamic_cast<Place*>(mixing_group) == NULL) {
      //Only Places can be group quarters
      continue;
    } else {
      Place* place = dynamic_cast<Place*>(mixing_group) ;
      if(place->is_group_quarters()) {
        G++;
        if(place->is_college()) {
          D++;
        }
        if(place->is_prison()) {
          J++;
        }
        if(place->is_nursing_home()) {
          L++;
        }
        if(place->is_military_base()) {
          B++;
        }
      }
    }
  }

  //Write to log file
  Utils::fred_log("\nDay %d GQ_INF: ", day);
  Utils::fred_log(" GQ %d College %d Prison %d Nursing_Home %d Military %d", G,D,J,L,B);
  Utils::fred_log("\n");

  //Store for daily output file
  track_value(day, (char*)"GQ", G);
  track_value(day, (char*)"College", D);
  track_value(day, (char*)"Prison", J);
  track_value(day, (char*)"Nursing_Home", L);
  track_value(day, (char*)"Military", B);
}

void Epidemic::report_place_of_infection(int day) {
  // type of place of infection
  int X = 0;
  int H = 0;
  int N = 0;
  int S = 0;
  int C = 0;
  int W = 0;
  int O = 0;
  int M = 0;

  for(int i = 0; i < this->people_becoming_infected_today; i++) {
    Person* infectee = this->daily_infections_list[i];
    char c = infectee->get_infected_mixing_group_type(this->id);
    switch(c) {
    case 'X':
      X++;
      break;
    case 'H':
      H++;
      break;
    case 'N':
      N++;
      break;
    case 'S':
      S++;
      break;
    case 'C':
      C++;
      break;
    case 'W':
      W++;
      break;
    case 'O':
      O++;
      break;
    case 'M':
      M++;
      break;
    }
  }

  //Write to log file
  Utils::fred_log("\nDay %d INF_PLACE: ", day);
  Utils::fred_log(" X %d H %d Nbr %d Sch %d", X, H, N, S);
  Utils::fred_log(" Cls %d Wrk %d Off %d Hosp %d", C, W, O, M);
  Utils::fred_log("\n");

  //Store for daily output file
  track_value(day, (char*)"X", X);
  track_value(day, (char*)"H", H);
  track_value(day, (char*)"Nbr", N);
  track_value(day, (char*)"Sch", S);
  track_value(day, (char*)"Cls", C);
  track_value(day, (char*)"Wrk", W);
  track_value(day, (char*)"Off", O);
  track_value(day, (char*)"Hosp", M);
}

void Epidemic::report_distance_of_infection(int day) {
  double tot_dist = 0.0;
  double ave_dist = 0.0;
  int n = 0;
  for(int i = 0; i < this->people_becoming_infected_today; ++i) {
    Person* infectee = this->daily_infections_list[i];
    Person* infector = infectee->get_infector(this->id);
    if(infector == NULL) {
      continue;
    }
    Mixing_Group* new_mixing_group = infectee->get_health()->get_infected_mixing_group(this->id);
    Mixing_Group* old_mixing_group = infector->get_health()->get_infected_mixing_group(this->id);
    if(dynamic_cast<Place*>(new_mixing_group) == NULL || dynamic_cast<Place*>(old_mixing_group) == NULL) {
      //Only Places have lat / lon
      continue;
    } else {
      Place* new_place = dynamic_cast<Place*>(new_mixing_group);
      Place* old_place = dynamic_cast<Place*>(old_mixing_group);
      fred::geo lat1 = new_place->get_latitude();
      fred::geo lon1 = new_place->get_longitude();
      fred::geo lat2 = old_place->get_latitude();
      fred::geo lon2 = old_place->get_longitude();
      double dist = Geo::xy_distance(lat1, lon1, lat2, lon2);
      tot_dist += dist;
      n++;
    }
  }
  if(n > 0) {
    ave_dist = tot_dist / n;
  }

  //Write to log file
  Utils::fred_log("\nDay %d INF_DIST: ", day);
  Utils::fred_log(" Dist %f ", 1000 * ave_dist);
  Utils::fred_log("\n");

  //Store for daily output file
  track_value(day, (char*)"Dist", 1000 * ave_dist);
}

void Epidemic::report_presenteeism(int day) {
  // daily totals
  int infections_in_pop = 0;
  vector<int> infections;
  vector<int> presenteeism;
  vector<int> presenteeism_with_sl;
  int infections_at_work = 0;

  infections.clear();
  presenteeism.clear();
  presenteeism_with_sl.clear();

  for(int i = 0; i <= Workplace::get_workplace_size_group_count(); ++i) {
    infections.push_back(0);
    presenteeism.push_back(0);
    presenteeism_with_sl.push_back(0);
  }

  int presenteeism_tot = 0;
  int presenteeism_with_sl_tot = 0;
  for(int i = 0; i < this->people_becoming_infected_today; ++i) {
    Person* infectee = this->daily_infections_list[i];
    char c = infectee->get_infected_mixing_group_type(this->id);
    infections_in_pop++;

    // presenteeism requires that place of infection is work or office
    if(c != Place::TYPE_WORKPLACE && c != Place::TYPE_OFFICE) {
      continue;
    }
    infections_at_work++;

    // get the work place size (note we don't care about the office size)
    Place* work = infectee->get_workplace();
    assert(work != NULL);
    int size = work->get_size();

    for(int j = 0; j <= Workplace::get_workplace_size_group_count(); ++j) {
      if(size < Workplace::get_workplace_size_max_by_group_id(j)) {
        infections[j]++;
        break;
      }
    }

    // presenteeism requires that the infector have symptoms
    Person* infector = infectee->get_infector(this->id);
    assert(infector != NULL);
    if(infector->is_symptomatic()) {

      // determine whether sick leave was available to infector
      bool infector_has_sick_leave = infector->is_sick_leave_available();

      for(int j = 0; j <= Workplace::get_workplace_size_group_count(); ++j) {
        if(size < Workplace::get_workplace_size_max_by_group_id(j)) {
          presenteeism[j]++;
          presenteeism_tot++;
          if(infector_has_sick_leave) {
            presenteeism_with_sl[j]++;
            presenteeism_with_sl_tot++;
          }
          break;
        }
      }
    }
  } // end loop over infectees

  //Store for daily output file
  for(int i = 0; i < Workplace::get_workplace_size_group_count(); ++i) {
    char wp_inf[30];
    char wp_symp[30];
    char wp_pres[30];
    char wp_pres_sl[30];
    char wp_n[30];
    if(i == 0 || (i + 1 < Workplace::get_workplace_size_group_count())) {
      int min = (i > 0 ? (Workplace::get_workplace_size_max_by_group_id(i - 1) + 1) : 0);
      int max = Workplace::get_workplace_size_max_by_group_id(i);

      sprintf(wp_inf, "wp_%d_%d_inf", min, max);
      Global::Daily_Tracker->set_index_key_pair(day, wp_inf, infections[i]);
      sprintf(wp_pres, "wp_%d_%d_pres", min, max);
      Global::Daily_Tracker->set_index_key_pair(day, wp_pres, presenteeism[i]);
      sprintf(wp_pres_sl, "wp_%d_%d_pres_sl", min, max);
      Global::Daily_Tracker->set_index_key_pair(day, wp_pres_sl, presenteeism_with_sl[i]);
      sprintf(wp_n, "wp_%d_%d_n", min, max);
      Global::Daily_Tracker->set_index_key_pair(day, wp_n, Workplace::get_count_workers_by_workplace_size(i));
      sprintf(wp_symp, "wp_%d_%d_tot_symp", min, max);
      Global::Daily_Tracker->set_index_key_pair(day, wp_symp, this->workplace_size_infection_counts_map[i].tot_ppl_evr_sympt);
    } else {
      int min = (Workplace::get_workplace_size_max_by_group_id(i - 1) + 1);

      sprintf(wp_inf, "wp_%d_up_inf", min);
      Global::Daily_Tracker->set_index_key_pair(day, wp_inf, infections[i]);
      sprintf(wp_pres, "wp_%d_up_pres", min);
      Global::Daily_Tracker->set_index_key_pair(day, wp_pres, presenteeism[i]);
      sprintf(wp_pres_sl, "wp_%d_up_pres_sl", min);
      Global::Daily_Tracker->set_index_key_pair(day, wp_pres_sl, presenteeism_with_sl[i]);
      sprintf(wp_n, "wp_%d_up_n", min);
      Global::Daily_Tracker->set_index_key_pair(day, wp_n, Workplace::get_count_workers_by_workplace_size(i));
      sprintf(wp_symp, "wp_%d_up_tot_symp", min);
      Global::Daily_Tracker->set_index_key_pair(day, wp_symp, this->workplace_size_infection_counts_map[i].tot_ppl_evr_sympt);
    }
  }
  Global::Daily_Tracker->set_index_key_pair(day, "presenteeism_tot", presenteeism_tot);
  Global::Daily_Tracker->set_index_key_pair(day, "presenteeism_with_sl_tot", presenteeism_with_sl_tot);
  Global::Daily_Tracker->set_index_key_pair(day, "inf_at_work", infections_at_work);
  Global::Daily_Tracker->set_index_key_pair(day, "tot_emp", Workplace::get_total_workers());
  Global::Daily_Tracker->set_index_key_pair(day, "N", this->N);

  Global::Daily_Tracker->set_index_key_pair(day, "tot_days_sympt", this->population_infection_counts.tot_days_sympt);
  Global::Daily_Tracker->set_index_key_pair(day, "tot_ppl_evr_sympt", this->population_infection_counts.tot_ppl_evr_sympt);

}

void Epidemic::report_school_attack_rates_by_income_level(int day) {

  // daily totals
  int presenteeism_Q1 = 0;
  int presenteeism_Q2 = 0;
  int presenteeism_Q3 = 0;
  int presenteeism_Q4 = 0;
  int presenteeism_Q1_with_sl = 0;
  int presenteeism_Q2_with_sl = 0;
  int presenteeism_Q3_with_sl = 0;
  int presenteeism_Q4_with_sl = 0;
  int infections_at_school = 0;

  for(int i = 0; i < this->people_becoming_infected_today; ++i) {

    Person* infectee = this->daily_infections_list[i];
    assert(infectee != NULL);
    char c = infectee->get_infected_mixing_group_type(this->id);

    // school presenteeism requires that place of infection is school or classroom
    if(c != Place::TYPE_SCHOOL && c != Place::TYPE_CLASSROOM) {
      continue;
    }
    infections_at_school++;

    // get the school income quartile
    School* school = static_cast<School*>(infectee->get_school());
    assert(school != NULL);
    int income_quartile = school->get_income_quartile();

    // presenteeism requires that the infector have symptoms
    Person* infector = infectee->get_infector(this->id);
    assert(infector != NULL);
    if(infector->is_symptomatic()) {

      // determine whether anyone was at home to watch child
      Household* hh = static_cast<Household*>(infector->get_household());
      assert(hh != NULL);
      bool infector_could_stay_home = hh->has_school_aged_child_and_unemployed_adult();

      if(income_quartile == Global::Q1) {  // Quartile 1
        presenteeism_Q1++;
        if(infector_could_stay_home) {
          presenteeism_Q1_with_sl++;
        }
      } else if(income_quartile == Global::Q2) {  // Quartile 2
        presenteeism_Q2++;
        if(infector_could_stay_home) {
          presenteeism_Q2_with_sl++;
        }
      } else if(income_quartile == Global::Q3) {  // Quartile 3
        presenteeism_Q3++;
        if(infector_could_stay_home) {
          presenteeism_Q3_with_sl++;
        }
      } else if(income_quartile == Global::Q4) {  // Quartile 4
        presenteeism_Q4++;
        if(infector_could_stay_home) {
          presenteeism_Q4_with_sl++;
        }
      }
    }
  } // end loop over infectees

  // raw counts
  int presenteeism = presenteeism_Q1 + presenteeism_Q2 + presenteeism_Q3
    + presenteeism_Q4;
  int presenteeism_with_sl = presenteeism_Q1_with_sl + presenteeism_Q2_with_sl
    + presenteeism_Q3_with_sl + presenteeism_Q4_with_sl;

  //Store for daily output file
  Global::Daily_Tracker->set_index_key_pair(day, "school_pres_Q1", presenteeism_Q1);
  Global::Daily_Tracker->set_index_key_pair(day, "school_pop_Q1", School::get_school_pop_income_quartile_1());
  Global::Daily_Tracker->set_index_key_pair(day, "school_pres_Q2", presenteeism_Q2);
  Global::Daily_Tracker->set_index_key_pair(day, "school_pop_Q2", School::get_school_pop_income_quartile_2());
  Global::Daily_Tracker->set_index_key_pair(day, "school_pres_Q3", presenteeism_Q3);
  Global::Daily_Tracker->set_index_key_pair(day, "school_pop_Q3", School::get_school_pop_income_quartile_3());
  Global::Daily_Tracker->set_index_key_pair(day, "school_pres_Q4", presenteeism_Q4);
  Global::Daily_Tracker->set_index_key_pair(day, "school_pop_Q4", School::get_school_pop_income_quartile_4());
  Global::Daily_Tracker->set_index_key_pair(day, "school_pres", presenteeism);
  Global::Daily_Tracker->set_index_key_pair(day, "school_pres_sl", presenteeism_with_sl);
  Global::Daily_Tracker->set_index_key_pair(day, "inf_at_school", infections_at_school);
  Global::Daily_Tracker->set_index_key_pair(day, "tot_school_pop", School::get_total_school_pop());
  Global::Daily_Tracker->set_index_key_pair(day, "N", this->N);

}

void Epidemic::report_household_income_stratified_results(int day) {

  for(int i = 0; i < Household_income_level_code::UNCLASSIFIED; ++i) {
    int temp_adult_count = 0;
    int temp_adult_inf_count = 0;
    int temp_adult_symp_count = 0;
    
    //AR
    if(Household::count_inhabitants_by_household_income_level_map[i] > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "AR",
							  (100.0 * static_cast<double>(this->household_income_infection_counts_map[i].tot_ppl_evr_inf)
							   / static_cast<double>(Household::count_inhabitants_by_household_income_level_map[i])));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "AR", static_cast<double>(0.0));
    }
    
    //AR_under_18
    if(Household::count_children_by_household_income_level_map[i] > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "AR_under_18",
							  (100.0 * static_cast<double>(this->household_income_infection_counts_map[i].tot_chldrn_evr_inf)
							   / static_cast<double>(Household::count_children_by_household_income_level_map[i])));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "AR_under_18", static_cast<double>(0.0));
    }
    
    //AR_adult
    temp_adult_count = Household::count_inhabitants_by_household_income_level_map[i]
      - Household::count_children_by_household_income_level_map[i];
    temp_adult_inf_count = this->household_income_infection_counts_map[i].tot_ppl_evr_inf
      - this->household_income_infection_counts_map[i].tot_chldrn_evr_inf;
    if(temp_adult_count > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "AR_adult",
							  (100.0 * static_cast<double>(temp_adult_inf_count) / static_cast<double>(temp_adult_count)));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "AR_adult", static_cast<double>(0.0));
    }
    
    //ARs
    if(Household::count_inhabitants_by_household_income_level_map[i] > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "ARs",
							  (100.0 * static_cast<double>(this->household_income_infection_counts_map[i].tot_ppl_evr_sympt)
							   / static_cast<double>(Household::count_inhabitants_by_household_income_level_map[i])));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "ARs", (double)0.0);
    }
    
    //ARs_under_18
    if(Household::count_children_by_household_income_level_map[i] > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "ARs_under_18",
							  (100.0 * static_cast<double>(this->household_income_infection_counts_map[i].tot_chldrn_evr_sympt)
							   / static_cast<double>(Household::count_children_by_household_income_level_map[i])));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "ARs_under_18", static_cast<double>(0.0));
    }
    
    //ARs_adult
    temp_adult_symp_count = this->household_income_infection_counts_map[i].tot_ppl_evr_sympt
      - this->household_income_infection_counts_map[i].tot_chldrn_evr_sympt;
    if(temp_adult_count > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "ARs_adult",
							  (100.0 * static_cast<double>(temp_adult_symp_count) / static_cast<double>(temp_adult_count)));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "ARs_adult", static_cast<double>(0.0));
    }
  }

}

void Epidemic::report_census_tract_stratified_results(int day) {

  for(std::set<long int>::iterator census_tract_itr = Household::census_tract_set.begin();
      census_tract_itr != Household::census_tract_set.end(); ++census_tract_itr) {
    int temp_adult_count = 0;
    int temp_adult_inf_count = 0;
    int temp_adult_symp_count = 0;
    
    //AR
    if(Household::count_inhabitants_by_census_tract_map[*census_tract_itr] > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "AR",
						(100.0 * static_cast<double>(this->census_tract_infection_counts_map[*census_tract_itr].tot_ppl_evr_inf)
						 / static_cast<double>(Household::count_inhabitants_by_census_tract_map[*census_tract_itr])));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "AR", static_cast<double>(0.0));
    }
    
    //AR_under_18
    if(Household::count_children_by_census_tract_map[*census_tract_itr] > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "AR_under_18",
						(100.0 * static_cast<double>(this->census_tract_infection_counts_map[*census_tract_itr].tot_chldrn_evr_inf)
						 / static_cast<double>(Household::count_children_by_census_tract_map[*census_tract_itr])));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "AR_under_18", static_cast<double>(0.0));
    }
    
    //AR_adult
    temp_adult_count = Household::count_inhabitants_by_census_tract_map[*census_tract_itr]
      - Household::count_children_by_census_tract_map[*census_tract_itr];
    temp_adult_inf_count = this->census_tract_infection_counts_map[*census_tract_itr].tot_ppl_evr_inf
      - this->census_tract_infection_counts_map[*census_tract_itr].tot_chldrn_evr_inf;
    if(temp_adult_count > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "AR_adult",
						(100.0 * static_cast<double>(temp_adult_inf_count) / static_cast<double>(temp_adult_count)));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "AR_adult", static_cast<double>(0.0));
    }
    
    //Symptomatic AR
    if(Household::count_inhabitants_by_census_tract_map[*census_tract_itr] > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "ARs",
						(100.0 * static_cast<double>(this->census_tract_infection_counts_map[*census_tract_itr].tot_ppl_evr_sympt)
						 / static_cast<double>(Household::count_inhabitants_by_census_tract_map[*census_tract_itr])));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "ARs", static_cast<double>(0.0));
    }
    
    
    //ARs_under_18
    if(Household::count_children_by_census_tract_map[*census_tract_itr] > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "ARs_under_18",
						(100.0 * static_cast<double>(this->census_tract_infection_counts_map[*census_tract_itr].tot_chldrn_evr_sympt)
						 / static_cast<double>(Household::count_children_by_census_tract_map[*census_tract_itr])));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "ARs_under_18", static_cast<double>(0.0));
    }
    
    
    //ARs_adult
    temp_adult_symp_count = this->census_tract_infection_counts_map[*census_tract_itr].tot_ppl_evr_sympt
      - this->census_tract_infection_counts_map[*census_tract_itr].tot_chldrn_evr_sympt;
    if(temp_adult_count > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "ARs_adult",
						(100.0 * static_cast<double>(temp_adult_symp_count) / static_cast<double>(temp_adult_count)));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "ARs_adult", static_cast<double>(0.0));
    }
  }
}


void Epidemic::track_value(int day, char* key, int value) {
  char key_str[80];
  if(this->id == 0) {
    sprintf(key_str, "%s", key);
  } else {
    sprintf(key_str, "%s_%d", key, this->id);
  }
  Global::Daily_Tracker->set_index_key_pair(day, key_str, value);
}

void Epidemic::track_value(int day, char* key, double value) {
  char key_str[80];
  if(this->id == 0) {
    sprintf(key_str, "%s", key);
  } else {
    sprintf(key_str, "%s_%d", key, this->id);
  }
  Global::Daily_Tracker->set_index_key_pair(day, key_str, value);
}

void Epidemic::track_value(int day, char* key, string value) {
  char key_str[80];
  if(this->id == 0) {
    sprintf(key_str, "%s", key);
  } else {
    sprintf(key_str, "%s_%d", key, this->id);
  }
  Global::Daily_Tracker->set_index_key_pair(day, key_str, value);
}


void Epidemic::become_immune(Person* person, bool susceptible, bool infectious, bool symptomatic) {
  if(!susceptible) {
    this->removed_people++;
  }
  if(symptomatic) {
    this->people_with_current_symptoms--;
  }
  this->immune_people++;
}

void Epidemic::create_visualization_data_directories() {
  char vis_var_dir[FRED_STRING_SIZE];

  // create sub directory for this condition
  sprintf(this->visualization_directory, "%s/cond%d", Global::Visualization_directory, this->id);
  Utils::fred_make_directory(this->visualization_directory);
    
  // create directories for specific output variables
  sprintf(vis_var_dir, "%s/I", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Ia", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Is", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/C", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Ci", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Cs", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/CF", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/P", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Pa", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Ps", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/N", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/R", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Vec", this->visualization_directory);
  Utils::fred_make_directory(vis_var_dir);

  if(Global::Enable_HAZEL) {
    sprintf(vis_var_dir, "%s/HH_primary_hc_unav", this->visualization_directory);
    Utils::fred_make_directory(vis_var_dir);
    sprintf(vis_var_dir, "%s/HH_accept_insr_hc_unav", this->visualization_directory);
    Utils::fred_make_directory(vis_var_dir);
    sprintf(vis_var_dir, "%s/HH_hc_unav", this->visualization_directory);
    Utils::fred_make_directory(vis_var_dir);
    sprintf(vis_var_dir, "%s/HC_DEFICIT", this->visualization_directory);
    Utils::fred_make_directory(vis_var_dir);
  }
}


void Epidemic::print_visualization_data_for_active_infections(int day) {
  char filename[FRED_STRING_SIZE];
  FILE* fp;
  Person* person;
  Place* household;
  long int tract;
  double lat, lon;
  char location[FRED_STRING_SIZE];

  sprintf(filename, "%s/C/households-%d.txt", this->visualization_directory, day);
  fp = fopen(filename, "w");
  for(std::set<Person*>::iterator it = this->new_infected_people.begin(); it != this->new_infected_people.end(); ++it) {
    person = (*it);
    household = person->get_household();
    tract = Global::Places.get_census_tract_for_place(household);
    lat = household->get_latitude();
    lon = household->get_longitude();
    fprintf(fp, "%f %f %ld\n", lat, lon, tract);
  }
  fclose(fp);

  sprintf(filename, "%s/Cs/households-%d.txt", this->visualization_directory, day);
  fp = fopen(filename, "w");
  for(std::set<Person*>::iterator it = this->new_symptomatic_people.begin(); it != this->new_symptomatic_people.end(); ++it) {
    person = (*it);
    household = person->get_household();
    tract = Global::Places.get_census_tract_for_place(household);
    lat = household->get_latitude();
    lon = household->get_longitude();
    fprintf(fp, "%f %f %ld\n", lat, lon, tract);
  }
  fclose(fp);

  sprintf(filename, "%s/Ci/households-%d.txt", this->visualization_directory, day);
  fp = fopen(filename, "w");
  for(std::set<Person*>::iterator it = this->new_infectious_people.begin(); it != this->new_infectious_people.end(); ++it) {
    person = (*it);
    household = person->get_household();
    tract = Global::Places.get_census_tract_for_place(household);
    lat = household->get_latitude();
    lon = household->get_longitude();
    fprintf(fp, "%f %f %ld\n", lat, lon, tract);
  }
  fclose(fp);

  sprintf(filename, "%s/R/households-%d.txt", this->visualization_directory, day);
  fp = fopen(filename, "w");
  for(std::set<Person*>::iterator it = this->recovered_people.begin(); it != this->recovered_people.end(); ++it) {
    person = (*it);
    household = person->get_household();
    tract = Global::Places.get_census_tract_for_place(household);
    lat = household->get_latitude();
    lon = household->get_longitude();
    fprintf(fp, "%f %f %ld\n", lat, lon, tract);
  }
  fclose(fp);

  // make sure visualization file for CF exists
  sprintf(filename, "%s/CF/households-%d.txt", this->visualization_directory, day);
  fp = fopen(filename, "r");
  if (fp == NULL) {
    fclose(fp);
    // create an empty file
    fp = fopen(filename, "w");
  }
  fclose(fp);

  sprintf(filename, "%s/I/households-%d.txt", this->visualization_directory, day);
  FILE* I_fp = fopen(filename, "w");
  sprintf(filename, "%s/Ia/households-%d.txt", this->visualization_directory, day);
  FILE* Ia_fp = fopen(filename, "w");
  sprintf(filename, "%s/Is/households-%d.txt", this->visualization_directory, day);
  FILE* Is_fp = fopen(filename, "w");
  sprintf(filename, "%s/P/households-%d.txt", this->visualization_directory, day);
  FILE* P_fp = fopen(filename, "w");
  sprintf(filename, "%s/Pa/households-%d.txt", this->visualization_directory, day);
  FILE* Pa_fp = fopen(filename, "w");
  sprintf(filename, "%s/Ps/households-%d.txt", this->visualization_directory, day);
  FILE* Ps_fp = fopen(filename, "w");

  for(std::set<Person*>::iterator it = this->infected_people.begin(); it != this->infected_people.end(); ++it) {
    person = (*it);
    if (person->is_infected(this->id)) {
      household = person->get_household();
      tract = Global::Places.get_census_tract_for_place(household);
      lat = household->get_latitude();
      lon = household->get_longitude();
      sprintf(location,  "%f %f %ld\n", lat, lon, tract);

      // person is currently infected
      fputs(location, P_fp);

      if (person->is_symptomatic(this->id)) {
	// current symptomatic
	fputs(location, Ps_fp);
      }
      else {
	// current asymptomatic
	fputs(location, Pa_fp);
      }
      
      if (person->is_infectious(this->id)==day) {
	// infectious
	fputs(location, I_fp);

	if (person->is_symptomatic(this->id)) {
	  // infectious and symptomatic
	  fputs(location, Is_fp);
	}
	else {
	  // infectious and asymptomatic
	  fputs(location, Ia_fp);
	}
      }
    }
  }
  fclose(I_fp);
  fclose(Ia_fp);
  fclose(Is_fp);
  fclose(P_fp);
  fclose(Pa_fp);
  fclose(Ps_fp);
}


void Epidemic::print_visualization_data_for_case_fatality(int day, Person* person) {
  Place* household = person->get_household();
  char filename[FRED_STRING_SIZE];
  for(int d = day; d < day + 31; ++d) {
    sprintf(filename, "%s/CF/households-%d.txt", this->visualization_directory, d);
    FILE* fp = fopen(filename, "a");
    fprintf(fp, "%f %f %ld\n", household->get_latitude(), household->get_longitude(), Global::Places.get_census_tract_for_place(household));
    fclose(fp);
  }
}


void Epidemic::transition_person(Person* person, int day, int state) {
  person->become_exposed(this->id, NULL, NULL, day);
  if(this->seeding_type != SEED_EXPOSED) {
    advance_seed_infection(person);
  }
  become_exposed(person, day);
}
