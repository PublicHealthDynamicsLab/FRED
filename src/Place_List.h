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
// File: Place_List.h
//
#ifndef _FRED_PLACE_LIST_H
#define _FRED_PLACE_LIST_H

#include <map>
#include <unordered_map>
#include <vector>
using namespace std;

#include "Census_Tract.h"
#include "Household.h"
#include "Neighborhood.h"
#include "Place.h"
#include "School.h"
#include "Workplace.h"

class Classroom;
class County;
class Office;

#define GRADES 20


class Place_List {

  typedef std::unordered_map<std::string, int> LabelMapT;
  typedef std::map<char, std::string> TypeNameMapT;
  typedef std::map<int, int> HospitalIDCountMapT;

public:

  /* Default Constructor */
  Place_List() {
    this->load_completed = false;
    this->is_primary_care_assignment_initialized = false;
    this->next_place_id = 0;
    init_place_type_name_lookup_map();
    this->place_label_map = new LabelMapT();
    this->places.clear();
    this->households.clear();
    this->neighborhoods.clear();
    this->schools.clear();
    this->workplaces.clear();
    this->hospitals.clear();
    for (int grade = 0; grade < GRADES; grade++) {
      schools_by_grade[grade].clear();
    }
  }

  ~Place_List();

  // initialization methods
  void get_parameters();
  void read_all_places(const std::vector<Utils::Tokens> & Demes);
  void read_places(const char* pop_dir, const char* pop_id, unsigned char deme_id);
  Place* add_place(char* label, char type, char subtype, fred::geo lon, fred::geo lat, long int census_tract);
  void quality_control();
  void reassign_workers();
  void prepare();
  void setup_group_quarters();
  void setup_households();
  void setup_classrooms();
  void setup_offices();
  void setup_counties();
  void setup_census_tracts();
  void setup_HAZEL_mobile_vans();
  void setup_household_childcare();
  void setup_school_income_quartile_pop_sizes();
  void setup_household_income_quartile_sick_days();
  int get_new_place_id() {
    int id = this->next_place_id;
    ++(this->next_place_id);
    return id;
  }
  void assign_hospitals_to_households();

  // reporting methods
  void report_school_distributions(int day);
  void report_household_distributions();
  void print_status_of_schools(int day);
  Place* get_place_from_label(const char* s) const;
  int get_housing_data(int* target_size, int* current_size);
  void get_initial_visualization_data_from_households();
  void get_visualization_data_from_households(int day, int condition_id, int output_code);
  void get_census_tract_data_from_households(int day, int condition_id, int output_code);
  void report_shelter_stats(int day);
  int get_number_of_demes() {
    return this->number_of_demes;
  }
  void print_household_size_distribution(char* dir, char* date_string, int run);

  // periodic updates
  void update(int day);
  void swap_houses(int house_index1, int house_index2);
  void swap_houses(Household* h1, Household* h2);
  void combine_households(int house_index1, int house_index2);
  Place* get_random_workplace();
  Place* get_random_school(int grade);

  /**
   * Uses a gravity model to find a random open hospital given the search parameters.
   * The location must allows overnight stays (have a subtype of NONE)
   * @param sim_day the simulation day
   * @param per the person we are trying to match (need the agent's household for distance and possibly need the agent's insurance)
   * @param check_insurance whether or not to use the agent's insurance in the matching
   */
  Hospital* get_random_open_hospital_matching_criteria(int sim_day, Person* per, bool check_insurance);

  /**
   * Uses a gravity model to find a random open healthcare location given the search parameters.
   * The search is ambivalent about the location allowing overnight stays.
   * @param sim_day the simulation day
   * @param per the person we are trying to match (need the agent's household for distance and possibly need the agent's insurance)
   * @param check_insurance whether or not to use the agent's insurance in the matching
   * @param use_search_radius_limit whether or not to cap the search radius
   */
  Hospital* get_random_open_healthcare_facility_matching_criteria(int sim_day, Person* per, bool check_insurance, bool use_search_radius_limit);

  /**
   * Uses a gravity model to find a random open healthcare location given the search parameters.
   * The search is ambivalent about the location allowing overnight stays, but it must be open on sim_day 0
   * @param per the person we are trying to match (need the agent's household for distance and possibly need the agent's insurance)
   * @param check_insurance whether or not to use the agent's insurance in the matching
   * @param use_search_radius_limit whether or not to cap the search radius
   */
  Hospital* get_random_primary_care_facility_matching_criteria(Person* per, bool check_insurance, bool use_search_radius_limit);
  void end_of_run();

  // access methods
  int get_min_household_income_by_percentile(int percentile);

  int get_number_of_counties() {
    return (int) this->counties.size();
  }

  County* get_county_with_index(int index) {
    assert(index < this->counties.size());
    return this->counties[index];
  }

