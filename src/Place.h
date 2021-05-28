/*
 * This file is part of the FRED system.
 *
 * Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette, Shawn Brown, 
 * Roni Rosenfield, Alona Fyshe, David Galloway, Nathan Stone, Jay DePasse, 
 * Anuroop Sriram, and Donald Burke
 * All rights reserved.
 *
 * Copyright (c) 2013-2019, University of Pittsburgh, John Grefenstette, Robert Frankeny,
 * David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
 * All rights reserved.
 *
 * FRED is distributed on the condition that users fully understand and agree to all terms of the 
 * End User License Agreement.
 *
 * FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.
 *
 * See the file "LICENSE" for more information.
 */

//
//
// File: Place.h
//

#ifndef _FRED_PLACE_H
#define _FRED_PLACE_H

#include <map>
#include <unordered_map>
#include <vector>
using namespace std;

#include "Geo.h"
#include "Global.h"
#include "Group.h"
#include "Place_Type.h"
#include "Census_Tract.h"

typedef std::unordered_map<std::string, int> LabelMapT;
typedef std::map<char, std::string> TypeNameMapT;
typedef std::map<int, int> HospitalIDCountMapT;

class Block_Group;
class Household;
class Hospital;
class Neighborhood_Patch;
class Person;
class School;

class Place : public Group {

 public:
  
  static char SUBTYPE_NONE;
  static char SUBTYPE_COLLEGE;
  static char SUBTYPE_PRISON;
  static char SUBTYPE_MILITARY_BASE;
  static char SUBTYPE_NURSING_HOME;
  static char SUBTYPE_HEALTHCARE_CLINIC;
  static char SUBTYPE_MOBILE_HEALTHCARE_CLINIC;

  /**
   * Constructor with default properties
   */
  Place(const char* lab = "", int _type_id = 0, fred::geo lon = 0.0, fred::geo lat = 0.0);

  virtual ~Place() {}

  virtual void print(int condition_id);

  void setup(int phase);
  virtual void prepare();
  void prepare_vaccination_rates();

  // test place types
  bool is_household() {
    return this->type_id == Place_Type::get_type_id("Household");
  }
  
  bool is_neighborhood() {
    return this->type_id == Place_Type::get_type_id("Neighborhood");
  }
  
  bool is_school() {
    return this->type_id == Place_Type::get_type_id("School");
  }
  
  bool is_classroom() {
    return this->type_id == Place_Type::get_type_id("Classroom");
  }
  
  bool is_workplace() {
    return this->type_id == Place_Type::get_type_id("Workplace");
  }
  
  bool is_office() {
    return this->type_id == Place_Type::get_type_id("Office");
  }
  
  bool is_hospital() {
    return this->type_id == Place_Type::get_type_id("Hospital");
  }
  
  // test place subtypes
  bool is_college() {
    return this->get_subtype() == Place::SUBTYPE_COLLEGE;
  }
  
  bool is_prison() {
    return this->get_subtype() == Place::SUBTYPE_PRISON;
  }
  
  bool is_nursing_home() {
    return this->get_subtype() == Place::SUBTYPE_NURSING_HOME;
  }
  
  bool is_military_base() {
    return this->get_subtype() == Place::SUBTYPE_MILITARY_BASE;
  }
    
  bool is_healthcare_clinic() {
    return this->get_subtype() == Place::SUBTYPE_HEALTHCARE_CLINIC;
  }

  bool is_mobile_healthcare_clinic() {
    return this->get_subtype() == Place::SUBTYPE_MOBILE_HEALTHCARE_CLINIC;
  }
    
  bool is_group_quarters() {
    return (is_college() || is_prison() || is_military_base() || is_nursing_home());
  }

  // test for household types
  bool is_college_dorm(){
    return is_household() && is_college();
  }
  
  bool is_prison_cell(){
    return is_household() && is_prison();
  }
  
  bool is_military_barracks() {
    return is_household() && is_military_base();
  }

  /**
   * Get the latitude.
   *
   * @return the latitude
   */
  fred::geo get_latitude() {
    return this->latitude;
  }

  /**
   * Get the longitude.
   *
   * @return the longitude
   */
  fred::geo get_longitude() {
    return this->longitude;
  }

