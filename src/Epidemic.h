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
// File: Epidemic.h
//

#ifndef _FRED_EPIDEMIC_H
#define _FRED_EPIDEMIC_H

#include <set>
#include <map>
#include <vector>
#include <set>

using namespace std;

#include "Global.h"

#define SEED_USER 'U'
#define SEED_RANDOM 'R'
#define SEED_EXPOSED 'E'
#define SEED_INFECTIOUS 'I'


class Condition;
class Events;
class Person;
class Place;

struct Time_Step_Map {
  int sim_day_start;
  int sim_day_end;
  int num_seeding_attempts;
  int condition_id;
  double seeding_attempt_prob;
  int min_num_successful;
  double lat;
  double lon;
  double radius;
  const std::string to_string() const {
    std::stringstream ss;
    ss << "Time Step Map ";
    ss << " sim_day_start " << sim_day_start;
    ss << " sim_day_end " << sim_day_end;
    ss << " num_seeding_attempts " << num_seeding_attempts;
    ss << " condition_id " << condition_id;
    ss << " seeding_attempt_prob " << seeding_attempt_prob;
    ss << " min_num_successful " << min_num_successful;
    ss << " lat " << lat;
    ss << " lon " << lon;
    ss << " radius " << radius;
    ss << std::endl;
    return ss.str();
  }
};

struct Condition_Count_Info {
  int tot_ppl_evr_inf;
  int tot_ppl_evr_sympt;
  int tot_chldrn_evr_inf;
  int tot_chldrn_evr_sympt;
  int tot_sch_age_chldrn_evr_inf;
  int tot_sch_age_chldrn_ever_sympt;
  int tot_sch_age_chldrn_w_home_adlt_crgvr_evr_inf;
  int tot_sch_age_chldrn_w_home_adlt_crgvr_evr_sympt;

