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

using namespace std;

#include "Epidemic.h"
#include "Global.h"
#include "Disease.h"
#include "Random.h"
#include "Params.h"
#include "Person.h"
#include "Population.h"
#include "Infection.h"
#include "Place_List.h"
#include "Place.h"
#include "Timestep_Map.h"
#include "Multistrain_Timestep_Map.h"
#include "Transmission.h"
#include "Date.h"
#include "Neighborhood_Layer.h"
#include "Vector_Layer.h"
#include "Household.h"
#include "Utils.h"
#include "Geo_Utils.h"
#include "Seasonality.h"
#include "Evolution.h"
#include "Workplace.h"
#include "Tracker.h"

Epidemic::Epidemic(Disease *dis, Timestep_Map* _primary_cases_map) {
  this->disease = dis;
  this->id = disease->get_id();
  this->primary_cases_map = _primary_cases_map;
  this->primary_cases_map->print();
  this->daily_cohort_size = new int[Global::Days];
  this->number_infected_by_cohort = new int[Global::Days];
  for(int i = 0; i < Global::Days; i++) {
    this->daily_cohort_size[i] = 0;
    this->number_infected_by_cohort[i] = 0;
  }

  this->inf_households.reserve(Global::Places.get_number_of_places(Place::HOUSEHOLD));
  this->inf_neighborhoods.reserve(Global::Places.get_number_of_places(Place::NEIGHBORHOOD));
  this->inf_classrooms.reserve(Global::Places.get_number_of_places(Place::CLASSROOM));
  this->inf_schools.reserve(Global::Places.get_number_of_places(Place::SCHOOL));
  this->inf_workplaces.reserve(Global::Places.get_number_of_places(Place::WORKPLACE));
  this->inf_offices.reserve(Global::Places.get_number_of_places(Place::OFFICE));
  this->inf_hospitals.reserve(Global::Places.get_number_of_places(Place::HOSPITAL));

  this->inf_households.clear();
  this->inf_neighborhoods.clear();
  this->inf_classrooms.clear();
  this->inf_schools.clear();
  this->inf_workplaces.clear();
  this->inf_offices.clear();
  this->inf_hospitals.clear();

  this->susceptible_people = 0;
  this->exposed_people = 0;
  this->infectious_people = 0;
  this->removed_people = 0;
  this->immune_people = 0;
  this->vaccinated_people = 0;
  this->report_generation_time = false;

  this->people_becoming_infected_today = 0;
  this->people_becoming_symptomatic_today = 0;
  this->people_with_current_symptoms = 0;

  this->population_infection_counts.total_people_ever_infected = 0;
  this->population_infection_counts.total_people_ever_symptomatic = 0;

  if(Global::Report_Mean_Household_Stats_Per_Income_Category) {
    //Values for household income based stratification
    for(int i = 0; i < Household_income_level_code::UNCLASSIFIED; ++i) {
      this->household_income_infection_counts_map[i].total_people_ever_infected = 0;
      this->household_income_infection_counts_map[i].total_people_ever_symptomatic = 0;
      this->household_income_infection_counts_map[i].total_children_ever_infected = 0;
      this->household_income_infection_counts_map[i].total_children_ever_symptomatic = 0;
    }
  }

  if(Global::Report_Epidemic_Data_By_Census_Tract) {
    //Values for household census_tract based stratification
    for(std::set<long int>::iterator census_tract_itr = Household::census_tract_set.begin();
          census_tract_itr != Household::census_tract_set.end(); ++census_tract_itr) {
      this->census_tract_infection_counts_map[*census_tract_itr].total_people_ever_infected = 0;
      this->census_tract_infection_counts_map[*census_tract_itr].total_people_ever_symptomatic = 0;
      this->census_tract_infection_counts_map[*census_tract_itr].total_children_ever_infected = 0;
      this->census_tract_infection_counts_map[*census_tract_itr].total_children_ever_symptomatic = 0;
    }
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

  this->place_person_list_reserve_size = 1;
  this->daily_infections_list.clear();

  this->case_fatality_incidence = 0;
  this->counties = 0;
  this->county_incidence = NULL;
  this->census_tract_incidence = NULL;
  this->census_tracts = 0;
  this->fraction_seeds_infectious = 0.0;


  this->seeding_type = SEED_EXPOSED;
}

void Epidemic::setup() {
  using namespace Utils;
  int temp;
  Params::get_param_from_string("report_generation_time", &temp);
  this->report_generation_time = (temp > 0);
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
  delete this->primary_cases_map;
}

void Epidemic::become_susceptible(Person *person) {
  // operations on bloque (underlying container for population) are thread-safe
  Global::Pop.set_mask_by_index(fred::Susceptible, person->get_pop_index());
}

void Epidemic::become_unsusceptible(Person *person) {
  // operations on bloque (underlying container for population) are thread-safe
  if(Global::Pop.check_mask_by_index(fred::Susceptible, person->get_pop_index())) {
    Global::Pop.clear_mask_by_index(fred::Susceptible, person->get_pop_index());
  } else {
    if (Global::Enable_Vector_Transmission == false) {
      FRED_VERBOSE(0,
		   "WARNING: become_unsusceptible: person %d not removed from \
        susceptible_list\n",
		   person->get_id());
    }
  }
}

void Epidemic::become_exposed(Person * person) {
#pragma omp atomic
  this->people_becoming_infected_today++;
  if(Global::Report_Mean_Household_Stats_Per_Income_Category) {
    if(person->get_household() == NULL) {
      return;
    } else {
      int income_level = ((Household *)person->get_household())->get_household_income_code();
      if(income_level >= Household_income_level_code::CAT_I &&
         income_level < Household_income_level_code::UNCLASSIFIED) {
        this->household_income_infection_counts_map[income_level].total_people_ever_infected++;
      }
    }
  }
  if(Global::Report_Epidemic_Data_By_Census_Tract) {
    if(person->get_household() == NULL) {
      return;
    } else {
      long int census_tract = Global::Places.get_census_tract_with_index(((Household *)person->get_household())->get_census_tract_index());
      if(Household::census_tract_set.find(census_tract) != Household::census_tract_set.end()) {
        this->census_tract_infection_counts_map[census_tract].total_people_ever_infected++;
        if(person->get_age() < 18) {
          this->census_tract_infection_counts_map[census_tract].total_children_ever_infected++;
        }
      }
    }
  }
#pragma omp atomic
  this->exposed_people++;

  // TODO the daily infections list may end up containing defunct pointers if
  // enable_deaths is in effect (whether or not we are running in parallel mode).
  // Make daily reports and purge list after each report to fix this.
  fred::Spin_Lock lock(this->spin_mutex);
  this->daily_infections_list.push_back(person);
}

void Epidemic::become_infectious(Person *person) {
#pragma omp atomic
  this->exposed_people--;
  // operations on bloque (underlying container for population) are thread-safe
  Global::Pop.set_mask_by_index(fred::Infectious, person->get_pop_index());
}

void Epidemic::become_uninfectious(Person *person) {
  // operations on bloque (underlying container for population) are thread-safe
  if(Global::Pop.check_mask_by_index(fred::Infectious, person->get_pop_index())) {
    Global::Pop.clear_mask_by_index(fred::Infectious, person->get_pop_index());
  } else {
    if (Global::Enable_Vector_Transmission == false) {
      FRED_VERBOSE(0,
		   "WARNING: become_uninfectious: person %d not removed from \
        infectious_list\n",
		   person->get_id());
    }
  }
}

void Epidemic::become_symptomatic(Person *person) {
#pragma omp atomic
  this->people_with_current_symptoms++;
  if(Global::Report_Mean_Household_Stats_Per_Income_Category) {
    if(person->get_household() == NULL) {
      return;
    } else {
      int income_level = ((Household *)person->get_household())->get_household_income_code();
      if(income_level >= Household_income_level_code::CAT_I &&
         income_level < Household_income_level_code::UNCLASSIFIED) {
        this->household_income_infection_counts_map[income_level].total_people_ever_symptomatic++;
      }
    }
  }
  if(Global::Report_Epidemic_Data_By_Census_Tract) {
    if(person->get_household() == NULL) {
      return;
    } else {
      long int census_tract = Global::Places.get_census_tract_with_index(((Household *)person->get_household())->get_census_tract_index());
      if(Household::census_tract_set.find(census_tract) != Household::census_tract_set.end()) {
        this->census_tract_infection_counts_map[census_tract].total_people_ever_symptomatic++;
        if(person->get_age() < 18) {
          this->census_tract_infection_counts_map[census_tract].total_children_ever_symptomatic++;
        }
      }
    }
  }
#pragma omp atomic
  this->people_becoming_symptomatic_today++;
}

void Epidemic::become_removed(Person *person, bool susceptible, bool infectious, bool symptomatic) {
  // operations on bloque (underlying container for population) are thread-safe
  if(susceptible) {
    if(Global::Pop.check_mask_by_index(fred::Susceptible, person->get_pop_index())) {
      Global::Pop.clear_mask_by_index(fred::Susceptible, person->get_pop_index());
      FRED_VERBOSE(1, "OK: become_removed: person %d removed from \
          susceptible_list\n",
          person->get_id());
    } else {
      if (Global::Enable_Vector_Transmission == false) {
	FRED_VERBOSE(0,
		     "WARNING: become_removed: person %d not removed \
          from susceptible_list\n",
		     person->get_id());
      }
    }
  }
  if(infectious) {
    if(Global::Pop.check_mask_by_index(fred::Infectious, person->get_pop_index())) {
      Global::Pop.clear_mask_by_index(fred::Infectious, person->get_pop_index());
      FRED_VERBOSE(1, "OK: become_removed: person %d removed from \
          infectious_list\n",
          person->get_id());
    } else {
      if (Global::Enable_Vector_Transmission == false) {
	FRED_VERBOSE(0,
		     "WARNING: become_removed: person %d not removed from \
          infectious_list\n",
		     person->get_id());
      }
    }
  }
  if(symptomatic) {
#pragma omp atomic
    this->people_with_current_symptoms--;
  }
#pragma omp atomic
  this->removed_people++;

  if(person->is_dead()) {
#pragma omp atomic
    this->daily_case_fatality_count++;
#pragma omp atomic
    this->total_case_fatality_count++;
  }
}


void Epidemic::become_immune(Person *person, bool susceptible, bool infectious, bool symptomatic) {
  if(susceptible) {
    if(Global::Pop.check_mask_by_index(fred::Susceptible, person->get_pop_index())) {
      Global::Pop.clear_mask_by_index(fred::Susceptible, person->get_pop_index());
      FRED_VERBOSE(1, "OK: become_immune: person %d removed from \
          susceptible_list\n",
          person->get_id());
    } else {
      if (Global::Enable_Vector_Transmission == false) {
	FRED_VERBOSE(0,
		     "WARNING: become_immune: person %d not removed from \
          susceptible_list\n",
		     person->get_id());
      }
    }
  }
  if(infectious) {
    if(Global::Pop.check_mask_by_index(fred::Infectious, person->get_pop_index())) {
      Global::Pop.clear_mask_by_index(fred::Infectious, person->get_pop_index());
      FRED_VERBOSE(1, "OK: become_immune: person %d removed from \
          infectious_list\n",
          person->get_id());
    } else {
      if (Global::Enable_Vector_Transmission == false) {
	FRED_VERBOSE(0,
		     "WARNING: become_immune: person %d not removed from \
          infectious_list\n",
		     person->get_id());
      }
    }
  }
  if(symptomatic) {
#pragma omp atomic
    this->people_with_current_symptoms--;
  }
#pragma omp atomic
  this->immune_people++;

#pragma omp atomic
  this->removed_people++;
}

void Epidemic::print_stats(int day) {
  FRED_VERBOSE(1, "epidemic print stats for disease %d day %d\n", id, day);

  // set population size, and remember original pop size
  if(day == 0) {
    this->N_init = this->N = this->disease->get_population()->get_pop_size();
  } else {
    this->N = this->disease->get_population()->get_pop_size();
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
      this->RR = (double) this->number_infected_by_cohort[cohort_day] / (double) cohort_size;
    }
  }

  this->susceptible_people = Global::Pop.size(fred::Susceptible);
  this->infectious_people = Global::Pop.size(fred::Infectious);

  this->population_infection_counts.total_people_ever_infected += this->people_becoming_infected_today;
  this->population_infection_counts.total_people_ever_symptomatic += this->people_becoming_symptomatic_today;

  this->attack_rate = (100.0 * this->population_infection_counts.total_people_ever_infected) / (double) this->N_init;
  this->symptomatic_attack_rate = (100.0 * this->population_infection_counts.total_people_ever_symptomatic) / (double) this->N_init;

  // preserve these quantities for use during the next day
  this->incidence = this->people_becoming_infected_today;
  this->symptomatic_incidence = this->people_becoming_symptomatic_today;
  this->prevalence_count = this->exposed_people + this->infectious_people;
  this->prevalence = (double) this->prevalence_count / (double) this->N;
  this->case_fatality_incidence = this->daily_case_fatality_count;
  double case_fatality_rate = 0.0;
  if(this->population_infection_counts.total_people_ever_symptomatic > 0) {
    case_fatality_rate = 100.0 * (double) this->total_case_fatality_count
        / (double) this->population_infection_counts.total_people_ever_symptomatic;
  }

  if (this->id == 0) {
    Global::Daily_Tracker->set_index_key_pair(day,"Date", Global::Sim_Current_Date->get_YYYYMMDD());
    Global::Daily_Tracker->set_index_key_pair(day,"WkDay", Global::Sim_Current_Date->get_day_of_week_string());
    Global::Daily_Tracker->set_index_key_pair(day,"Year", Global::Sim_Current_Date->get_epi_week_year());
    Global::Daily_Tracker->set_index_key_pair(day,"Week", Global::Sim_Current_Date->get_epi_week());
    Global::Daily_Tracker->set_index_key_pair(day,"N", this->N);
  }

  track_value(day,(char *)"S", this->susceptible_people);
  track_value(day,(char *)"E", this->exposed_people);
  track_value(day,(char *)"I", this->infectious_people);
  track_value(day,(char *)"Is", this->people_with_current_symptoms);
  track_value(day,(char *)"R", this->removed_people);
  if(disease->is_case_fatality_enabled()) {
    track_value(day,(char *)"D", this->daily_case_fatality_count);
    track_value(day,(char *)"CF", case_fatality_rate);
  }
  track_value(day,(char *)"M", this->immune_people);
  track_value(day,(char *)"P",this->prevalence_count);
  track_value(day,(char *)"C", this->incidence);
  track_value(day,(char *)"Cs", this->symptomatic_incidence);
  track_value(day,(char *)"AR", this->attack_rate);
  track_value(day,(char *)"ARs", this->symptomatic_attack_rate);
  track_value(day,(char *)"RR", this->RR);

  /*
   if(Global::Enable_Seasonality) {
     double average_seasonality_multiplier = 1.0;
     average_seasonality_multiplier = Global::Clim->get_average_seasonality_multiplier(
         this->disease->get_id());
     fprintf(Global::Outfp, " SM %2.4f", average_seasonality_multiplier);
     FRED_STATUS(0, " SM %2.4f", average_seasonality_multiplier);
   }
 
   // Print Residual Immunuties
   // THIS NEEDS TO BE FIXED: DO NOT USE
   if(Global::Track_Residual_Immunity) {
     Evolution *evol = this->disease->get_evolution();
     for(int i = 0; i < this->disease->get_num_strains(); i++) {
       double res_immunity = 0.0;
       Population *pop = this->disease->get_population();
       for(int j = 0; j < pop->get_index_size(); j++) {
         if (pop->get_person_by_index(j) != NULL) {
           res_immunity = evol->residual_immunity(pop->get_person_by_index(j), i, day);
         }
       }
       res_immunity /= pop->get_pop_size();
       if(res_immunity != 0) {
         //fprintf(Global::Outfp, " ResM_%d %lf", i, res_immunity);
         fprintf(Global::Outfp, " S_%d %lf", i, 1.0 - res_immunity);
         //FRED_STATUS(0, " ResM_%d %lf", i, res_immunity);
         FRED_STATUS(0, " S_%d %lf", i, 1.0-res_immunity);
       }
     }
   }
  */

  if(Global::Report_Presenteeism) {
    report_presenteeism(day);
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
  int age_count[Demographics::MAX_AGE+1];				// age group counts
  double mean_age = 0.0;
  int count_infections = 0;
  for(int i = 0; i <= Demographics::MAX_AGE; i++) {
    age_count[i] = 0;
  }

  for(int i = 0; i < this->people_becoming_infected_today; i++) {
    Person * infectee = this->daily_infections_list[i];
    int age = infectee->get_age();
    mean_age += age;
    count_infections++;
    int age_group = age / 5;
    if(age_group > 20) {
      age_group = 20;
    }
    if (Global::Report_Age_Of_Infection > 2) {
      age_group = age;
      if (age_group > Demographics::MAX_AGE) { age_group = Demographics::MAX_AGE; }
    }

    age_count[age_group]++;
    double real_age = infectee->get_real_age();
    if (Global::Report_Age_Of_Infection == 1) {
      if (real_age < 0.5) infants++;
      else if (real_age < 2.0) toddlers++;
      else if (real_age < 6.0) pre_school++;
      else if (real_age < 12.0) elementary++;
      else if (real_age < 18.0) high_school++;
      else if (real_age < 21.0) young_adults++;
      else if (real_age < 65.0) adults++;
      else if (65 <= real_age) elderly++;
    }
    else if (Global::Report_Age_Of_Infection == 2) {
      if (real_age < 19.0/12.0) infants++;
      else if (real_age < 3.0) toddlers++;
      else if (real_age < 5.0) pre_school++;
      else if (real_age < 12.0) elementary++;
      else if (real_age < 18.0) high_school++;
      else if (real_age < 21.0) young_adults++;
      else if (real_age < 65.0) adults++;
      else if (65 <= real_age) elderly++;
    }
  }    
  if(count_infections > 0) {
    mean_age /= count_infections;
  }
  //Write to log file
  Utils::fred_log("\nday %d INF_AGE: ", day);
  Utils::fred_log("\nAge_at_infection %f", mean_age);

  //Store for daily output file
  track_value(day,(char *)"Age_at_infection", mean_age);

  if (Global::Report_Age_Of_Infection == 1) {
    track_value(day,(char *)"Infants", infants);
    track_value(day,(char *)"Toddlers", toddlers);
    track_value(day,(char *)"Preschool", pre_school);
    track_value(day,(char *)"Students", elementary+high_school);
    track_value(day,(char *)"Elementary", elementary);
    track_value(day,(char *)"Highschool", high_school);
    track_value(day,(char *)"Young_adults", young_adults);
    track_value(day,(char *)"Adults", adults);
    track_value(day,(char *)"Elderly", elderly);
  }
  if (Global::Report_Age_Of_Infection == 2) {
    track_value(day,(char *)"Infants", infants);
    track_value(day,(char *)"Toddlers", toddlers);
    track_value(day,(char *)"Pre-k", pre_school);
    track_value(day,(char *)"Elementary", elementary);
    track_value(day,(char *)"Highschool", high_school);
    track_value(day,(char *)"Young_adults", young_adults);
    track_value(day,(char *)"Adults", adults);
    track_value(day,(char *)"Elderly", elderly);
  }
  if (Global::Report_Age_Of_Infection == 3) {
    for(int i = 0; i <= Demographics::MAX_AGE; i++) {
      char temp_str[10];
      sprintf(temp_str, "A%d", i);
      track_value(day,temp_str, age_count[i]);
      sprintf(temp_str, "Age%d", i);
      track_value(day,temp_str, 
		  Global::Popsize_by_age[i] ?
		  (100000.0*age_count[i]/(double)Global::Popsize_by_age[i])
		  : 0.0);
    }
  }
  else {
    if(Global::Age_Of_Infection_Log_Level >= Global::LOG_LEVEL_LOW) {
      report_transmission_by_age_group(day);
    }
    if(Global::Age_Of_Infection_Log_Level >= Global::LOG_LEVEL_MED) {
      for(int i = 0; i <= 20; i++) {
	char temp_str[10];
	//Write to log file
	sprintf(temp_str, "A%d", i * 5);
	//Store for daily output file
	track_value(day,temp_str, age_count[i]);
	Utils::fred_log(" A%d_%d %d", i * 5, age_count[i], this->id);
      }
    }
  }
  Utils::fred_log("\n");
}

void Epidemic::report_serial_interval(int day) {

  for(int i = 0; i < this->people_becoming_infected_today; i++) {
    Person * infectee = this->daily_infections_list[i];
    Person * infector = infectee->get_infector(id);
    if(infector != NULL) {
      int serial_interval = infectee->get_exposure_date(this->id)
	- infector->get_exposure_date(this->id);
      this->total_serial_interval += (double) serial_interval;
      this->total_secondary_cases++;
    }
  }

  double mean_serial_interval = 0.0;
  if(this->total_secondary_cases > 0) {
    mean_serial_interval = this->total_serial_interval / (double) this->total_secondary_cases;
  }

  if(Global::Report_Serial_Interval) {
    //Write to log file
    Utils::fred_log("\nday %d SERIAL_INTERVAL:", day);
    Utils::fred_log("\n ser_int %.2f\n", mean_serial_interval);
    
    //Store for daily output file
    Global::Daily_Tracker->set_index_key_pair(day,"ser_int", mean_serial_interval);
  }

  track_value(day, (char *)"Tg", mean_serial_interval);
}

void Epidemic::report_transmission_by_age_group(int day) {
  FILE *fp;
  char file[1024];
  sprintf(file, "%s/AGE.%d", Global::Output_directory, day);
  fp = fopen(file, "w");
  if(fp == NULL) {
    Utils::fred_abort("Can't open file to report age transmission matrix\n");
  }
  int age_count[100][100];				// age group counts
  for(int i = 0; i < 100; i++)
    for(int j = 0; j < 100; j++)
      age_count[i][j] = 0;
  int group = 1;
  int groups = 100 / group;
  for(int i = 0; i < this->people_becoming_infected_today; i++) {
    Person * infectee = this->daily_infections_list[i];
    Person * infector = this->daily_infections_list[i]->get_infector(id);
    if(infector == NULL)
      continue;
    int a1 = infector->get_age();
    int a2 = infectee->get_age();
    if(a1 > 99)
      a1 = 99;
    if(a2 > 99)
      a2 = 99;
    a1 = a1 / group;
    a2 = a2 / group;
    age_count[a1][a2]++;
  }
  for(int i = 0; i < groups; i++) {
    for(int j = 0; j < groups; j++) {
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
    for(int i = 0; i < this->counties; i++) {
      this->county_incidence[i] = 0;
    }
  }
  for(int i = 0; i < this->people_becoming_infected_today; i++) {
    Person * infectee = this->daily_infections_list[i];
    Household * h = (Household *) infectee->get_household();
    int c = h->get_county();
    assert(0 <= c && c < this->counties);
    this->county_incidence[c]++;
  }
  for(int c = 0; c < this->counties; c++) {
    char name[80];
    sprintf(name, "County_%d", Global::Places.get_county_with_index(c));
    track_value(day,name, this->county_incidence[c]);
    sprintf(name, "N_%d", Global::Places.get_county_with_index(c));
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
    for(int i = 0; i < this->census_tracts; i++) {
      this->census_tract_incidence[i] = 0;
    }
  }
  for(int i = 0; i < this->people_becoming_infected_today; i++) {
    Person * infectee = this->daily_infections_list[i];
    Household * h = (Household *) infectee->get_household();
    int t = h->get_census_tract_index();
    assert(0 <= t && t < this->census_tracts);
    this->census_tract_incidence[t]++;
  }
  for(int t = 0; t < this->census_tracts; t++) {
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

  for(int i = 0; i < this->people_becoming_infected_today; i++) {
    Person * infectee = this->daily_infections_list[i];
    // record infections occurring in group quarters
    Place * place = infectee->get_infected_place(id);
    if (place != NULL && place->is_group_quarters()) {
      G++;
      if (place->is_college()) D++;
      if (place->is_prison()) J++;
      if (place->is_nursing_home()) L++;
      if (place->is_military_base()) B++;
    }
  }

  //Write to log file
  Utils::fred_log("\nDay %d GQ_INF: ", day);
  Utils::fred_log(" GQ %d College %d Prison %d Nursing_Home %d Military %d", G,D,J,L,B);
  Utils::fred_log("\n");

  //Store for daily output file
  track_value(day,(char *)"GQ", G);
  track_value(day,(char *)"College", D);
  track_value(day,(char *)"Prison", J);
  track_value(day,(char *)"Nursing_Home", L);
  track_value(day,(char *)"Military", B);
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
      case 'X': X++; break;
      case 'H': H++; break;
      case 'N': N++; break;
      case 'S': S++; break;
      case 'C': C++; break;
      case 'W': W++; break;
      case 'O': O++; break;
      case 'M': M++; break;
    }
  }

  //Write to log file
  Utils::fred_log("\nDay %d INF_PLACE: ", day);
  Utils::fred_log(" X %d H %d Nbr %d Sch %d", X, H, N, S);
  Utils::fred_log(" Cls %d Wrk %d Off %d Hosp %d", C, W, O, M);
  Utils::fred_log("\n");

  //Store for daily output file
  track_value(day,(char *)"X", X);
  track_value(day,(char *)"H", H);
  track_value(day,(char *)"Nbr", N);
  track_value(day,(char *)"Sch", S);
  track_value(day,(char *)"Cls", C);
  track_value(day,(char *)"Wrk", W);
  track_value(day,(char *)"Off", O);
  track_value(day,(char *)"Hosp", M);
}

void Epidemic::report_distance_of_infection(int day) {
  double tot_dist = 0.0;
  double ave_dist = 0.0;
  int n = 0;
  for(int i = 0; i < this->people_becoming_infected_today; i++) {
    Person * infectee = this->daily_infections_list[i];
    Person * infector = infectee->get_infector(id);
    if(infector == NULL)
      continue;
    Place * new_place = infectee->get_health()->get_infected_place(id);
    if(new_place == NULL)
      continue;
    Place * old_place = infector->get_health()->get_infected_place(id);
    if(old_place == NULL)
      continue;
    double lat1 = new_place->get_latitude();
    double lon1 = new_place->get_longitude();
    double lat2 = old_place->get_latitude();
    double lon2 = old_place->get_longitude();
    double dist = Geo_Utils::xy_distance(lat1, lon1, lat2, lon2);
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
  track_value(day,(char *)"Dist", 1000 * ave_dist);
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

  for(int i = 0; i < this->people_becoming_infected_today; i++) {
    Person * infectee = this->daily_infections_list[i];
    char c = infectee->get_infected_place_type(id);
    infections_in_pop++;

    // presenteeism requires that place of infection is work or office
    if(c != 'W' && c != 'O') {
      continue;
    }
    infections_at_work++;

    // get the work place size (note we don't care about the office size)
    Place * work = infectee->get_workplace();
    assert(work != NULL);
    int size = work->get_size();

    // presenteeism requires that the infector have symptoms
    Person * infector = infectee->get_infector(this->id);
    assert(infector != NULL);
    if(infector->is_symptomatic()) {

      // determine whether sick leave was available to infector
      bool infector_has_sick_leave = infector->is_sick_leave_available();

      if(size < small) {			// small workplace
        presenteeism_small++;
        if(infector_has_sick_leave)
          presenteeism_small_with_sl++;
      } else if(size < medium) {		// medium workplace
        presenteeism_med++;
        if(infector_has_sick_leave)
          presenteeism_med_with_sl++;
      } else if(size < large) {		// large workplace
        presenteeism_large++;
        if(infector_has_sick_leave)
          presenteeism_large_with_sl++;
      } else {					// xlarge workplace
        presenteeism_xlarge++;
        if(infector_has_sick_leave)
          presenteeism_xlarge_with_sl++;
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
  Global::Daily_Tracker->set_index_key_pair(day,"small", presenteeism_small);
  Global::Daily_Tracker->set_index_key_pair(day,"small_n", Workplace::get_workers_in_small_workplaces());
  Global::Daily_Tracker->set_index_key_pair(day,"med", presenteeism_med);
  Global::Daily_Tracker->set_index_key_pair(day,"med_n", Workplace::get_workers_in_medium_workplaces());
  Global::Daily_Tracker->set_index_key_pair(day,"large", presenteeism_large);
  Global::Daily_Tracker->set_index_key_pair(day,"large_n", Workplace::get_workers_in_large_workplaces());
  Global::Daily_Tracker->set_index_key_pair(day,"xlarge", presenteeism_xlarge);
  Global::Daily_Tracker->set_index_key_pair(day,"xlarge_n", Workplace::get_workers_in_xlarge_workplaces());
  Global::Daily_Tracker->set_index_key_pair(day,"pres", presenteeism);
  Global::Daily_Tracker->set_index_key_pair(day,"pres_sl", presenteeism_with_sl);
  Global::Daily_Tracker->set_index_key_pair(day,"inf_at_work", infections_at_work);
  Global::Daily_Tracker->set_index_key_pair(day,"tot_emp", Workplace::get_total_workers());
  Global::Daily_Tracker->set_index_key_pair(day,"N", this->N);
}

void Epidemic::report_household_income_stratified_results(int day) {

  for(int i = 0; i < Household_income_level_code::UNCLASSIFIED; ++i) {
    int temp_adult_count = 0;
    int temp_adult_inf_count = 0;
    int temp_adult_symp_count = 0;
    
    //AR
    if(Household::count_inhabitants_by_household_income_level_map[i] > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "AR", (100.0 * (double)this->household_income_infection_counts_map[i].total_people_ever_infected
                                                                    / (double)Household::count_inhabitants_by_household_income_level_map[i]));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "AR", (double)0.0);
    }
    
    //AR_under_18
    if(Household::count_children_by_household_income_level_map[i] > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "AR_under_18", (100.0 * (double)this->household_income_infection_counts_map[i].total_children_ever_infected
                                                                             / (double)Household::count_children_by_household_income_level_map[i]));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "AR_under_18", (double)0.0);
    }
    
    //AR_adult
    temp_adult_count = Household::count_inhabitants_by_household_income_level_map[i]
    - Household::count_children_by_household_income_level_map[i];
    temp_adult_inf_count = this->household_income_infection_counts_map[i].total_people_ever_infected
    - this->household_income_infection_counts_map[i].total_children_ever_infected;
    if(temp_adult_count > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "AR_adult", (100.0 * (double)temp_adult_inf_count / (double)temp_adult_count));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "AR_adult", (double)0.0);
    }
    
    //ARs
    if(Household::count_inhabitants_by_household_income_level_map[i] > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "ARs", (100.0 * (double)this->household_income_infection_counts_map[i].total_people_ever_symptomatic
                                                                     / (double)Household::count_inhabitants_by_household_income_level_map[i]));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "ARs", (double)0.0);
    }
    
    //ARs_under_18
    if(Household::count_children_by_household_income_level_map[i] > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "ARs_under_18", (100.0 * (double)this->household_income_infection_counts_map[i].total_children_ever_symptomatic
                                                                              / (double)Household::count_children_by_household_income_level_map[i]));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "ARs_under_18", (double)0.0);
    }
    
    //ARs_adult
    temp_adult_symp_count = this->household_income_infection_counts_map[i].total_people_ever_symptomatic
    - this->household_income_infection_counts_map[i].total_children_ever_symptomatic;
    if(temp_adult_count > 0) {
      Global::Income_Category_Tracker->set_index_key_pair(i, "ARs_adult", (100.0 * (double)temp_adult_symp_count / (double)temp_adult_count));
    } else {
      Global::Income_Category_Tracker->set_index_key_pair(i, "ARs_adult", (double)0.0);
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
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "AR", (100.0 * (double)this->census_tract_infection_counts_map[*census_tract_itr].total_people_ever_infected
                                                                          / (double)Household::count_inhabitants_by_census_tract_map[*census_tract_itr]));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "AR", (double)0.0);
    }
    
    //AR_under_18
    if(Household::count_children_by_census_tract_map[*census_tract_itr] > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "AR_under_18", (100.0 * (double)this->census_tract_infection_counts_map[*census_tract_itr].total_children_ever_infected
                                                                                   / (double)Household::count_children_by_census_tract_map[*census_tract_itr]));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "AR_under_18", (double)0.0);
    }
    
    //AR_adult
    temp_adult_count = Household::count_inhabitants_by_census_tract_map[*census_tract_itr]
    - Household::count_children_by_census_tract_map[*census_tract_itr];
    temp_adult_inf_count = this->census_tract_infection_counts_map[*census_tract_itr].total_people_ever_infected
    - this->census_tract_infection_counts_map[*census_tract_itr].total_children_ever_infected;
    if(temp_adult_count > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "AR_adult", (100.0 * (double)temp_adult_inf_count / (double)temp_adult_count));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "AR_adult", (double)0.0);
    }
    
    //Symptomatic AR
    if(Household::count_inhabitants_by_census_tract_map[*census_tract_itr] > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "ARs", (100.0 * (double)this->census_tract_infection_counts_map[*census_tract_itr].total_people_ever_symptomatic
                                                                           / (double)Household::count_inhabitants_by_census_tract_map[*census_tract_itr]));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "ARs", (double)0.0);
    }
    
    
    //ARs_under_18
    if(Household::count_children_by_census_tract_map[*census_tract_itr] > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "ARs_under_18", (100.0 * (double)this->census_tract_infection_counts_map[*census_tract_itr].total_children_ever_symptomatic
                                                                                    / (double)Household::count_children_by_census_tract_map[*census_tract_itr]));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "ARs_under_18", (double)0.0);
    }
    
    
    //ARs_adult
    temp_adult_symp_count = this->census_tract_infection_counts_map[*census_tract_itr].total_people_ever_symptomatic
                            - this->census_tract_infection_counts_map[*census_tract_itr].total_children_ever_symptomatic;
    if(temp_adult_count > 0) {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "ARs_adult", (100.0 * (double)temp_adult_symp_count / (double)temp_adult_count));
    } else {
      Global::Tract_Tracker->set_index_key_pair(*census_tract_itr, "ARs_adult", (double)0.0);
    }
  }
}