  static double distance_between_places(Place* p1, Place* p2);

  double get_distance(Place* place) {
    double x1 = this->get_x();
    double y1 = this->get_y();
    double x2 = place->get_x();
    double y2 = place->get_y();
    double distance = sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
    return distance;
  }

  /**
   * Set the latitude.
   *
   * @property x the new latitude
   */
  void set_latitude(double x) {
    this->latitude = x;
  }

  /**
   * Set the longitude.
   *
   * @property x the new longitude
   */
  void set_longitude(double x) {
    this->longitude = x;
  }

  /**
   * Get the patch where this place is.
   *
   * @return a pointer to the patch where this place is
   */
  Neighborhood_Patch* get_patch() {
    return this->patch;
  }

  /**
   * Set the patch where this place will be.
   *
   * @property p the new patch
   */
  void set_patch(Neighborhood_Patch* p) {
    this->patch = p;
  }
  
  void turn_workers_into_teachers(Place* school);
  void reassign_workers(Place* place);

  double get_x() {
    return Geo::get_x(this->longitude);
  }

  double get_y() {
    return Geo::get_y(this->latitude);
  }

  static double get_mean_latitude() {
    return 0.5*(Place::min_lat + Place::max_lat);
  }

  static double get_mean_longitude() {
    return 0.5*(Place::min_lon + Place::max_lon);
  }

  static void update_elevations();

  void set_elevation(double elev);

  void set_partition_elevation(double elev);

  double get_elevation() {
    return this->elevation;
  }

  int get_staff_size() {
    return this->staff_size;
  }
  
  void set_staff_size(int _staff_size) {
    this->staff_size = _staff_size;
  }

  int get_state_admin_code();

  int get_county_admin_code();

  long long int get_census_tract_admin_code();

  long long int get_block_group_admin_code();

  Block_Group* get_block_group();

  long long int get_admin_code() {
    return this->admin_code;
  }

  void set_admin_code(long long int _admin_code) {
    this->admin_code = _admin_code;
  }

  int get_adi_state_rank();

  int get_adi_national_rank();

  double get_seeds(int dis, int sim_day);

  Place_Type* get_place_type() {
    return Place_Type::get_place_type(this->type_id);
  }

  double get_transmission_prob(int condition_id, Person* i, Person* s) {
    return 1.0;
  }


  void setup_partitions(int partition_type_id, int partition_capacity, string partition_basis, int min_age_partition, int max_age_partition);

  Place* select_partition(Person* person);

  int get_number_of_partitions() {
    return this->partitions.size();
  }

  int get_number_of_partitions_by_age(int age) {
    return this->partitions_by_age[age].size();
  }

  int get_size_by_age(int age) {
    int n = get_number_of_partitions_by_age(age);
    int size = 0;
    for(int i = 0; i < n; ++i) {
      size += partitions_by_age[age][i]->get_size();
    }
    return size;
  }

  int get_original_size_by_age(int age) {
    return this->original_size_by_age[age];
  }

  void set_container(Place* place) {
    this->container = place;
  }

  int get_container_size() {
    return this->container->get_size();
  }

  Place* get_partition(int i) {
    if(0 <= i && i < this->get_number_of_partitions()) {
      return this->partitions[i];
    } else {
      return NULL;
    }
  }

  // static methods

  static void get_place_properties();
  static char* get_place_label(Place* p);

  // initialization methods
  static void read_all_places();
  static void verify_pop_directory(const char* loc_id);
  static void read_places(const char* loc_id);
  static void read_gq_places(const char* loc_id);
  static void get_elevation_data();
  static Place* add_place(char* label, int place_type_id, char subtype, fred::geo lon, fred::geo lat, double elevation, long long int census_tract);
  static void save_place(Place* place) {
    Place::place_list.push_back(place);
  }
  static Place* get_place_from_sp_id(long long int n);
  static void quality_control();
  static void reassign_workers();
  static void prepare_places();
  static void get_elevation(Place* place);
  static void setup_group_quarters();
  static void setup_households();
  static void setup_partitions();
  static void setup_block_groups();
  static void setup_counties();
  static int get_new_place_id() {
    int id = Place::next_place_id;
    ++(Place::next_place_id);
    return id;
  }
  static void assign_hospitals_to_households();