  int get_fips_of_county_with_index(int index);
  int get_population_of_county_with_index(int index);
  int get_population_of_county_with_index(int index, int age);
  int get_population_of_county_with_index(int index, int age, char sex);
  int get_population_of_county_with_index(int index, int age_min, int age_max, char sex);
  void increment_population_of_county_with_index(int index, Person* person);
  void decrement_population_of_county_with_index(int index, Person* person);
  int get_index_of_county_with_fips(int fips) {
    int index = -1;
    try {
      index = this->fips_to_county_map.at(fips);
    }
    catch (const std::out_of_range& oor) {
      Utils::fred_abort("No county found for fips code %d\n", fips);
    }
    return index;
  }

  int get_index_of_census_tract_with_fips(long int fips) {
    int index = -1;
    try {
      index = this->fips_to_census_tract_map.at(fips);
    }
    catch (const std::out_of_range& oor) {
      Utils::fred_abort("No census_tract found for fips code %ld\n", fips);
    }
    return index;
  }

  County* get_county(int fips) {
    std::map<int,int>::iterator itr;
    itr = this->fips_to_county_map.find(fips);
    if (itr == this->fips_to_county_map.end()) {
      return NULL;
    } else {
      return this->counties[itr->second];
    }
  }

  Census_Tract* get_census_tract(long int fips) {
    std::map<long int,int>::iterator itr;
    itr = this->fips_to_census_tract_map.find(fips);
    if (itr == this->fips_to_census_tract_map.end()) {
      return NULL;
    } else {
      return this->census_tracts[itr->second];
    }
  }

  int get_population_of_county(int fips) {
    int index = get_index_of_county_with_fips(fips);
    return get_population_of_county_with_index(index);
  }

  int get_population_of_county(int fips, int age) {
    int index = get_index_of_county_with_fips(fips);
    return get_population_of_county_with_index(index, age);
  }

  int get_population_of_county(int fips, int age, char sex) {
    int index = get_index_of_county_with_fips(fips);
    return get_population_of_county_with_index(index, age, sex);
  }

  int get_population_of_county(int fips, int age_min, int age_max, char sex) {
    int index = get_index_of_county_with_fips(fips);
    return get_population_of_county_with_index(index, age_min, age_max, sex);
  }

  void increment_population_of_county(int fips, Person* person) {
    int index = get_index_of_county_with_fips(fips);
    increment_population_of_county_with_index(index, person);
  }

  void decrement_population_of_county(int fips, Person* person) {
    int index = get_index_of_county_with_fips(fips);
    decrement_population_of_county_with_index(index, person);
  }

  void report_county_populations();

  int get_number_of_census_tracts() {
    return (int)this->census_tracts.size();
  }

  long int get_census_tract_with_index(int index) {
    assert(index < this->census_tracts.size());
    return this->census_tracts[index]->get_fips();
  }

  long int get_census_tract_for_place(Place* place) {
    return place->get_census_tract_fips();
  }

  int get_county_for_place(Place* place) {
    return place->get_county_fips();
  }

  bool is_load_completed() {
    return this->load_completed;
  }

  void update_population_dynamics(int day);

  void delete_place_label_map();

  void print_stats(int day);

  static int get_HAZEL_disaster_start_sim_day();
  static int get_HAZEL_disaster_end_sim_day();
  static void increment_hospital_ID_current_assigned_size_map(int hospital_id) {
    Place_List::Hospital_ID_current_assigned_size_map.at(hospital_id)++;
  }

  // access function for places by type

  int get_number_of_households() {
    return (int)this->households.size();
  }

  int get_number_of_neighborhoods() {
    return (int)this->neighborhoods.size();
  }

  int get_number_of_schools() {
    return (int) this->schools.size();
  }
  
  int get_number_of_workplaces() {
    return (int)this->workplaces.size();
  }

  int get_number_of_hospitals() {
    return (int)this->hospitals.size();
  }

  Household* get_household(int i) {
    if(0 <= i && i < get_number_of_households()) {
      return static_cast<Household*>(this->households[i]);
    } else {
      return NULL;
    }
  }

  Neighborhood* get_neighborhood(int i) {
    if(0 <= i && i < get_number_of_neighborhoods()) {
      return static_cast<Neighborhood*>(this->neighborhoods[i]);
    } else {
      return NULL;
    }
  }

  School* get_school(int i) {
    if(0 <= i && i < get_number_of_schools()) {
      return static_cast<School*>(this->schools[i]);
    } else {
      return NULL;
    }
  }

  Workplace* get_workplace(int i) {
    if(0 <= i && i < get_number_of_workplaces()) {
      return static_cast<Workplace*>(this->workplaces[i]);
    } else {
      return NULL;
    }
  }

  Hospital* get_hospital(int i) {
    if(0 <= i && i < get_number_of_hospitals()) {
      return static_cast<Hospital*>(this->hospitals[i]);
    } else {
      return NULL;
    }
  }

private:

  // unique place id counter
  int next_place_id;

  // geo info
  fred::geo min_lat, max_lat, min_lon, max_lon;

  // map of place type names
  TypeNameMapT place_type_name_lookup_map;
  LabelMapT* place_label_map;

  // set when all input files have been processed
  bool load_completed;

