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

#include "Classroom.h"
#include "Condition.h"
#include "Date.h"
#include "Epidemic.h"
#include "Events.h"
#include "Geo.h"
#include "Global.h"
#include "HIV_Epidemic.h"
#include "Hospital.h"
#include "Household.h"
#include "Natural_History.h"
#include "Neighborhood_Layer.h"
#include "Office.h"
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


/**
 * This static factory method is used to get an instance of a specific
 * Epidemic Model.  Depending on the model parameter, it will create a
 * specific Epidemic Model and return a pointer to it.
 *
 * @param a string containing the requested Epidemic model type
 * @return a pointer to a Epidemic model
 */

Epidemic* Epidemic::get_epidemic(Condition* condition) {

  if (strcmp(condition->get_natural_history_model(), "hiv") == 0) {
    return new HIV_Epidemic(condition);
  }
  else {
    return new Epidemic(condition);
  }

}

//////////////////////////////////////////////////////
//
// CONSTRUCTOR
//
//

Epidemic::Epidemic(Condition* _condition) {
  this->condition = _condition;
  this->id = _condition->get_id();
  strcpy(this->name, _condition->get_natural_history()->get_name());

  // these are current counts across the pop (prevalence)
  this->current_susceptible_people = 0;
  this->current_active_people = 0;
  this->current_exposed_people = 0;
  this->current_infectious_people = 0;
  this->current_removed_people = 0;
  this->current_symptomatic_people = 0;
  this->current_immune_people = 0;

  // these are daily incidence counts
  this->people_becoming_active_today = 0;
  this->people_becoming_infectious_today = 0;
  this->people_becoming_symptomatic_today = 0;
  this->people_becoming_immune_today = 0;
  this->people_becoming_case_fatality_today = 0;
  this->county_incidence = NULL;
  this->census_tract_incidence = NULL;
  this->census_tract_symp_incidence = NULL;

  // these are total (among current population)
  this->total_cases = 0;
  this->total_symptomatic_cases = 0;
  this->total_severe_symptomatic_cases = 0;
  this->total_case_fatality_count = 0;

  // reporting 
  this->report_generation_time = false;
  this->report_transmission_by_age = false;
  this->total_serial_interval = 0.0;
  this->total_secondary_cases = 0;
  this->enable_health_records = false;

  this->N_init = 0;
  this->N = 0;
  this->RR = 0.0;
  this->counties = 0;
  this->census_tracts = 0;
  this->natural_history = NULL;
  sprintf(this->output_var_suffix,"");

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
      this->county_infection_counts_map[fips].current_active = 0;
      this->county_infection_counts_map[fips].current_symptomatic = 0;
      this->county_infection_counts_map[fips].current_case_fatalities = 0;
    }
  }

  if(Global::Report_Epidemic_Data_By_Census_Tract) {
    //Values for household census_tract based stratification
    for(std::set<long int>::iterator census_tract_itr = Household::census_tract_set.begin();
	census_tract_itr != Household::census_tract_set.end(); ++census_tract_itr) {
      this->census_tract_infection_counts_map[*census_tract_itr].current_active = 0;
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

  this->import_map.clear();
  this->active_people_list.clear();
  this->infectious_people_list.clear();
  this->new_active_people_list.clear();
  this->new_symptomatic_people_list.clear();
  this->new_infectious_people_list.clear();
  this->recovered_people_list.clear();

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

  // read optional parameters
  Params::disable_abort_on_failure();
  
  // read import_map
  char import_file_name[FRED_STRING_SIZE];
  strcpy(import_file_name,"none");
  Params::get_param(this->condition->get_condition_name(), "import_file", import_file_name);

  // If this parameter is "none", then there is no import file
  if(strncmp(import_file_name, "none", 4) != 0){
    Utils::get_fred_file_name(import_file_name);
    ifstream* imp_input = new ifstream(import_file_name);
    if(!imp_input->is_open()) {
      Utils::fred_abort("Help!  Can't read import_file %s\n", import_file_name);
      abort();
    }
    string line;
    while(getline(*imp_input,line)){
      // only valid data lines are processed
      if(line.substr(0,7) != "start =") {
	continue;
      }
      char cstr[FRED_STRING_SIZE];
      // printf("|%s|\n", cstr);
      std::strcpy(cstr, line.c_str());
      Import_Map* import_spec = new Import_Map;
      int n = sscanf(cstr,
		     "start = %d end = %d max = %d per_cap = %lf lat = %lf lon = %lf rad = %lf fips = %ld min_age = %lf max_age = %lf",
		     &import_spec->sim_day_start, &import_spec->sim_day_end,
		     &import_spec->max, &import_spec->per_cap,
		     &import_spec->lat, &import_spec->lon, &import_spec->radius, &import_spec->fips,
		     &import_spec->min_age, &import_spec->max_age);
      if(n < 10) {
	Utils::fred_abort("Bad format in import file %s n = %d line = |%s|\n",
			  import_file_name, n, cstr);
      }
      // upper age must be strictly greater than lower age; otherwise, no one qualifies
      assert(import_spec->min_age < import_spec->max_age);

      this->import_map.push_back(import_spec);
    }
    imp_input->close();
  }
  if(Global::Verbose > 0) {
    for(int i = 0; i < this->import_map.size(); ++i) {
      string ss = this->import_map[i]->to_string();
      printf("%s\n", ss.c_str());
    }
  }

  int temp = 0;
  Params::get_param(this->condition->get_condition_name(), "report_generation_time", &temp);
  this->report_generation_time = temp;

  temp = 0;
  Params::get_param(this->condition->get_condition_name(), "report_transmission_by_age", &temp);
  this->report_transmission_by_age = temp;

  temp = 0;
  Params::get_param(this->condition->get_condition_name(), "enable_health_records", &temp);
  this->enable_health_records = temp;

  this->report_epi_stats = 1;
  Params::get_param(this->condition->get_condition_name(), "report_epi_stats", &this->report_epi_stats);

  // restore requiring parameters
  Params::set_abort_on_failure();

  // initialize state specific-variables here:
  this->natural_history = this->condition->get_natural_history();

  this->number_of_states = this->natural_history->get_number_of_states();
  FRED_VERBOSE(0, "Epidemic::setup states = %d\n", this->number_of_states);

  // initialize state counters
  incidence_count = new int [this->number_of_states];
  cumulative_count = new int [this->number_of_states];
  prevalence_count = new int [this->number_of_states];

  // read state specific parameters
  visualize_state = new bool [this->number_of_states];

  // these are optional parameters
  Params::disable_abort_on_failure();
  
  for (int i = 0; i < this->number_of_states; i++) {
    char label[FRED_STRING_SIZE];
    this->incidence_count[i] = 0;
    this->cumulative_count[i] = 0;
    this->prevalence_count[i] = 0;

    sprintf(label, "%s.%s",
	    this->natural_history->get_name(),
	    this->natural_history->get_state_name(i).c_str());

    // do we collect data for visualization?
    int n = 0;
    Params::get_param(label, "visualize", &n);
    this->visualize_state[i] = n;

  }

  // restore requiring parameters
  Params::set_abort_on_failure();

  FRED_VERBOSE(0, "setup for epidemic condition %s finished\n",
	       this->condition->get_condition_name());

}


//////////////////////////////////////////////////////
//
// FINAL PREP FOR THE RUN
//
//

void Epidemic::prepare() {

  // initialize the population
  int day = 0;
  int popsize = Global::Pop.get_population_size();
  for(int p = 0; p < popsize; ++p) {
    Person* person = Global::Pop.get_person(p);
    int old_state = person->get_health_state(this->id);
    int new_state = this->natural_history->get_initial_state(person);
    
    FRED_VERBOSE(1, "INITIAL_STATE COND %d day %d id %d age %0.2f race %d sex %c old_state %d new_state %d\n", 
		 this->id, day, person->get_id(), person->get_real_age(), person->get_race(), person->get_sex(), old_state, new_state);

    if (new_state < 0) {
      // person is initially immune
      person->become_immune(this->id);
    }
    else {
      if (new_state == this->natural_history->get_exposed_state()) {
	// check age range
	double age = person->get_real_age();
	if (this->natural_history->get_min_age() <= age && age <= this->natural_history->get_max_age()) {
	  // person is initially exposed
	  person->become_exposed(this->id, NULL, NULL, day);
	}
	else {
	  // printf("AGE_OUT_OF_BOUNDS: person %d age %d\n", person->get_id(), person->get_age());
	  // roll back to state 0:
	  new_state = 0;
	}
      }
      
      // update epidemic counters
      update_state_of_person(person, day, new_state);
    }
  }
  
  // suffix for printing epidemic stats to the output file
  // By default, the suffix is empty the there is only one condition,
  // and equal to the condition name otherwise
  if (Global::Conditions.get_number_of_conditions() == 1) {
    sprintf(this->output_var_suffix, "");
  }
  else {
    sprintf(this->output_var_suffix, "_%s", this->condition->get_condition_name());
  }

  if (Global::Enable_Visualization_Layer) {
    create_visualization_data_directories();
    FRED_VERBOSE(0, "visualization directories created\n");
  }

  FRED_VERBOSE(0, "epidemic prepare finished\n");

}


///////////////////////////////////////////////////////
//
// DAILY UPDATES
//
//

void Epidemic::update(int day) {

  FRED_VERBOSE(1, "epidemic update for condition %s day %d\n",
	       this->condition->get_condition_name(), day);
  Utils::fred_start_epidemic_timer();

  // if(Global::Enable_Visualization_Layer) {
    this->new_active_people_list.clear();
    this->new_symptomatic_people_list.clear();
    this->new_infectious_people_list.clear();
    // this->recovered_people_list.clear();
    // }

  // import infections from unknown sources
  get_imported_cases(day);
  // Utils::fred_print_epidemic_timer("imported infections");

  // handle scheduled transitions
  int size = this->state_transition_event_queue.get_size(day);
  FRED_VERBOSE(1, "TRANSITION_EVENT_QUEUE day %d %s size %d\n",
	       day, Date::get_date_string().c_str(), size);
    
  for(int i = 0; i < size; ++i) {
    Person* person = this->state_transition_event_queue.get_event(day, i);
    update_state_of_person(person, day, -1);
  }
  this->state_transition_event_queue.clear_events(day);

  // FRED_VERBOSE(0, "day %d ACTIVE_PEOPLE_LIST size = %d\n", day, this->active_people_list.size());

  // update list of active people
  for(person_set_iterator itr = this->active_people_list.begin(); itr != this->active_people_list.end(); ) {
    Person* person = (*itr);
    FRED_VERBOSE(1, "update_condition for person %d day %d\n", person->get_id(), day);
    person->update_condition(day, this->id);

    // handle case fatality
    if(person->is_case_fatality(this->id)) {
      // update epidemic fatality counters
      this->people_becoming_case_fatality_today++;
      this->total_case_fatality_count++;
      this->current_active_people--;
      if(Global::Enable_Visualization_Layer) {
	print_visualization_data_for_case_fatality(day, person);
      }
      if(Global::Report_Epidemic_Data_By_County) {
	int fips = Global::Places.get_county_for_place(person->get_household());
	this->county_infection_counts_map[fips].current_active--;
	this->county_infection_counts_map[fips].current_case_fatalities++;
      }
    }

    // note: case fatalities will be uninfected at this point
    if(person->is_infected(this->id) == false) {
      // delete from active list
      // FRED_VERBOSE(0, "ERASE ACTIVE_PEOPLE_LIST day %d person %d\n", day, person->get_id());
      this->active_people_list.erase(itr++);
    } else {
      // update person's mixing group infection counters
      person->update_household_counts(day, this->id);
      person->update_school_counts(day, this->id);
      // move on the next active person    
      ++itr;
    }
  }

  // FRED_VERBOSE(0, "day %d ACTIVE_PEOPLE_LIST size = %d\n", day, this->active_people_list.size());
  // FRED_VERBOSE(0, "day %d INFECTIOUS_PEOPLE_LIST size = %d\n", day, this->infectious_people_list.size());

  this->current_infectious_people = this->infectious_people_list.size();

  // update the daily activities of infectious people
  for(person_set_iterator itr = this->infectious_people_list.begin(); itr != this->infectious_people_list.end(); ++itr ) {
    Person* person = (*itr);

    if(strcmp("sexual", this->condition->get_transmission_mode()) == 0) {
      FRED_VERBOSE(1, "ADDING INF person %d\n", person->get_id());
      // this will insert the infectious person onto the infectious list in sexual partner network
      Sexual_Transmission_Network* st_network = Global::Sexual_Partner_Network;
      st_network->add_infectious_person(this->id, person);
    } else {
      FRED_VERBOSE(1, "updating activities of infectious person %d -- out of %d\n", person->get_id(), this->current_infectious_people);
      person->update_daily_activities(day);
      // note: places visited by infectious person will be added to the daily places in find_active_places_of_type()
    }
  }
  Utils::fred_print_epidemic_timer("schedules updated");

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

  FRED_VERBOSE(1, "epidemic update finished for condition %d day %d\n", id, day);
  return;
}


static bool compare_id (Person* p1, Person* p2) {
  return p1->get_id() < p2->get_id();
}

void Epidemic::get_imported_cases(int day) {

  FRED_VERBOSE(0, "GET_IMPORTED_CASES day %d map_size %d\n",  day, this->import_map.size());

  this->N = Global::Pop.get_population_size();

  for(int i = 0; i < this->import_map.size(); ++i) {
    Import_Map* import_spec = this->import_map[i];
    if(import_spec->sim_day_start <= day && day <= import_spec->sim_day_end) {
      FRED_VERBOSE(1,"IMPORT SPEC:\n");
      int max_imported = import_spec->max;
      fred::geo lat = import_spec->lat;
      fred::geo lon = import_spec->lon;
      double radius = import_spec->radius;
      long int fips = import_spec->fips;
      double min_age = import_spec->min_age;
      double max_age = import_spec->max_age;

      // number imported so far
      int imported_cases = 0;

      // list of susceptible people that qualify by distance and age
      std::vector<Person*> people;
      int searches_within_given_location = 1;
      while(searches_within_given_location <= 10) {
        FRED_VERBOSE(1,"IMPORT search number %d ", searches_within_given_location);

        // clear the list of candidates
        people.clear();

        // find households that qualify by location
        int hsize = Global::Places.get_number_of_households();
        // printf("IMPORT: houses  %d\n", hsize); fflush(stdout);
        for(int i = 0; i < hsize; ++i) {
          Household* hh = Global::Places.get_household(i);
	  if (fips) {
	    if (hh->get_census_tract_fips() != fips) {
	      continue;
	    }
	  }
	  else {
	    double dist = 0.0;
	    if(radius > 0) {
	      dist = Geo::xy_distance(lat,lon,hh->get_latitude(),hh->get_longitude());
	      if(radius < dist) {
		continue;
	      }
	    }
	  }
          // this household qualifies by location
          // find all susceptible housemates who qualify by age.
          int size = hh->get_size();
          // printf("IMPORT: house %d %s size %d fips %ld\n", i, hh->get_label(), size, hh->get_census_tract_fips()); fflush(stdout);
          for(int j = 0; j < size; ++j) {
            Person* person = hh->get_enrollee(j);
            if(person->get_health()->is_susceptible(this->id)) {
              double age = person->get_real_age();
              if(min_age <= age && age < max_age &&
		 this->natural_history->get_min_age() <= age && age < this->natural_history->get_max_age()) {
                people.push_back(person);
              }
            }
          }
	}

        // sort the candidates
        std::sort(people.begin(), people.end(), compare_id);

        int imported_cases_remaining = max_imported - imported_cases;
	if (import_spec->per_cap > 0.0) {
	  // exposed a fixed percentage of the age-appropriate population
	  imported_cases_remaining = import_spec->per_cap * people.size();
	}

	FRED_VERBOSE(0, "IMPORT: seeking %d candidates, found %d\n", imported_cases_remaining, (int) people.size());

        if(imported_cases_remaining <= people.size()) {
          // we have at least the minimum number of candidates.
          for(int n = 0; n < imported_cases_remaining; ++n) {
            FRED_VERBOSE(1, "IMPORT candidate %d people.size %d\n", n, (int)people.size());

            // pick a candidate without replacement
            int pos = Random::draw_random_int(0,people.size()-1);
            Person* person = people[pos];
            people[pos] = people[people.size() - 1];
            people.pop_back();

            // infect the candidate
            FRED_VERBOSE(1, "HEALTH IMPORT infecting candidate %d person %d\n", n, person->get_id());
	    person->become_exposed(this->id, NULL, NULL, day);
	    become_exposed(person, day);
            imported_cases++;
          }
          FRED_VERBOSE(0, "IMPORT SUCCESS: %d imported cases\n", imported_cases);
          return; // success!
        } else {
          // infect all the candidates
          for(int n = 0; n < people.size(); ++n) {
            Person* person = people[n];
            FRED_VERBOSE(1, "HEALTH IMPORT infecting candidate %d person %d\n", n, person->get_id());
	    person->become_exposed(this->id, NULL, NULL, day);
	    become_exposed(person, day);
            imported_cases++;
          }
        }

        if(radius > 0) {
          // expand the distance and try again
          radius = 2 * radius;
          FRED_VERBOSE(0, "IMPORT: increasing radius to %f\n", radius);
          searches_within_given_location++;
        } else {
          // return with a warning
          FRED_VERBOSE(0, "IMPORT FAILURE: only %d imported cases out of %d\n", imported_cases, max_imported);
          return;
        }
      } //End while(searches_within_given_location <= 10)
      // after 10 tries, return with a warning
      FRED_VERBOSE(0, "IMPORT FAILURE: only %d imported cases out of %d\n", imported_cases, max_imported);
      return;
    }
  }
  // FRED_VERBOSE(0, "get_imported_cases finished\n");
}


void Epidemic::find_active_places_of_type(int day, int place_type) {

  FRED_VERBOSE(1, "find_active_places_of_type %d\n", place_type);
  this->active_places_list.clear();
  FRED_VERBOSE(1, "find_active_places_of_type %d infectious_people = %d\n", place_type, this->current_infectious_people);
  for(person_set_iterator itr = this->infectious_people_list.begin(); itr != this->infectious_people_list.end(); ++itr ) {
    Person* person = (*itr);
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
      this->active_places_list.insert(place);
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
          this->active_places_list.insert(place);
        }
      }
      break;
    case 2:
      // add schools
      size = Global::Places.get_number_of_schools();
      for(int i = 0; i < size; ++i) {
        Place* place = Global::Places.get_school(i);
        if(place->get_infectious_vectors(this->id) > 0) {
          this->active_places_list.insert(place);
        }
      }
      break;
    case 4:
      // add workplaces
      size = Global::Places.get_number_of_workplaces();
      for(int i = 0; i < size; ++i) {
        Place* place = Global::Places.get_workplace(i);
        if(place->get_infectious_vectors(this->id) > 0) {
          this->active_places_list.insert(place);
        }
      }
      break;
    }
  }

  FRED_VERBOSE(1, "find_active_places_of_type %d found %d\n", place_type, this->active_places_list.size());
}
  
