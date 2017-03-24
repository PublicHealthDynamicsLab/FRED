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
// File: Epidemic.h
//

#ifndef _FRED_EPIDEMIC_H
#define _FRED_EPIDEMIC_H

#include <set>
#include <map>
#include <vector>
using namespace std;

#include "Events.h"
#include "Person.h"
#include "Place.h"

#define SEED_USER 'U'
#define SEED_RANDOM 'R'
#define SEED_EXPOSED 'E'
#define SEED_INFECTIOUS 'I'

class Condition;
class Natural_History;

struct person_id_compare {
  bool operator()(const Person* x, const Person* y) const {
    return x->get_id() < y->get_id();
  }
};

struct place_id_compare {
  bool operator()(const Place* x, const Place* y) const {
    return x->get_id() < y->get_id();
  }
};

typedef  std::set<Person*, person_id_compare> person_set_t;
typedef  person_set_t::iterator person_set_iterator;

typedef  std::set<Place*, place_id_compare> place_set_t;
typedef  place_set_t::iterator place_set_iterator;


struct Import_Map {
  int sim_day_start;
  int sim_day_end;
  int max;
  double per_cap;
  double lat;
  double lon;
  double radius;
  long int fips;
  double min_age;
  double max_age;
  const std::string to_string() const {
    std::stringstream ss;
    ss << "Import Map ";
    ss << " sim_day_start " << sim_day_start;
    ss << " sim_day_end " << sim_day_end;
    ss << " max " << max;
    ss << " per_cap " << per_cap;
    ss << " lat " << lat;
    ss << " lon " << lon;
    ss << " radius " << radius;
    ss << " fips " << fips;
    ss << " min_age " << min_age;
    ss << " max_age " << max_age;
    ss << std::endl;
    return ss.str();
  }
};

struct Condition_Count_Info {
  int current_active;
  int current_symptomatic;
  int current_case_fatalities;
  int tot_ppl_evr_inf;
  int tot_ppl_evr_sympt;
  int tot_days_sympt;
  int tot_ppl_w_sl_evr_inf;
  int tot_ppl_w_sl_evr_sympt;
  int tot_chldrn_evr_inf;
  int tot_chldrn_evr_sympt;
  int tot_sch_age_chldrn_evr_inf;
  int tot_sch_age_chldrn_evr_sympt;
  int tot_sch_age_chldrn_w_home_adlt_crgvr_evr_inf;
  int tot_sch_age_chldrn_w_home_adlt_crgvr_evr_sympt;
  int tot_Cs_0_4;
  int tot_Cs_5_17;
  int tot_Cs_18_49;
  int tot_Cs_50_64;
  int tot_Cs_65_up;

  const std::string to_string() const {
    std::stringstream ss;
    ss << "Condition Count Info ";
    ss << " current_active " << current_active;
    ss << " current_symptomatic " << current_symptomatic;
    ss << " current_case_fatalities " << current_case_fatalities;
    ss << " tot_ppl_evr_inf " << tot_ppl_evr_inf;
    ss << " tot_ppl_evr_sympt " << tot_ppl_evr_sympt;
    ss << " tot_days_sympt " << tot_days_sympt;
    ss << " tot_ppl_w_sl_evr_inf " << tot_ppl_w_sl_evr_inf;
    ss << " tot_ppl_w_sl_evr_sympt " << tot_ppl_w_sl_evr_sympt;
    ss << " tot_chldrn_evr_inf " << tot_chldrn_evr_inf;
    ss << " tot_chldrn_evr_sympt " << tot_chldrn_evr_sympt;
    ss << " tot_sch_age_chldrn_evr_inf " << tot_sch_age_chldrn_evr_inf;
    ss << " tot_sch_age_chldrn_evr_sympt " << tot_sch_age_chldrn_evr_sympt;
    ss << " tot_sch_age_chldrn_w_home_adlt_crgvr_evr_inf " << tot_sch_age_chldrn_w_home_adlt_crgvr_evr_inf;
    ss << " tot_sch_age_chldrn_w_home_adlt_crgvr_evr_sympt " << tot_sch_age_chldrn_w_home_adlt_crgvr_evr_sympt;
    ss << " tot_Cs_0_4 " << tot_Cs_0_4;
    ss << " tot_Cs_5_17 " << tot_Cs_5_17;
    ss << " tot_Cs_18_49 " << tot_Cs_18_49;
    ss << " tot_Cs_50_64 " << tot_Cs_50_64;
    ss << " tot_Cs_65_up " << tot_Cs_65_up;
    ss << std::endl;
    return ss.str();
  }
};

class Epidemic {
public:

  /**
   * This static factory method is used to get an instance of a specific
   * Epidemic Model.  Depending on the model parameter, it will create a
   * specific Epidemic Model and return a pointer to it.
   *
   * @param a string containing the requested Epidemic model type
   * @return a pointer to a Epidemic model
   */
  static Epidemic* get_epidemic(Condition* condition);

  Epidemic(Condition* condition);
  virtual ~Epidemic();
 
  virtual void setup();
  virtual void prepare();
  void create_visualization_data_directories();
  void create_plot_data_directories();
  void report(int day);
  void print_stats(int day);
  void print_visualization_data_for_active_infections(int day);
  void print_visualization_data_for_case_fatality(int day, Person* person);
  void report_by_county(int day);
  void report_age_of_infection(int day);
  void report_distance_of_infection(int day);
  void report_transmission_by_age_group(int day);
  void report_transmission_by_age_group_to_file(int day);
  void report_incidence_by_county(int day);
  void report_incidence_by_census_tract(int day);
  void report_symptomatic_incidence_by_census_tract(int day);
  void report_place_of_infection(int day);
  void report_presenteeism(int day);
  void report_school_attack_rates_by_income_level(int day);
  void report_serial_interval(int day);
  void report_household_income_stratified_results(int day);
  void report_census_tract_stratified_results(int day);
  void report_group_quarters_incidence(int day);
  virtual void report_condition_specific_stats(int day) {}
  void read_time_step_map();
  void track_value(int day, char* key, int value);
  void track_value(int day, char* key, double value);
  void track_value(int day, char* key, string value);