  // number of population files to include
  int number_of_demes;

  // lists of places by type
  place_vector_t places;
  place_vector_t households;
  place_vector_t neighborhoods;
  place_vector_t schools;
  place_vector_t schools_by_grade[GRADES];
  place_vector_t workplaces;
  place_vector_t hospitals;

  void read_household_file(unsigned char deme_id, char* location_file);
  void read_workplace_file(unsigned char deme_id, char* location_file);
  void read_school_file(unsigned char deme_id, char* location_file);
  void read_hospital_file(unsigned char deme_id, char* location_file);
  void read_group_quarters_file(unsigned char deme_id, char* location_file);

  void reassign_workers_to_schools(char place_type, int fixed_staff, double staff_ratio);
  void reassign_workers_to_places_of_type(char place_type, int fixed_staff, double resident_to_staff_ratio);
  void reassign_workers_to_group_quarters(char subtype, int fixed_staff, double resident_to_staff_ratio);
  void prepare_primary_care_assignment();

  /**
   * @param hh a pointer to a Household object
   *
   * If there is already a Hospital assigned to a Household int the map household_hospital_map, then just return it.
   * Otherwise, find a suitable hospital (must allow overnight stays) and assign it to a household (put it in the map for later)
   *
   * @return a pointer to the Hospital that is assigned to the Household
   */
  Hospital* get_hospital_assigned_to_household(Household* hh);
  bool is_primary_care_assignment_initialized;

  // input files
  char MSA_file[FRED_STRING_SIZE];
  char Counties_file[FRED_STRING_SIZE];
  char States_file[FRED_STRING_SIZE];

  // list of counties
  std::vector<County*> counties;

  // map from fips code to county
  std::map<int,int> fips_to_county_map;

  // list of census_tracts
  // std::vector<long int> census_tracts;
  std::vector<Census_Tract*> census_tracts;

  // map from fips code to census tract
  std::map<long int,int> fips_to_census_tract_map;

  static bool Static_variables_set;

  // mean size of "household" associated with group quarters
  static double College_dorm_mean_size;
  static double Military_barracks_mean_size;
  static double Prison_cell_mean_size;
  static double Nursing_home_room_mean_size;

  // non-resident staff for group quarters
  static int College_fixed_staff;
  static double College_resident_to_staff_ratio;
  static int Prison_fixed_staff;
  static double Prison_resident_to_staff_ratio;
  static int Nursing_home_fixed_staff;
  static double Nursing_home_resident_to_staff_ratio;
  static int Military_fixed_staff;
  static double Military_resident_to_staff_ratio;

  // support for household shelter:
  static int Enable_copy_files;
  static int Shelter_duration_mean;
  static int Shelter_duration_std;
  static int Shelter_delay_mean;
  static int Shelter_delay_std;
  static double Pct_households_sheltering;
  static bool High_income_households_sheltering;
  static double Early_shelter_rate;
  static double Shelter_decay_rate;

  // Hospital support
  std::map<std::string, std::string>  hh_label_hosp_label_map;
  std::map<std::string, int>  hosp_label_hosp_id_map;

  static bool Household_hospital_map_file_exists;
  static int Hospital_fixed_staff;
  static double Hospital_worker_to_bed_ratio;
  static double Hospital_outpatients_per_day_per_employee;
  static double Healthcare_clinic_outpatients_per_day_per_employee;
  static int Hospital_min_bed_threshold;
  static double Hospitalization_radius;

  static HospitalIDCountMapT Hospital_ID_total_assigned_size_map;
  static HospitalIDCountMapT Hospital_ID_current_assigned_size_map;
  static int Hospital_overall_panel_size;
  
  // support for HAZEL project
  static int HAZEL_disaster_start_sim_day;
  static int HAZEL_disaster_end_sim_day;
  static int HAZEL_disaster_evac_start_offset;
  static int HAZEL_disaster_evac_end_offset;
  static int HAZEL_disaster_return_start_offset;
  static int HAZEL_disaster_return_end_offset;
  static double HAZEL_disaster_evac_prob_per_day;
  static double HAZEL_disaster_return_prob_per_day;
  static int HAZEL_mobile_van_max;

  // School support
  static int School_fixed_staff;
  static double School_student_teacher_ratio;

  // household income
  int min_household_income;
  int max_household_income;
  int median_household_income;
  int first_quartile_household_income;
  int third_quartile_household_income;

  // private methods

  void report_household_incomes();
  void select_households_for_shelter();
  void shelter_household(Household* h);
  void select_households_for_evacuation();
  void evacuate_household(Household* h);
  void update_geo_boundaries(fred::geo lat, fred::geo lon);

  void set_number_of_demes(int n) {
    this->number_of_demes = n;
  }

  void init_place_type_name_lookup_map();

  string lookup_place_type_name(char place_type) {
    assert(this->place_type_name_lookup_map.find(place_type) != this->place_type_name_lookup_map.end());
    return this->place_type_name_lookup_map[place_type];
  }

};

#endif // _FRED_PLACE_LIST_H
