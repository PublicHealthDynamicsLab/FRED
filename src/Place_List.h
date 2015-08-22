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

#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>
#include "County.h"
#include "Health.h"
#include "Household.h"
#include "Place.h"
#include "Utils.h"

using namespace std;

#include "School.h"
class Neighborhood;
#include "Household.h"
class Office;
#include "Hospital.h"
class Classroom;
#include "Workplace.h"

#define GRADES 20

typedef std::vector<Place*> place_vec_t;
typedef std::unordered_map<std::string, int> LabelMapT;

// Helper class used during read_all_places/read_places; definition
// after Place_List class
class Place_Init_Data;

class Place_List {
  typedef std::set<Place_Init_Data> InitSetT;
  typedef std::pair<InitSetT::iterator, bool> SetInsertResultT;
  typedef std::map<char, int> TypeCountsMapT;
  typedef std::map<char, std::string> TypeNameMapT;
  typedef std::map<std::string, int> HouseholdHospitalIDMapT;
  typedef std::map<int, int> HospitalIDCountMapT;

public:

  /* Default Constructor */
  Place_List() {
    this->load_completed = false;
    this->is_primary_care_assignment_initialized = false;
    this->places.clear();
    this->schools.clear();
    this->workplaces.clear();
    this->next_place_id = 0;
    init_place_type_name_lookup_map();
    this->place_label_map = new LabelMapT();
  }

  ~Place_List();

  void read_all_places(const std::vector<Utils::Tokens> & Demes);
  void read_places(const char* pop_dir, const char* pop_id, unsigned char deme_id, InitSetT &pids);

  void reassign_workers();
  void prepare();
  void print_status_of_schools(int day);
  void update(int day);
  void quality_control();
  void report_school_distributions(int day);
  void report_household_distributions();
  void get_parameters();
  Place* get_place_at_position(int i) {
    return this->places[i];
  }

  int get_new_place_id() {
    int id = this->next_place_id;
    ++(this->next_place_id);
    return id;
  }

  void setup_group_quarters();
  void setup_households();
  void setup_classrooms();
  void setup_offices();
  void setup_HAZEL_mobile_vans();
  void setup_school_income_quartile_pop_sizes();
  void setup_household_income_quartile_sick_days();
  int get_min_household_income_by_percentile(int percentile);
  Place* get_place_from_label(const char* s) const;
  Place* get_random_workplace();
  void assign_hospitals_to_households();

  /**
   * Uses a gravity model to find a random open hospital given the search parameters.
   * The location must allows overnight stays (have a subtype of NONE)
   * @param sim_day the simulation day
   * @param per the person we are trying to match (need the agent's household for distance and possibly need the agent's insurance)
   * @param check_insurance whether or not to use the agent's insurance in the matching
   * @param use_search_radius_limit whether or not to cap the search radius
   */
  Hospital* get_random_open_hospital_matching_criteria(int sim_day, Person* per, bool check_insurance, bool use_search_radius_limit);

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
  void print_household_size_distribution(char* dir, char* date_string, int run);
  void report_shelter_stats(int day);
  void end_of_run();

  int get_number_of_demes() {
    return this->number_of_demes;
  }

  int get_housing_data(int* target_size, int* current_size);
  void get_initial_visualization_data_from_households();
  void get_visualization_data_from_households(int disease_id, int output_code);
  void get_census_tract_data_from_households(int disease_id, int output_code);
  void swap_houses(int house_index1, int house_index2);
  void combine_households(int house_index1, int house_index2);

  Place* select_school(int county_index, int grade);

  int get_number_of_counties() {
    return (int) this->counties.size();
  }

  County* get_county_with_index(int index) {
    assert(index < this->counties.size());
    return this->counties[index];
  }

  int get_fips_of_county_with_index(int index) {
    if(index < 0) {
      return 99999;
    }
    assert(index < this->counties.size());
    return this->counties[index]->get_fips();
  }

  int get_population_of_county_with_index(int index) {
    if(index < 0) {
      return 0;
    }
    assert(index < this->counties.size());
    return this->counties[index]->get_popsize();
  }

  void increment_population_of_county_with_index(int index) {
    if(index < 0) {
      return;
    }
    assert(index < this->counties.size());
    int fips = this->counties[index]->get_fips();
    this->counties[index]->increment_popsize();
    return;
  }