  // reporting methods
  static void report_school_distributions(int day);
  static void report_household_distributions();
  static void print_status_of_schools(int day);
  static Place* get_household_from_label(const char* s);
  static Place* get_school_from_label(const char* s);
  static Place* get_workplace_from_label(const char* s);
  static int get_housing_data(int* target_size, int* current_size);
  static void print_household_size_distribution(char* dir, char* date_string, int run);

  // periodic updates
  static void update(int day);
  static void swap_houses(int house_index1, int house_index2);
  static void swap_houses(Household* h1, Household* h2);
  static void combine_households(int house_index1, int house_index2);
  static Place* get_random_household();
  static Place* get_random_workplace();
  static Place* get_random_school(int grade);

  /**
   * Uses a gravity model to find a random open hospital given the search properties.
   * The location must allows overnight stays (have a subtype of NONE)
   * @property sim_day the simulation day
   * @property per the person we are trying to match (need the agent's household for distance and possibly need the agent's insurance)
   * @property check_insurance whether or not to use the agent's insurance in the matching
   */
  static Hospital* get_random_open_hospital_matching_criteria(int sim_day, Person* per, bool check_insurance);

  /**
   * Uses a gravity model to find a random open healthcare location given the search properties.
   * The search is ambivalent about the location allowing overnight stays.
   * @property sim_day the simulation day
   * @property per the person we are trying to match (need the agent's household for distance and possibly need the agent's insurance)
   * @property check_insurance whether or not to use the agent's insurance in the matching
   * @property use_search_radius_limit whether or not to cap the search radius
   */
  static Hospital* get_random_open_healthcare_facility_matching_criteria(int sim_day, Person* per, bool check_insurance, bool use_search_radius_limit);

  /**
   * Uses a gravity model to find a random open healthcare location given the search properties.
   * The search is ambivalent about the location allowing overnight stays, but it must be open on sim_day 0
   * @property per the person we are trying to match (need the agent's household for distance and possibly need the agent's insurance)
   * @property check_insurance whether or not to use the agent's insurance in the matching
   * @property use_search_radius_limit whether or not to cap the search radius
   */
  static Hospital* get_random_primary_care_facility_matching_criteria(Person* per, bool check_insurance, bool use_search_radius_limit);
  static void finish();

  // access methods

  static int get_county_for_place(Place* place) {
    return place->get_county_admin_code();
  }

  static bool is_load_completed() {
    return Place::load_completed;
  }

  static void update_population_dynamics(int day);

  static void delete_place_label_map();

  static void print_stats(int day);

  static void increment_hospital_ID_current_assigned_size_map(int hospital_id) {
    Place::Hospital_ID_current_assigned_size_map.at(hospital_id)++;
  }

  // access function for places by type

  static int get_number_of_households() {
    return Place_Type::get_household_place_type()->get_number_of_places();
  }

  static int get_number_of_neighborhoods() {
    return Place_Type::get_neighborhood_place_type()->get_number_of_places();
  }

  static int get_number_of_schools() {
    return Place_Type::get_school_place_type()->get_number_of_places();
  }
  
  static int get_number_of_workplaces() {
    return Place_Type::get_workplace_place_type()->get_number_of_places();
  }

  static int get_number_of_hospitals() {
    return Place_Type::get_hospital_place_type()->get_number_of_places();
  }

  static Household* get_household(int i);
  static Place* get_neighborhood(int i);
  static Place* get_school(int i);
  static Place* get_workplace(int i);
  static Hospital* get_hospital(int i);

  static int get_number_of_location_ids() {
    return location_id.size();
  }

  static void get_population_directory(char* pop_dir, int i) {
    assert (0 <= i && i < Place::location_id.size());
    sprintf(pop_dir, "%s/%s/%s/%s",
        Place::Population_directory,
        Place::Country,
        Place::Population_version,
        Place::get_location_id(i));
  }

  static void get_country_directory(char* dir) {
    sprintf(dir, "%s/%s",
        Place::Population_directory,
        Place::Country);
  }