  const std::string to_string() const {
    std::stringstream ss;
    ss << "Condition Count Info ";
    ss << " tot_ppl_evr_inf " << tot_ppl_evr_inf;
    ss << " tot_ppl_evr_sympt " << tot_ppl_evr_sympt;
    ss << " tot_chldrn_evr_inf " << tot_chldrn_evr_inf;
    ss << " tot_chldrn_evr_sympt " << tot_chldrn_evr_sympt;
    ss << " tot_sch_age_chldrn_evr_inf " << tot_sch_age_chldrn_evr_inf;
    ss << " tot_sch_age_chldrn_ever_sympt " << tot_sch_age_chldrn_ever_sympt;
    ss << " tot_sch_age_chldrn_w_home_adlt_crgvr_evr_inf " << tot_sch_age_chldrn_w_home_adlt_crgvr_evr_inf;
    ss << " tot_sch_age_chldrn_w_home_adlt_crgvr_evr_sympt " << tot_sch_age_chldrn_w_home_adlt_crgvr_evr_sympt;
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
  void report(int day);
  void print_stats(int day);
  void print_visualization_data_for_active_infections(int day);
  void print_visualization_data_for_case_fatality(int day, Person* person);
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
  void report_infections_by_workplace_size(int day);
  void report_serial_interval(int day);
  void report_household_income_stratified_results(int day);
  void report_census_tract_stratified_results(int day);
  void report_group_quarters_incidence(int day);
  virtual void report_condition_specific_stats(int day) {}
  void read_time_step_map();
  void track_value(int day, char* key, int value);
  void track_value(int day, char* key, double value);
  void track_value(int day, char* key, string value);

  virtual void get_imported_infections(int day);
  void become_exposed(Person* person, int day);

  virtual void update(int day);
  virtual void markov_updates(int day) {}

  void find_active_places_of_type(int day, int place_type);
  void spread_infection_in_active_places(int day);

  int get_susceptible_people() {
    return this->susceptible_people;
  }

  int get_exposed_people() {
    return this->exposed_people;
  }

  int get_infectious_people() {
    return this->infectious_people;
  }

  int get_removed_people() {
    return this->removed_people;
  }

  int get_immune_people() {
    return this->immune_people;
  }

  int get_people_becoming_infected_today() {
    return this->people_becoming_infected_today;
  }

  int get_total_people_ever_infected() {
    return this->population_infection_counts.tot_ppl_evr_inf;
  }

  int get_people_becoming_symptomatic_today() {
    return this->people_becoming_symptomatic_today;
  }

  int get_people_with_current_symptoms() {
    return this->people_with_current_symptoms;
  }

  int get_daily_case_fatality_count() {
    return this->daily_case_fatality_count;
  }

  int get_total_case_fatality_count() {
    return this->total_case_fatality_count;
  }

  double get_RR() {
    return this->RR;
  }

  double get_attack_rate() {
    return this->attack_rate;
  }

  double get_symptomatic_attack_rate() {
    return this->symptomatic_attack_rate;
  }

  double get_symptomatic_prevalence() {
    return static_cast<double>(this->people_with_current_symptoms) / static_cast<double>(this->N);
  }

  int get_incidence() {
    return this->incidence;
  }

  int get_symptomatic_incidence() {
    return this->symptomatic_incidence;
  }

  int get_symptomatic_incidence_by_tract_index(int index_);

  int get_prevalence_count() {
    return this->prevalence_count;
  }

  double get_prevalence() {
    return this->prevalence;
  }

  int get_incident_infections() {
    return get_incidence();
  }

  void increment_cohort_infectee_count(int cohort_day) {
    ++(this->number_infected_by_cohort[cohort_day]);
  }

  void become_immune(Person* person, bool susceptible, bool infectious, bool symptomatic);

  int get_id() {
    return this->id;
  }

  virtual void transition_person(Person* person, int day, int state) {}

  // events processing
  void process_infectious_start_events(int day);
  void process_infectious_end_events(int day);
  void recover(Person* person, int day);
  void process_symptoms_start_events(int day);
  void process_symptoms_end_events(int day);
  void process_immunity_start_events(int day);
  void process_immunity_end_events(int day);
  void cancel_symptoms_start(int day, Person* person);
  void cancel_symptoms_end(int day, Person* person);
  void cancel_infectious_start(int day, Person* person);
  void cancel_infectious_end(int day, Person* person);
  void cancel_immunity_start(int day, Person* person);
  void cancel_immunity_end(int day, Person* person);
  virtual void end_of_run() {}
  virtual void terminate_person(Person* person, int day);

protected:
  Condition* condition;
  int id;
  int N;          // current population size
  int N_init;     // initial population size
  
  bool causes_infection;
  bool report_generation_time;
  bool report_transmission_by_age;

  // event queues
  Events* infectious_start_event_queue;
  Events* infectious_end_event_queue;
  Events* symptoms_start_event_queue;
  Events* symptoms_end_event_queue;
  Events* immunity_start_event_queue;
  Events* immunity_end_event_queue;

  // active sets
  std::set<Person*> infected_people;
  std::set<Person*> potentially_infectious_people;
  std::vector<Person*> actually_infectious_people;
  std::set<Place*> active_places;
  std::vector<Place*> active_place_vec;

  // set used for visualization
  std::set<Person*> new_infected_people;
  std::set<Person*> new_symptomatic_people;
  std::set<Person*> new_infectious_people;
  std::set<Person*> recovered_people;

  // seeding imported cases
  std::vector<Time_Step_Map*> imported_cases_map;
  bool import_by_age;
  double import_age_lower_bound;
  double import_age_upper_bound;

  // valid seeding types are:
  // "user_specified" => SEED_USER 'U'
  // "random" => SEED_RANDOM 'R'
  // see Epidemic::advance_seed_infection"
  char seeding_type_name[FRED_STRING_SIZE];
  char seeding_type;
  double fraction_seeds_infectious; 

  /// advances infection either to the first infetious day (SEED_INFECTIOUS)
  /// or to a random day in the trajectory (SEED_RANDOM)
  /// this is accomplished by moving the exposure date back as appropriate;
  /// (ultimately done in Infection::advance_infection)
  void advance_seed_infection(Person* person);

  vector<Person*> daily_infections_list;
  vector<Person*> daily_symptomatic_list;

  static int get_age_group(int age);

  // population health state counters
  int susceptible_people;
  int exposed_people;
  int infectious_people;
  int removed_people;
  int immune_people;
  int vaccinated_people;

  int people_becoming_infected_today;
  struct Condition_Count_Info population_infection_counts;

  //Values for household income based stratification
  std::map<int, struct Condition_Count_Info> household_income_infection_counts_map;
  std::map<int, struct Condition_Count_Info> census_tract_infection_counts_map;

  //Values for school income based stratification
  std::map<int, struct Condition_Count_Info> school_income_infection_counts_map;

  int people_becoming_symptomatic_today;
  int people_with_current_symptoms;

  int daily_case_fatality_count;
  int total_case_fatality_count;

  // used for computing reproductive rate:
  double RR;
  int* daily_cohort_size;
  int* number_infected_by_cohort;

  // attack rates
  double attack_rate;
  double symptomatic_attack_rate;

  // serial interval
  double total_serial_interval;
  int total_secondary_cases;

  // used for maintining quantities from previous day;
  int incidence;
  int symptomatic_incidence;
  int prevalence_count;
  double prevalence;
  int case_fatality_incidence;

  // used for incidence counts by county
  int counties;
  int* county_incidence;

  // used for incidence counts by census_tracts
  int census_tracts;
  int* census_tract_incidence;
  int* census_tract_symp_incidence;

  // directory for visualization data
  char visualization_directory[FRED_STRING_SIZE];

  // location data for recovered people
  

};

#endif // _FRED_EPIDEMIC_H