  void decrement_population_of_county_with_index(int index) {
    if(index < 0) {
      return;
    }
    assert(index < this->counties.size());
    this->counties[index]->decrement_popsize();
    return;
  }

  int get_number_of_census_tracts() {
    return (int)this->census_tracts.size();
  }

  long int get_census_tract_with_index(int index) {
    assert(index < this->census_tracts.size());
    return this->census_tracts[index];
  }

  void find_visitors_to_infectious_places(int day);

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

  int get_number_of_places() {
    return (int) this->places.size();
  }

  int get_number_of_places(char place_type);

  int get_number_of_households() {
    return (int) this->households.size();
  }

  Place* get_household(int i) {
    if(0 <= i && i < get_number_of_households()) {
      return this->households[i];
    } else {
      return NULL;
    }
  }
  
  int get_number_of_schools() {
    return (int) this->schools.size();
  }
  
  Place* get_school(int i) {
    if(0 <= i && i < get_number_of_schools()) {
      return this->schools[i];
    } else {
      return NULL;
    }
  }
  
  int get_number_of_workplaces() {
    return (int) this->workplaces.size();
  }

  Place* get_workplace(int i) {
    if(0 <= i && i < get_number_of_workplaces()) {
      return this->workplaces[i];
    } else {
      return NULL;
    }
  }

  int get_number_of_hospitals() {
    return (int) this->hospitals.size();
  }

  Place* get_hospital(int i) {
    if(0 <= i && i < get_number_of_hospitals()) {
      return this->hospitals[i];
    } else {
      return NULL;
    }
  }

  // access function for when we need a Household pointer
  Household* get_household_ptr(int i) {
    return static_cast<Household*>(get_household(i));
  }

  // access function for when we need a School pointer
  School* get_school_ptr(int i) {
    return static_cast<School*>(get_school(i));
  }

  // access function for when we need a Workplace pointer
  Workplace* get_workplace_ptr(int i) {
    return static_cast<Workplace*>(get_workplace(i));
  }

  // access function for when we need a Hospital pointer
  Hospital* get_hospital_ptr(int i) {
    return static_cast<Hospital*>(get_hospital(i));
  }

  place_vec_t * get_households() {
    return &(this->households);
  }

  place_vec_t * get_schools() {
    return &(this->schools);
  }

  place_vec_t * get_workplaces() {
    return &(this->workplaces);
  }

  place_vec_t * get_hospitals() {
    return &(this->hospitals);
  }

private:

  // lists of places by type
  place_vec_t places;
  place_vec_t households;
  place_vec_t schools;
  place_vec_t schools_by_grade[GRADES];
  place_vec_t workplaces;
  place_vec_t hospitals;

  void read_household_file(unsigned char deme_id, char* location_file, InitSetT &pids);
  void read_workplace_file(unsigned char deme_id, char* location_file, InitSetT &pids);
  void read_hospital_file(unsigned char deme_id, char* location_file, InitSetT &pids);
  void read_school_file(unsigned char deme_id, char* location_file, InitSetT &pids);
  void read_group_quarters_file(unsigned char deme_id, char* location_file, InitSetT &pids);
  void reassign_workers_to_places_of_type(char place_type, int fixed_staff, double resident_to_staff_ratio);
  void reassign_workers_to_group_quarters(fred::place_subtype subtype, int fixed_staff, double resident_to_staff_ratio);
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
  int number_of_demes;
  bool is_primary_care_assignment_initialized;

  // input files
  char MSA_file[FRED_STRING_SIZE];
  char Counties_file[FRED_STRING_SIZE];
  char States_file[FRED_STRING_SIZE];

  // list of counties
  std::vector<County*> counties;

  // list of census_tracts
  std::vector<long int> census_tracts;

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

  // the following support household shelter:
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

  bool load_completed;
  int min_household_income;
  int max_household_income;
  int median_household_income;
  int first_quartile_household_income;
  int third_quartile_household_income;
  void report_household_incomes();
  void select_households_for_shelter();
  void shelter_household(Household* h);
  void select_households_for_evacuation();
  void evacuate_household(Household* h);

  // For hospitalization
  HouseholdHospitalIDMapT household_hospital_map;

  void set_number_of_demes(int n) {
    this->number_of_demes = n;
  }

  fred::geo min_lat, max_lat, min_lon, max_lon;

  void parse_lines_from_stream(std::istream & stream, std::vector<Place_Init_Data> & pids);

  TypeNameMapT place_type_name_lookup_map;

