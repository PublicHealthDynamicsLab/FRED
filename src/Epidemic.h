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

#include "Global.h"

#define SEED_USER 'U'
#define SEED_RANDOM 'R'
#define SEED_EXPOSED 'E'
#define SEED_INFECTIOUS 'I'

using namespace std;

class Disease;
class Person;
class Place;

struct Time_Step_Map {
  int sim_day_start;
  int sim_day_end;
  int num_seeding_attempts;
  int disease_id;
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
    ss << " disease_id " << disease_id;
    ss << " seeding_attempt_prob " << seeding_attempt_prob;
    ss << " min_num_successful " << min_num_successful;
    ss << " lat " << lat;
    ss << " lon " << lon;
    ss << " radius " << radius;
    ss << std::endl;
    return ss.str();
  }
};


struct Disease_Count_Info {
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
    ss << "Disease Count Info ";
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
  Epidemic(Disease* disease);
  ~Epidemic();
 
  void setup();

  /**
   * Output daily Epidemic statistics to the files
   * @param day the simulation day
   */
  void print_stats(int day);

  void report_age_of_infection(int day);
  void report_distance_of_infection(int day);
  void report_transmission_by_age_group(int day);
  void report_transmission_by_age_group_to_file(int day);
  void report_incidence_by_county(int day);
  void report_incidence_by_census_tract(int day);
  void report_place_of_infection(int day);
  void report_presenteeism(int day);
  void report_school_attack_rates_by_income_level(int day);
  void report_serial_interval(int day);
  void report_household_income_stratified_results(int day);
  void report_census_tract_stratified_results(int day);
  void report_group_quarters_incidence(int day);
  void read_time_step_map();

  /**
   * Add an infectious place of a given type to this Epidemic's list
   * @param p the Place to be added
   */
  void add_infectious_place(Place* p);

  /**
   *
   */
  void get_infectious_places(int day);
  void get_imported_infections(int day);
  void become_susceptible(Person* person);
  void become_unsusceptible(Person* person);
  void become_exposed(Person* person, int day);
  void become_infectious(Person* person);
  void become_uninfectious(Person* person);
  void become_symptomatic(Person* person);
  void become_removed(Person* person, bool susceptible, bool infectious, bool symptomatic);
  void become_immune(Person* person, bool susceptible, bool infectious, bool symptomatic);

  void update_visited_places(int day, int disease_id);
  void find_infectious_places(int day, int disease_id);
  void add_susceptibles_to_infectious_places(int day, int disisease_id);

  void increment_cohort_infectee_count(int cohort_day) {
#pragma omp atomic
    ++(this->number_infected_by_cohort[cohort_day]);
  }

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

  int get_prevalence_count() {
    return this->prevalence_count;
  }

  double get_prevalence() {
    return this->prevalence;
  }

  int get_incident_infections() {
    return get_incidence();
  }

  void update(int day);

  void get_visitors_to_infectious_places(int day);

  void track_value(int day, char* key, int value);
  void track_value(int day, char* key, double value);
  void track_value(int day, char* key, string value);

private:
  Disease* disease;
  int id;
  int N;          // current population size
  int N_init;     // initial population size
  
  bool report_generation_time;
  bool report_transmission_by_age;

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

  static int get_age_group(int age);

  // lists of susceptible and infectious Persons now kept as
  // bit maskes in Population "Bloque"
  vector<Person*> daily_infections_list;

  vector<Place*> inf_households;
  vector<Place*> inf_neighborhoods;
  vector<Place*> inf_classrooms;
  vector<Place*> inf_schools;
  vector<Place*> inf_workplaces;
  vector<Place*> inf_offices;
  vector<Place*> inf_hospitals;

  // population health state counters
  int susceptible_people;
  int exposed_people;
  int infectious_people;
  int removed_people;
  int immune_people;
  int vaccinated_people;

  int people_becoming_infected_today;
  struct Disease_Count_Info population_infection_counts;

  //Values for household income based stratification
  std::map<int, struct Disease_Count_Info> household_income_infection_counts_map;
  std::map<int, struct Disease_Count_Info> census_tract_infection_counts_map;

  //Values for school income based stratification
  std::map<int, struct Disease_Count_Info> school_income_infection_counts_map;

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

  //fred::Mutex mutex;
  fred::Spin_Mutex neighborhood_mutex;
  fred::Spin_Mutex household_mutex;
  fred::Spin_Mutex workplace_mutex;
  fred::Spin_Mutex office_mutex;
  fred::Spin_Mutex school_mutex;
  fred::Spin_Mutex classroom_mutex;
  fred::Spin_Mutex hospital_mutex;

  fred::Spin_Mutex spin_mutex;

  size_t place_person_list_reserve_size;

};

#endif // _FRED_EPIDEMIC_H