void Epidemic::add_infectious_place(Place *place) {
  if (place->is_household()) {
    fred::Spin_Lock lock(this->household_mutex);
    this->inf_households.push_back(place);
  } else if(place->is_neighborhood()) {
    fred::Spin_Lock lock(this->neighborhood_mutex);
    this->inf_neighborhoods.push_back(place);
  } else if(place->is_classroom()) {
    fred::Spin_Lock lock(this->classroom_mutex);
    this->inf_classrooms.push_back(place);
  } else if(place->is_school()) {
    fred::Spin_Lock lock(this->school_mutex);
    this->inf_schools.push_back(place);
  } else if(place->is_workplace()) {
    fred::Spin_Lock lock(this->workplace_mutex);
    this->inf_workplaces.push_back(place);
  } else if(place->is_office()) {
    fred::Spin_Lock lock(this->office_mutex);
    this->inf_offices.push_back(place);
  } else if(place->is_hospital()) {
    fred::Spin_Lock lock(this->hospital_mutex);
    this->inf_hospitals.push_back(place);
  }
}

void Epidemic::get_primary_infections(int day) {
  Population *pop = this->disease->get_population();
  this->N = pop->get_pop_size();

  Multistrain_Timestep_Map * ms_map = ((Multistrain_Timestep_Map *) this->primary_cases_map);

  for(Multistrain_Timestep_Map::iterator ms_map_itr = ms_map->begin(); ms_map_itr != ms_map->end();
      ms_map_itr++) {

    Multistrain_Timestep_Map::Multistrain_Timestep * mst = *ms_map_itr;

    if(mst->is_applicable(day, Global::Epidemic_offset)) {

      int extra_attempts = 1000;
      int successes = 0;

      vector<Person *> people;

      if(mst->has_location()) {
        vector<Place *> households = Global::Neighborhoods->get_households_by_distance(mst->get_lat(),
            mst->get_lon(), mst->get_radius());
        for(vector<Place *>::iterator hi = households.begin(); hi != households.end(); hi++) {
          vector<Person *> hs = ((Household *) (*hi))->get_inhabitants();
          // This cast is ugly and should be fixed.  
          // Problem is that Place::update (which clears susceptible list for the place) is called before Epidemic::update.
          // If households were stored as Household in the Patch (rather than the base Place class) this wouldn't be needed.
          people.insert(people.end(), hs.begin(), hs.end());
        }
      }

      for(int i = 0; i < mst->get_num_seeding_attempts(); i++) {
        int r, n;
        Person * person;

        // each seeding attempt is independent
        if(mst->get_seeding_prob() < 1) {
          if( RANDOM()> mst->get_seeding_prob() ) {continue;}
        }
        // if a location is specified in the timestep map select from that area, otherwise pick a person from the entire population
        if(mst->has_location()) {
          r = IRAND(0, people.size() - 1);
          person = people[r];
        } else if(Global::Seed_by_age) {
          person = pop->select_random_person_by_age(Global::Seed_age_lower_bound,
              Global::Seed_age_upper_bound);
        } else {
          person = pop->select_random_person();
        }

        if(person == NULL) { // nobody home
          FRED_WARNING("Person selected for seeding in Epidemic update is NULL.\n");
          continue;
        }

        if(person->get_health()->is_susceptible(this->id)) {
          Transmission transmission = Transmission(NULL, NULL, day);
          transmission.set_initial_loads(this->disease->get_primary_loads(day));
          person->become_exposed(this->disease, transmission);
          successes++;
          if(this->seeding_type != SEED_EXPOSED) {
            advance_seed_infection(person);
          }
        }

        if(successes < mst->get_min_successes() && i == (mst->get_num_seeding_attempts() - 1)
            && extra_attempts > 0) {
          extra_attempts--;
          i--;
        }
      }

      if(successes < mst->get_min_successes()) {
        FRED_WARNING(
            "A minimum of %d successes was specified, but only %d successful transmissions occurred.",
            mst->get_min_successes(), successes);
      }
    }
  }
}