  void init_place_type_name_lookup_map();

  string lookup_place_type_name(char place_type) {
    assert(this->place_type_name_lookup_map.find(place_type) != this->place_type_name_lookup_map.end());
    return this->place_type_name_lookup_map[place_type];
  }

  bool add_place(Place* p);

  template<typename Place_Type>
    void add_preallocated_places(char place_type, Place::Allocator<Place_Type> & pal) {
      // make sure that the expected number of places were allocated
      assert(pal.get_number_of_contiguous_blocks_allocated() == 1);
      assert(pal.get_number_of_remaining_allocations() == 0);

      int places_added = 0;
      Place_Type* place = pal.get_base_pointer();
      int places_allocated = pal.size();

      for(int i = 0; i < places_allocated; ++i) {
        if(add_place(place)) {
          ++(places_added);
        }
        ++(place);
      }
      FRED_STATUS(0, "Added %7d %16s places to Place_List\n", places_added,
          lookup_place_type_name( place_type ).c_str());
      FRED_CONDITIONAL_WARNING(places_added != places_allocated,
          "%7d %16s places were added to the Place_List, but %7d were allocated\n", places_added,
          lookup_place_type_name( place_type ).c_str(), places_allocated);
      // update/set place_type_counts for this place_type
      this->place_type_counts[place_type] = places_added;
    }

  // map to hold counts for each place type
  TypeCountsMapT place_type_counts;

  int next_place_id;

  LabelMapT* place_label_map;
};

struct Place_Init_Data {

  char s[80];
  char place_type;
  fred::place_subtype place_subtype;
  long int admin_id;
  int income;
  unsigned char deme_id;
  fred::geo lat, lon;
  bool is_group_quarters;
  int county;
  int census_tract_index;
  int num_workers_assigned;
  int group_quarters_units;
  char gq_type[8];
  char gq_workplace[32];

  void setup(char _s[], char _place_type, fred::place_subtype _place_subtype, const char* _lat, const char* _lon,
      unsigned char _deme_id, int _county, int _census_tract_index, const char* _income, bool _is_group_quarters,
      int _num_workers_assigned, int _group_quarters_units, const char* _gq_type, const char* _gq_workplace) {
    place_type = _place_type;
    place_subtype = _place_subtype;
    strcpy(s, _s);
    sscanf(_lat, "%f", &lat);
    sscanf(_lon, "%f", &lon);
    county = _county;
    census_tract_index = _census_tract_index;
    sscanf(_income, "%d", &income);

    if(!(lat >= -90 && lat <= 90) || !(lon >= -180 && lon <= 180)) {
      printf("BAD LAT-LON: type = %c lat = %f  lon = %f  inc = %d  s = %s\n", place_type, lat, lon, income, s);
      lat = 34.999999;
    }
    assert(lat >= -90 && lat <= 90);
    assert(lon >= -180 && lon <= 180);

    is_group_quarters = _is_group_quarters;
    num_workers_assigned = _num_workers_assigned;
    group_quarters_units = _group_quarters_units;
    strcpy(gq_type, _gq_type);
    strcpy(gq_workplace, _gq_workplace);
  }

  Place_Init_Data(char _s[], char _place_type, fred::place_subtype _place_subtype, const char* _lat, const char* _lon,
      unsigned char _deme_id, int _county = -1, int _census_tract = -1, const char* _income = "0",
      bool _is_group_quarters = false, int _num_workers_assigned = 0, int _group_quarters_units = 0,
      const char* gq_type = "X", const char* gq_workplace = "") {
    setup(_s, _place_type, _place_subtype, _lat, _lon, _deme_id, _county, _census_tract, _income, _is_group_quarters,
        _num_workers_assigned, _group_quarters_units, gq_type, gq_workplace);
  }

  bool operator<(const Place_Init_Data & other) const {

    if(place_type != other.place_type) {
      return place_type < other.place_type;
    } else if(strcmp(s, other.s) < 0) {
      return true;
    } else {
      return false;
    }
  }

  const std::string to_string() const {
    std::stringstream ss;
    ss << "Place Init Data ";
    ss << place_type << " ";
    ss << lat << " ";
    ss << lon << " ";
    ss << census_tract_index << " ";
    ss << s << " ";
    ss << int(deme_id) << " ";
    ss << num_workers_assigned << std::endl;
    return ss.str();
  }

};

#endif // _FRED_PLACE_LIST_H