void Epidemic::spread_infection_in_active_places(int day) {
  // FRED_VERBOSE(0, "spread_infection_in_active_places day %d\n", day);
  for(place_set_iterator itr = active_places_list.begin(); itr != this->active_places_list.end(); ++itr) {
    Place* place = *itr;
    // FRED_VERBOSE(0, "spread_infection_in_active_place day %d place %d\n", day, place->get_id());
    this->condition->get_transmission()->spread_infection(day, this->id, place);
    place->clear_infectious_people(this->id);
  }
  return;
}


//////////////////////////////////////////////////////////////
//
// HANDLING CHANGES TO AN INDIVIDUAL'S STATUS
//
//


// called from get_imported_cases() and attempt_transmission():

void Epidemic::become_exposed(Person* person, int day) {
  int new_state = this->natural_history->get_exposed_state();
  update_state_of_person(person, day, new_state);
}

// called from update_state_of_person():

void Epidemic::become_active(Person* person, int day) {

  FRED_VERBOSE(1, "Epidemic::become_active day %d person %d\n", day, person->get_id());

  FRED_VERBOSE(1, "INSERT ACTIVE_PEOPLE_LIST day %d person %d\n", day, person->get_id());
  this->active_people_list.insert(person);
  if(Global::Enable_Visualization_Layer) {
    this->new_active_people_list.insert(person);
  }

  // update epidemic counters
  this->current_exposed_people++;
  this->current_active_people++;
  this->total_cases++;
  this->people_becoming_active_today++;

  this->daily_infections_list.push_back(person);

  if(Global::Report_Mean_Household_Stats_Per_Income_Category) {
    if(person->get_household() != NULL) {
      Household* hh = person->get_household();
      int income_level = hh->get_household_income_code();
      if(income_level >= Household_income_level_code::CAT_I &&
         income_level < Household_income_level_code::UNCLASSIFIED) {
        this->household_income_infection_counts_map[income_level].tot_ppl_evr_inf++;
      }
    }
  }

  if(Global::Report_Epidemic_Data_By_County) {
    int fips = Global::Places.get_county_for_place(person->get_household());
    this->county_infection_counts_map[fips].current_active++;
  }

  if(Global::Report_Epidemic_Data_By_Census_Tract) {
    if(person->get_household() != NULL) {
      Household* hh = person->get_household();
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
      School* schl = person->get_school();
      Household* hh = person->get_household();
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

}

void Epidemic::become_infectious(Person* person, int day) {

  FRED_VERBOSE(1, "Epidemic::become_infectious day %d person %d\n", day, person->get_id());

  // add to infectious_people_list
  FRED_VERBOSE(1, "INSERT INFECTIOUS_PEOPLE_LIST day %d person %d\n", day, person->get_id());
  this->infectious_people_list.insert(person);
  this->people_becoming_infectious_today++;

  if(Global::Enable_Visualization_Layer) {
    this->new_infectious_people_list.insert(person);
  }

  // update epidemic counters
  this->current_exposed_people--;

  // update person's health record
  person->become_infectious(this->id);

  // decide whether to confine to household due to contact tracing
  if (this->total_severe_symptomatic_cases > this->natural_history->get_contact_tracing_trigger()) {
    if (this->natural_history->is_confined_to_household_due_to_contact_tracing(person)) {
      person->confine_to_household(this->id);
    }
  }

}

void Epidemic::become_noninfectious(Person* person, int day) {

  FRED_VERBOSE(1, "become noninfectious day %d person %d\n", day, person->get_id());

  person_set_iterator itr = this->infectious_people_list.find(person);
  if(itr != this->infectious_people_list.end()) {
    // delete from infectious list
    FRED_VERBOSE(1, "DELETE INFECTIOUS_PEOPLE_LIST day %d person %d\n", Global::Simulation_Day, person->get_id());
    this->infectious_people_list.erase(itr);
  }

  // update person's health record
  person->become_noninfectious(this->id);

  // release from household confinement if necessary
  person->clear_household_confinement(this->id);

}

void Epidemic::become_symptomatic(Person* person, int day) {

  FRED_VERBOSE(1, "become symptomatic day %d person %d\n", day, person->get_id());

  // update epidemic counters
  this->current_symptomatic_people++;
  this->total_symptomatic_cases++;
  this->people_becoming_symptomatic_today++;
  if(Global::Enable_Visualization_Layer) {
    this->new_symptomatic_people_list.insert(person);
  }

  if(Global::Report_Epidemic_Data_By_County) {
    int fips = Global::Places.get_county_for_place(person->get_household());
    this->county_infection_counts_map[fips].current_symptomatic++;
    // FRED_VERBOSE(0, "SYMP_START_EVENT day %d person %d  fips %d count %d\n",
    // day, person->get_id(), fips, this->county_infection_counts_map[fips].current_symptomatic);
  }

  if(Global::Report_Mean_Household_Stats_Per_Income_Category) {
    if(person->get_household() != NULL) {
      int income_level = person->get_household()->get_household_income_code();
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
      Household* hh = person->get_household();
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
    Workplace* wp = person->get_workplace();
    int index = Activities::get_index_of_sick_leave_dist(person);
    if(wp != NULL && index != -1) {
      this->workplace_size_infection_counts_map[index].tot_ppl_evr_sympt++;
    }
  }

  if(Global::Report_Childhood_Presenteeism) {
    if(person->get_household() != NULL &&
       person->is_student() &&
       person->get_school() != NULL) {
      Household* hh = person->get_household();
      School* schl = person->get_school();
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

  this->daily_symptomatic_list.push_back(person);

  // update person's health record
  person->become_symptomatic(this->id);
}

void Epidemic::become_asymptomatic(Person* person, int day) {

  // FRED_VERBOSE(0, "become asymptomatic day %d person %d\n", day, person->get_id());

  // update epidemic counters
  this->current_symptomatic_people--;

  // update person's health record
  person->resolve_symptoms(this->id);
        
  if(Global::Report_Epidemic_Data_By_County) {
    int fips = Global::Places.get_county_for_place(person->get_household());
    this->county_infection_counts_map[fips].current_symptomatic--;
  }
}


void Epidemic::recover(Person* person, int day) {

  FRED_VERBOSE(1, "recover day %d person %d\n", day, person->get_id());
  
  this->current_active_people--;
  this->current_removed_people++;

  person_set_iterator itr = this->infectious_people_list.find(person);
  if(itr != this->infectious_people_list.end()) {
    // delete from infectious list
    FRED_VERBOSE(1, "DELETE from INFECTIOUS_PEOPLE_LIST day %d person %d\n", Global::Simulation_Day, person->get_id());
    this->infectious_people_list.erase(itr);
  }

  itr = this->active_people_list.find(person);
  if(itr != this->active_people_list.end()) {
    // delete from active list
    FRED_VERBOSE(1, "DELETE from ACTIVE_PEOPLE_LIST day %d person %d\n", Global::Simulation_Day, person->get_id());
    this->active_people_list.erase(itr);
  }

  if(Global::Report_Epidemic_Data_By_County) {
    int fips = Global::Places.get_county_for_place(person->get_household());
    this->county_infection_counts_map[fips].current_active--;
  }

  if (Global::Enable_Visualization_Layer) {
    this->recovered_people_list.insert(person);
  }

  // update person's health record
  person->recover(this->id, day);
}

void Epidemic::terminate_person(Person* person, int day) {

  FRED_VERBOSE(0, "EPIDEMIC TERMINATE person %d day %d\n",
               person->get_id(), day);

  // delete from state list
  int state = person->get_health_state(this->id);
  if (0 <= state) {
    prevalence_count[state]--;
    FRED_VERBOSE(0, "EPIDEMIC TERMINATE person %d day %d %s removed from state %d\n",
		 person->get_id(), day, Date::get_date_string().c_str(), state);
  }

  // update current case counts
  if (person->is_infected(this->id)) {
    this->current_active_people--;
  }

  // update total case count
  if (person->ever_active(this->id)) {
    this->total_cases--;
  }

  // update immune count
  if (person->is_immune(this->id)) {
    this->current_immune_people--;
  }

  // cancel any scheduled transition
  int transition_day = person->get_next_health_transition_day(this->id);
  if (day <= transition_day) {
    // printf("person %d delete_event for transition_day %d\n", person->get_id(), transition_day);
    this->state_transition_event_queue.delete_event(transition_day, person);
  }

  person->set_health_state(this->id, -1, day);

  if(Global::Report_Epidemic_Data_By_County) {
    int fips = Global::Places.get_county_for_place(person->get_household());
    this->county_infection_counts_map[fips].current_active--;
  }

  FRED_VERBOSE(1, "EPIDEMIC TERMINATE person %d finished\n", person->get_id());
}

//////////////////////////////////////////////////////////////
//
// REPORTING
//
//

void Epidemic::report(int day) {
  if (this->report_epi_stats) {
    print_stats(day);
  }
  if (Global::Enable_Visualization_Layer) {
    print_visualization_data_for_active_infections(day);
  }
}

void Epidemic::print_stats(int day) {
  FRED_VERBOSE(1, "epidemic print stats for condition %d day %d\n", id, day);

  FILE* fp;
  char file[FRED_STRING_SIZE];

  // set population size, and remember original pop size
  if(day == 0) {
    this->N_init = this->N = Global::Pop.get_population_size();
  } else {
    this->N = Global::Pop.get_population_size();
  }

  // get reproductive rate for the cohort exposed RR_delay days ago
  // unless RR_delay == 0
  this->daily_cohort_size[day] = this->people_becoming_active_today;
  this->RR = 0.0;         // reproductive rate for a fixed cohort of infectors
  if(0 < Global::RR_delay && Global::RR_delay <= day) {
    int cohort_day = day - Global::RR_delay;    // exposure day for cohort
    int cohort_size = this->daily_cohort_size[cohort_day];        // size of cohort
    if(cohort_size > 0) {
      // compute reproductive rate for this cohort
      this->RR = static_cast<double>(this->number_infected_by_cohort[cohort_day]) / static_cast<double>(cohort_size);
    }
  }

  if(this->id == 0) {
    Global::Daily_Tracker->set_index_key_pair(day,"Date", Date::get_date_string());
    char epiweek[FRED_STRING_SIZE];
    sprintf(epiweek, "%d.%02d", Date::get_epi_year(), Date::get_epi_week());
    string epiwk = string(epiweek);
    Global::Daily_Tracker->set_index_key_pair(day,"EpiWeek", epiwk);
    Global::Daily_Tracker->set_index_key_pair(day,"Popsize", this->N);
  }

  this->current_susceptible_people = this->N - this->current_active_people - this->current_removed_people - this->current_immune_people;

  track_value(day, (char*) "S", this->current_susceptible_people);
  track_value(day, (char*) "E", this->current_exposed_people);
  track_value(day, (char*) "I", this->current_infectious_people);
  track_value(day, (char*) "Im", this->current_immune_people);
  track_value(day, (char*) "CS", this->current_symptomatic_people);
  track_value(day, (char*) "R", this->current_removed_people);
  track_value(day, (char*) "P", this->current_active_people);
  track_value(day, (char*) "RR", this->RR);

  track_value(day, (char*) "newC", this->people_becoming_active_today);
  track_value(day, (char*) "newI", this->people_becoming_infectious_today);
  track_value(day, (char*) "newCS", this->people_becoming_symptomatic_today);
  track_value(day, (char*) "newCF", this->people_becoming_case_fatality_today);
  track_value(day, (char*) "newIm", this->people_becoming_immune_today);

  track_value(day, (char*) "totC", this->total_cases);
  track_value(day, (char*) "totCF", this->total_case_fatality_count);
  track_value(day, (char*) "totCS", this->total_symptomatic_cases);

  // state stats
  for (int i = 0; i < this->number_of_states; i++) {
    char key[FRED_STRING_SIZE];
    sprintf(key, "%s.%s", this->name, this->natural_history->get_state_name(i).c_str());
    track_value(day, key, this->prevalence_count[i]);
    sprintf(key, "%s.new%s", this->name, this->natural_history->get_state_name(i).c_str());
    track_value(day, key, this->incidence_count[i]);
    sprintf(key, "%s.tot%s", this->name, this->natural_history->get_state_name(i).c_str());
    track_value(day, key, this->cumulative_count[i]);
  }

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

  FRED_VERBOSE(1, "report place of infection\n");
  report_place_of_infection(day);

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

  //Only report AR and ARs on last day
  if(Global::Report_Epidemic_Data_By_Census_Tract && day == (Global::Days - 1)) {
    report_census_tract_stratified_results(day);
  }

  if(1 || Global::Enable_Group_Quarters) {
    // report_group_quarters_incidence(day);
  }

  if(Global::Verbose) {
    fprintf(Global::Statusfp, "\n");
    fflush(Global::Statusfp);
  }

  // prepare for next day
  this->people_becoming_active_today = 0;
  this->people_becoming_infectious_today = 0;
  this->people_becoming_symptomatic_today = 0;
  this->people_becoming_immune_today = 0;
  this->people_becoming_case_fatality_today = 0;
  this->daily_infections_list.clear();
  this->daily_symptomatic_list.clear();
  if (this->natural_history != NULL) {
    for (int i = 0; i < this->number_of_states; ++i) {
      this->incidence_count[i] = 0;
    }
  }

  FRED_VERBOSE(1, "epidemic print stats for condition %d day %d\n", id, day);
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

  for(int i = 0; i < this->people_becoming_active_today; ++i) {
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
    // track_value(day, (char*)"tot_Cs_0_4", this->population_infection_counts.tot_Cs_0_4);
    // track_value(day, (char*)"tot_Cs_5_17", this->population_infection_counts.tot_Cs_5_17);
    // track_value(day, (char*)"tot_Cs_18_49", this->population_infection_counts.tot_Cs_18_49);
    // track_value(day, (char*)"tot_Cs_50_64", this->population_infection_counts.tot_Cs_50_64);
    // track_value(day, (char*)"tot_Cs_65_up", this->population_infection_counts.tot_Cs_65_up);
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
    track_value(day, str, this->county_infection_counts_map[fips].current_active);

    sprintf(str, "Ps:%d", fips);
    track_value(day, str, this->county_infection_counts_map[fips].current_symptomatic);

    sprintf(str, "Pa:%d", fips);
    track_value(day, str, this->county_infection_counts_map[fips].current_active - this->county_infection_counts_map[fips].current_symptomatic);

    sprintf(str, "N:%d", fips);
    track_value(day, str, Global::Places.get_population_of_county_with_index(i));

    sprintf(str, "CF:%d", fips);
    track_value(day, str, this->county_infection_counts_map[fips].current_case_fatalities);

    this->county_infection_counts_map[fips].current_case_fatalities = 0;
  }
}

void Epidemic::report_serial_interval(int day) {

  for(int i = 0; i < this->people_becoming_active_today; ++i) {
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
  for(int i = 0; i < this->people_becoming_active_today; ++i) {
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
  for(int i = 0; i < this->people_becoming_active_today; ++i) {
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
  int active = this->people_becoming_active_today;
  for(int i = 0; i < active; ++i) {
    Person* infectee = this->daily_infections_list[i];
    // FRED_VERBOSE(0, "person %d is %d out of %d\n", infectee->get_id(), i, active);
    Household* hh = infectee->get_household();
    if(hh == NULL) {
      if(Global::Enable_Hospitals && infectee->is_hospitalized() && infectee->get_permanent_household() != NULL) {
        hh = infectee->get_permanent_household();
      }
    }

    int fips = hh->get_county_fips();
    int c = Global::Places.get_index_of_county_with_fips(fips);
    assert(0 <= c && c < this->counties);
    this->county_incidence[c]++;
    // FRED_VERBOSE(0, "county %d incidence %d %d out of %d person %d \n", c, this->county_incidence[c], i, active, infectee->get_id());
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
    Person* infectee = this->daily_symptomatic_list[i];
    Household* h = (Household*) infectee->get_household();
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

int Epidemic::get_symptomatic_incidence_by_tract_index(int index_) {
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
  for(int i = 0; i < this->people_becoming_active_today; ++i) {
    Person* infectee = this->daily_infections_list[i];
    Household* hh = infectee->get_household();
    if(hh == NULL) {
      if(Global::Enable_Hospitals && infectee->is_hospitalized() && infectee->get_permanent_household() != NULL) {
        hh = infectee->get_permanent_household();
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

  for(int i = 0; i < this->people_becoming_active_today; ++i) {
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
  Utils::fred_log(" GQ %d COLL %d PRIS %d NURS %d MIL %d", G,D,J,L,B);
  Utils::fred_log("\n");

  //Store for daily output file
  track_value(day, (char*) "newGQ", G);
  track_value(day, (char*) "newCOLL", D);
  track_value(day, (char*) "newPRIS", J);
  track_value(day, (char*) "newNURS", L);
  track_value(day, (char*) "newMIL", B);
}

void Epidemic::report_place_of_infection(int day) {

  FILE* fp;
  char file[FRED_STRING_SIZE];

  // type of place of infection
  int X = 0;
  int H = 0;
  int Nbr = 0;
  int S = 0;
  int C = 0;
  int W = 0;
  int O = 0;
  int M = 0;

  FRED_VERBOSE(1, "DAILY INFECTIONS LIST size = %d %d\n", this->people_becoming_active_today, this->daily_infections_list.size());
  for(int i = 0; i < this->people_becoming_active_today; i++) {
    Person* infectee = this->daily_infections_list[i];
    FRED_VERBOSE(1, "i %d person %d\n", i, infectee->get_id());
    char c = infectee->get_infected_mixing_group_type(this->id);
    switch(c) {
    case 'X':
      X++;
      break;
    case 'H':
      H++;
      break;
    case 'N':
      Nbr++;
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

  //Store for daily output file
  char key[FRED_STRING_SIZE];
  track_value(day, (char*) "newIMP", X);
  track_value(day, (char*) "newHH", H);
  track_value(day, (char*) "newNBR", Nbr);
  track_value(day, (char*) "newSCH", S+C);
  track_value(day, (char*) "newCLS", C);
  track_value(day, (char*) "newWRK", W+O);
  track_value(day, (char*) "newOFF", O);
  track_value(day, (char*) "newHOS", M);

}

void Epidemic::report_distance_of_infection(int day) {
  double tot_dist = 0.0;
  double ave_dist = 0.0;
  int n = 0;
  for(int i = 0; i < this->people_becoming_active_today; ++i) {
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
  track_value(day, (char*) "Dist", 1000 * ave_dist);
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
  for(int i = 0; i < this->people_becoming_active_today; ++i) {
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
  // Global::Daily_Tracker->set_index_key_pair(day, "N", this->N);

  // Global::Daily_Tracker->set_index_key_pair(day, "tot_days_sympt", this->population_infection_counts.tot_days_sympt);
  // Global::Daily_Tracker->set_index_key_pair(day, "tot_ppl_evr_sympt", this->population_infection_counts.tot_ppl_evr_sympt);

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

  for(int i = 0; i < this->people_becoming_active_today; ++i) {

    Person* infectee = this->daily_infections_list[i];
    assert(infectee != NULL);
    char c = infectee->get_infected_mixing_group_type(this->id);

    // school presenteeism requires that place of infection is school or classroom
    if(c != Place::TYPE_SCHOOL && c != Place::TYPE_CLASSROOM) {
      continue;
    }
    infections_at_school++;

    // get the school income quartile
    School* school = infectee->get_school();
    assert(school != NULL);
    int income_quartile = school->get_income_quartile();

    // presenteeism requires that the infector have symptoms
    Person* infector = infectee->get_infector(this->id);
    assert(infector != NULL);
    if(infector->is_symptomatic()) {

      // determine whether anyone was at home to watch child
      Household* hh = infector->get_household();
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
  // Global::Daily_Tracker->set_index_key_pair(day, "N", this->N);

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


void Epidemic::track_value(int day, char* keystr, int value) {
  int length = strlen(this->name);
  if ((strncmp(this->name, keystr, length) == 0) && (keystr[length]=='.')) {
    // this is a state variable of this condition -- add no suffix
    Global::Daily_Tracker->set_index_key_pair(day, keystr, value);
  }
  else {
    // this is not a state variable, so add the condition as suffix
    // (suffix may be null)
    char key[FRED_STRING_SIZE];
    sprintf(key, "%s%s", keystr, this->output_var_suffix);
    Global::Daily_Tracker->set_index_key_pair(day, key, value);
  }
}

void Epidemic::track_value(int day, char* keystr, double value) {
  int length = strlen(this->name);
  if ((strncmp(this->name, keystr, length) == 0) && (keystr[length]=='.')) {
    // this is a state variable of this condition -- add no suffix
    Global::Daily_Tracker->set_index_key_pair(day, keystr, value);
  }
  else {
    // this is not a state variable, so add the condition as suffix
    // (suffix may be null)
    char key[FRED_STRING_SIZE];
    sprintf(key, "%s%s", keystr, this->output_var_suffix);
    Global::Daily_Tracker->set_index_key_pair(day, key, value);
  }
}

void Epidemic::track_value(int day, char* keystr, string value) {
  int length = strlen(this->name);
  if ((strncmp(this->name, keystr, length) == 0) && (keystr[length]=='.')) {
    // this is a state variable of this condition -- add no suffix
    Global::Daily_Tracker->set_index_key_pair(day, keystr, value);
  }
  else {
    // this is not a state variable, so add the condition as suffix
    // (suffix may be null)
    char key[FRED_STRING_SIZE];
    sprintf(key, "%s%s", keystr, this->output_var_suffix);
    Global::Daily_Tracker->set_index_key_pair(day, key, value);
  }
}


void Epidemic::become_immune(Person* person, bool susceptible, bool infectious, bool symptomatic) {
  if(!susceptible) {
    this->current_removed_people++;
  }
  if (symptomatic) {
    this->current_symptomatic_people--;
  }
  this->people_becoming_immune_today++;
  this->current_immune_people++;
}

void Epidemic::create_plot_data_directories() {
}


void Epidemic::create_visualization_data_directories() {
  char vis_var_dir[FRED_STRING_SIZE];

  // create directories for specific output variables
  sprintf(vis_var_dir, "%s/I%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Ia%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Is%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/C%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Ci%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Cs%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/CF%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/P%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Pa%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Ps%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/N%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/R%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);
  sprintf(vis_var_dir, "%s/Vec%s", Global::Visualization_directory, this->output_var_suffix);
  Utils::fred_make_directory(vis_var_dir);

  for (int i = 0; i < this->number_of_states; i++) {
    sprintf(vis_var_dir, "%s/%s.%s",
	    Global::Visualization_directory,
	    this->name,
	    this->natural_history->get_state_name(i).c_str());
    Utils::fred_make_directory(vis_var_dir);
    sprintf(vis_var_dir, "%s/%s.new%s",
	    Global::Visualization_directory,
	    this->name,
	    this->natural_history->get_state_name(i).c_str());
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
  FILE* statefp[this->number_of_states];
  FILE* newstatefp[this->number_of_states];

  sprintf(filename, "%s/C%s/households-%d.txt", Global::Visualization_directory, this->output_var_suffix, day);
  fp = fopen(filename, "w");
  for(person_set_iterator itr = this->new_active_people_list.begin(); itr != this->new_active_people_list.end(); ++itr) {
    person = (*itr);
    household = person->get_household();
    tract = Global::Places.get_census_tract_for_place(household);
    lat = household->get_latitude();
    lon = household->get_longitude();
    fprintf(fp, "%f %f %ld\n", lat, lon, tract);
  }
  fclose(fp);

  sprintf(filename, "%s/Cs%s/households-%d.txt", Global::Visualization_directory, this->output_var_suffix, day);
  fp = fopen(filename, "w");
  for(person_set_iterator itr = this->new_symptomatic_people_list.begin(); itr != this->new_symptomatic_people_list.end(); ++itr) {
    person = (*itr);
    household = person->get_household();
    tract = Global::Places.get_census_tract_for_place(household);
    lat = household->get_latitude();
    lon = household->get_longitude();
    fprintf(fp, "%f %f %ld\n", lat, lon, tract);
  }
  fclose(fp);

  sprintf(filename, "%s/Ci%s/households-%d.txt", Global::Visualization_directory, this->output_var_suffix, day);
  fp = fopen(filename, "w");
  for(person_set_iterator itr = this->new_infectious_people_list.begin(); itr != this->new_infectious_people_list.end(); ++itr) {
    person = (*itr);
    household = person->get_household();
    tract = Global::Places.get_census_tract_for_place(household);
    lat = household->get_latitude();
    lon = household->get_longitude();
    fprintf(fp, "%f %f %ld\n", lat, lon, tract);
  }
  fclose(fp);

  sprintf(filename, "%s/R%s/households-%d.txt", Global::Visualization_directory, this->output_var_suffix, day);
  fp = fopen(filename, "w");
  for(person_set_iterator itr = this->recovered_people_list.begin(); itr != this->recovered_people_list.end(); ++itr) {
    person = (*itr);
    household = person->get_household();
    tract = Global::Places.get_census_tract_for_place(household);
    lat = household->get_latitude();
    lon = household->get_longitude();
    fprintf(fp, "%f %f %ld\n", lat, lon, tract);
  }
  fclose(fp);

  // make sure visualization file for CF exists
  sprintf(filename, "%s/CF%s/households-%d.txt", Global::Visualization_directory, this->output_var_suffix, day);
  fp = fopen(filename, "r");
  if (fp == NULL) {
    // create an empty file
    fp = fopen(filename, "w");
  }
  fclose(fp);

  // open file pointers for all epidemic stats
  sprintf(filename, "%s/I%s/households-%d.txt", Global::Visualization_directory, this->output_var_suffix, day);
  FILE* I_fp = fopen(filename, "w");
  sprintf(filename, "%s/Ia%s/households-%d.txt", Global::Visualization_directory, this->output_var_suffix, day);
  FILE* Ia_fp = fopen(filename, "w");
  sprintf(filename, "%s/Is%s/households-%d.txt", Global::Visualization_directory, this->output_var_suffix, day);
  FILE* Is_fp = fopen(filename, "w");
  sprintf(filename, "%s/P%s/households-%d.txt", Global::Visualization_directory, this->output_var_suffix, day);
  FILE* P_fp = fopen(filename, "w");
  sprintf(filename, "%s/Pa%s/households-%d.txt", Global::Visualization_directory, this->output_var_suffix, day);
  FILE* Pa_fp = fopen(filename, "w");
  sprintf(filename, "%s/Ps%s/households-%d.txt", Global::Visualization_directory, this->output_var_suffix, day);
  FILE* Ps_fp = fopen(filename, "w");

  // open file pointers for each state
  for (int i = 0; i < this->number_of_states; i++) {
    if (this->visualize_state[i]) {
      sprintf(filename, "%s/%s.%s/households-%d.txt",
	      Global::Visualization_directory,
	      this->name,
	      this->natural_history->get_state_name(i).c_str(),
	      day);
      statefp[i] = fopen(filename, "w");
      sprintf(filename, "%s/%s.new%s/households-%d.txt",
	      Global::Visualization_directory,
	      this->name,
	      this->natural_history->get_state_name(i).c_str(),
	      day);
      newstatefp[i] = fopen(filename, "w");
    }
  }

  for(person_set_iterator itr = this->active_people_list.begin(); itr != this->active_people_list.end(); ++itr) {
    person = (*itr);
    int state = person->get_health_state(this->id);
    if (state < 0) {
      Utils::fred_abort("Help: person %d age %d state %d (should be at least 0)\n",
			person->get_id(),
			person->get_age(),
			state);
    }
    household = person->get_household();

    tract = Global::Places.get_census_tract_for_place(household);
    lat = household->get_latitude();
    lon = household->get_longitude();
    sprintf(location,  "%f %f %ld\n", lat, lon, tract);
    if (this->visualize_state[state]) {
      fputs(location, statefp[state]);
      if (day == person->get_last_health_transition_day(this->id)) {
	fputs(location, newstatefp[state]);
      }
    }

    if (person->is_infected(this->id)) {
      // person is currently active
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

  // print locations of households associated with each state
  /*
    int popsize = Global::Pop.get_population_size();
    for(int p = 0; p < popsize; ++p) {
    Person* person = Global::Pop.get_person(p);
    int state = person->get_health_state(this->id);
    if (state >= 0) {
    household = person->get_household();
    tract = Global::Places.get_census_tract_for_place(household);
    lat = household->get_latitude();
    lon = household->get_longitude();
    sprintf(location,  "%f %f %ld\n", lat, lon, tract);
    fputs(location, statefp[state]);
    }
    }
  */

  // close file pointers for each state
  for (int i = 0; i < this->number_of_states; i++) {
    if (this->visualize_state[i]) {
      fclose(statefp[i]);
      fclose(newstatefp[i]);
    }
  }

}


void Epidemic::print_visualization_data_for_case_fatality(int day, Person* person) {
  Place* household = person->get_household();
  char filename[FRED_STRING_SIZE];
  for(int d = day; d < day + 30; ++d) {
    sprintf(filename, "%s/CF%s/households-%d.txt", Global::Visualization_directory, this->output_var_suffix, d);
    FILE* fp = fopen(filename, "a");
    fprintf(fp, "%f %f %ld\n", household->get_latitude(), household->get_longitude(), Global::Places.get_census_tract_for_place(household));
    fclose(fp);
  }
}


void Epidemic::delete_from_active_people_list(Person* person) {
  // this only happens for case fatalities
  FRED_VERBOSE(0, "deleting terminated person %d from active_people_list list\n", person->get_id());

  person_set_iterator itr = this->infectious_people_list.find(person);
  if(itr != this->infectious_people_list.end()) {
    // delete from infectious list
    FRED_VERBOSE(0, "DELETE from INFECTIOUS_PEOPLE_LIST day %d person %d\n", Global::Simulation_Day, person->get_id());
    this->infectious_people_list.erase(itr);
  }

  itr = this->active_people_list.find(person);
  if(itr != this->active_people_list.end()) {
    // delete from active list
    FRED_VERBOSE(0, "DELETE from ACTIVE_PEOPLE_LIST day %d person %d\n", Global::Simulation_Day, person->get_id());
    this->active_people_list.erase(itr);
  }

  this->people_becoming_case_fatality_today++;
  print_visualization_data_for_case_fatality(Global::Simulation_Day, person);
}


void Epidemic::update_state_of_person(Person* person, int day, int new_state) {

  int old_state = person->get_health_state(this->id);

  // no update for people outside the age range:
  double age = person->get_real_age();
  if (age < natural_history->get_min_age() || natural_history->get_max_age() < age) {
    printf("UPDATE_STATE_ABORTED due to AGE_OUT_OF_BOUNDS: person %d age %d cond %d old_state %d new_state %d\n",
	   person->get_id(), person->get_age(), this->id, old_state, new_state);
    // person->become_unsusceptible(this->id);
    return;
  }

  if (new_state < 0) {
    // this is a schedule state transition
    new_state = this->natural_history->get_next_state(person, old_state);
    assert(0 <= new_state);

    // this handles a new exposure as the result of get_next_state()
    if (new_state == this->natural_history->get_exposed_state() && person->get_exposure_date(this->id) < 0) {
      person->become_exposed(this->id, NULL, NULL, day);
    }

  }

  // get the update period associated with the new state
  double transition_period = this->natural_history->get_transition_period(new_state);

  FRED_VERBOSE(1, "UPDATE_STATE day %d person %d age %0.2f race %d sex %c old_state %d new_state %d transition_period %f exp %d\n", 
	       day, person->get_id(), age, person->get_race(), person->get_sex(), old_state, new_state, transition_period, person->get_exposure_date(this->id));

  if (transition_period > 0.0) {

    // schedule next transition test
    int transition_day = day + round(transition_period);
    // ensure that the next transition test will occur by scheduling it at least one day in the future
    if (transition_day == day) {
      transition_day++;
    }
    FRED_VERBOSE(0, "UPDATE_STATE day %d adding person %d to state_transition_event_queue for day %d\n",
		 day, person->get_id(), transition_day);
    this->state_transition_event_queue.add_event(transition_day, person);
  }

  if (old_state != new_state) {

    // update the epidemic variables for this person's new state

    if (0 <= old_state) {
      this->prevalence_count[old_state]--;
    }

    if (0 <= new_state) {
      this->incidence_count[new_state]++;
      this->cumulative_count[new_state]++;
      this->prevalence_count[new_state]++;
    }

    // note: person's health state is still old_state

    if (new_state == this->natural_history->get_exposed_state() && person->get_exposure_date(this->id)==day) {
      become_active(person, day);
    }

    if (this->natural_history->get_symptoms_level(new_state) > 0 && person->is_symptomatic(this->id)==false) {
      become_symptomatic(person, day);
    }
    
    if (this->natural_history->get_symptoms_level(new_state) == 0 && person->is_symptomatic(this->id)) {
      become_asymptomatic(person, day);
    }
    
    if (this->natural_history->get_infectivity(new_state) > 0.0 && person->is_infectious(this->id)==false) {
      become_infectious(person, day);
    }
    
    if (this->natural_history->get_infectivity(new_state) == 0.0 && person->is_infectious(this->id)) {
      become_noninfectious(person, day);
    }
    
    if (this->natural_history->get_susceptibility(new_state) > 0.0 && person->is_susceptible(this->id)==false) {
      // become_susceptible(person, day);
    }
    
    if (this->natural_history->get_susceptibility(new_state) == 0.0 && person->is_susceptible(this->id)) {
      // become_unsusceptible(person, day);
    }
    
    if (this->natural_history->is_recovered_state(new_state)) {
      recover(person, day);
    }

    // update severe symptoms count
    if (this->natural_history->get_symptoms_level(new_state) == Global::SEVERE_SYMPTOMS &&
	(old_state < 0 || this->natural_history->get_symptoms_level(old_state) != Global::SEVERE_SYMPTOMS)) {
      this->total_severe_symptomatic_cases++;
    }

    // now finally, update person's health state, including symptoms, infectivity etc
    person->set_health_state(this->id, new_state, day);

    // update person health record
    if (this->enable_health_records && Global::Enable_Health_Records) {
      fprintf(Global::HealthRecordfp,
	      "HEALTH RECORD: %s day %d person %d age %.1f sex %c race %d CONDITION %s CHANGES from %s to %s\n",
	      Date::get_date_string().c_str(), day,
	      person->get_id(), age,
	      person->get_sex(),
	      person->get_race(),
	      this->condition->get_condition_name(),
	      this->natural_history->get_state_name(old_state).c_str(),
	      this->natural_history->get_state_name(new_state).c_str());
    }
  }

  // see if we need to modify any other condition
  for (int i = 0; i < Global::Conditions.get_number_of_conditions(); ++i) {
    int istate = person->get_health_state(i);
    if (0 <= istate) {
      int mod_state = this->natural_history->get_state_modifier(new_state,i,istate);
      if (mod_state != istate) {
	if (0 && Global::Enable_Health_Records) {
	  fprintf(Global::HealthRecordfp,
		  "HEALTH RECORD: %s day %d person %d ENTERING CONDITION %s state %s MODIFIES COND %d STATE %d to state %d\n",
		  Date::get_date_string().c_str(), day,
		  person->get_id(),
		  this->condition->get_condition_name(),
		  this->natural_history->get_state_name(new_state).c_str(),
		  i, istate, mod_state);
	}
	Global::Conditions.get_condition(i)->get_epidemic()->update_state_of_person(person, day, mod_state);
      }
    }
  }

  if (this->natural_history->is_fatal(person, new_state)) {
    // update person's health record
    person->become_case_fatality(this->id, day);
  }

  // FRED_VERBOSE(0, "update_state_of_person %d day %d to state %d finished\n", person->get_id(), day, new_state);
}