void Epidemic::advance_seed_infection(Person * person) {
  // if advanced_seeding is infectious or random
  int d = this->disease->get_id();
  int advance = 0;
  int duration = person->get_recovered_date(d) - person->get_exposure_date(d);
  assert(duration > 0);
  if(this->seeding_type == SEED_RANDOM) {
    advance = IRAND(0, duration);
  } else if( RANDOM()< this->fraction_seeds_infectious ) {
    advance = person->get_infectious_date( d ) - person->get_exposure_date( d );
  }
  assert(advance <= duration);
  FRED_VERBOSE(0, "%s %s %s %d %s %d %s\n", "advanced_seeding:", seeding_type_name,
      "=> advance infection trajectory of duration", duration, "by", advance, "days");
  person->advance_seed_infection(d, advance);
}

void Epidemic::update(int day) {
  FRED_VERBOSE(1, "epidemic update for disease %d day %d\n", id, day);
  Population *pop = this->disease->get_population();

  // import infections from unknown sources
  get_primary_infections(day);

  int infectious_places;
  infectious_places = (int) this->inf_households.size();
  infectious_places += (int) this->inf_neighborhoods.size();
  infectious_places += (int) this->inf_schools.size();
  infectious_places += (int) this->inf_classrooms.size();
  infectious_places += (int) this->inf_workplaces.size();
  infectious_places += (int) this->inf_offices.size();
  infectious_places += (int) this->inf_hospitals.size();

  FRED_STATUS(0, "Number of infectious places        => %9d\n", infectious_places);
  FRED_STATUS(0, "Number of infectious households    => %9d\n", (int) this->inf_households.size());
  FRED_STATUS(0, "Number of infectious neighborhoods => %9d\n", (int) this->inf_neighborhoods.size());
  FRED_STATUS(0, "Number of infectious schools       => %9d\n", (int) this->inf_schools.size());
  FRED_STATUS(0, "Number of infectious classrooms    => %9d\n", (int) this->inf_classrooms.size());
  FRED_STATUS(0, "Number of infectious workplaces    => %9d\n", (int) this->inf_workplaces.size());
  FRED_STATUS(0, "Number of infectious offices       => %9d\n", (int) this->inf_offices.size());
  FRED_STATUS(0, "Number of infectious hospitals     => %9d\n", (int) this->inf_hospitals.size());

#pragma omp parallel
  {
    // schools (and classrooms)
#pragma omp for schedule(dynamic,10)
    for(int i = 0; i < this->inf_schools.size(); ++i) {
      this->inf_schools[i]->spread_infection(day, id);
    }
    Utils::fred_print_lap_time("day %d epidemic in schools for disease %d", day, id);
      
#pragma omp for schedule(dynamic,10)
    for(int i = 0; i < this->inf_classrooms.size(); ++i) {
      this->inf_classrooms[i]->spread_infection(day, id);
    }
    Utils::fred_print_lap_time("day %d epidemic in classrooms for disease %d", day, id);

    // workplaces (and offices)
#pragma omp for schedule(dynamic,10)
    for(int i = 0; i < this->inf_workplaces.size(); ++i) {
      this->inf_workplaces[i]->spread_infection(day, id);
    }
    Utils::fred_print_lap_time("day %d epidemic in workplaces for disease %d", day, id);

#pragma omp for schedule(dynamic,10)
    for(int i = 0; i < this->inf_offices.size(); ++i) {
      this->inf_offices[i]->spread_infection(day, id);
    }
    Utils::fred_print_lap_time("day %d epidemic in offices for disease %d", day, id);

    // neighborhoods (and households)
#pragma omp for schedule(dynamic,100)
    for(int i = 0; i < this->inf_neighborhoods.size(); ++i) {
      this->inf_neighborhoods[i]->spread_infection(day, id);
    }
    Utils::fred_print_lap_time("day %d epidemic in neighborhoods for disease %d", day, id);

#pragma omp for schedule(dynamic,100)
    for(int i = 0; i < this->inf_households.size(); ++i) {
      this->inf_households[i]->spread_infection(day, id);
    }
    Utils::fred_print_lap_time("day %d epidemic in households for disease %d", day, id);

    // hospitals
#pragma omp for schedule(dynamic,100)
    for(int i = 0; i < this->inf_hospitals.size(); ++i) {
      this->inf_hospitals[i]->spread_infection(day, id);
    }
    Utils::fred_print_lap_time("day %d epidemic in hospitals for disease %d", day, id);
  }

  this->inf_households.clear();
  this->inf_neighborhoods.clear();
  this->inf_classrooms.clear();
  this->inf_schools.clear();
  this->inf_workplaces.clear();
  this->inf_offices.clear();
  this->inf_hospitals.clear();
  this->disease->get_evolution()->update(day);
  FRED_VERBOSE(1, "epidemic update finished for disease %d day %d\n", id, day);
}