  static const char * get_location_id(int i) {
    if(0 <= i && i < Place::location_id.size()) {
      return Place::location_id[i].c_str();
    } else {
      return NULL;
    }
  }

  static bool is_country_usa() {
    return Place::country_is_usa;
  }

  static bool is_country_colombia() {
    return Place::country_is_colombia;
  }

  static bool is_country_india() {
    return Place::country_is_india;
  }

  static int get_number_of_state_admin_code() {
    return Place::state_admin_code.size();
  }

  static int get_state_admin_code_with_index(int n) {
    return Place::state_admin_code[n];;
  }

  static place_vector_t get_candidate_places(Place* target, int type_id);
  static void read_place_file(char* location_file, int type);
  static Hospital* get_hospital_assigned_to_household(Household* hh);
  static void read_household_file(char* location_file);
  static void read_workplace_file(char* location_file);
  static void read_school_file(char* location_file);
  static void read_hospital_file(char* location_file);
  static void read_group_quarters_file(char* location_file);
  static void update_household_file(char* location_file);
  static void update_school_file(char* location_file);
  static void update_workplace_file(char* location_file);
  static void update_hospital_file(char* location_file);
  static void update_gq_file(char* location_file);
  static void reassign_workers_to_schools();
  static void reassign_workers_to_hospitals();
  static void reassign_workers_to_group_quarters(char subtype, int fixed_staff, double resident_to_staff_ratio);
  static void prepare_primary_care_assignment();
  static void select_households_for_evacuation();
  static void evacuate_household(Household* h);
  static void update_geo_boundaries(fred::geo lat, fred::geo lon);
  static void init_place_type_name_lookup_map();

  bool is_open(int day);
  bool has_admin_closure();

  void set_vaccination_rate(double rate) {
    this->vaccination_rate = rate;
  }
  double get_vaccination_rate() {
    return this->vaccination_rate;
  }
  bool is_low_vaccination_place();

 protected:
  fred::geo latitude;				// geo location
  fred::geo longitude;				// geo location
  double elevation;				// elevation (in meters)
  long long int admin_code;			       // block group admin code

  int staff_size;			// outside workers in this place

  Neighborhood_Patch* patch;		     // geo patch for this place

  place_vector_t partitions;
  place_vector_t* partitions_by_age;
  int next_partition;
  int* original_size_by_age;
  Place* container;

  // STATIC VARIABLES

  // unique place id counter
  static int next_place_id;

  // list of all places
  static place_vector_t place_list;

  // geo info
  static fred::geo min_lat, max_lat, min_lon, max_lon;
  static bool country_is_usa;
  static bool country_is_colombia;
  static bool country_is_india;

  // map of place type names
  static TypeNameMapT place_type_name_lookup_map;
  static LabelMapT* household_label_map;
  static LabelMapT* school_label_map;
  static LabelMapT* workplace_label_map;

  // set when all input files have been processed
  static bool load_completed;

  // lists of places by type
  static place_vector_t schools_by_grade[Global::GRADES];
  static place_vector_t gq;

  static bool is_primary_care_assignment_initialized;

  // input files
  static char MSA_file[FRED_STRING_SIZE];
  static char Counties_file[FRED_STRING_SIZE];
  static char States_file[FRED_STRING_SIZE];

  // list of state location codes
  static std::vector<int> state_admin_code;

  // update elevations of all places
  static bool Update_elevation;

  // population properties
  static char Population_directory[];
  static char Country[];
  static char Population_version[];
  static vector<string> location_id;

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

  // Hospital support
  static std::map<std::string, std::string>  hh_label_hosp_label_map;
  static std::map<std::string, int>  hosp_label_hosp_id_map;

  static bool Household_hospital_map_file_exists;
  static int Hospital_fixed_staff;
  static double Hospital_worker_to_bed_ratio;
  static double Hospitalization_radius;

  static HospitalIDCountMapT Hospital_ID_total_assigned_size_map;
  static HospitalIDCountMapT Hospital_ID_current_assigned_size_map;
  static int Hospital_overall_panel_size;
  
  // School support
  static int School_fixed_staff;
  static double School_student_teacher_ratio;

  double vaccination_rate;
};


#endif // _FRED_PLACE_H
