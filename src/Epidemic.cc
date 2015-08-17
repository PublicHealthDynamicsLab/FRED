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
#include "Disease.h"
#include "Epidemic.h"
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
#include "Tracker.h"
#include "Transmission.h"
#include "Utils.h"
#include "Workplace.h"

Epidemic::Epidemic(Disease* dis) {
  this->disease = dis;
  this->id = disease->get_id();
  this->daily_cohort_size = new int[Global::Days];
  this->number_infected_by_cohort = new int[Global::Days];
  for(int i = 0; i < Global::Days; ++i) {
    this->daily_cohort_size[i] = 0;
    this->number_infected_by_cohort[i] = 0;
  }
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

  if(Global::Report_Mean_Household_Stats_Per_Income_Category) {
    //Values for household income based stratification
    for(int i = 0; i < Household_income_level_code::UNCLASSIFIED; ++i) {
      this->household_income_infection_counts_map[i].tot_ppl_evr_inf = 0;
      this->household_income_infection_counts_map[i].tot_ppl_evr_sympt = 0;
      this->household_income_infection_counts_map[i].tot_chldrn_evr_inf = 0;
      this->household_income_infection_counts_map[i].tot_chldrn_evr_sympt = 0;
    }
  }

  if(Global::Report_Epidemic_Data_By_Census_Tract) {
    //Values for household census_tract based stratification
    for(std::set<long int>::iterator census_tract_itr = Household::census_tract_set.begin();
	census_tract_itr != Household::census_tract_set.end(); ++census_tract_itr) {
      this->census_tract_infection_counts_map[*census_tract_itr].tot_ppl_evr_inf = 0;
      this->census_tract_infection_counts_map[*census_tract_itr].tot_ppl_evr_sympt = 0;
      this->census_tract_infection_counts_map[*census_tract_itr].tot_chldrn_evr_inf = 0;
      this->census_tract_infection_counts_map[*census_tract_itr].tot_chldrn_evr_sympt = 0;
    }
  }

  if(Global::Report_Childhood_Presenteeism) {
    //    //Values for household census_tract based stratification
    //    for(std::set<long int>::iterator census_tract_itr = Household::census_tract_set.begin();
    //          census_tract_itr != Household::census_tract_set.end(); ++census_tract_itr) {
    //      this->census_tract_infection_counts_map[*census_tract_itr].tot_ppl_evr_inf = 0;
    //      this->census_tract_infection_counts_map[*census_tract_itr].tot_ppl_evr_sympt = 0;
    //      this->census_tract_infection_counts_map[*census_tract_itr].tot_chldrn_evr_inf = 0;
    //      this->census_tract_infection_counts_map[*census_tract_itr].tot_chldrn_evr_sympt = 0;
    //    }
  }

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

  this->daily_infections_list.reserve(Global::Pop.get_pop_size());
  this->daily_infections_list.clear();

  this->case_fatality_incidence = 0;
  this->counties = 0;
  this->county_incidence = NULL;
  this->census_tract_incidence = NULL;
  this->census_tracts = 0;
  this->fraction_seeds_infectious = 0.0;

  this->imported_cases_map.clear();
  this->import_by_age = false;
  this->import_age_lower_bound = 0;
  this->import_age_upper_bound = Demographics::MAX_AGE;
  this->seeding_type = SEED_EXPOSED;

  this->infectious_start_event_queue = new Events<Epidemic>;
  this->infectious_end_event_queue = new Events<Epidemic>;
  this->symptoms_start_event_queue = new Events<Epidemic>;
  this->symptoms_end_event_queue = new Events<Epidemic>;
  this->immunity_start_event_queue = new Events<Epidemic>;
  this->immunity_end_event_queue = new Events<Epidemic>;

  this->infected_people.clear();
  this->potentially_infectious_people.clear();

  this->actually_infectious_people.reserve(Global::Pop.get_pop_size());
  this->actually_infectious_people.clear();
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

void Epidemic::infectious_start_event_handler(int day, Person* person) {
  FRED_VERBOSE(1,"infectious_start_event_handler day %d person %d\n",
	       day, person->get_id());

  // update next event list
  int infectious_end_date = person->get_infectious_end_date(this->id);
  infectious_end_event_queue->add_event(infectious_end_date, person);

  // add to active people list
  this->potentially_infectious_people.insert(person);

  // update epidemic counters
  this->exposed_people--;

  // update person's health chart
  person->become_infectious(this->disease);
}

void Epidemic::infectious_end_event_handler(int day, Person* person) {
  FRED_VERBOSE(1,"infectious_end_event_handler day %d person %d\n",
	       day, person->get_id());

  // remove from active list
  this->potentially_infectious_people.erase(person);

  // update epidemic counters
  if(person->is_dead()) {
    this->daily_case_fatality_count++;
    this->total_case_fatality_count++;
  }
  this->removed_people++;

  // update person's health chart
  person->recover(this->disease);
}


void Epidemic::symptoms_start_event_handler(int day, Person* person) {

  // update next event list
  int symptoms_end_date = person->get_symptoms_end_date(this->id);
  symptoms_end_event_queue->add_event(symptoms_end_date, person);

  // update epidemic counters
  this->people_with_current_symptoms++;
  this->people_becoming_symptomatic_today++;

  if(Global::Report_Mean_Household_Stats_Per_Income_Category) {
    if(person->get_household() != NULL) {
      int income_level = static_cast<Household*>(person->get_household())->get_household_income_code();
      if(income_level >= Household_income_level_code::CAT_I &&
         income_level < Household_income_level_code::UNCLASSIFIED) {
        this->household_income_infection_counts_map[income_level].tot_ppl_evr_sympt++;
      }
    }
  }

  if(Global::Report_Epidemic_Data_By_Census_Tract) {
    if(person->get_household() != NULL) {
      Household* hh = static_cast<Household*>(person->get_household());
      long int census_tract = Global::Places.get_census_tract_with_index(hh->get_census_tract_index());
      if(Household::census_tract_set.find(census_tract) != Household::census_tract_set.end()) {
        this->census_tract_infection_counts_map[census_tract].tot_ppl_evr_sympt++;
        if(person->is_child()) {
          this->census_tract_infection_counts_map[census_tract].tot_chldrn_evr_sympt++;
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
        this->school_income_infection_counts_map[income_quartile].tot_chldrn_evr_sympt++;
        this->school_income_infection_counts_map[income_quartile].tot_sch_age_chldrn_ever_sympt++;
      }

      if(hh->has_school_aged_child_and_unemployed_adult()) {
        this->school_income_infection_counts_map[income_quartile].tot_sch_age_chldrn_w_home_adlt_crgvr_evr_sympt++;
      }
    }
  }

  // update person's health chart
  // person->start_symptoms(this->disease);
}

void Epidemic::symptoms_end_event_handler(int day, Person* person) {

  // update epidemic counters
  this->people_with_current_symptoms--;

  // update person's health chart
  // person->end_symptoms(this->disease);
}

void Epidemic::immunity_start_event_handler(int day, Person* person) {

  // update epidemic counters
  this->immune_people++;

  // update person's health chart
  // person->become_immune(this->id);
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

void Epidemic::immunity_end_event_handler(int day, Person* person) {

  // update epidemic counters
  this->immune_people--;

  // update epidemic counters
  this->removed_people++;

  // update person's health chart
  person->become_susceptible(this->id);
}


void Epidemic::setup() {
  using namespace Utils;
  char paramstr[FRED_STRING_SIZE];
  char map_file_name[FRED_STRING_SIZE];
  int temp;

  // read time_step_map
  Params::get_param_from_string("primary_cases_file", map_file_name);
  // If this parameter is "none", then there is no map
  if(strncmp(map_file_name,"none",4)!=0){
    Utils::get_fred_file_name(map_file_name);
    ifstream* ts_input = new ifstream(map_file_name);
    if(!ts_input->is_open()) {
      Utils::fred_abort("Help!  Can't read %s Timestep Map\n", map_file_name);
      abort();
    }
    string line;
    while(getline(*ts_input,line)){
      if ( line[0] == '\n' || line[0] == '#' ) { // empty line or comment
	continue;
      }
      char cstr[FRED_STRING_SIZE];
      std::strcpy(cstr, line.c_str());
      Time_Step_Map * tmap = new Time_Step_Map;
      int n = sscanf(cstr,
		     "%d %d %d %d %lf %d %lf %lf %lf",
		     &tmap->sim_day_start, &tmap->sim_day_end, &tmap->num_seeding_attempts,
		     &tmap->disease_id, &tmap->seeding_attempt_prob, &tmap->min_num_successful,
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
        tmap->disease_id = 0;
      }
      this->imported_cases_map.push_back(tmap);
    }
    ts_input->close();
  }
  if (Global::Verbose > 1) {
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

Epidemic::~Epidemic() {
}

void Epidemic::become_exposed(Person* person, int day) {

  infected_people.insert(person);

  // update next event list
  int infectious_start_date = person->get_infectious_start_date(this->id);
  infectious_start_event_queue->add_event(infectious_start_date, person);

  int symptoms_start_date = person->get_symptoms_start_date(this->id);
  symptoms_start_event_queue->add_event(symptoms_start_date, person);

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

  if(Global::Report_Epidemic_Data_By_Census_Tract) {
    if(person->get_household() != NULL) {
      Household* hh = static_cast<Household*>(person->get_household());
      long int census_tract = Global::Places.get_census_tract_with_index(hh->get_census_tract_index());
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


void Epidemic::print_stats(int day) {
  FRED_VERBOSE(1, "epidemic print stats for disease %d day %d\n", id, day);

  // set population size, and remember original pop size
  if(day == 0) {
    this->N_init = this->N = Global::Pop.get_pop_size();
  } else {
    this->N = Global::Pop.get_pop_size();
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
  this->prevalence_count = this->exposed_people + this->infectious_people;
  this->prevalence = static_cast<double>(this->prevalence_count) / static_cast<double>(this->N);
  this->case_fatality_incidence = this->daily_case_fatality_count;
  double case_fatality_rate = 0.0;
  if(this->population_infection_counts.tot_ppl_evr_sympt > 0) {
    case_fatality_rate = 100.0 * static_cast<double>(this->total_case_fatality_count)
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
  if(this->disease->get_natural_history()->is_case_fatality_enabled()) {
    track_value(day, (char*)"D", this->daily_case_fatality_count);
    track_value(day, (char*)"CF", case_fatality_rate);
  }
  track_value(day, (char*)"M", this->immune_people);
  track_value(day, (char*)"P",this->prevalence_count);
  track_value(day, (char*)"C", this->incidence);
  track_value(day, (char*)"Cs", this->symptomatic_incidence);
  track_value(day, (char*)"AR", this->attack_rate);
  track_value(day, (char*)"ARs", this->symptomatic_attack_rate);
  track_value(day, (char*)"RR", this->RR);

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
    report_place_of_infection(day);
  }

  if(Global::Report_Age_Of_Infection) {
    report_age_of_infection(day);
  }

  if(Global::Report_Distance_Of_Infection) {
    report_distance_of_infection(day);
  }

  if(Global::Report_Incidence_By_County) {
    report_incidence_by_county(day);
  }

  if(Global::Report_Incidence_By_Census_Tract) {
    report_incidence_by_census_tract(day);
  }

  if(this->report_generation_time || Global::Report_Serial_Interval) {
    report_serial_interval(day);
  }
  
  //Only report AR and ARs on last day
  if(Global::Report_Mean_Household_Stats_Per_Income_Category && day == (Global::Days - 1)) {
    report_household_income_stratified_results(day);
  }

  if (Global::Enable_Household_Shelter) {
    Global::Places.report_shelter_stats(day);
  }

  //Only report AR and ARs on last day
  if(Global::Report_Epidemic_Data_By_Census_Tract && day == (Global::Days - 1)) {
    report_census_tract_stratified_results(day);
  }

  if (Global::Enable_Group_Quarters) {
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
    } else if (Global::Report_Age_Of_Infection == 3) {
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
      track_value(day,temp_str, age_count[i]);
      sprintf(temp_str, "Age%d", i);
      track_value(day, temp_str,
		  Global::Popsize_by_age[i] ?
		  (100000.0 * age_count[i] / static_cast<double>(Global::Popsize_by_age[i]))
		  : 0.0);
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
  for(int i = 0; i < this->people_becoming_infected_today; ++i) {
    Person* infectee = this->daily_infections_list[i];
    Household* hh = static_cast<Household*>(infectee->get_household());
    if(hh == NULL) {
      if(Global::Enable_Hospitals && infectee->is_hospitalized() && infectee->get_permanent_household() != NULL) {
        hh = static_cast<Household*>(infectee->get_permanent_household());;
      }
    }

    int c = hh->get_county_index();
    assert(0 <= c && c < this->counties);
    this->county_incidence[c]++;
  }
  for(int c = 0; c < this->counties; ++c) {
    char name[80];
    sprintf(name, "County_%d", Global::Places.get_fips_of_county_with_index(c));
    track_value(day,name, this->county_incidence[c]);
    sprintf(name, "N_%d", Global::Places.get_fips_of_county_with_index(c));
    track_value(day,name, Global::Places.get_population_of_county_with_index(c));
    // prepare for next day
    this->county_incidence[c] = 0;
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
    int t = hh->get_census_tract_index();
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
    Place* place = infectee->get_infected_place(id);
    if(place != NULL && place->is_group_quarters()) {
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
    Person * infectee = this->daily_infections_list[i];
    char c = infectee->get_infected_place_type(id);
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
    Person* infector = infectee->get_infector(id);
    if(infector == NULL) {
      continue;
    }
    Place* new_place = infectee->get_health()->get_infected_place(id);
    if(new_place == NULL) {
      continue;
    }
    Place* old_place = infector->get_health()->get_infected_place(id);
    if(old_place == NULL) {
      continue;
    }
    fred::geo lat1 = new_place->get_latitude();
    fred::geo lon1 = new_place->get_longitude();
    fred::geo lat2 = old_place->get_latitude();
    fred::geo lon2 = old_place->get_longitude();
    double dist = Geo::xy_distance(lat1, lon1, lat2, lon2);
    tot_dist += dist;
    n++;
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
  int presenteeism_small = 0;
  int presenteeism_med = 0;
  int presenteeism_large = 0;
  int presenteeism_xlarge = 0;
  int presenteeism_small_with_sl = 0;
  int presenteeism_med_with_sl = 0;
  int presenteeism_large_with_sl = 0;
  int presenteeism_xlarge_with_sl = 0;
  int infections_at_work = 0;

  // company size limits
  static int small;
  static int medium;
  static int large;
  if(day == 0) {
    small = Workplace::get_small_workplace_size();
    medium = Workplace::get_medium_workplace_size();
    large = Workplace::get_large_workplace_size();
  }

  for(int i = 0; i < this->people_becoming_infected_today; ++i) {
    Person* infectee = this->daily_infections_list[i];
    char c = infectee->get_infected_place_type(this->id);
    infections_in_pop++;

    // presenteeism requires that place of infection is work or office
    if(c != Place::WORKPLACE && c != Place::OFFICE) {
      continue;
    }
    infections_at_work++;

    // get the work place size (note we don't care about the office size)
    Place* work = infectee->get_workplace();
    assert(work != NULL);
    int size = work->get_size();

    // presenteeism requires that the infector have symptoms
    Person* infector = infectee->get_infector(this->id);
    assert(infector != NULL);
    if(infector->is_symptomatic()) {

      // determine whether sick leave was available to infector
      bool infector_has_sick_leave = infector->is_sick_leave_available();

      if(size < small) {			// small workplace
        presenteeism_small++;
        if(infector_has_sick_leave) {
          presenteeism_small_with_sl++;
        }
      } else if(size < medium) {		// medium workplace
        presenteeism_med++;
        if(infector_has_sick_leave) {
          presenteeism_med_with_sl++;
        }
      } else if(size < large) {		// large workplace
        presenteeism_large++;
        if(infector_has_sick_leave) {
          presenteeism_large_with_sl++;
        }
      } else {					// xlarge workplace
        presenteeism_xlarge++;
        if(infector_has_sick_leave) {
          presenteeism_xlarge_with_sl++;
        }
      }
    }
  } // end loop over infectees

  // raw counts
  int presenteeism = presenteeism_small + presenteeism_med + presenteeism_large
    + presenteeism_xlarge;
  int presenteeism_with_sl = presenteeism_small_with_sl + presenteeism_med_with_sl
    + presenteeism_large_with_sl + presenteeism_xlarge_with_sl;

  //Write to log file
  Utils::fred_log("\nDay %d PRESENTEE: ", day);
  Utils::fred_log(" small %d ", presenteeism_small);
  Utils::fred_log("small_n %d ", Workplace::get_workers_in_small_workplaces());
  Utils::fred_log("med %d ", presenteeism_med);
  Utils::fred_log("med_n %d ", Workplace::get_workers_in_medium_workplaces());
  Utils::fred_log("large %d ", presenteeism_large);
  Utils::fred_log("large_n %d ", Workplace::get_workers_in_large_workplaces());
  Utils::fred_log("xlarge %d ", presenteeism_xlarge);
  Utils::fred_log("xlarge_n %d ", Workplace::get_workers_in_xlarge_workplaces());
  Utils::fred_log("pres %d ", presenteeism);
  Utils::fred_log("pres_sl %d ", presenteeism_with_sl);
  Utils::fred_log("inf_at_work %d ", infections_at_work);
  Utils::fred_log("tot_emp %d ", Workplace::get_total_workers());
  Utils::fred_log("N %d\n", this->N);

  //Store for daily output file
  Global::Daily_Tracker->set_index_key_pair(day, "small", presenteeism_small);
  Global::Daily_Tracker->set_index_key_pair(day, "small_n", Workplace::get_workers_in_small_workplaces());
  Global::Daily_Tracker->set_index_key_pair(day, "med", presenteeism_med);
  Global::Daily_Tracker->set_index_key_pair(day, "med_n", Workplace::get_workers_in_medium_workplaces());
  Global::Daily_Tracker->set_index_key_pair(day, "large", presenteeism_large);
  Global::Daily_Tracker->set_index_key_pair(day, "large_n", Workplace::get_workers_in_large_workplaces());
  Global::Daily_Tracker->set_index_key_pair(day, "xlarge", presenteeism_xlarge);
  Global::Daily_Tracker->set_index_key_pair(day, "xlarge_n", Workplace::get_workers_in_xlarge_workplaces());
  Global::Daily_Tracker->set_index_key_pair(day, "pres", presenteeism);
  Global::Daily_Tracker->set_index_key_pair(day, "pres_sl", presenteeism_with_sl);
  Global::Daily_Tracker->set_index_key_pair(day, "inf_at_work", infections_at_work);
  Global::Daily_Tracker->set_index_key_pair(day, "tot_emp", Workplace::get_total_workers());
  Global::Daily_Tracker->set_index_key_pair(day, "N", this->N);
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
    char c = infectee->get_infected_place_type(this->id);

    // school presenteeism requires that place of infection is school or classroom
    if(c != Place::SCHOOL && c != Place::CLASSROOM) {
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

void Epidemic::get_imported_infections(int day) {
  this->N = Global::Pop.get_pop_size();

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
	FRED_VERBOSE(0, "IMPORT import by age %0.2f %0.2f\n",
		     this->import_age_lower_bound, this->import_age_upper_bound);
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
        FRED_VERBOSE(0, "IMPORT: seeking %d candidates, found %d\n",
		     imported_cases_remaining, (int) people.size());

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
	    FRED_VERBOSE(0, "infecting candidate %d id %d\n", n, infectee->get_id());
	    infectee->become_exposed(this->id, NULL, NULL, day);
	    FRED_VERBOSE(0, "exposed candidate %d id %d\n", n, infectee->get_id());
	    if(this->seeding_type != SEED_EXPOSED) {
	      advance_seed_infection(infectee);
	    }
	    become_exposed(infectee, day);
	    imported_cases++;
	  }
	  FRED_VERBOSE(0, "IMPORT SUCCESS: %d imported cases\n", imported_cases);
	  return; // success!
	} else {
	  // infect all the candidates
	  for(int n = 0; n < people.size(); ++n) {
	    Person* infectee = people[n];
	    infectee->become_exposed(this->id, NULL, NULL, day);
	    if(this->seeding_type != SEED_EXPOSED) {
	      advance_seed_infection(infectee);
	    }
	    become_exposed(infectee, day);
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
	  FRED_VERBOSE(0, "IMPORT FAILURE: only %d imported cases out of %d\n",
		       imported_cases, imported_cases_requested);
	  return;
	}
      } //End while(searches_within_given_location <= 10)
      // after 10 tries, return with a warning
      FRED_VERBOSE(0, "IMPORT FAILURE: only %d imported cases out of %d\n",
		   imported_cases, imported_cases_requested);
      return;
    }
  }
}

void Epidemic::advance_seed_infection(Person* person) {
  // if advanced_seeding is infectious or random
  int d = this->disease->get_id();
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


void Epidemic::update(int day) {

  FRED_VERBOSE(0, "epidemic update for disease %d day %d\n", id, day);
  Utils::fred_start_epidemic_timer();

  // import infections from unknown sources
  get_imported_infections(day);
  Utils::fred_print_epidemic_timer("imported infections");

  // transition to infectious
  FRED_VERBOSE(0, "INF_START_EVENT_QUEUE day %d size %d\n", day, this->infectious_start_event_queue->get_size(day));
  EpidemicMemFn func = &Epidemic::infectious_start_event_handler;
  this->infectious_start_event_queue->event_handler(day, this, func);

  // transition to noninfectious
  FRED_VERBOSE(0, "INF_END__EVENT_QUEUE day %d size %d\n", day, this->infectious_end_event_queue->get_size(day));
  func = &Epidemic::infectious_end_event_handler;
  this->infectious_end_event_queue->event_handler(day, this, func);
    
  // transition to symptomatic
  FRED_VERBOSE(0, "SYMP_START_EVENT_QUEUE day %d size %d\n", day, this->symptoms_start_event_queue->get_size(day));
  func = &Epidemic::symptoms_start_event_handler;
  this->symptoms_start_event_queue->event_handler(day, this, func);

  // transition to asymptomatic
  FRED_VERBOSE(0, "SYMP_END_EVENT_QUEUE day %d size %d\n", day, this->symptoms_end_event_queue->get_size(day));
  func = &Epidemic::symptoms_end_event_handler;
  this->symptoms_end_event_queue->event_handler(day, this, func);

  // transition to immune
  FRED_VERBOSE(0, "IMMUNITY_START_EVENT_QUEUE day %d size %d\n", day, this->immunity_start_event_queue->get_size(day));
  func = &Epidemic::immunity_start_event_handler;
  this->immunity_start_event_queue->event_handler(day, this, func);

  // transition to susceptible
  FRED_VERBOSE(0, "IMMUNITY_END_EVENT_QUEUE day %d size %d\n", day, this->immunity_end_event_queue->get_size(day));
  func = &Epidemic::immunity_end_event_handler;
  this->immunity_end_event_queue->event_handler(day, this, func);

  Utils::fred_print_epidemic_timer("transitions");

  // update list of infected people
  for (std::set<Person*>::iterator it = this->infected_people.begin(); it != this->infected_people.end(); ) {
    Person* person = (*it);
    FRED_VERBOSE(1, "update_infection for person %d\n", person->get_id());
    person->update_infection(day, this->id);
    if (person->is_infected(this->id) == false) {
      this->infected_people.erase(it++);
    }
    else {
      ++it;
    }
  }

  // get list of actually infectious people
  actually_infectious_people.clear();
  for (std::set<Person*>::iterator it = this->potentially_infectious_people.begin(); it != this->potentially_infectious_people.end(); ++it) {
    Person* person = (*it);
    if (person->is_infectious(this->id)) {
      actually_infectious_people.push_back(person);
      FRED_VERBOSE(1, "ACTUALLY INF person %d\n", person->get_id());
    }
  }
  this->infectious_people = actually_infectious_people.size();
  Utils::fred_print_epidemic_timer("identifying actually infections people");

  // update the daily activities of infectious people
  for (int i = 0; i < this->infectious_people; i++) {
    Person* person = actually_infectious_people[i];
    FRED_VERBOSE(1, "updating activities of infectious person %d -- %d out of %d\n", person->get_id(), i, this->infectious_people);
    // this will insert the infectious person onto the infectious list at each place attended
    person->update_activities_of_infectious_person(day);
  }
  Utils::fred_print_epidemic_timer("scheduled updated");

  // spread infection in places attended by actually infectious people
  for (int type = 0; type < 7; type++) {
    find_active_places_of_type(day, type);
    spread_infection_in_active_places(day);
    char msg[80];
    sprintf(msg, "spread_infection for type %d", type);
    Utils::fred_print_epidemic_timer(msg);
  }

  FRED_VERBOSE(0, "epidemic update finished for disease %d day %d\n", id, day);
  return;
}


void Epidemic::find_active_places_of_type(int day, int place_type) {

  FRED_VERBOSE(1, "find_active_places_of_type %d\n", place_type);
  active_places.clear();
  FRED_VERBOSE(1, "find_active_places_of_type %d actual %d\n", place_type, this->infectious_people);
  for (int i = 0; i < this->infectious_people; i++) {
    Person* person = actually_infectious_people[i];
    assert(person!=NULL);
    Place *place = NULL;
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
    if (place != NULL && person->is_present(day, place) && person->is_infectious(this->id)) {
      FRED_VERBOSE(1, "add_infection_person %d place %s\n", person->get_id(), place->get_label());
      place->add_infectious_person(this->id, person);
      active_places.insert(place);
    }
  }
  
  if (Global::Enable_Vector_Transmission) {
    // add all places that have any infectious vectors
    std::vector<Place*> * place_vec = NULL;
    if (place_type == 0) {
      place_vec = Global::Places.get_households();
    }
    if (place_type == 2) {
      place_vec = Global::Places.get_schools();
    }
    if (place_type == 4) {
      place_vec = Global::Places.get_workplaces();
    }
    if (place_vec != NULL) {
      int size = place_vec->size();
      for (int i = 0; i < size; i++) {
	Place * place = (*place_vec)[i];
	if (place->get_infectious_vectors(this->id) > 0) {
	  active_places.insert(place);
	}
      }
    }
  }


  FRED_VERBOSE(1, "find_active_places_of_type %d found %d\n", place_type, active_places.size());

  // convert active set to vector
  active_place_vec.clear();
  for (std::set<Place*>::iterator it = active_places.begin(); it != active_places.end(); ++it) {
    active_place_vec.push_back(*it);
  }
  FRED_VERBOSE(0, "find_active_places_of_type %d found %d\n", place_type, active_place_vec.size());

}
  
void Epidemic::spread_infection_in_active_places(int day) {
  for(int i = 0; i < this->active_place_vec.size(); ++i) {
    Place* place = this->active_place_vec[i];
    // place->spread_infection(day, this->id);
    Transmission::spread_infection(day, this->id, place);
    place->clear_infectious_people(this->id);
  }
  return;
}