void Epidemic::infectious_sampler::operator()(Person & person) {
  if(RANDOM()< this->prob) {
#pragma omp critical(EPIDEMIC_INFECTIOUS_SAMPLER)
    this->samples->push_back( &person );
  }
}

void Epidemic::get_infectious_samples(vector<Person *> &samples, double prob = 1.0) {
  if(prob > 1.0) {
    prob = 1.0;
  }
  if(prob <= 0.0) {
    return;
  }
  samples.clear();
  infectious_sampler sampler;
  sampler.samples = &samples;
  sampler.prob = prob;
  Global::Pop.parallel_masked_apply(fred::Infectious, sampler);
}

void Epidemic::track_value(int day, char * key, int value) {
  char key_str[80];
  if (this->id == 0) {
    sprintf(key_str, "%s", key);
  }
  else {
    sprintf(key_str, "%s_%d", key, this->id);
  }
  Global::Daily_Tracker->set_index_key_pair(day, key_str, value);
}

void Epidemic::track_value(int day, char * key, double value) {
  char key_str[80];
  if (this->id == 0) {
    sprintf(key_str, "%s", key);
  }
  else {
    sprintf(key_str, "%s_%d", key, this->id);
  }
  Global::Daily_Tracker->set_index_key_pair(day, key_str, value);
}

void Epidemic::track_value(int day, char * key, string value) {
  char key_str[80];
  if (this->id == 0) {
    sprintf(key_str, "%s", key);
  }
  else {
    sprintf(key_str, "%s_%d", key, this->id);
  }
  Global::Daily_Tracker->set_index_key_pair(day, key_str, value);
}