  void get_imported_cases(int day);
  void become_exposed(Person* person, int day);
  void become_active(Person* person, int day);
  void become_infectious(Person* person, int day);
  void become_noninfectious(Person* person, int day);
  void become_symptomatic(Person* person, int day);
  void become_asymptomatic(Person* person, int day);
  void become_immune(Person* person, bool susceptible, bool infectious, bool symptomatic);
  void recover(Person* person, int day);

  virtual void update(int day);
  void update_state_of_person(Person* person, int day, int new_state = -1);

  void find_active_places_of_type(int day, int place_type);
  void spread_infection_in_active_places(int day);

  int get_susceptible_people() {
    return this->current_susceptible_people;
  }

  int get_exposed_people() {
    return this->current_exposed_people;
  }

  int get_infectious_people() {
    return this->current_infectious_people;
  }

  int get_removed_people() {
    return this->current_removed_people;
  }

  int get_immune_people() {
    return this->current_immune_people;
  }

  int get_symptomatic_people() {
    return this->current_symptomatic_people;
  }

  int get_people_becoming_active_today() {
    return this->people_becoming_active_today;
  }

  int get_people_becoming_symptomatic_today() {
    return this->people_becoming_symptomatic_today;
  }

  int get_people_becoming_case_fatality_today() {
    return this->people_becoming_case_fatality_today;
  }

  int get_total_case_fatality_count() {
    return this->total_case_fatality_count;
  }

  double get_RR() {
    return this->RR;
  }

  double get_symptomatic_prevalence() {
    return static_cast<double>(this->current_symptomatic_people) / static_cast<double>(this->N);
  }

  int get_symptomatic_incidence_by_tract_index(int index_);

  void increment_cohort_infectee_count(int cohort_day) {
    ++(this->number_infected_by_cohort[cohort_day]);
  }

  int get_id() {
    return this->id;
  }

  double get_attack_rate() {
    return (double) this->total_cases / (double) this->N;
  }

  int get_total_symptomatic_cases() {
    return this->total_symptomatic_cases;
  }

  int get_total_severe_symptomatic_cases() {
    return this->total_severe_symptomatic_cases;
  }

  double get_symptomatic_attack_rate() {
    return (double) this->total_symptomatic_cases / (double) this->N;
  }

  void delete_from_active_people_list(Person* person);

  virtual void end_of_run() {}
  virtual void terminate_person(Person* person, int day);

protected:
  Condition* condition;
  char name[FRED_STRING_SIZE];
  int id;
  int N;          // current population size
  int N_init;     // initial population size
  
  // boolean flags
  bool report_generation_time;
  bool report_transmission_by_age;

  // prevalence lists of people
  person_set_t active_people_list;
  person_set_t infectious_people_list;

  // daily list of people
  vector<Person*> daily_infections_list;
  vector<Person*> daily_symptomatic_list;

  // places attended today by infectious people:
  place_set_t active_places_list;

  // directory for visualization data
  char visualization_directory[FRED_STRING_SIZE];

  // sets used for visualization
  person_set_t new_active_people_list;
  person_set_t new_symptomatic_people_list;
  person_set_t new_infectious_people_list;
  person_set_t recovered_people_list;

  // imported cases
  std::vector<Import_Map*> import_map;

  static int get_age_group(int age);

  // population health state counters
  int current_susceptible_people;
  int current_active_people;
  int current_exposed_people;
  int current_infectious_people;
  int current_removed_people;
  int current_symptomatic_people;
  int current_immune_people;

  int people_becoming_active_today;
  int people_becoming_infectious_today;
  int people_becoming_symptomatic_today;
  int people_becoming_immune_today;
  int people_becoming_case_fatality_today;

  int total_cases;
  int total_symptomatic_cases;
  int total_severe_symptomatic_cases;
  int total_case_fatality_count;

  // epidemic counters for each state of the condition
  int* incidence_count;		// number of people entering state today
  int* cumulative_count;	       // number of people ever entering
  int* prevalence_count;	  // number of people currently in state

  bool* visualize_state;	  // if true, collect visualization data

  struct Condition_Count_Info population_infection_counts;

  //Values for household income based stratification
  std::map<int, struct Condition_Count_Info> household_income_infection_counts_map;
  std::map<int, struct Condition_Count_Info> census_tract_infection_counts_map;
  std::map<long int, struct Condition_Count_Info> county_infection_counts_map;

  //Values for school income based stratification
  std::map<int, struct Condition_Count_Info> school_income_infection_counts_map;
     
  //Values for workplace-size based stratification
  std::map<int, struct Condition_Count_Info> workplace_size_infection_counts_map;

  // used for computing reproductive rate:
  double RR;
  int* daily_cohort_size;
  int* number_infected_by_cohort;

  // serial interval
  double total_serial_interval;
  int total_secondary_cases;

  // used for incidence counts by county
  int counties;
  int* county_incidence;

  // used for incidence counts by census_tracts
  int census_tracts;
  int* census_tract_incidence;
  int* census_tract_symp_incidence;

  // link to natural hisotory
  Natural_History* natural_history;

  // event queue
  int number_of_states;
  Events state_transition_event_queue;

  char output_var_suffix[FRED_STRING_SIZE];
  int report_epi_stats;

  // report detailed changes in health records for this epidemic
  bool enable_health_records;

};

#endif // _FRED_EPIDEMIC_H
