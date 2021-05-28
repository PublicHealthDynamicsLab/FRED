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
// File: Place.cc
//
#include <algorithm>
#include <climits>
#include <sstream>
#include <iostream>
#include <limits>
#include <math.h>
#include <new>
#include <set>
#include <string>
#include <typeinfo>
#include <unistd.h>

#include "Place.h"

#include "Block_Group.h"
#include "Date.h"
#include "Condition.h"
#include "Geo.h"
#include "Global.h"
#include "Household.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include "Property.h"
#include "Person.h"
#include "Random.h"
#include "Utils.h"
#include "Census_Tract.h"
#include "County.h"
#include "Geo.h"
#include "Hospital.h"
#include "Place_Type.h"
#include "Regional_Layer.h"
#include "Regional_Patch.h"
#include "State.h"
#include "Visualization_Layer.h"

#define PI 3.14159265359

// static place subtype codes
char Place::SUBTYPE_NONE = 'X';
char Place::SUBTYPE_COLLEGE = 'C';
char Place::SUBTYPE_PRISON = 'P';
char Place::SUBTYPE_MILITARY_BASE = 'M';
char Place::SUBTYPE_NURSING_HOME = 'N';
char Place::SUBTYPE_HEALTHCARE_CLINIC = 'I';
char Place::SUBTYPE_MOBILE_HEALTHCARE_CLINIC = 'Z';

bool Place::load_completed = false;
bool Place::is_primary_care_assignment_initialized = false;

int Place::next_place_id = 0;

// geo info
fred::geo Place::min_lat = 0;
fred::geo Place::max_lat = 0;
fred::geo Place::min_lon = 0;
fred::geo Place::max_lon = 0;
bool Place::country_is_usa = false;
bool Place::country_is_colombia = false;
bool Place::country_is_india = false;
std::vector<int> Place::state_admin_code;
std::map<std::string, std::string>  Place::hh_label_hosp_label_map;
std::map<std::string, int>  Place::hosp_label_hosp_id_map;

// map of place type names
LabelMapT* Place::household_label_map;
LabelMapT* Place::school_label_map;
LabelMapT* Place::workplace_label_map;

// lists of places by type
place_vector_t Place::place_list;
place_vector_t Place::schools_by_grade[Global::GRADES];
place_vector_t Place::gq;

bool Place::Update_elevation = false;

char Place::Population_directory[FRED_STRING_SIZE];
char Place::Country[FRED_STRING_SIZE];
char Place::Population_version[FRED_STRING_SIZE];

// mean size of "household" associated with group quarters
double Place::College_dorm_mean_size = 3.5;
double Place::Military_barracks_mean_size = 12;
double Place::Prison_cell_mean_size = 1.5;
double Place::Nursing_home_room_mean_size = 1.5;

// non-resident staff for group quarters
int Place::College_fixed_staff = 0;
double Place::College_resident_to_staff_ratio = 0;
int Place::Prison_fixed_staff = 0;
double Place::Prison_resident_to_staff_ratio = 0;
int Place::Nursing_home_fixed_staff = 0;
double Place::Nursing_home_resident_to_staff_ratio = 0;
int Place::Military_fixed_staff = 0;
double Place::Military_resident_to_staff_ratio = 0;
int Place::School_fixed_staff = 0;
double Place::School_student_teacher_ratio = 0;
bool Place::Household_hospital_map_file_exists = false;
int Place::Hospital_fixed_staff = 1.0;
double Place::Hospital_worker_to_bed_ratio = 1.0;
double Place::Hospitalization_radius = 0.0;
int Place::Hospital_overall_panel_size = 0;
vector<string> Place::location_id;
HospitalIDCountMapT Place::Hospital_ID_total_assigned_size_map;
HospitalIDCountMapT Place::Hospital_ID_current_assigned_size_map;


Place::Place(const char* lab, int _type_id, fred::geo lon, fred::geo lat) : Group(lab, _type_id) {
  this->set_id(-1);      // actual id assigned in Place::add_place
  this->set_subtype(Place::SUBTYPE_NONE);
  this->staff_size = 0;
  this->members.reserve(8); // initial slots for 8 people -- this is expanded in begin_membership()
  this->members.clear();
  this->patch = NULL;
  this->container = NULL;
  this->longitude = lon;
  this->latitude = lat;
  this->admin_code = 0;  // assigned elsewhere

  this->original_size_by_age = NULL;
  this->partitions_by_age = NULL;

  int conditions = Condition::get_number_of_conditions();
  this->transmissible_people = new person_vector_t[conditions];

  // zero out all condition-specific counts
  for(int d = 0; d < conditions; ++d) {
    this->transmissible_people[d].clear();
  }

  this->elevation = 0.0;
  this->income = 0;
  this->partitions.clear();
  this->next_partition = 0;
  this->vaccination_rate = -1.0;
}

void Place::prepare() {
  FRED_VERBOSE(1, "Prepare place %d %s\n", this->id, this->label);
  this->N_orig = this->members.size();

  // find median income
  int size = get_size();
  std::vector<int> income_list;
  income_list.reserve(size);
  for(int i = 0; i < size; ++i) {
    income_list.push_back(this->members[i]->get_income());
  }
  std::sort(income_list.begin(), income_list.end());
  if(size > 0) {
    int median = income_list[size/2];
    this->set_income(median);
  } else {
    this->set_income(0);
  }

  // set elevation of partitions
  int rooms = this->partitions.size();
  for(int i = 0; i < rooms; ++i) {
    this->partitions[i]->set_partition_elevation(this->get_elevation());
  }

  if(this->is_school()) {
    prepare_vaccination_rates();
  }

  FRED_VERBOSE(1, "Prepare place %d %s finished\n", this->id, this->label);
}

void Place::print(int condition_id) {
  FRED_STATUS(0, "place %d %s\n\n", this->id, this->label);
}

void Place::turn_workers_into_teachers(Place* school) {
  std::vector<Person*> workers;
  workers.reserve(static_cast<int>(this->members.size()));
  workers.clear();
  for(int i = 0; i < static_cast<int>(this->members.size()); ++i) {
    workers.push_back(this->members[i]);
  }
  FRED_VERBOSE(1, "turn_workers_into_teachers: place %d %s has %d workers\n", this->get_id(), this->get_label(), this->members.size());
  int new_teachers = 0;
  for(int i = 0; i < static_cast<int>(workers.size()); ++i) {
    Person* person = workers[i];
    assert(person != NULL);
    FRED_VERBOSE(1, "Potential teacher %d age %d\n", person->get_id(), person->get_age());
    if(person->become_a_teacher(school)) {
      new_teachers++;
      FRED_VERBOSE(1, "new teacher %d age %d moved from workplace %d %s to school %d %s\n",
          person->get_id(), person->get_age(), this->get_id(),
          this->get_label(), school->get_id(), school->get_label());
    }
  }
  school->set_staff_size(school->get_staff_size() + new_teachers);
  FRED_VERBOSE(0, "%d new teachers reassigned from workplace %s to school %s\n", new_teachers,
	       this->get_label(), school->get_label());
}

void Place::reassign_workers(Place* new_place) {
  person_vector_t workers;
  workers.reserve((int)this->members.size());
  workers.clear();
  for(int i = 0; i < static_cast<int>(this->members.size()); ++i) {
    workers.push_back(this->members[i]);
  }
  int reassigned_workers = 0;
  for(int i = 0; i < static_cast<int>(workers.size()); ++i) {
    workers[i]->change_workplace(new_place, 0);
    // printf("worker %d age %d moving from workplace %s to place %s\n",
    //   workers[i]->get_id(), workers[i]->get_age(), label, new_place->get_label());
    reassigned_workers++;
  }
  new_place->set_staff_size(new_place->get_staff_size() + reassigned_workers);
  FRED_VERBOSE(0, "%d workers reassigned from workplace %s to place %s\n", reassigned_workers,
	       this->get_label(), new_place->get_label());
}

//////////////////////////////////////////////////////////
//
// PLACE SPECIFIC DATA
//
//////////////////////////////////////////////////////////


char* Place::get_place_label(Place* p) {
  return ((p == NULL) ? (char*) "-1" : p->get_label());
}

long long int Place::get_block_group_admin_code() {
  return this->admin_code;
}

Block_Group* Place::get_block_group() {
  return Block_Group::get_block_group_with_admin_code(this->admin_code);
}

long long int Place::get_census_tract_admin_code() {
  return get_block_group_admin_code() / 10;
}

int Place::get_county_admin_code() {
  if(Place::is_country_usa()) {
    return (int)(this->get_census_tract_admin_code() / 1000000);
  }
  if(Place::is_country_india()) {
    return (int)((this->admin_code/1000000) % 1000);
  } else {
    return (int)this->admin_code;
  }
}

int Place::get_state_admin_code() {
  if(Place::is_country_usa()) {
    return this->get_county_admin_code() / 1000;
  }
  if(Place::is_country_india()) {
    return (int)(this->admin_code / 1000000000);
  } else {
    return get_county_admin_code();
  }
}

void Place::set_elevation(double elev) {
  this->elevation = elev;
}

int Place::get_adi_state_rank() {
  return Block_Group::get_block_group_with_admin_code(get_block_group_admin_code())->get_adi_state_rank();
}

int Place::get_adi_national_rank() {
  return Block_Group::get_block_group_with_admin_code(get_block_group_admin_code())->get_adi_national_rank();
}

//// STATIC METHODS

Household* Place::get_household(int i) {
  if(0 <= i && i < Place::get_number_of_households()) {
    return static_cast<Household*>(Place_Type::get_household_place_type()->get_place(i));
  } else {
    return NULL;
  }
}

Place* Place::get_neighborhood(int i) {
  if(0 <= i && i < Place::get_number_of_neighborhoods()) {
    return Place_Type::get_neighborhood_place_type()->get_place(i);
  } else {
    return NULL;
  }
}

Place* Place::get_school(int i) {
  if(0 <= i && i < Place::get_number_of_schools()) {
    return Place_Type::get_school_place_type()->get_place(i);
  } else {
    return NULL;
  }
}

Place* Place::get_workplace(int i) {
  if(0 <= i && i < Place::get_number_of_workplaces()) {
    return Place_Type::get_workplace_place_type()->get_place(i);
  } else {
    return NULL;
  }
}

Hospital* Place::get_hospital(int i) {
  if(0 <= i && i < Place::get_number_of_hospitals()) {
    return static_cast<Hospital*>(Place_Type::get_hospital_place_type()->get_place(i));
  } else {
    return NULL;
  }
}


double Place::distance_between_places(Place* p1, Place* p2) {
  return Geo::xy_distance(p1->get_latitude(), p1->get_longitude(), p2->get_latitude(), p2->get_longitude());
}

void Place::get_place_properties() {

  char property_name[FRED_STRING_SIZE];

  // read optional properties
  Property::disable_abort_on_failure();
  
  Place::household_label_map = new LabelMapT();
  Place::school_label_map = new LabelMapT();
  Place::workplace_label_map = new LabelMapT();
  Place::gq.clear();
  for(int grade = 0; grade < Global::GRADES; ++grade) {
    Place::schools_by_grade[grade].clear();
  }

  // get properties for derived subclasses
  Household::get_properties();
  Hospital::get_properties();
  Block_Group::read_adi_file();

  // population properties
  strcpy(Place::Population_directory, "$FRED_HOME/data/country");
  Property::get_property("population_directory", Place::Population_directory);

  strcpy(Place::Population_version, "RTI_2010_ver1");
  Property::get_property("population_version", Place::Population_version);

  strcpy(Place::Country, "usa");
  Property::get_property("country", Place::Country);
  for(int i = 0; i < strlen(Place::Country); ++i) {
    Place::Country[i] = tolower(Place::Country[i]);
  }

  Place::country_is_usa = strcmp(Place::Country, "usa") == 0;
  Place::country_is_colombia = strcmp(Place::Country, "colombia") == 0;
  Place::country_is_india = strcmp(Place::Country, "india") == 0;

  Place::location_id.clear();

  strcpy(property_name, "locations_file");
  if(Property::does_property_exist(property_name)) {
    char locations_filename[FRED_STRING_SIZE];
    strcpy(locations_filename, "$FRED_HOME/data/locations.txt");
    Property::get_property(property_name, locations_filename);
    // printf("AFTER get_property |%s| = |%s|\n", property_name, locations_filename);
    FILE* loc_fp = Utils::fred_open_file(locations_filename);
    if(loc_fp != NULL) {
      char loc_id[FRED_STRING_SIZE];
      // int n = 0;
      while(fscanf(loc_fp, "%s", loc_id) == 1) {
        Place::location_id.push_back(string(loc_id));
      }
      fclose(loc_fp);
      // exit(0);
    } else {
      char msg[FRED_STRING_SIZE];
      sprintf(msg, "Can't find locations_file |%s|", locations_filename);
      Utils::print_error(msg);
    }
  } else {
    std::vector<std::string> location_names;
    location_names.clear();
    char loc_value[FRED_STRING_SIZE];
    strcpy(loc_value,"");
    Property::get_property("locations", loc_value);
  
    // split the property value into separate strings
    char *pch;
    pch = strtok(loc_value," \t\n\r");
    while(pch != NULL) {
      location_names.push_back(string(pch));
      pch = strtok(NULL, " \t\n\r");
    }

    if(location_names.size()>0) {
      char locations_filename[FRED_STRING_SIZE];
      sprintf(locations_filename, "$FRED_HOME/data/country/%s/locations.txt", Place::Country);
      FILE * loc_fp = Utils::fred_open_file(locations_filename);
      if(loc_fp != NULL) {
        char* line = NULL;
        size_t linecap = 0;
        size_t linelen;
        string linestr;
        while((linelen = getline(&line, &linecap, loc_fp)) > 0) {
          if(linelen == -1) {
            break;
          }
          // remove newline char
          if(line[strlen(line)-1] == '\n') {
            line[strlen(line) - 1] = '\0';
          }
          string linestr = string(line);
          for(int i = 0; i < location_names.size(); ++i) {
            string loc = location_names[i] + " ";
            // printf("loc = |%s|\n", loc.c_str());
            if(linestr.find(loc) == 0) {
              // get remainder of the line
              string fips_codes = linestr.substr(loc.length());
              // split fips_codes into separate strings
              char fips[FRED_STRING_SIZE];
              strcpy(fips, fips_codes.c_str());
              // printf("fips = |%s|\n",fips);
              char *pch;
              pch = strtok(fips," \t\n\r");
              while(pch != NULL) {
                Place::location_id.push_back(string(pch));
                pch = strtok(NULL, " \t\n\r");
              }
            }
          }
        }
        fclose(loc_fp);
      } else {
        char msg[FRED_STRING_SIZE];
        sprintf(msg, "Can't find locations_file |%s|", locations_filename);
        Utils::print_error(msg);
      }
    }
  }
  
  // remove any duplicate location ids
  int size = Place::location_id.size();
  if (size==0) {
    char error_file[FRED_STRING_SIZE];
    sprintf(error_file, "%s/errors.txt", Global::Simulation_directory);
    FILE* fp = fopen(error_file, "a");
    fprintf(fp, "FRED Error (file %s) No locations specified\n", Global::Program_file);
    fclose(fp);
    exit(0);
  }

  for(int j = size - 1; j > 0; --j) {
    bool duplicate = false;
    for(int i = 0; duplicate==false && i < j; ++i) {
      if(Place::location_id[i] == Place::location_id[j]) {
        duplicate = true;
      }
    }
    if(duplicate) {
      for(int k = j; k < Place::location_id.size() - 1; ++k) {
        Place::location_id[k] = Place::location_id[k+1];
      }
      Place::location_id.pop_back();
    }
  }
  for(int i = 0; i < Place::location_id.size(); ++i) {
    FRED_VERBOSE(0, "location_id[%d] = %s\n", i, get_location_id(i));
    Place::verify_pop_directory(get_location_id(i));
  }

  Property::get_property("update_elevation", &Place::Update_elevation);

  // school staff size
  Property::get_property("School_fixed_staff", &Place::School_fixed_staff);
  Property::get_property("School_student_teacher_ratio", &Place::School_student_teacher_ratio);

  // group quarter properties
  Property::get_property("college_dorm_mean_size", &Place::College_dorm_mean_size);
  Property::get_property("military_barracks_mean_size", &Place::Military_barracks_mean_size);
  Property::get_property("prison_cell_mean_size", &Place::Prison_cell_mean_size);
  Property::get_property("nursing_home_room_mean_size", &Place::Nursing_home_room_mean_size);

  Property::get_property("college_fixed_staff", &Place::College_fixed_staff);
  Property::get_property("college_resident_to_staff_ratio", &Place::College_resident_to_staff_ratio);
  Property::get_property("prison_fixed_staff", &Place::Prison_fixed_staff);
  Property::get_property("prison_resident_to_staff_ratio", &Place::Prison_resident_to_staff_ratio);
  Property::get_property("nursing_home_fixed_staff", &Place::Nursing_home_fixed_staff);
  Property::get_property("nursing_home_resident_to_staff_ratio", &Place::Nursing_home_resident_to_staff_ratio);
  Property::get_property("military_fixed_staff", &Place::Military_fixed_staff);
  Property::get_property("military_resident_to_staff_ratio", &Place::Military_resident_to_staff_ratio);

  // hospitalization properties
  Property::get_property("Hospital_worker_to_bed_ratio", &Place::Hospital_worker_to_bed_ratio);
  Place::Hospital_worker_to_bed_ratio = (Place::Hospital_worker_to_bed_ratio == 0.0 ? 1.0 : Place::Hospital_worker_to_bed_ratio);
  Property::get_property("hospitalization_radius", &Place::Hospitalization_radius);
  Property::get_property("Hospital_fixed_staff", &Place::Hospital_fixed_staff);

  char hosp_file_dir[FRED_STRING_SIZE];
  char hh_hosp_map_file_name[FRED_STRING_SIZE];

  Property::get_property("Household_Hospital_map_file_directory", hosp_file_dir);
  Property::get_property("Household_Hospital_map_file", hh_hosp_map_file_name);

  if(strcmp(hh_hosp_map_file_name, "none") == 0) {
    Place::Household_hospital_map_file_exists = false;
  } else {
    //If there is a file mapping Households to Hospitals, open it
    FILE* hospital_household_map_fp = NULL;

    char filename[FRED_STRING_SIZE];
    sprintf(filename, "%s%s", hosp_file_dir, hh_hosp_map_file_name);
    hospital_household_map_fp = Utils::fred_open_file(filename);

    if(hospital_household_map_fp != NULL) {
      Place::Household_hospital_map_file_exists = true;

      char hh_label[FRED_STRING_SIZE];
      char hosp_label[FRED_STRING_SIZE];
      while (fscanf(hospital_household_map_fp, "%s,%s", hh_label, hosp_label) == 2) {
        if (strcmp(hh_label, "hh_id") == 0) {
          continue;
        }
        if(strcmp(hh_label, "sp_id") == 0) {
          continue;
        }
        string hh_label_str(hh_label);
        string hosp_label_str(hosp_label);
        Place::hh_label_hosp_label_map.insert(std::pair<string, string>(hh_label_str, hosp_label_str));
      }
      fclose(hospital_household_map_fp);
    }
  }

  // the following are included here to make them visible to check_properties.
  // they are conditionally read in elsewhere.

  char elevation_data_dir[FRED_STRING_SIZE];
  sprintf(elevation_data_dir, "none");
  Property::get_property("elevation_data_directory", elevation_data_dir);

  char map_file_dir[FRED_STRING_SIZE];
  char map_file_name[FRED_STRING_SIZE];
  Property::get_property("Household_Hospital_map_file_directory", map_file_dir);
  Property::get_property("Household_Hospital_map_file", map_file_name);

  // restore requiring properties
  Property::set_abort_on_failure();

}

void Place::read_all_places() {

  // clear the vectors and maps
  Place::state_admin_code.clear();
  Place::hosp_label_hosp_id_map.clear();
  Place::hh_label_hosp_label_map.clear();

  // to compute the region's bounding box
  Place::min_lat = Place::min_lon = 999;
  Place::max_lat = Place::max_lon = -999;

  // process each specified location
  int locs = get_number_of_location_ids();
  for(int i = 0; i < locs; ++i) {
    Place::read_places(get_location_id(i));
  }

  // temporarily compute income levels to use for group quarters
  Place_Type::get_household_place_type()->prepare();

  // read group quarters separately so that we can assign household incomes
  for(int i = 0; i < locs; ++i) {
    Place::read_gq_places(get_location_id(i));
  }

  int total = 0;
  for(int i = 0; i < Place_Type::get_number_of_place_types(); ++i) {
    total += Place_Type::get_place_type(i)->get_number_of_places();
  }
  FRED_STATUS(0, "total count = %d\n", total);
  FRED_STATUS(0, "finished total places = %d\n", next_place_id);
  assert(total == next_place_id);

  if(Global::Use_Mean_Latitude) {
    // Make projection based on the location file.
    fred::geo mean_lat = (min_lat + max_lat) / 2.0;
    Geo::set_km_per_degree(mean_lat);
    Utils::fred_log("min_lat: %f  max_lat: %f  mean_lat: %f\n", min_lat, max_lat, mean_lat);
  } else {
    // DEFAULT: Use mean US latitude (see Geo.cc)
    Utils::fred_log("min_lat: %f  max_lat: %f\n", min_lat, max_lat);
  }

  // create geographical grids
  Global::Simulation_Region = new Regional_Layer(min_lon, min_lat, max_lon, max_lat);

  // layer containing neighborhoods
  Global::Neighborhoods = new Neighborhood_Layer();

  // add all places to the Neighborhood Layer
  Place_Type::add_places_to_neighborhood_layer();

  int number_of_neighborhoods = Global::Neighborhoods->get_number_of_neighborhoods();

  // Neighborhood_Layer::setup call Neighborhood_Patch::make_neighborhood
  Global::Neighborhoods->setup();
  FRED_VERBOSE(0, "Created %d neighborhoods\n", Place::get_number_of_neighborhoods());

  // add workplaces to Regional grid (for worker reassignment)
  int number_places = Place::get_number_of_workplaces();
  for(int p = 0; p < number_places; ++p) {
    Global::Simulation_Region->add_workplace(get_workplace(p));
  }

  // add hospitals to Regional grid (for household hospital assignment)
  number_places = get_number_of_hospitals();
  for(int p = 0; p < number_places; ++p) {
    // printf("ADD HOSP %d %s\n", p, get_hospital(p)->get_label());
    Global::Simulation_Region->add_hospital(get_hospital(p));
  }

  Place::load_completed = true;
  number_places = get_number_of_households() + get_number_of_neighborhoods()
    + get_number_of_schools() + get_number_of_workplaces() + get_number_of_hospitals();

  FRED_STATUS(0, "read_all_places finished: households = %d\n", get_number_of_households());
  FRED_STATUS(0, "read_all_places finished: neighborhoods = %d\n", get_number_of_neighborhoods());
  FRED_STATUS(0, "read_all_places finished: schools = %d\n", get_number_of_schools());
  FRED_STATUS(0, "read_all_places finished: workplaces = %d\n", get_number_of_workplaces());
  FRED_STATUS(0, "read_all_places finished: hospitals = %d\n", get_number_of_hospitals());
  FRED_STATUS(0, "read_all_places finished: Places = %d\n", number_places);

}

void Place::verify_pop_directory(const char* loc_id) {
  char pop_dir[FRED_STRING_SIZE];
  sprintf(pop_dir, "%s/%s/%s/%s", Place::Population_directory,
	  Place::Country, Place::Population_version, loc_id);

  if (Utils::does_path_exist(pop_dir)==false) {
    char msg[FRED_STRING_SIZE];
    sprintf(msg, "Can't find population directory |%s|", pop_dir);
    Utils::print_error(msg);
  }
}

void Place::read_places(const char* loc_id) {

  FRED_STATUS(0, "read places entered\n", "");

  verify_pop_directory(loc_id);

  char pop_dir[FRED_STRING_SIZE];
  sprintf(pop_dir, "%s/%s/%s/%s", Place::Population_directory,
	  Place::Country, Place::Population_version, loc_id);

  // record the actual synthetic population in the log file
  Utils::fred_log("POPULATION_FILE: %s\n", pop_dir);

  if (Global::Compile_FRED && Place_Type::get_number_of_place_types() <= 7) {
    return;
  }

  // read household locations
  char location_file[FRED_STRING_SIZE];
  sprintf(location_file, "%s/households.txt", pop_dir);
  read_household_file(location_file);
  Utils::fred_print_lap_time("Places.read_household_file");
  // FRED_VERBOSE(0, "after %s num_households = %d\n", location_file, get_number_of_households());

  // read school locations
  sprintf(location_file, "%s/schools.txt", pop_dir);
  read_school_file(location_file);

  // read workplace locations
  sprintf(location_file, "%s/workplaces.txt", pop_dir);
  read_workplace_file(location_file);

  // read hospital locations
  sprintf(location_file, "%s/hospitals.txt", pop_dir);
  read_hospital_file(location_file);

  // read in user-defined place types
  Place_Type::read_places(pop_dir);

  FRED_STATUS(0, "read places %s finished\n", loc_id);
}


void Place::read_gq_places(const char* loc_id) {

  FRED_STATUS(0, "read gq_places entered\n", "");

  if(Global::Enable_Group_Quarters || Place::Update_elevation) {

    char pop_dir[FRED_STRING_SIZE];
    sprintf(pop_dir, "%s/%s/%s/%s", Place::Population_directory,
	    Place::Country, Place::Population_version, loc_id);

    // read group quarters locations (a new workplace and household is created 
    // for each group quarters)
    char location_file[FRED_STRING_SIZE];
    sprintf(location_file, "%s/gq.txt", pop_dir);
    read_group_quarters_file(location_file);
    Utils::fred_print_lap_time("Places.read_group_quarters_file");
    // FRED_VERBOSE(0, "after %s num_households = %d\n", location_file, get_number_of_households());

  }
  FRED_STATUS(0, "read gq_places finished\n", "");
}


void Place::read_household_file(char* location_file) {

  char line[FRED_STRING_SIZE];

  // data to fill in from input file
  int type_id = Place_Type::HOUSEHOLD;
  char place_subtype = Place::SUBTYPE_NONE;
  char new_label[FRED_STRING_SIZE];
  char label[FRED_STRING_SIZE];
  long long int admin_code = 0;
  long long int sp_id = 0;
  double lat = 0;
  double lon = 0;
  double elevation = 0;
  int race;
  int income;
  int n = 0;

  FILE* fp = Utils::fred_open_file(location_file);

  // skip header line
  fgets(label, FRED_STRING_SIZE, fp);

  // read first data line
  strcpy(line, "");
  fgets(line, FRED_STRING_SIZE, fp);
  int items = sscanf(line, "%s %lld %d %d %lf %lf %lf", label, &admin_code, &race, &income, &lat, &lon, &elevation);
  
  while(6 <= items) {
      
    // debugging:
    // printf("HH %d %s %lld %d %d %lf %lf %lf\n", n, label, admin_code, race, income, lat, lon, elevation); fflush(stdout);



    // debugging:
    // printf("HH added: %d %s %lld %d %d %lf %lf %lf\n", n, place->get_label(), place->get_admin_code(),
    // race, income, place->get_latitude(), place->get_longitude(), place->get_elevation()); fflush(stdout);

    sscanf(label, "%lld", &sp_id);
    if(!Group::sp_id_exists(sp_id + 100000000)) {
      // negative income disallowed
      if(income < 0) {
        income = 0;
      }

      sprintf(new_label, "H-%s", label);

      Household* place = static_cast<Household*>(add_place(new_label, type_id, place_subtype, lon, lat, elevation, admin_code));
      place->set_sp_id(sp_id + 100000000);

      // household race and income
      place->set_household_race(race);
      place->set_income(income);

      n++;

      Place::update_geo_boundaries(lat, lon);
    }

    // get next line
    strcpy(line, "");
    fgets(line, FRED_STRING_SIZE, fp);
    items = sscanf(line, "%s %lld %d %d %lf %lf %lf", label, &admin_code, &race, &income, &lat, &lon, &elevation);

  }
  fclose(fp);
  FRED_VERBOSE(0, "finished reading in %d households\n", n);
  return;
}

void Place::read_workplace_file(char* location_file) {
  char line[FRED_STRING_SIZE];

  // data to fill in from input file
  int type_id = Place_Type::WORKPLACE;
  char place_subtype = Place::SUBTYPE_NONE;
  char label[FRED_STRING_SIZE];
  char new_label[FRED_STRING_SIZE];
  double lat;
  double lon;
  double elevation = 0;
  long long int admin_code = 0;
  long long int sp_id = 0;

  FILE* fp = Utils::fred_open_file(location_file);
  if (fp == NULL) {
    return;
  }

  // skip header line
  fgets(label, FRED_STRING_SIZE, fp);

  // read first data line
  strcpy(line, "");
  fgets(line, FRED_STRING_SIZE, fp);
  int items = sscanf(line, "%s %lf %lf %lf", label, &lat, &lon, &elevation);

  while(3 <= items) {
    sprintf(new_label, "W-%s", label);
    sscanf(label, "%lld", &sp_id);

    if(!Group::sp_id_exists(sp_id)) {
      // printf("%s %lf %lf %lf\n", new_label, lat, lon, elevation); fflush(stdout);
      Place* place = add_place(new_label, type_id, place_subtype, lon, lat, elevation, admin_code);
      place->set_sp_id(sp_id);
    }

    // read next data line
    strcpy(line, "");
    fgets(line, FRED_STRING_SIZE, fp);
    items = sscanf(line, "%s %lf %lf %lf", label, &lat, &lon, &elevation);
  }
  fclose(fp);
  return;
}


void Place::read_hospital_file(char* location_file) {
  char line[FRED_STRING_SIZE];

  // data to fill in from input file
  int type_id = Place_Type::HOSPITAL;
  char place_subtype = Place::SUBTYPE_NONE;
  char label[FRED_STRING_SIZE];
  char new_label[FRED_STRING_SIZE];
  double lat;
  double lon;
  double elevation = 0;
  int workers;
  int physicians;
  int beds;
  long long int admin_code = 0;
  long long int sp_id = 0;

  FILE* fp = Utils::fred_open_file(location_file);
  if(fp == NULL) {
    return;
  }

  // skip header line
  fgets(label, FRED_STRING_SIZE, fp);

  if(fp != NULL) {
    // read first data line
    strcpy(line, "");
    fgets(line, FRED_STRING_SIZE, fp);
    int items = sscanf(line, "%s %d %d %d %lf %lf %lf", label, &workers, &physicians, &beds, &lat, &lon, &elevation);

    while (6 <= items) {
      sprintf(new_label, "M-%s", label);
      sscanf(label, "%lld", &sp_id);
      if(!Group::sp_id_exists(sp_id + 600000000)) {
        Hospital* place = static_cast<Hospital*>(add_place(new_label, type_id, place_subtype, lon, lat, elevation, admin_code));

        place->set_sp_id(sp_id + 600000000);

        place->set_employee_count(workers);
        place->set_physician_count(physicians);
        place->set_bed_count(beds);

        string hosp_label_str(label);
        int hosp_id = get_number_of_hospitals() - 1;
        Place::hosp_label_hosp_id_map.insert(std::pair<string, int>(hosp_label_str, hosp_id));
      }

      // read next data line
      strcpy(line, "");
      fgets(line, FRED_STRING_SIZE, fp);
      items = sscanf(line, "%s %d %d %d %lf %lf %lf", label, &workers, &physicians, &beds, &lat, &lon, &elevation);
    }
    fclose(fp);
  }
  FRED_VERBOSE(0, "read_hospital_file: found %d hospitals\n", get_number_of_hospitals());
  return;
}


void Place::read_school_file(char* location_file) {
  char line[FRED_STRING_SIZE];

  // data to fill in from input file
  int type_id = Place_Type::SCHOOL;
  char place_subtype = Place::SUBTYPE_NONE;
  char new_label[FRED_STRING_SIZE];
  char label[FRED_STRING_SIZE];
  long long int admin_code = 0;
  double lat;
  double lon;
  double elevation = 0;
  long long int sp_id = 0;

  FILE* fp = Utils::fred_open_file(location_file);
  if (fp == NULL) {
    return;
  }

  // skip header line
  fgets(label, FRED_STRING_SIZE, fp);

  // read first data line
  strcpy(line, "");
  fgets(line, FRED_STRING_SIZE, fp);
  int items = sscanf(line, "%s %lld %lf %lf %lf", label, &admin_code, &lat, &lon, &elevation);
  
  while (4 <= items) {

    if(Place::country_is_usa) {
      // convert county admin code to block group code
      admin_code *= 10000000;
    }

    sscanf(label, "%lld", &sp_id);
    if(!Group::sp_id_exists(sp_id)) {
      sprintf(new_label, "S-%s", label);
      // printf("%s %lld %lf %lf %lf\n", new_label, admin_code, lat, lon, elevation); fflush(stdout);
      Place* place = add_place(new_label, type_id, place_subtype, lon, lat, elevation, admin_code);

      place->set_sp_id(sp_id);
    }

    // read next data line
    strcpy(line, "");
    fgets(line, FRED_STRING_SIZE, fp);
    items = sscanf(line, "%s %lld %lf %lf %lf", label, &admin_code, &lat, &lon, &elevation);
  }
  fclose(fp);
  return;
}


void Place::read_group_quarters_file(char* location_file) {
  char line[FRED_STRING_SIZE];

  // data to fill in from input file
  char place_subtype = Place::SUBTYPE_NONE;
  char gq_type;
  char id[FRED_STRING_SIZE];
  char label[FRED_STRING_SIZE];
  long long int admin_code = 0;
  double lat;
  double lon;
  double elevation = 0;
  int capacity;
  int income;
  long long int sp_id = 0;

  FILE* fp = Utils::fred_open_file(location_file);
  if (fp == NULL) {
    return;
  }

  // skip header line
  fgets(label, FRED_STRING_SIZE, fp);

  // read first data line
  strcpy(line, "");
  fgets(line, FRED_STRING_SIZE, fp);
  int items = sscanf(line, "%s %c %lld %d %lf %lf %lf", id, &gq_type, &admin_code, &capacity, &lat, &lon, &elevation);
  
  while (6 <= items) {
    // printf("read_gq_file: %s %c %lld %d %lf %lf %lf\n", id, gq_type, admin_code, capacity, lat, lon, elevation); fflush(stdout);

    update_geo_boundaries(lat, lon);

    // set number of units and subtype for this group quarters
    int number_of_units = 0;
    if(gq_type == 'C') {
      number_of_units = capacity / Place::College_dorm_mean_size;
      place_subtype = Place::SUBTYPE_COLLEGE;
      income = Place_Type::get_household_place_type()->get_income_second_quartile();
    }
    if(gq_type == 'M') {
      number_of_units = capacity / Place::Military_barracks_mean_size;
      place_subtype = Place::SUBTYPE_MILITARY_BASE;
      income = Place_Type::get_household_place_type()->get_income_second_quartile();
    }
    if(gq_type == 'P') {
      number_of_units = capacity / Place::Prison_cell_mean_size;
      place_subtype = Place::SUBTYPE_PRISON;
      income = Place_Type::get_household_place_type()->get_income_first_quartile();
    }
    if(gq_type == 'N') {
      number_of_units = capacity / Place::Nursing_home_room_mean_size;
      place_subtype = Place::SUBTYPE_NURSING_HOME;
      income = Place_Type::get_household_place_type()->get_income_first_quartile();
    }
    if(number_of_units == 0) {
      number_of_units = 1;
    }

    // add a workplace for this group quarters
    sprintf(label, "GW-%s", id);
    // sprintf(label, "WRK-%s", label);
    FRED_VERBOSE(1, "Adding GQ Workplace %s subtype %c\n", label, place_subtype);
    Place* workplace = add_place(label, Place_Type::WORKPLACE, place_subtype, lon, lat, elevation, admin_code);
    sscanf(id, "%lld", &sp_id);
    sp_id *= 10000;
    workplace->set_sp_id(sp_id);
    
    // add as household
    sprintf(label, "GH-%s",id);
    FRED_VERBOSE(1, "Adding GQ Household %s subtype %c\n", label, place_subtype);
    Household *place = static_cast<Household *>(add_place(label, Place_Type::HOUSEHOLD, place_subtype, lon, lat, elevation, admin_code));
    place->set_group_quarters_units(number_of_units);
    place->set_group_quarters_workplace(workplace);
    place->set_income(income);
    sp_id += 1;
    place->set_sp_id(sp_id);

    // add this to the list of externally defined gq's
    Place::gq.push_back(place);

    // generate additional household units associated with this group quarters
    for(int i = 1; i < number_of_units; ++i) {
      sprintf(label, "GH-%s-%03d", id, i+1);
      Household *place = static_cast<Household *>(add_place(label, Place_Type::HOUSEHOLD, place_subtype, lon, lat, elevation, admin_code));
      FRED_VERBOSE(1, "Adding GQ Household %s subtype %c out of %d units\n", label, place_subtype, number_of_units);
      place->set_income(income);
      sp_id += 1;
      place->set_sp_id(sp_id);
    }

    // read next data line
    strcpy(line, "");
    fgets(line, FRED_STRING_SIZE, fp);
    items = sscanf(line, "%s %c %lld %d %lf %lf %lf", id, &gq_type, &admin_code, &capacity, &lat, &lon, &elevation);
  }
  fclose(fp);
  return;
}


void Place::read_place_file(char* location_file, int type_id) {
  char line[FRED_STRING_SIZE];

  // data to fill in from input file
  char place_subtype = Place::SUBTYPE_NONE;
  char label[FRED_STRING_SIZE];
  double lat;
  double lon;
  long long int admin_code = 0;
  double elevation = 0;
  long long int sp_id = 0;

  FILE* fp = Utils::fred_open_file(location_file);
  if (fp == NULL) {
    return;
  }

  // skip header line
  fgets(label, FRED_STRING_SIZE, fp);

  // read first data line
  strcpy(line, "");
  fgets(line, FRED_STRING_SIZE, fp);
  int items = sscanf(line, "%lld %lf %lf %lf", &sp_id, &lat, &lon, &elevation);
  if (sp_id == 0) {
    sp_id = Place_Type::get_place_type(type_id)->get_next_sp_id();
  }
  
  while (4 <= items) {
    sprintf(label, "%s-%lld", Place_Type::get_place_type_name(type_id).c_str(), sp_id);
    printf("%s %lld %lf %lf %lf\n", label, sp_id, lat, lon, elevation); fflush(stdout);
    if(!Group::sp_id_exists(sp_id)) {
      Place* place = add_place(label, type_id, place_subtype, lon, lat, elevation, admin_code);
      place->set_sp_id(sp_id);
    }
    // read next data line
    strcpy(line, "");
    fgets(line, FRED_STRING_SIZE, fp);
    items = sscanf(line, "%lld %lf %lf %lf", &sp_id, &lat, &lon, &elevation);
    if (sp_id == 0) {
      sp_id = Place_Type::get_place_type(type_id)->get_next_sp_id();
    }
  }
  fclose(fp);
  return;
}


void Place::setup_block_groups() {

  FRED_VERBOSE(0, "setup_block_groups BLOCK\n");

  int size = get_number_of_households();
  for(int p = 0; p < size; ++p) {
    Place* place = Place_Type::get_household_place_type()->get_place(p);
    long long int admin_code = place->get_admin_code();

    // get the block group associated with this code, creating a new one if necessary
    Block_Group* block_group = Block_Group::get_block_group_with_admin_code(admin_code);
    block_group->add_household(place);

  }

  FRED_VERBOSE(0, "setup_block_groups finished BLOCK\n");
}


void Place::prepare_places() {

  FRED_STATUS(0, "prepare_places entered\n", "");

  // this is needed when reading in place files
  // delete_place_label_map();

  for (int i = 0; i < Place_Type::get_number_of_place_types(); i++) {
    int n = Place_Type::get_place_type(i)->get_number_of_places();
    for (int p = 0; p < n; p++) {
      Place* place = Place_Type::get_place_type(i)->get_place(p);
      place->prepare();
    }
  }
  
  Global::Neighborhoods->prepare();

  // create lists of school by grade
  int number_of_schools = get_number_of_schools();
  for(int p = 0; p < number_of_schools; ++p) {
    Place* school = get_school(p);
    for(int grade = 0; grade < Global::GRADES; ++grade) {
      if(school->get_original_size_by_age(grade) > 0) {
        Place::schools_by_grade[grade].push_back(school);
      }
    }
  }

  if(Global::Verbose > 1) {
    // check the schools by grade lists
    printf("\n");
    for(int grade = 0; grade < Global::GRADES; ++grade) {
      int size = Place::schools_by_grade[grade].size();
      printf("GRADE = %d SCHOOLS = %d: ", grade, size);
      for(int i = 0; i < size; ++i) {
        printf("%s ", Place::schools_by_grade[grade][i]->get_label());
      }
      printf("\n");
    }
    printf("\n");
  }
  if (Global::Verbose > 0) {
    print_status_of_schools(0);
  }

  if (Global::Enable_Visualization_Layer) {
    // add list of counties to visualization data directory
    char filename[256];
    sprintf(filename, "%s/COUNTIES", Global::Visualization_directory);
    FILE* fp = fopen(filename, "w");
    for(int i = 0; i < County::get_number_of_counties(); ++i) {
      if (Place::country_is_usa) {
        fprintf(fp, "%05d\n", (int) County::get_county_with_index(i)->get_admin_division_code());
      } else {
        fprintf(fp, "%d\n", (int) County::get_county_with_index(i)->get_admin_division_code());
      }
    }
    fclose(fp);
    
    // add list of census_tracts to visualization data directory
    sprintf(filename, "%s/CENSUS_TRACTS", Global::Visualization_directory);
    fp = fopen(filename, "w");
    for(int i = 0; i < Census_Tract::get_number_of_census_tracts(); ++i) {
      long long int admin_code = Census_Tract::get_census_tract_with_index(i)->get_admin_division_code();
      fprintf(fp, "%011lld\n", admin_code);
    }
    fclose(fp);
    
    // add geographical bounding box to visualization data directory
    sprintf(filename, "%s/BBOX", Global::Visualization_directory);
    fp = fopen(filename, "w");
    fprintf(fp, "ymin = %0.6f\n", Place::min_lat);
    fprintf(fp, "xmin = %0.6f\n", Place::min_lon);
    fprintf(fp, "ymax = %0.6f\n", Place::max_lat);
    fprintf(fp, "xmax = %0.6f\n", Place::max_lon);
    fclose(fp);
  }

  if(Place_Type::get_school_place_type()->is_vaccination_rate_enabled()) {
    // printf("PREP_VAX households\n");
    for(int p = 0; p < get_number_of_households(); ++p) {
      static_cast<Household*>(get_household(p))->set_household_vaccination();
    }
  }

  // log state info
  for(int i = 0; i < State::get_number_of_states(); ++i) {
    int admin_code = (int) State::get_state_with_index(i)->get_admin_division_code();
    int hh = State::get_state_with_index(i)->get_number_of_households();
    int pop = State::get_state_with_index(i)->get_population_size();
    if (Place::country_is_usa) {
      FRED_VERBOSE(0, "STATE[%d] = %02d  hh = %d pop = %d\n", i, admin_code, hh, pop);
    } else {
      FRED_VERBOSE(0, "STATE[%d] = %d  hh = %d  pop = %d\n", i, admin_code, hh, pop);
    }
  }

  // log county info
  for(int i = 0; i < County::get_number_of_counties(); ++i) {
    int admin_code = (int) County::get_county_with_index(i)->get_admin_division_code();
    int hh = County::get_county_with_index(i)->get_number_of_households();
    int pop = County::get_county_with_index(i)->get_population_size();
    if (Place::country_is_usa) {
      FRED_VERBOSE(0, "COUNTIES[%d] = %05d  hh = %d pop = %d\n", i, admin_code, hh, pop);
    } else {
      FRED_VERBOSE(0, "COUNTIES[%d] = %d  hh = %d  pop = %d\n", i, admin_code, hh, pop);
    }
  }

  // log census tract info
  for(int i = 0; i < Census_Tract::get_number_of_census_tracts(); ++i) {
    long long int admin_code = Census_Tract::get_census_tract_with_index(i)->get_admin_division_code();
    int hh = Census_Tract::get_census_tract_with_index(i)->get_number_of_households();
    int pop = Census_Tract::get_census_tract_with_index(i)->get_population_size();
    FRED_VERBOSE(0, "CENSUS_TRACTS[%d] = %011ld  households = %d  pop = %d\n", i, admin_code, hh, pop);
  }

  // log block group info
  for(int i = 0; i < Block_Group::get_number_of_block_groups(); ++i) {
    /*
    long long int admin_code = Block_Group::get_block_group_with_index(i)->get_admin_division_code();
    int hh = Block_Group::get_block_group_with_index(i)->get_number_of_households();
    int pop = Block_Group::get_block_group_with_index(i)->get_population_size();
    int national_rank = Block_Group::get_block_group_with_index(i)->get_adi_national_rank();
    int state_rank = Block_Group::get_block_group_with_index(i)->get_adi_state_rank();
    FRED_VERBOSE(0, "BLOCK_GROUP[%d] = %011ld  households = %d  pop = %d  adi_national_rank = %d adi_state_rank = %d\n",
    i, admin_code, hh, pop, national_rank, state_rank);
    */
  }

}

void Place::print_status_of_schools(int day) {
  int students_per_grade[Global::GRADES];
  for(int i = 0; i < Global::GRADES; ++i) {
    students_per_grade[i] = 0;
  }

  int number_of_schools = get_number_of_schools();
  for(int p = 0; p < number_of_schools; ++p) {
    Place *school = get_school(p);
    for(int grade = 0; grade < Global::GRADES; ++grade) {
      int total = school->get_original_size();
      int orig = school->get_original_size_by_age(grade);
      int now = school->get_size_by_age(grade);
      students_per_grade[grade] += now;
      if(0 && total > 1500 && orig > 0) {
        printf("%s GRADE %d ORIG %d NOW %d DIFF %d\n", school->get_label(), grade,
            school->get_original_size_by_age(grade), school->get_size_by_age(grade),
            school->get_size_by_age(grade) - school->get_original_size_by_age(grade));
      }
    }
  }

  int year = day / 365;
  // char filename[256];
  // sprintf(filename, "students.%d", year);
  // FILE *fp = fopen(filename,"w");
  int total_students = 0;
  for(int i = 0; i < Global::GRADES; ++i) {
    // fprintf(fp, "%d %d\n", i,students_per_grade[i]);
    printf("YEAR %d GRADE %d STUDENTS %d\n", year, i, students_per_grade[i]);
    total_students += students_per_grade[i];
  }
  // fclose(fp);
  printf("YEAR %d TOTAL_STUDENTS %d\n", year, total_students);
}

void Place::update(int day) {

  FRED_STATUS(1, "update places entered\n", "");

  if (Global::Enable_Health_Insurance) {
    for(int p = 0; p < get_number_of_households(); ++p) {
      get_household(p)->reset_healthcare_info();
    }
  }
  
  if(Global::Enable_Hospitals) {
    for(int p = 0; p < get_number_of_hospitals(); ++p) {
      get_hospital(p)->reset_current_daily_patient_count();
    }
  }

  FRED_STATUS(1, "update places finished\n", "");
}

Place* Place::get_household_from_label(const char* s) {
  assert(Place::household_label_map != NULL);
  if(s[0] == '\0' || strcmp(s, "X") == 0) {
    return NULL;
  }
  LabelMapT::iterator itr;
  string str(s);
  itr = Place::household_label_map->find(str);
  if(itr != Place::household_label_map->end()) {
    return get_household(itr->second);
  } else {
    FRED_VERBOSE(1, "Help!  can't find household with label = %s\n", str.c_str());
    return NULL;
  }
}

Place* Place::get_school_from_label(const char* s) {
  assert(Place::school_label_map != NULL);
  if(s[0] == '\0' || strcmp(s, "X") == 0) {
    return NULL;
  }
  LabelMapT::iterator itr;
  string str(s);
  itr = Place::school_label_map->find(str);
  if(itr != Place::school_label_map->end()) {
    return get_school(itr->second);
  } else {
    FRED_VERBOSE(1, "Help!  can't find school with label = %s\n", str.c_str());
    return NULL;
  }
}

Place* Place::get_workplace_from_label(const char* s) {
  assert(Place::workplace_label_map != NULL);
  if(s[0] == '\0' || strcmp(s, "X") == 0) {
    return NULL;
  }
  LabelMapT::iterator itr;
  string str(s);
  itr = Place::workplace_label_map->find(str);
  if(itr != Place::workplace_label_map->end()) {
    return get_workplace(itr->second);
  } else {
    FRED_VERBOSE(1, "Help!  can't find workplace with label = %s\n", str.c_str());
    return NULL;
  }
}

Place* Place::add_place(char* label, int type_id, char subtype, fred::geo lon, fred::geo lat, double elevation, long long int admin_code) {

  FRED_VERBOSE(1, "add_place %s type %d = %s subtype %c\n", label, type_id, Place_Type::get_place_type_name(type_id).c_str(), subtype);

  string label_str;
  label_str.assign(label);

  if (Place::country_is_usa == false) {
    if (type_id == Place_Type::HOUSEHOLD) {
      if(Place::household_label_map->find(label_str) != Place::household_label_map->end()) {
        if (Global::Verbose > 0) {
          FRED_WARNING("duplicate household label found: %s\n", label);
        }
        return get_household_from_label(label);
      }
    }
    if(type_id == Place_Type::SCHOOL) {
      if(Place::school_label_map->find(label_str) != Place::school_label_map->end()) {
        if(Global::Verbose > 1) {
          FRED_WARNING("duplicate school label found: %s\n", label);
        }
        return get_school_from_label(label);
      }
    }
    if(type_id == Place_Type::WORKPLACE) {
      if(Place::workplace_label_map->find(label_str) != Place::workplace_label_map->end()) {
        if(Global::Verbose > 1) {
          FRED_WARNING("duplicate workplace label found: %s\n", label);
        }
        return get_workplace_from_label(label);
      }
    }
  }

  Place* place = NULL;
  if(type_id == Place_Type::HOUSEHOLD) {
    place = new Household(label, subtype, lon, lat);
    Place_Type::get_place_type(type_id)->add_place(place);
    Place::household_label_map->insert(std::make_pair(label_str, get_number_of_households()-1));
  } else if(type_id == Place_Type::WORKPLACE) {
    place = new Place(label, type_id, lon, lat);
    Place_Type::get_place_type(type_id)->add_place(place);
    Place::workplace_label_map->insert(std::make_pair(label_str, get_number_of_workplaces()-1));
  } else if(type_id == Place_Type::SCHOOL) {
    place = new Place(label, type_id, lon, lat);
    Place_Type::get_place_type(type_id)->add_place(place);
    Place::school_label_map->insert(std::make_pair(label_str, get_number_of_schools()-1));
  } else if(type_id == Place_Type::HOSPITAL) {
    place = new Hospital(label, subtype, lon, lat);
    Place_Type::get_place_type(type_id)->add_place(place);
  } else {
    place = new Place(label, type_id, lon, lat);
    Place_Type::get_place_type(type_id)->add_place(place);
  }

  int id = get_new_place_id();
  place->set_id(id);
  place->set_subtype(subtype);
  place->set_admin_code(admin_code);
  place->set_elevation(elevation);
  Place::save_place(place);
    
  FRED_VERBOSE(1, "add_place finished id %d lab %s type %d = %s subtype %c lat %f lon %f admin %lld elev %f\n",
      place->get_id(), place->get_label(), place->get_type_id(),
      Place_Type::get_place_type_name(type_id).c_str(), place->get_subtype(),
      place->get_latitude(), place->get_longitude(), place->get_admin_code(), place->get_elevation());

  return place;
}


void Place::setup_group_quarters() {

  FRED_STATUS(0, "setup group quarters entered\n", "");

  int p = 0;
  int units = 0;
  int num_households = get_number_of_households();
  while(p < num_households) {
    Household* house = Place::get_household(p++);
    Household* new_house;
    if(house->is_group_quarters()) {
      int gq_size = house->get_size();
      int gq_units = house->get_group_quarters_units();
      FRED_VERBOSE(1, "GQ_setup: house %d label %s subtype %c initial size %d units %d\n", p, house->get_label(),
          house->get_subtype(), gq_size, gq_units);
      int units_filled = 1;
      if(gq_units > 1) {
        person_vector_t Housemates;
        Housemates.clear();
        for(int i = 0; i < gq_size; ++i) {
          Person* person = house->get_member(i);
          Housemates.push_back(person);
        }
        int min_per_unit = gq_size / gq_units;
        int larger_units = gq_size - min_per_unit * gq_units;
        int smaller_units = gq_units - larger_units;
        FRED_VERBOSE(1, "GQ min_per_unit %d smaller = %d  larger = %d total = %d  orig = %d\n", min_per_unit,
            smaller_units, larger_units, smaller_units*min_per_unit + larger_units*(min_per_unit+1), gq_size);
        int next_person = min_per_unit;
        for(int i = 1; i < smaller_units; ++i) {
          // assert(units_filled < gq_units);
          new_house = Place::get_household(p++);
          FRED_VERBOSE(1,"GQ smaller new_house %s unit %d subtype %c size %d\n",
              new_house->get_label(), i, new_house->get_subtype(), new_house->get_size());
          for(int j = 0; j < min_per_unit; ++j) {
            Person* person = Housemates[next_person++];
            person->change_household(new_house);
          }
          FRED_VERBOSE(1,"GQ smaller new_house %s subtype %c size %d\n",
              new_house->get_label(), new_house->get_subtype(), new_house->get_size());
          units_filled++;
          FRED_VERBOSE(1, "GQ size of smaller unit %s = %d remaining in main house %d\n",
              new_house->get_label(), new_house->get_size(), house->get_size());
        }
        for(int i = 0; i < larger_units; ++i) {
          new_house = Place::get_household(p++);
          printf("GQ larger new_house %s\n", new_house->get_label()); fflush(stdout);
          for(int j = 0; j < min_per_unit + 1; ++j) {
            Person* person = Housemates[next_person++];
            person->change_household(new_house);
          }
          // printf("GQ larger new_house %s subtype %c size %d\n", new_house->get_label(), new_house->get_subtype(), new_house->get_size()); fflush(stdout);
          units_filled++;
          // printf("GQ size of larger unit %s = %d -- remaining in main house %d\n", new_house->get_label(), new_house->get_size(), house->get_size());
        }
      }
      units += units_filled;
    }
  }
  FRED_STATUS(0, "setup group quarters finished, units = %d\n", units);
}

void Place::setup_households() {

  FRED_STATUS(0, "setup households entered\n", "");

  int num_households = get_number_of_households();
  for(int p = 0; p < num_households; ++p) {
    Household* house = Place::get_household(p);
    if(house->get_size() == 0) {
      FRED_VERBOSE(0, "Warning: house %d label %s has zero size.\n", house->get_id(), house->get_label());
      continue;
    }

    // ensure that each household has an identified householder
    Person* person_with_max_age = NULL;
    Person* head_of_household = NULL;
    int max_age = -99;
    for(int j = 0; j < house->get_size() && head_of_household == NULL; ++j) {
      Person* person = house->get_member(j);
      assert(person != NULL);
      if(person->is_householder()) {
        head_of_household = person;
        continue;
      } else {
        int age = person->get_age();
        if(age > max_age) {
          max_age = age;
          person_with_max_age = person;
        }
      }
    }

    if(head_of_household == NULL) {
      assert(person_with_max_age != NULL);
      person_with_max_age->make_householder();
      head_of_household = person_with_max_age;
    }
    assert(head_of_household != NULL);

    // make sure everyone know who's the head
    for(int j = 0; j < house->get_size(); j++) {
      Person* person = house->get_member(j);
      if(person != head_of_household && person->is_householder()) {
        person->set_household_relationship(Household_Relationship::HOUSEMATE);
      }
    }
    assert(head_of_household != NULL);
    FRED_VERBOSE(1, "HOLDER: house %d label %s is_group_quarters %d householder %d age %d\n", house->get_id(),
        house->get_label(), house->is_group_quarters()?1:0, head_of_household->get_id(), head_of_household->get_age());

    // setup household structure type
    house->set_household_structure();
    house->set_orig_household_structure();
  }

  FRED_STATUS(0, "setup households finished\n", "");
}


void Place::setup_partitions() {
  FRED_STATUS(0, "setup partitions entered\n");
  int number_of_place_types = Place_Type::get_number_of_place_types();
  for (int i = 0; i < number_of_place_types; i++) {
    Place_Type::get_place_type(i)->setup_partitions();
  }
  FRED_STATUS(0, "setup partitions finished\n");
}

void Place::reassign_workers() {

  if(Global::Assign_Teachers) {
    reassign_workers_to_schools();
  }

  if(Global::Enable_Hospitals) {
    reassign_workers_to_hospitals();
  }

  if(Global::Enable_Group_Quarters) {
    Place::reassign_workers_to_group_quarters(Place::SUBTYPE_COLLEGE, Place::College_fixed_staff,
        Place::College_resident_to_staff_ratio);
    Place::reassign_workers_to_group_quarters(Place::SUBTYPE_PRISON, Place::Prison_fixed_staff,
        Place::Prison_resident_to_staff_ratio);
    Place::reassign_workers_to_group_quarters(Place::SUBTYPE_MILITARY_BASE, Place::Military_fixed_staff,
        Place::Military_resident_to_staff_ratio);
    Place::reassign_workers_to_group_quarters(Place::SUBTYPE_NURSING_HOME, Place::Nursing_home_fixed_staff,
        Place::Nursing_home_resident_to_staff_ratio);
  }

  Utils::fred_print_lap_time("reassign workers");
}

void Place::reassign_workers_to_schools() {

  // staff data from:
  // http://www.statemaster.com/graph/edu_ele_sec_pup_rat-elementary-secondary-pupil-teacher-ratio
  int fixed_staff = Place::School_fixed_staff;
  double staff_ratio = Place::School_student_teacher_ratio;

  int number_of_schools = get_number_of_schools();

  Utils::fred_log("reassign workers to schools entered. schools = %d fixed_staff = %d staff_ratio = %f \n",
      number_of_schools, fixed_staff, staff_ratio);

  for(int p = 0; p < number_of_schools; p++) {
    Place* school = get_school(p);
    fred::geo lat = school->get_latitude();
    fred::geo lon = school->get_longitude();
    double x = Geo::get_x(lon);
    double y = Geo::get_y(lat);
    FRED_VERBOSE(1, "Reassign teachers to school %s in county %d at (%f,%f) \n",
        school->get_label(), school->get_county_admin_code(), x, y);
    
    // ignore school if it is outside the region
    Regional_Patch* regional_patch = Global::Simulation_Region->get_patch(lat, lon);
    if(regional_patch == NULL) {
      FRED_VERBOSE(0, "school %s OUTSIDE_REGION lat %f lon %f \n",
          school->get_label(), lat, lon);
      continue;
    }

    // target staff size
    int n = school->get_original_size();
    int staff = fixed_staff;
    if(staff_ratio > 0.0) {
      staff += (0.5 + (double)n / staff_ratio);
    }
    FRED_VERBOSE(1, "school %s students %d fixed_staff = %d tot_staff = %d\n",
        school->get_label(), n, fixed_staff, staff);

    Place* nearby_workplace = regional_patch->get_nearby_workplace(school, staff);
    if(nearby_workplace != NULL) {
      // make all the workers in selected workplace teachers at the nearby school
      nearby_workplace->turn_workers_into_teachers(school);
    } else {
      FRED_VERBOSE(0, "NO NEARBY_WORKPLACE FOUND FOR SCHOOL %s in county %d at lat %f lon %f \n",
          school->get_label(), school->get_county_admin_code(), lat, lon);
    }
  }
}

void Place::reassign_workers_to_hospitals() {

  int number_places = get_number_of_hospitals();
  Utils::fred_log("reassign workers to hospitals entered. places = %d\n", number_places);

  int fixed_staff = Place::Hospital_fixed_staff;
  double staff_ratio = (1.0 / Place::Hospital_worker_to_bed_ratio);

  for(int p = 0; p < number_places; p++) {
    Hospital* hosp = get_hospital(p);
    fred::geo lat = hosp->get_latitude();
    fred::geo lon = hosp->get_longitude();
    double x = Geo::get_x(lon);
    double y = Geo::get_y(lat);
    FRED_VERBOSE(0, "Reassign workers to hospital %s in county %d at (%f,%f) \n", hosp->get_label(), hosp->get_county_admin_code(), x, y);

    // ignore hospital if it is outside the region
    Regional_Patch* regional_patch = Global::Simulation_Region->get_patch(lat, lon);
    if(regional_patch == NULL) {
      FRED_VERBOSE(0, "hospital OUTSIDE_REGION lat %f lon %f \n", lat, lon);
      continue;
    }

    // target staff size
    int n = hosp->get_employee_count(); // From the input file
    FRED_VERBOSE(1, "Size %d\n", n);

    int staff = fixed_staff;
    if(staff_ratio > 0.0) {
      staff += (0.5 + (double)n / staff_ratio);
    }
    
    Place* nearby_workplace = regional_patch->get_nearby_workplace(hosp, staff);
    if(nearby_workplace != NULL) {
      // make all the workers in selected workplace as workers in the target place
      nearby_workplace->reassign_workers(hosp);
    } else {
      FRED_VERBOSE(0, "NO NEARBY_WORKPLACE FOUND for hospital %s in county %d at lat %f lon %f \n",
          hosp->get_label(), hosp->get_county_admin_code(), lat, lon);
    }
  }
}

void Place::reassign_workers_to_group_quarters(char subtype, int fixed_staff, double resident_to_staff_ratio) {
  int number_places = get_number_of_workplaces();
  Utils::fred_log("reassign workers to group quarters subtype %c entered. total workplaces = %d\n", subtype, number_places);
  for(int p = 0; p < number_places; ++p) {
    Place* place = get_workplace(p);
    if(place->get_subtype() == subtype) {
      fred::geo lat = place->get_latitude();
      fred::geo lon = place->get_longitude();
      double x = Geo::get_x(lon);
      double y = Geo::get_y(lat);
      // target staff size
      FRED_VERBOSE(1, "Size %d ", place->get_size());
      int staff = fixed_staff;
      if(resident_to_staff_ratio > 0.0) {
        staff += 0.5 + (double)place->get_size() / resident_to_staff_ratio;
      }

      FRED_VERBOSE(0, "REASSIGN WORKERS to GQ %s subtype %c target staff %d at (%f,%f) \n", place->get_label(), subtype, staff, lat, lon);

      // ignore place if it is outside the region
      Regional_Patch* regional_patch = Global::Simulation_Region->get_patch(lat, lon);
      if(regional_patch == NULL) {
        FRED_VERBOSE(0, "REASSIGN WORKERS to place GQ %s subtype %c FAILED -- OUTSIDE_REGION lat %f lon %f \n", place->get_label(), subtype, lat, lon);
        continue;
      }

      Place* nearby_workplace = regional_patch->get_nearby_workplace(place, staff);
      if(nearby_workplace != NULL) {
        // make all the workers in selected workplace as workers in the target place
        FRED_VERBOSE(0, "REASSIGN WORKERS: NEARBY_WORKPLACE FOUND %s for GQ %s subtype %c at lat %f lon %f \n",
            nearby_workplace->get_label(), place->get_label(), subtype, lat, lon);
        nearby_workplace->reassign_workers(place);
      }
      else {
        FRED_VERBOSE(0, "REASSIGN WORKERS: NO NEARBY_WORKPLACE FOUND for GQ %s subtype %c at lat %f lon %f \n", place->get_label(), subtype, lat, lon);
      }
    }
  }
}


Place* Place::get_random_household() {
  int size = get_number_of_households();
  if(size > 0) {
    return get_household(Random::draw_random_int(0, size - 1));
  } else {
    return NULL;
  }
}

Place* Place::get_random_workplace() {
  int size = get_number_of_workplaces();
  if(size > 0) {
    return get_workplace(Random::draw_random_int(0, size - 1));
  } else {
    return NULL;
  }
}

Place* Place::get_random_school(int grade) {
  int size = static_cast<int>(Place::schools_by_grade[grade].size());
  if(size > 0) {
    return Place::schools_by_grade[grade][Random::draw_random_int(0, size - 1)];
  } else {
    return NULL;
  }
}


void Place::assign_hospitals_to_households() {

  if(!Global::Enable_Hospitals) {
    return;
  }

  FRED_STATUS(0, "assign_hospitals_to_household entered\n");

  int number_hh = (int)get_number_of_households();
  for(int i = 0; i < number_hh; ++i) {
    Household* hh = get_household(i);
    Hospital* hosp = static_cast<Hospital*>(Place::get_hospital_assigned_to_household(hh));
    assert(hosp != NULL);
    if(hosp != NULL) {
      hh->set_household_visitation_hospital(hosp);
      string hh_label_str(hh->get_label());
      string hosp_label_str(hosp->get_label());

      Place::hh_label_hosp_label_map.insert(std::pair<string, string>(hh_label_str, hosp_label_str));
    }
  }

  int number_hospitals = get_number_of_hospitals();
  int catchment_count[number_hospitals];
  double catchment_age[number_hospitals];
  double catchment_dist[number_hospitals];
  for (int i = 0; i < number_hospitals; i++) {
    catchment_count[i] = 0;
    catchment_age[i] = 0;
    catchment_dist[i] = 0;
  }

  for(int i = 0; i < number_hh; ++i) {
    Household* hh = get_household(i);
    Hospital* hosp = hh->get_household_visitation_hospital();
    assert(hosp != NULL);
    string hosp_label_str(hosp->get_label());
    int hosp_id = -1;
    if(Place::hosp_label_hosp_id_map.find(hosp_label_str) != Place::hosp_label_hosp_id_map.end()) {
      hosp_id = Place::hosp_label_hosp_id_map.find(hosp_label_str)->second;
    }
    assert(0 <= hosp_id && hosp_id < number_hospitals);
    // printf("CATCH house %s hosp_id %d %s\n", hh->get_label(), hosp_id, hosp->get_label());
    catchment_count[hosp_id] += hh->get_size();
    catchment_dist[hosp_id] += hh->get_size()*(Place::distance_between_places(hh,hosp));
    for (int j = 0; j < hh->get_size(); j++) {
      double age = hh->get_member(j)->get_real_age();
      catchment_age[hosp_id] += age;
    }
  }

  for(int i = 0; i < number_hospitals; ++i) {
    if(catchment_count[i] > 0) {
      catchment_dist[i] /= catchment_count[i];
      catchment_age[i] /= catchment_count[i];
    }
    FRED_STATUS(0, "HOSPITAL CATCHMENT %d %s beds %d count %d age %f dist %f\n", i,
        get_hospital(i)->get_label(),
        get_hospital(i)->get_bed_count(0),
        catchment_count[i], catchment_age[i], catchment_dist[i]);
  }

  //Write the mapping file if it did not already exist (or if it was incomplete)
  if(!Place::Household_hospital_map_file_exists) {

    char map_file_dir[FRED_STRING_SIZE];
    char map_file_name[FRED_STRING_SIZE];
    Property::get_property("Household_Hospital_map_file_directory", map_file_dir);
    Property::get_property("Household_Hospital_map_file", map_file_name);

    if(strcmp(map_file_name, "none") == 0) {
      Place::hh_label_hosp_label_map.clear();
      return;
    }

    char filename[FRED_STRING_SIZE];
    sprintf(filename, "%s%s", map_file_dir, map_file_name);

    Utils::get_fred_file_name(filename);
    FILE* hh_label_hosp_label_map_fp = fopen(filename, "w");
    if(hh_label_hosp_label_map_fp == NULL) {
      Utils::fred_abort("Can't open %s\n", filename);
    }

    for(std::map<std::string, string>::iterator itr = Place::hh_label_hosp_label_map.begin(); itr != Place::hh_label_hosp_label_map.end(); ++itr) {
      fprintf(hh_label_hosp_label_map_fp, "%s,%s\n", itr->first.c_str(), itr->second.c_str());
    }

    fflush(hh_label_hosp_label_map_fp);
    fclose(hh_label_hosp_label_map_fp);
  }

  Place::hh_label_hosp_label_map.clear();
  FRED_STATUS(0, "assign_hospitals_to_household finished\n");
}

void Place::prepare_primary_care_assignment() {

  if(Place::is_primary_care_assignment_initialized) {
    return;
  }

  if(Global::Enable_Hospitals && Place::is_load_completed() && Person::is_load_completed()) {
    int tot_pop_size = Person::get_population_size();
    assert(Place::Hospital_overall_panel_size > 0);
    //Determine the distribution of population that should be assigned to each hospital location
    for(int i = 0; i < get_number_of_hospitals(); ++i) {
      Hospital* hosp = Place::get_hospital(i);
      double proprtn_of_total_panel = 0;
      if(hosp->get_subtype() != Place::SUBTYPE_MOBILE_HEALTHCARE_CLINIC) {
        proprtn_of_total_panel = static_cast<double>(hosp->get_daily_patient_capacity(0)) / static_cast<double>(Place::Hospital_overall_panel_size);
      }
      Place::Hospital_ID_total_assigned_size_map.insert(std::pair<int, int>(hosp->get_id(), ceil(proprtn_of_total_panel * tot_pop_size)));
      Place::Hospital_ID_current_assigned_size_map.insert(std::pair<int, int>(hosp->get_id(), 0));
    }
    Place::is_primary_care_assignment_initialized = true;
  }
}

Hospital* Place::get_random_open_hospital_matching_criteria(int sim_day, Person* per, bool check_insurance) {
  return NULL;
  /*
  if(!Global::Enable_Hospitals) {
    return NULL;
  }

  if(check_insurance) {
    assert(Global::Enable_Health_Insurance);
  }
  assert(per != NULL);

  int overnight_cap = 0;
  Hospital* assigned_hospital = NULL;
  Household* hh = per->get_household();
  assert(hh != NULL);

  // ignore place if it is outside the region
  fred::geo lat = hh->get_latitude();
  fred::geo lon = hh->get_longitude();
  Regional_Patch* hh_patch = Global::Simulation_Region->get_patch(lat, lon);

  vector<Place*> possible_hosp = Global::Simulation_Region->get_nearby_hospitals(hh_patch->get_row(), hh_patch->get_col(), lat, lon, 5);
  int number_hospitals = static_cast<int>(possible_hosp.size());
  if(number_hospitals <= 0) {
    Utils::fred_abort("Found no nearby Hospitals in simulation that has Enabled Hospitalization", "");
  }

  int number_possible_hospitals = 0;
  //First, only try Hospitals within a certain radius (* that accept insurance)
  std::vector<double> hosp_probs;
  double probability_total = 0.0;
  for(int i = 0; i < number_hospitals; ++i) {
    Hospital* hospital = static_cast<Hospital*>(possible_hosp[i]);
    double distance = Place::distance_between_places(hh, hospital);
    double cur_prob = 0.0;
    int increment = 0;
    overnight_cap = hospital->get_bed_count(sim_day);
    //Need to make sure place is not a healthcare clinic && there are beds available
    if(distance > 0.0 && !hospital->is_healthcare_clinic() && !hospital->is_mobile_healthcare_clinic()
       && hospital->should_be_open(sim_day)
       && (hospital->get_occupied_bed_count() < overnight_cap)) {
      if(check_insurance) {
        Insurance_assignment_index::e per_insur = per->get_insurance_type();
        if(hospital->accepts_insurance(per_insur)) {
          //Hospital accepts the insurance so we are good
          cur_prob = static_cast<double>(overnight_cap) / distance;
          increment = 1;
        } else {
          //Not possible (Doesn't accept insurance)
          cur_prob = 0.0;
          increment = 0;
        }
      } else {
        //We don't care about insurance so good to go
        cur_prob = static_cast<double>(overnight_cap) / distance;
        increment = 1;
      }
    } else {
      //Not possible
      cur_prob = 0.0;
      increment = 0;
    }
    hosp_probs.push_back(cur_prob);
    probability_total += cur_prob;
    number_possible_hospitals += increment;
  }
  assert(static_cast<int>(hosp_probs.size()) == number_hospitals);
  FRED_VERBOSE(1,"CATCH HOSP FOR HH %s number_hospitals %d number_poss_hosp %d\n",
	       hh->get_label(), number_hospitals, number_possible_hospitals);


  if(number_possible_hospitals > 0) {
    if(probability_total > 0.0) {
      for(int i = 0; i < number_hospitals; ++i) {
        hosp_probs[i] /= probability_total;
	// printf("%f ", hosp_probs[i]);
      }
    }
    // printf("\n");

    double rand = Random::draw_random();
    double cum_prob = 0.0;
    int i = 0;
    while(i < number_hospitals) {
      cum_prob += hosp_probs[i];
      if(rand < cum_prob) {
	// printf("picked i = %d %f\n", i, hosp_probs[i]);
        return static_cast<Hospital*>(possible_hosp[i]);
      }
      ++i;
    }
    printf("HOSP CATCHMENT picked default i = %d %f\n", number_hospitals-1, hosp_probs[number_hospitals-1]);
    return static_cast<Hospital*>(possible_hosp[number_hospitals - 1]);
  } else {
    //No hospitals in the simulation match search criteria
    return NULL;
  }
  */
}


Hospital* Place::get_random_open_healthcare_facility_matching_criteria(int sim_day, Person* per, bool check_insurance, bool use_search_radius_limit) {
  return NULL;
  /*
  if(!Global::Enable_Hospitals) {
    return NULL;
  }

  if(check_insurance) {
    assert(Global::Enable_Health_Insurance);
  }
  assert(per != NULL);

  int daily_hosp_cap = 0;
  Hospital* assigned_hospital = NULL;
  int number_hospitals = get_number_of_hospitals();
  if(number_hospitals == 0) {
    Utils::fred_abort("No Hospitals in simulation that has Enabled Hospitalization", "");
  }
  int number_possible_hospitals = 0;
  Household* hh = per->get_household();
  assert(hh != NULL);
  //First, only try Hospitals within a certain radius (* that accept insurance)
  std::vector<double> hosp_probs;
  double probability_total = 0.0;
  for(int i = 0; i < number_hospitals; ++i) {
    Hospital* hospital = get_hospital(i);
    daily_hosp_cap = hospital->get_daily_patient_capacity(sim_day);
    double distance = Place::distance_between_places(hh, hospital);
    double cur_prob = 0.0;
    int increment = 0;

    //Need to make sure place is open and not over capacity
    if(distance > 0.0 && hospital->should_be_open(sim_day)
       && hospital->get_current_daily_patient_count() < daily_hosp_cap) {
      if(use_search_radius_limit) {
        if(distance <= Place::Hospitalization_radius) {
          if(check_insurance) {
            Insurance_assignment_index::e per_insur = per->get_insurance_type();
            if(hospital->accepts_insurance(per_insur)) {
              //Hospital accepts the insurance so we are good
              cur_prob = static_cast<double>(daily_hosp_cap) / (distance * distance);
              increment = 1;
            } else {
              //Not possible (Doesn't accept insurance)
              cur_prob = 0.0;
              increment = 0;
            }
          } else {
            //We don't care about insurance so good to go
            cur_prob = static_cast<double>(daily_hosp_cap) / (distance * distance);
            increment = 1;
          }
        } else {
          //Not possible (not within the radius)
          cur_prob = 0.0;
          increment = 0;
        }
      } else { //Don't car about search radius
        if(check_insurance) {
          Insurance_assignment_index::e per_insur = per->get_insurance_type();
          if(hospital->accepts_insurance(per_insur)) {
            //Hospital accepts the insurance so we are good
            cur_prob = static_cast<double>(daily_hosp_cap) / (distance * distance);
            increment = 1;
          } else {
            //Not possible (Doesn't accept insurance)
            cur_prob = 0.0;
            increment = 0;
          }
        } else {
          //We don't care about insurance so good to go
          cur_prob = static_cast<double>(daily_hosp_cap) / (distance * distance);
          increment = 1;
        }
      }
    } else {
      //Not possible
      cur_prob = 0.0;
      increment = 0;
    }
    hosp_probs.push_back(cur_prob);
    probability_total += cur_prob;
    number_possible_hospitals += increment;
  } // end for loop

  assert(static_cast<int>(hosp_probs.size()) == number_hospitals);
  if(number_possible_hospitals > 0) {
    if(probability_total > 0.0) {
      for(int i = 0; i < number_hospitals; ++i) {
        hosp_probs[i] /= probability_total;
      }
    }

    double rand = Random::draw_random();
    double cum_prob = 0.0;
    int i = 0;
    while(i < number_hospitals) {
      cum_prob += hosp_probs[i];
      if(rand < cum_prob) {
        return get_hospital(i);
      }
      ++i;
    }
    return get_hospital(number_hospitals - 1);
  } else {
    //No hospitals in the simulation match search criteria
    return NULL;
  }
  */
}


Hospital* Place::get_random_primary_care_facility_matching_criteria(Person* per, bool check_insurance, bool use_search_radius_limit) {
  return NULL;
  /*
  if(!Global::Enable_Hospitals) {
    return NULL;
  }

  if(check_insurance) {
    assert(Global::Enable_Health_Insurance);
  }
  assert(per != NULL);

  //This is the initial primary care assignment
  if(!Place::is_primary_care_assignment_initialized) {
    Place::prepare_primary_care_assignment();
  }

  int daily_hosp_cap = 0;
  Hospital* assigned_hospital = NULL;
  int number_hospitals = get_number_of_hospitals();
  if(number_hospitals == 0) {
    Utils::fred_abort("No Hospitals in simulation that has Enabled Hospitalization", "");
  }
  int number_possible_hospitals = 0;
  Household* hh = per->get_household();
  assert(hh != NULL);
  //First, only try Hospitals within a certain radius (* that accept insurance)
  std::vector<double> hosp_probs;
  double probability_total = 0.0;
  for(int i = 0; i < number_hospitals; ++i) {
    Hospital* hospital = get_hospital(i);
    daily_hosp_cap = hospital->get_daily_patient_capacity(0);
    double distance = Place::distance_between_places(hh, hospital);
    double cur_prob = 0.0;
    int increment = 0;

    //Need to make sure place is open and not over capacity
    if(distance > 0.0 && hospital->should_be_open(0)) {
      if(use_search_radius_limit) {
        if(distance <= Place::Hospitalization_radius) {
          if(check_insurance) {
            Insurance_assignment_index::e per_insur = per->get_insurance_type();
            if(hospital->accepts_insurance(per_insur)) {
              //Hospital accepts the insurance so can check further
              if(Place::Hospital_ID_current_assigned_size_map.at(hospital->get_id())
		 < Place::Hospital_ID_total_assigned_size_map.at(hospital->get_id())) {
                //Hospital accepts the insurance and it hasn't been filled so we are good
                cur_prob = static_cast<double>(daily_hosp_cap) / (distance * distance);
                increment = 1;
              } else {
                //Not possible
                cur_prob = 0.0;
                increment = 0;
              }
            } else {
              //Not possible (Doesn't accept insurance)
              cur_prob = 0.0;
              increment = 0;
            }
          } else {
            //We don't care about insurance so can check further
            if(Place::Hospital_ID_current_assigned_size_map.at(hospital->get_id())
	       < Place::Hospital_ID_total_assigned_size_map.at(hospital->get_id())) {
              //Hospital accepts the insurance and it hasn't been filled so we are good
              cur_prob = static_cast<double>(daily_hosp_cap) / (distance * distance);
              increment = 1;
            } else {
              //Not possible
              cur_prob = 0.0;
              increment = 0;
            }
          }
        } else {
          //Not possible (not within the radius)
          cur_prob = 0.0;
          increment = 0;
        }
      } else { //Don't car about search radius
        if(check_insurance) {
          Insurance_assignment_index::e per_insur = per->get_insurance_type();
          if(hospital->accepts_insurance(per_insur)) {
            //Hospital accepts the insurance so can check further
            if(Place::Hospital_ID_current_assigned_size_map.at(hospital->get_id())
	       < Place::Hospital_ID_total_assigned_size_map.at(hospital->get_id())) {
              //Hospital accepts the insurance and it hasn't been filled so we are good
              cur_prob = static_cast<double>(daily_hosp_cap) / (distance * distance);
              increment = 1;
            } else {
              //Not possible
              cur_prob = 0.0;
              increment = 0;
            }
          } else {
            //Not possible (Doesn't accept insurance)
            cur_prob = 0.0;
            increment = 0;
          }
        } else {
          //We don't care about insurance so can check further
          if(Place::Hospital_ID_current_assigned_size_map.at(hospital->get_id())
	     < Place::Hospital_ID_total_assigned_size_map.at(hospital->get_id())) {
            //Hospital accepts the insurance and it hasn't been filled so we are good
            cur_prob = static_cast<double>(daily_hosp_cap) / (distance * distance);
            increment = 1;
          } else {
            //Not possible
            cur_prob = 0.0;
            increment = 0;
          }
        }
      }
    } else {
      //Not possible
      cur_prob = 0.0;
      increment = 0;
    }
    hosp_probs.push_back(cur_prob);
    probability_total += cur_prob;
    number_possible_hospitals += increment;
  }  // end for loop

  assert(static_cast<int>(hosp_probs.size()) == number_hospitals);
  if(number_possible_hospitals > 0) {
    if(probability_total > 0.0) {
      for(int i = 0; i < number_hospitals; ++i) {
        hosp_probs[i] /= probability_total;
      }
    }

    double rand = Random::draw_random();
    double cum_prob = 0.0;
    int i = 0;
    while(i < number_hospitals) {
      cum_prob += hosp_probs[i];
      if(rand < cum_prob) {
        return get_hospital(i);
      }
      ++i;
    }
    return get_hospital(number_hospitals - 1);
  } else {
    //No hospitals in the simulation match search criteria
    return NULL;
  }
  */
}


void Place::print_household_size_distribution(char* dir, char* date_string, int run) {
  FILE* fp;
  int count[11];
  double pct[11];
  char filename[FRED_STRING_SIZE];
  sprintf(filename, "%s/household_size_dist_%s.%02d", dir, date_string, run);
  Utils::fred_log("print_household_size_dist entered, filename = %s\n", filename);
  for(int i = 0; i < 11; ++i) {
    count[i] = 0;
  }
  int total = 0;
  int number_households = get_number_of_households();
  for(int p = 0; p < number_households; ++p) {
    int n = get_household(p)->get_size();
    if(n < 11) {
      count[n]++;
    } else {
      count[10]++;
    }
    total++;
  }
  fp = fopen(filename, "w");
  for(int i = 0; i < 11; i++) {
    pct[i] = (100.0 * count[i]) / number_households;
    fprintf(fp, "size %d count %d pct %f\n", i * 5, count[i], pct[i]);
  }
  fclose(fp);
}

void Place::delete_place_label_map() {
  if(Place::household_label_map) {
    delete Place::household_label_map;
    Place::household_label_map = NULL;
  }
  if(Place::school_label_map) {
    delete Place::school_label_map;
    Place::school_label_map = NULL;
  }
  if(Place::workplace_label_map) {
    delete Place::workplace_label_map;
    Place::workplace_label_map = NULL;
  }
}

void Place::finish() {
  return;
}

int Place::get_housing_data(int* target_size, int* current_size) {
  int num_households = get_number_of_households();
  for(int i = 0; i < num_households; ++i) {
    Household* h = Place::get_household(i);
    current_size[i] = h->get_size();
    target_size[i] = h->get_original_size();
  }
  return num_households;
}

void Place::swap_houses(int house_index1, int house_index2) {

  Household* h1 = Place::get_household(house_index1);
  Household* h2 = Place::get_household(house_index2);
  if(h1 == NULL || h2 == NULL)
    return;

  FRED_VERBOSE(1, "HOUSING: swapping house %s with %d beds and %d occupants with %s with %d beds and %d occupants\n",
	       h1->get_label(), h1->get_original_size(), h1->get_size(), h2->get_label(), h2->get_original_size(), h2->get_size());

  // get pointers to residents of house h1
  person_vector_t temp1;
  temp1.clear();
  person_vector_t Housemates1 = h1->get_inhabitants();
  for(person_vector_t::iterator itr = Housemates1.begin(); itr != Housemates1.end(); ++itr) {
    temp1.push_back(*itr);
  }

  // get pointers to residents of house h2
  person_vector_t temp2;
  temp2.clear();
  person_vector_t Housemates2 = h2->get_inhabitants();
  for(person_vector_t::iterator itr = Housemates2.begin(); itr != Housemates2.end(); ++itr) {
    temp2.push_back(*itr);
  }

  // move first group into house h2
  for(person_vector_t::iterator itr = temp1.begin(); itr != temp1.end(); ++itr) {
    (*itr)->change_household(h2);
  }

  // move second group into house h1
  for(person_vector_t::iterator itr = temp2.begin(); itr != temp2.end(); ++itr) {
    (*itr)->change_household(h1);
  }

  FRED_VERBOSE(1, "HOUSING: swapped house %s with %d beds and %d occupants with %s with %d beds and %d occupants\n",
	       h1->get_label(), h1->get_original_size(), h1->get_size(), h2->get_label(), h2->get_original_size(), h2->get_size());
}

void Place::swap_houses(Household* h1, Household* h2) {

  if(h1 == NULL || h2 == NULL)
    return;

  FRED_VERBOSE(0, "HOUSING: swapping house %s with %d beds and %d occupants with %s with %d beds and %d occupants\n",
	       h1->get_label(), h1->get_original_size(), h1->get_size(), h2->get_label(), h2->get_original_size(), h2->get_size());

  // get pointers to residents of house h1
  person_vector_t temp1;
  temp1.clear();
  person_vector_t Housemates1 = h1->get_inhabitants();
  for(person_vector_t::iterator itr = Housemates1.begin(); itr != Housemates1.end(); ++itr) {
    temp1.push_back(*itr);
  }

  // get pointers to residents of house h2
  person_vector_t temp2;
  temp2.clear();
  person_vector_t Housemates2 = h2->get_inhabitants();
  for(person_vector_t::iterator itr = Housemates2.begin(); itr != Housemates2.end(); ++itr) {
    temp2.push_back(*itr);
  }

  // move first group into house h2
  for(person_vector_t::iterator itr = temp1.begin(); itr != temp1.end(); ++itr) {
    (*itr)->change_household(h2);
  }

  // move second group into house h1
  for(person_vector_t::iterator itr = temp2.begin(); itr != temp2.end(); ++itr) {
    (*itr)->change_household(h1);
  }

  FRED_VERBOSE(1, "HOUSING: swapped house %s with %d beds and %d occupants with %s with %d beds and %d occupants\n",
	       h1->get_label(), h1->get_original_size(), h1->get_size(), h2->get_label(), h2->get_original_size(), h2->get_size());
}

void Place::combine_households(int house_index1, int house_index2) {

  Household* h1 = Place::get_household(house_index1);
  Household* h2 = Place::get_household(house_index2);
  if(h1 == NULL || h2 == NULL)
    return;

  FRED_VERBOSE(1, "HOUSING: combining house %s with %d beds and %d occupants with %s with %d beds and %d occupants\n",
	       h1->get_label(), h1->get_original_size(), h1->get_size(), h2->get_label(), h2->get_original_size(), h2->get_size());

  // get pointers to residents of house h2
  person_vector_t temp2;
  temp2.clear();
  person_vector_t Housemates2 = h2->get_inhabitants();
  for(person_vector_t::iterator itr = Housemates2.begin(); itr != Housemates2.end(); ++itr) {
    temp2.push_back(*itr);
  }

  // move into house h1
  for(person_vector_t::iterator itr = temp2.begin(); itr != temp2.end(); ++itr) {
    (*itr)->change_household(h1);
  }

  printf("HOUSING: combined house %s with %d beds and %d occupants with %s with %d beds and %d occupants\n",
	 h1->get_label(), h1->get_original_size(), h1->get_size(), h2->get_label(), h2->get_original_size(), h2->get_size());

}

Hospital* Place::get_hospital_assigned_to_household(Household* hh) {
  assert(Place::is_load_completed());
  if(Place::hh_label_hosp_label_map.find(string(hh->get_label())) != Place::hh_label_hosp_label_map.end()) {
    string hosp_label = Place::hh_label_hosp_label_map.find(string(hh->get_label()))->second;
    if(Place::hosp_label_hosp_id_map.find(hosp_label) != Place::hosp_label_hosp_id_map.end()) {
      int hosp_id = Place::hosp_label_hosp_id_map.find(hosp_label)->second;
      return static_cast<Hospital*>(Place::get_hospital(hosp_id));
    } else {
      return NULL;
    }
  } else {
    if(Place::Household_hospital_map_file_exists) {
      //List is incomplete so set this so we can print out a new file
      Place::Household_hospital_map_file_exists = false;
    }

    Hospital* hosp = NULL;
    if(hh->get_size() > 0) {
      Person* per = hh->get_member(0);
      assert(per != NULL);
      if(Global::Enable_Health_Insurance) {
        hosp = Place::get_random_open_hospital_matching_criteria(0, per, true);
      } else {
        hosp = Place::get_random_open_hospital_matching_criteria(0, per, false);
      }

      //If it still came back with nothing, ignore health insurance
      if(hosp == NULL) {
        hosp = Place::get_random_open_hospital_matching_criteria(0, per, false);
      }
    }
    assert(hosp != NULL);
    return hosp;
  }
}

void Place::update_population_dynamics(int day) {

  if(!Global::Enable_Population_Dynamics) {
    return;
  }

  int number_counties = County::get_number_of_counties();
  for(int i = 0; i < number_counties; ++i) {
    County::get_county_with_index(i)->update(day);
  }

}


void Place::update_geo_boundaries(fred::geo lat, fred::geo lon) {
  // update max and min geo coords
  if(lat != 0.0) {
    if(lat < Place::min_lat) {
      Place::min_lat = lat;
    }
    if(Place::max_lat < lat) {
      Place::max_lat = lat;
    }
  }
  if(lon != 0.0) {
    if(lon < Place::min_lon) {
      Place::min_lon = lon;
    }
    if(Place::max_lon < lon) {
      Place::max_lon = lon;
    }
  }
  return;
}

void Place::get_elevation_data() {

  Utils::fred_print_lap_time("Places.get_elevation_started");

  int miny = (int) Global::Simulation_Region->get_min_lat();
  int maxy = 1 + (int) Global::Simulation_Region->get_max_lat();

  int maxw = -( (int) Global::Simulation_Region->get_min_lon() -1);
  int minw = -( (int) Global::Simulation_Region->get_max_lon());

  printf("miny %d maxy %d minw %d maxw %d\n", miny,maxy,minw,maxw);

  // read optional properties
  Property::disable_abort_on_failure();
  
  char elevation_data_dir[FRED_STRING_SIZE];
  sprintf(elevation_data_dir, "none");
  Property::get_property("elevation_data_directory", elevation_data_dir);

  // restore requiring properties
  Property::set_abort_on_failure();

  if (strcmp(elevation_data_dir, "none")==0) {
    return;
  }

  char outdir[FRED_STRING_SIZE];
  sprintf(outdir, "%s/ELEV", Global::Simulation_directory);
  Utils::fred_make_directory(outdir);
  char cmd[FRED_STRING_SIZE];
  char key[FRED_STRING_SIZE];
  char zip_file[FRED_STRING_SIZE];
  char elevation_file[FRED_STRING_SIZE];

  for(int y = miny; y <= maxy; ++y) {
    for(int x = minw; x <= maxw; ++x) {
      sprintf(key, "n%dw%03d", y, x);
      sprintf(zip_file, "%s/%s.zip", elevation_data_dir, key);
      printf("looking for %s\n", zip_file);
      FILE* ftest = fopen(zip_file, "r");
      if(ftest != NULL) {
        fclose(ftest);
        printf("process zip file %s\n", zip_file); fflush(stdout);
        sprintf(elevation_file, "%s/%s.txt", outdir, key);
        printf("elevation_file = |%s|\n", elevation_file); fflush(stdout);
        sprintf(cmd, "rm -f %s", elevation_file);
        // printf("cmd = |%s|\n", cmd); fflush(stdout);
        system(cmd);
        sprintf(cmd, "unzip %s/%s -d %s", elevation_data_dir, key, outdir);
        // printf("cmd = |%s|\n", cmd); fflush(stdout);
        system(cmd);
        FILE* fp = Utils::fred_open_file(elevation_file);
        if(fp != NULL) {
          fred::geo lat, lon;
          double elev;
          double x, y;
          while(fscanf(fp, "%lf %lf %lf ", &x, &y, &elev) == 3) {
            lat = y;
            lon = x;
            // printf("new elevation site %f %f %f\n", y,x,elev);
            Neighborhood_Patch* patch = Global::Neighborhoods->get_patch(lat, lon);
            if(patch != NULL) {
              int row = patch->get_row();
              int col = patch->get_col();
              for(int r = row - 1; r <= row + 1; ++r) {
                for(int c = col -1 ; c <= col + 1; ++c) {
                  Neighborhood_Patch* p = Global::Neighborhoods->get_patch(r,c);
                  if(p != NULL) {
                    // printf("add elevation site %f %f %f to patch %d %d\n", y,x,elev,r,c);
                    p->add_elevation_site(lat, lon, elev);
                  }
                }
              }
            } else {
              // printf("no patch found for elevation site %f %f %f\n", y,x,elev); fflush(stdout);
            }
          }
          fclose(fp);
          Utils::fred_print_lap_time("Places.get_elevation process elevation file");
          unlink(elevation_file);
        } else {
          printf("file %s could not be opened\n", elevation_file); fflush(stdout);
          exit(0);
        }
      }
    }
  }
  Utils::fred_print_lap_time("Places.get_elevation");
}

void Place::update_household_file(char* location_file) {

  char line[FRED_STRING_SIZE];

  // data to fill in from input file
  char label[FRED_STRING_SIZE];
  char admin_code_str[FRED_STRING_SIZE];
  char new_file[FRED_STRING_SIZE];
  double lat;
  double lon;
  double elevation = 0;
  int race;
  int income;

  FILE* fp = Utils::fred_open_file(location_file);
  if (fp == NULL) {
    return;
  }

  sprintf(new_file, "%s-elev", location_file);
  FILE* newfp = fopen(new_file, "w");

  // update header line
  fgets(label, FRED_STRING_SIZE, fp);
  sprintf(label,"sp_id\tstcotrbg\trace\thh_income\tlatitude\tlongitude\televation\n");
  fputs(label, newfp);

  // read first data line
  strcpy(line, "");
  fgets(line, FRED_STRING_SIZE, fp);
  int items = sscanf(line, "%s %s %d %d %lf %lf %lf", label, admin_code_str, &race, &income, &lat, &lon, &elevation);
  
  int n = 0;
  while (6 <= items) {
    elevation = get_household(n)->get_elevation();
    fprintf(newfp, "%s\t%s\t%d\t%d\t%0.7lf\t%0.7lf\t%lf\n", label, admin_code_str, race, income, lat, lon, elevation);

    // get next line
    strcpy(line, "");
    fgets(line, FRED_STRING_SIZE, fp);
    items = sscanf(line, "%s %s %d %d %lf %lf %lf", label, admin_code_str, &race, &income, &lat, &lon, &elevation);
    n++;
  }
  fclose(fp);
  fclose(newfp);

  // cleanup
  char cmd[FRED_STRING_SIZE];
  sprintf(cmd, "mv %s %s", new_file, location_file);
  system(cmd);

  FRED_VERBOSE(0, "finished updating %d households\n", n);
  return;
}

void Place::update_school_file(char* location_file) {

  char line[FRED_STRING_SIZE];

  // data to fill in from input file
  char label[FRED_STRING_SIZE];
  char admin_code_str[FRED_STRING_SIZE];
  char new_file[FRED_STRING_SIZE];
  double lat;
  double lon;
  double elevation = 0;

  FILE* fp = Utils::fred_open_file(location_file);
  if (fp == NULL) {
    return;
  }

  sprintf(new_file, "%s-elev", location_file);
  FILE* newfp = fopen(new_file, "w");

  // update eader line
  fgets(label, FRED_STRING_SIZE, fp);
  sprintf(label,"sp_id\tstco\tlatitude\tlongitude\televation\n");
  fputs(label, newfp);

  // read first data line
  strcpy(line, "");
  fgets(line, FRED_STRING_SIZE, fp);
  int items = sscanf(line, "%s %s %lf %lf %lf", label, admin_code_str, &lat, &lon, &elevation);
  
  int n = 0;
  while (4 <= items) {
    elevation = get_school(n)->get_elevation();
    fprintf(newfp, "%s\t%s\t%0.7lf\t%0.7lf\t%lf\n", label, admin_code_str, lat, lon, elevation);

    // get next line
    strcpy(line, "");
    fgets(line, FRED_STRING_SIZE, fp);
    items = sscanf(line, "%s %s %lf %lf %lf", label, admin_code_str, &lat, &lon, &elevation);
    n++;
  }
  fclose(fp);
  fclose(newfp);

  // cleanup
  char cmd[FRED_STRING_SIZE];
  sprintf(cmd, "mv %s %s", new_file, location_file);
  system(cmd);

  FRED_VERBOSE(0, "finished updating %d schools\n", n);
  return;
}

void Place::update_workplace_file(char* location_file) {

  char line[FRED_STRING_SIZE];

  // data to fill in from input file
  char label[FRED_STRING_SIZE];
  char new_file[FRED_STRING_SIZE];
  double lat;
  double lon;
  double elevation = 0;

  FILE* fp = Utils::fred_open_file(location_file);
  if (fp == NULL) {
    return;
  }

  sprintf(new_file, "%s-elev", location_file);
  FILE* newfp = fopen(new_file, "w");

  // update header line
  fgets(label, FRED_STRING_SIZE, fp);
  sprintf(label,"sp_id\tlatitude\tlongitude\televation\n");
  fputs(label, newfp);

  // read first data line
  strcpy(line, "");
  fgets(line, FRED_STRING_SIZE, fp);
  int items = sscanf(line, "%s %lf %lf %lf", label, &lat, &lon, &elevation);
  
  int n = 0;
  while (3 <= items) {
    elevation = get_workplace(n)->get_elevation();
    fprintf(newfp, "%s\t%0.7lf\t%0.7lf\t%lf\n", label, lat, lon, elevation);

    // get next line
    strcpy(line, "");
    fgets(line, FRED_STRING_SIZE, fp);
    items = sscanf(line, "%s %lf %lf %lf", label, &lat, &lon, &elevation);
    n++;
  }
  fclose(fp);
  fclose(newfp);

  // cleanup
  char cmd[FRED_STRING_SIZE];
  sprintf(cmd, "mv %s %s", new_file, location_file);
  system(cmd);

  FRED_VERBOSE(0, "finished updating %d workplaces\n", n);
  return;
}

void Place::update_hospital_file(char* location_file) {

  char line[FRED_STRING_SIZE];

  // data to fill in from input file
  char label[FRED_STRING_SIZE];
  char new_file[FRED_STRING_SIZE];
  double lat;
  double lon;
  double elevation = 0;
  int workers, physicians, beds;

  FILE* fp = Utils::fred_open_file(location_file);
  if (fp == NULL) {
    return;
  }

  sprintf(new_file, "%s-elev", location_file);
  FILE* newfp = fopen(new_file, "w");

  // update header line
  fgets(label, FRED_STRING_SIZE, fp);
  sprintf(label,"hosp_id\tworkers\tphysicians\tbeds\tlatitude\tlongitude\televation\n");
  fputs(label, newfp);

  // read first data line
  strcpy(line, "");
  fgets(line, FRED_STRING_SIZE, fp);
  int items = sscanf(line, "%s %d %d %d %lf %lf %lf", label, &workers, &physicians, &beds, &lat, &lon, &elevation);
  
  int n = 0;
  while (6 <= items) {
    elevation = get_hospital(n)->get_elevation();
    fprintf(newfp, "%s\t%d\t%d\t%d\t%0.7lf\t%0.7lf\t%lf\n", label, workers, physicians, beds, lat, lon, elevation);

    // get next line
    strcpy(line, "");
    fgets(line, FRED_STRING_SIZE, fp);
    items = sscanf(line, "%s %d %d %d %lf %lf %lf", label, &workers, &physicians, &beds, &lat, &lon, &elevation);
    n++;
  }
  fclose(fp);
  fclose(newfp);

  // cleanup
  char cmd[FRED_STRING_SIZE];
  sprintf(cmd, "mv %s %s", new_file, location_file);
  system(cmd);

  FRED_VERBOSE(0, "finished updating %d hospitals\n", n);
  return;
}

void Place::update_gq_file(char* location_file) {

  char line[FRED_STRING_SIZE];

  // data to fill in from input file
  char label[FRED_STRING_SIZE];
  char type_str[FRED_STRING_SIZE];
  char admin_code[FRED_STRING_SIZE];
  char new_file[FRED_STRING_SIZE];
  int persons;
  double lat;
  double lon;
  double elevation = 0;

  FILE* fp = Utils::fred_open_file(location_file);
  if (fp == NULL) {
    return;
  }

  sprintf(new_file, "%s-elev", location_file);
  FILE* newfp = fopen(new_file, "w");

  // update header line
  fgets(label, FRED_STRING_SIZE, fp);
  sprintf(label,"sp_id\tgq_type\tstcotrbg\tpersons\tlatitude\tlongitude\televation\n");
  fputs(label, newfp);
  fflush(newfp);

  // read first data line
  strcpy(line, "");
  fgets(line, FRED_STRING_SIZE, fp);
  int items = sscanf(line, "%s %s %s %d %lf %lf %lf", label, type_str, admin_code, &persons, &lat, &lon, &elevation);
  
  int n = 0;
  while (6 <= items) {
    elevation = Place::gq[n]->get_elevation();
    fprintf(newfp, "%s\t%c\t%s\t%d\t%0.7lf\t%0.7lf\t%lf\n", label, type_str[0], admin_code, persons, lat, lon, elevation);
    fflush(newfp);

    // get next line
    strcpy(line, "");
    fgets(line, FRED_STRING_SIZE, fp);
    items = sscanf(line, "%s %s %s %d %lf %lf %lf", label, type_str, admin_code, &persons, &lat, &lon, &elevation);
    n++;
  }
  fclose(fp);
  fclose(newfp);

  // cleanup
  char cmd[FRED_STRING_SIZE];
  sprintf(cmd, "mv %s %s", new_file, location_file);
  system(cmd);

  FRED_VERBOSE(0, "finished updating %d group_quarters\n", n);
  return;
}


place_vector_t Place::get_candidate_places(Place* target, int type_id) { 
  int max_dist = Place_Type::get_place_type(type_id)->get_max_dist();

  place_vector_t results;
  place_vector_t tmp;
  Neighborhood_Patch* patch = target->get_patch();
  if (patch == NULL) {
    printf("target %s has bad patch\n", target->get_label());
  }
  for (int dist = 0; dist <= max_dist; dist++) {
    FRED_VERBOSE(1,"get_candidate_places distance = %d\n", dist);
    tmp = patch->get_places_at_distance(type_id, dist);
    for (int i = 0; i < tmp.size(); i++) {
      Place* place = tmp[i];
      FRED_VERBOSE(1, "place %s row %d col %d\n",
		   place->get_label(), place->get_patch()->get_row(), place->get_patch()->get_col()); fflush(stdout);
    }

    // append results from one distance to overall results
    results.insert(std::end(results), std::begin(tmp), std::end(tmp));
  }
  return results;

}

void Place::report_school_distributions(int day) {
  // original size distribution
  int count[21];
  int osize[21];
  int nsize[21];
  // size distribution of schools
  for(int c = 0; c <= 20; ++c) {
    count[c] = 0;
    osize[c] = 0;
    nsize[c] = 0;
  }
  for(int p = 0; p < get_number_of_schools(); ++p) {
    int os = get_school(p)->get_original_size();
    int ns = get_school(p)->get_size();
    int n = os / 50;
    if(n > 20) {
      n = 20;
    }
    count[n]++;
    osize[n] += os;
    nsize[n] += ns;
  }
  fprintf(Global::Statusfp, "SCHOOL SIZE distribution: ");
  for(int c = 0; c <= 20; ++c) {
    fprintf(Global::Statusfp, "%d %d %0.2f %0.2f | ",
	    c, count[c], count[c] ? (1.0 * osize[c]) / (1.0 * count[c]) : 0, count[c] ? (1.0 * nsize[c]) / (1.0 * count[c]) : 0);
  }
  fprintf(Global::Statusfp, "\n");

  return;

  int year = day / 365;
  char filename[FRED_STRING_SIZE];
  FILE* fp = NULL;

  sprintf(filename, "%s/school-%d.txt", Global::Simulation_directory, year);
  fp = fopen(filename, "w");
  for(int p = 0; p < get_number_of_schools(); ++p) {
    Place* h = get_school(p);
    fprintf(fp, "%s original_size %d current_size %d\n", h->get_label(), h->get_original_size()-h->get_staff_size(), h->get_size()-h->get_staff_size());
  }
  fclose(fp);

  sprintf(filename, "%s/work-%d.txt", Global::Simulation_directory, year);
  fp = fopen(filename, "w");
  for(int p = 0; p < get_number_of_workplaces(); ++p) {
    Place* h = get_workplace(p);
    fprintf(fp, "%s original_size %d current_size %d\n", h->get_label(), h->get_original_size(), h->get_size());
  }
  fclose(fp);

  sprintf(filename, "%s/house-%d.txt", Global::Simulation_directory, year);
  fp = fopen(filename, "w");
  for(int p = 0; p < get_number_of_households(); ++p) {
    Place* h = get_household(p);
    fprintf(fp, "%s original_size %d current_size %d\n", h->get_label(), h->get_original_size(), h->get_size());
  }
  fclose(fp);
}

void Place::report_household_distributions() {
  int number_of_households = get_number_of_households();
  if(Global::Verbose > 0) {
    int count[20];
    int total = 0;
    // size distribution of households
    for(int c = 0; c <= 10; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      int n = get_household(p)->get_size();
      if(n <= 10) {
	count[n]++;
      } else {
	count[10]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "Household size distribution: N = %d ", total);
    for(int c = 0; c <= 10; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%) ",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
    
    // original size distribution
    int hsize[20];
    total = 0;
    // size distribution of households
    for(int c = 0; c <= 10; ++c) {
      count[c] = 0;
      hsize[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      int n = get_household(p)->get_original_size();
      int hs = get_household(p)->get_size();
      if(n <= 10) {
	count[n]++;
	hsize[n] += hs;
      } else {
	count[10]++;
	hsize[10] += hs;
      }
      total++;
    }
    fprintf(Global::Statusfp, "Household orig distribution: N = %d ", total);
    for(int c = 0; c <= 10; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%) %0.2f ",
	      c, count[c], (100.0 * count[c]) / total, count[c] ? ((double)hsize[c] / (double)count[c]) : 0.0);
    }
    fprintf(Global::Statusfp, "\n");
  }

  return;

  if(Global::Verbose > 0) {
    int count[100];
    int total = 0;
    // age distribution of heads of households
    for(int c = 0; c < 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      Household* h = get_household(p);
      Person* per = NULL;
      for(int i = 0; i < h->get_size(); ++i) {
	if(h->get_member(i)->is_householder()) {
	  per = h->get_member(i);
	}
      }
      if(per == NULL) {
	FRED_WARNING("Help! No head of household found for household id %d label %s groupquarters: %d\n",
		     h->get_id(), h->get_label(), h->is_group_quarters()?1:0);
	count[0]++;
      } else {
	int a = per->get_age();
	if(a < 100) {
	  count[a]++;
	} else {
	  count[99]++;
	}
	total++;
      }
    }
    fprintf(Global::Statusfp, "\nAge distribution of heads of households: %d households\n", total);
    for(int c = 0; c < 100; ++c) {
      fprintf(Global::Statusfp, "age %2d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    int count[20];
    int total = 0;
    // adult distribution of households
    for (int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      Household* h = get_household(p);
      int n = h->get_adults();
      if(n < 15) {
	count[n]++;
      } else {
	count[14]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "\nHousehold adult size distribution: %d households\n", total);
    for(int c = 0; c < 15; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    int count[20];
    int total = 0;
    // children distribution of households
    for(int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      Household* h = get_household(p);
      int n = h->get_children();
      if(n < 15) {
	count[n]++;
      } else {
	count[14]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "\nHousehold children size distribution: %d households\n", total);
    for (int c = 0; c < 15; c++) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    int count[20];
    int total = 0;
    // adult distribution of households with children
    for(int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      Household* h = get_household(p);
      if(h->get_children() == 0) {
	continue;
      }
      int n = h->get_adults();
      if(n < 15) {
	count[n]++;
      } else {
	count[14]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "\nHousehold w/ children, adult size distribution: %d households\n", total);
    for(int c = 0; c < 15; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  /*
  // household_relationship between children and decision maker
  if(Global::Verbose > 1 && Global::Enable_Behaviors) {
  // find adult decision maker for each child
  for(int p = 0; p < number_of_households; ++p) {
  Household* h = get_household(p);
  if(h->get_children() == 0) {
  continue;
  }
  int size = h->get_size();
  for(int i = 0; i < size; ++i) {
  Person* child = h->get_member(i);
  int ch_age = child->get_age();
  if (ch_age < 18) {
  int ch_rel = child->get_household_relationship();
  Person* dm = child->get_health_decision_maker();
  if(dm == NULL) {
  printf("DECISION_MAKER: household %d %s  child: %d %d is making own health decisions\n",
  h->get_id(), h->get_label(), ch_age, ch_rel);
  } else {
  int dm_age = dm->get_age();
  int dm_rel = dm->get_household_relationship();
  if(dm_rel != 1 || ch_rel != 3) {
  printf("DECISION_MAKER: household %d %s  decision_maker: %d %d child: %d %d\n",
  h->get_id(), h->get_label(), dm_age, dm_rel, ch_age, ch_rel);
  }
  }
  }
  }
  }
  }
  */

  if(Global::Verbose > 0) {
    int count[100];
    int total = 0;
    // age distribution of heads of households
    for(int c = 0; c < 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      Household* h = get_household(p);
      Person* per = NULL;
      for(int i = 0; i < h->get_size(); ++i) {
	if(h->get_member(i)->is_householder()) {
	  per = h->get_member(i);
	}
      }
      if(per == NULL) {
	FRED_WARNING("Help! No head of household found for household id %d label %s groupquarters: %d\n",
		     h->get_id(), h->get_label(), h->is_group_quarters() ? 1 : 0);
	count[0]++;
      } else {
	int a = per->get_age();
	if(a < 100) {
	  count[a]++;
	} else {
	  count[99]++;
	}
	total++;
      }
    }
    fprintf(Global::Statusfp, "\nAge distribution of heads of households: %d households\n", total);
    for(int c = 0; c < 100; ++c) {
      fprintf(Global::Statusfp, "age %2d: %6d (%.2f%%)\n",
	      c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    int count[100];
    int total = 0;
    int children = 0;
    // age distribution of heads of households with children
    for(int c = 0; c < 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      Household* h = get_household(p);
      Person* per = NULL;
      if(h->get_children() == 0){
        continue;
      }
      children += h->get_children();
      for(int i = 0; i < h->get_size(); ++i) {
        if(h->get_member(i)->is_householder()) {
          per = h->get_member(i);
        }
      }
      if(per == NULL) {
        FRED_WARNING("Help! No head of household found for household id %d label %s groupquarters: %d\n",
            h->get_id(), h->get_label(), h->is_group_quarters()?1:0);
        count[0]++;
      } else {
        int a = per->get_age();
        if(a < 100) {
          count[a]++;
        } else {
          count[99]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nAge distribution of heads of households with children: %d households\n", total);
    for(int c = 0; c < 100; ++c) {
      fprintf(Global::Statusfp, "age %2d: %6d (%.2f%%)\n", c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "children = %d\n", children);
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    int count_has_school_age = 0;
    int count_has_school_age_and_unemployed_adult = 0;
    int total_hh = 0;

    //Households with school-age children and at least one unemployed adult
    for(int p = 0; p < number_of_households; ++p) {
      Household* h = get_household(p);
      Person* per = NULL;
      total_hh++;
      if(h->get_children() == 0) {
        continue;
      }
      if(h->has_school_aged_child()) {
        count_has_school_age++;
      }
      if(h->has_school_aged_child_and_unemployed_adult()) {
        count_has_school_age_and_unemployed_adult++;
      }
    }
    fprintf(Global::Statusfp, "\nHouseholds with school-aged children and at least one unemployed adult\n");
    fprintf(Global::Statusfp, "Total Households: %d\n", total_hh);
    fprintf(Global::Statusfp, "Total Households with school-age children: %d\n", count_has_school_age);
    fprintf(Global::Statusfp, "Total Households with school-age children and at least one unemployed adult: %d\n", count_has_school_age_and_unemployed_adult);
  }

}

///
/// QUALITY CONTROL 
///

void Place::quality_control() {
  //Can't do the quality control until all of the population files have been read
  assert(Person::is_load_completed());

  int number_of_households = get_number_of_households();
  int number_of_schools = get_number_of_schools();
  int number_of_neighborhoods = get_number_of_neighborhoods();
  int number_of_workplaces = get_number_of_workplaces();

  FRED_STATUS(0, "places quality control check for places\n");

  if(Global::Verbose > 0) {
    int hn = 0;
    int nn = 0;
    int sn = 0;
    int wn = 0;
    double hsize = 0.0;
    double nsize = 0.0;
    double ssize = 0.0;
    double wsize = 0.0;
    // mean size by place type

    for(int p = 0; p < number_of_households; ++p) {
      int n = get_household(p)->get_size();
      hn++;
      hsize += n;
    }

    for(int p = 0; p < number_of_neighborhoods; ++p) {
      int n = get_neighborhood(p)->get_size();
      nn++;
      nsize += n;
    }

    for(int p = 0; p < number_of_schools; ++p) {
      int n = get_school(p)->get_size();
      sn++;
      ssize += n;
    }

    for(int p = 0; p < number_of_workplaces; ++p) {
      int n = get_workplace(p)->get_size();
      wn++;
      wsize += n;
    }

    if(hn) {
      hsize /= hn;
    }
    if(nn) {
      nsize /= nn;
    }
    if(sn) {
      ssize /= sn;
    }
    if(wn) {
      wsize /= wn;
    }
    fprintf(Global::Statusfp, "\nMEAN PLACE SIZE: H %.2f N %.2f S %.2f W %.2f\n",
	    hsize, nsize, ssize, wsize);
  }

  if(Global::Verbose > 1) {
    char filename[FRED_STRING_SIZE];
    sprintf(filename, "%s/houses.dat", Global::Simulation_directory);
    FILE* fp = fopen(filename, "w");
    for(int p = 0; p < number_of_households; p++) {
      Place* h = get_household(p);
      double x = Geo::get_x(h->get_longitude());
      double y = Geo::get_y(h->get_latitude());
      fprintf(fp, "%f %f\n", x, y);
    }
    fclose(fp);
  }

  // household type
  int htypes = 21;
  enum htype_t {
    SINGLE_FEMALE,
    SINGLE_MALE,
    OPP_SEX_SIM_AGE_PAIR,
    OPP_SEX_DIF_AGE_PAIR,
    OPP_SEX_TWO_PARENTS,
    SINGLE_PARENT,
    SINGLE_PAR_MULTI_GEN_FAMILY,
    TWO_PAR_MULTI_GEN_FAMILY,
    UNATTENDED_KIDS,
    OTHER_FAMILY,
    YOUNG_ROOMIES,
    OLDER_ROOMIES,
    MIXED_ROOMIES,
    SAME_SEX_SIM_AGE_PAIR,
    SAME_SEX_DIF_AGE_PAIR,
    SAME_SEX_TWO_PARENTS,
    DORM_MATES,
    CELL_MATES,
    BARRACK_MATES,
    NURSING_HOME_MATES,
    UNKNOWN,
  };

  string htype[21] = {
    "single-female",
    "single-male",
    "opp-sex-sim-age-pair",
    "opp-sex-dif-age-pair",
    "opp-sex-two-parent-family",
    "single-parent-family",
    "single-parent-multigen-family",
    "two-parent-multigen-family",
    "unattended-minors",
    "other-family",
    "young-roomies",
    "older-roomies",
    "mixed-roomies",
    "same-sex-sim-age-pair",
    "same-sex-dif-age-pair",
    "same-sex-two-parent-family",
    "dorm-mates",
    "cell-mates",
    "barrack-mates",
    "nursing-home_mates",
    "unknown",
  };

  int type[htypes];
  int ttotal[htypes];
  int hnum = 0;
  for(int i = 0; i < htypes; ++i) {
    type[i] = 0;
    ttotal[i] = 0;
  }

  for(int p = 0; p < number_of_households; ++p) {
    Person* per = NULL;
    hnum++;
    Household* h = get_household(p);
    int t = h->get_orig_household_structure();
    type[t]++;
    ttotal[t] += h->get_size();
  }

  printf("HOUSEHOLD_TYPE DISTRIBUTION\n");
  for(int t = 0; t < htypes; ++t) {
    printf("HOUSEHOLD TYPE DIST: ");
    printf("%30s: %8d households (%5.1f%%) with %8d residents (%5.1f%%)\n",
        htype[t].c_str(), type[t],
        (100.0*type[t])/hnum, ttotal[t],
        (100.0*ttotal[t]/Person::get_population_size()));
  }

//  if(false) {
//    FILE* hfp = fopen("households.txt", "w");
//    FILE* pfp = fopen("people.txt", "w");
//    int pi = 0;
//    for(int p = 0; p < number_of_households; ++p) {
//      Household* house = get_household(p);
//      fprintf(hfp, "hid %d household %s size %d htype %s\n",
//          p, house->get_label(), house->get_size(), house->get_orig_household_structure_label());
//      int hsize = house->get_size();
//      for(int j = 0; j < hsize; ++j) {
//        Person* person = house->get_member(j);
//        fprintf(pfp, "pid %d person %d age %d sex %c household %s htype %s\n",
//            pi++, person->get_id(), person->get_age(), person->get_sex(),
//            person->get_household()->get_label(),
//            person->get_household_structure_label());
//      }
//    }
//    fclose(hfp);
//    fclose(pfp);
//  }

  if(Global::Verbose > 0) {
    int count[20];
    int total = 0;
    // size distribution of households
    for(int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      int n = get_household(p)->get_size();
      if(n < 15) {
        count[n]++;
      } else {
        count[14]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "\nHousehold size distribution: %d households\n", total);
    for(int c = 0; c < 15; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n", c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    int count[20];
    int total = 0;
    // adult distribution of households
    for(int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      Household* h = get_household(p);
      int n = h->get_adults();
      if(n < 15) {
        count[n]++;
      } else {
        count[14]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "\nHousehold adult size distribution: %d households\n", total);
    for(int c = 0; c < 15; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n", c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    int count[20];
    int total = 0;
    // children distribution of households
    for (int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      Household* h = get_household(p);
      int n = h->get_children();
      if(n < 15) {
        count[n]++;
      } else {
        count[14]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "\nHousehold children size distribution: %d households\n", total);
    for(int c = 0; c < 15; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",  c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    int count[20];
    int total = 0;
    // adult distribution of households with children
    for(int c = 0; c < 15; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      Household* h = get_household(p);
      if(h->get_children() == 0) {
        continue;
      }
      int n = h->get_adults();
      if(n < 15) {
        count[n]++;
      } else {
        count[14]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "\nHousehold w/ children, adult size distribution: %d households\n", total);
    for(int c = 0; c < 15; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n", c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    int count[100];
    int total = 0;
    // age distribution of heads of households
    for(int c = 0; c < 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      Person* per = NULL;
      Household* h = get_household(p);
      for(int i = 0; i < h->get_size(); ++i) {
        if(h->get_member(i)->is_householder()) {
          per = h->get_member(i);
        }
      }
      if(per == NULL) {
        FRED_WARNING("Help! No head of household found for household id %d label %s size %d groupquarters: %d\n",
            h->get_id(), h->get_label(), h->get_size(), h->is_group_quarters() ? 1 : 0);
        count[0]++;
      } else {
        int a = per->get_age();
        if(a < 100) {
          count[a]++;
        } else {
          count[99]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nAge distribution of heads of households: %d households\n", total);
    for(int c = 0; c < 100; ++c) {
      fprintf(Global::Statusfp, "age %2d: %6d (%.2f%%)\n", c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    int count[100];
    int total = 0;
    int children = 0;
    // age distribution of heads of households with children
    for(int c = 0; c < 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_households; ++p) {
      Person* per = NULL;
      Household* h = get_household(p);
      if(h->get_children() == 0) {
        continue;
      }
      children += h->get_children();
      for(int i = 0; i < h->get_size(); ++i) {
        if(h->get_member(i)->is_householder()) {
          per = h->get_member(i);
        }
      }
      if(per == NULL) {
        FRED_WARNING("Help! No head of household found for household id %d label %s groupquarters: %d\n",
            h->get_id(), h->get_label(), h->is_group_quarters() ? 1 : 0);
        count[0]++;
      } else {
        int a = per->get_age();
        if(a < 100) {
          count[a]++;
        } else {
          count[99]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nAge distribution of heads of households with children: %d households\n", total);
    for(int c = 0; c < 100; ++c) {
      fprintf(Global::Statusfp, "age %2d: %6d (%.2f%%)\n", c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "children = %d\n", children);
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    int count_has_school_age = 0;
    int count_has_school_age_and_unemployed_adult = 0;
    int total_hh = 0;

    //Households with school-age children and at least one unemployed adult
    for(int p = 0; p < number_of_households; ++p) {
      Person* per = NULL;
      Household* h = get_household(p);
      total_hh++;
      if(h->get_children() == 0) {
        continue;
      }
      if(h->has_school_aged_child()) {
        count_has_school_age++;
      }
      if(h->has_school_aged_child_and_unemployed_adult()) {
        count_has_school_age_and_unemployed_adult++;
      }
    }
    fprintf(Global::Statusfp, "\nHouseholds with school-aged children and at least one unemployed adult\n");
    fprintf(Global::Statusfp, "Total Households: %d\n", total_hh);
    fprintf(Global::Statusfp, "Total Households with school-age children: %d\n", count_has_school_age);
    fprintf(Global::Statusfp, "Total Households with school-age children and at least one unemployed adult: %d\n", count_has_school_age_and_unemployed_adult);
  }

  if(Global::Verbose > 0) {
    int count[100];
    int total = 0;
    int tot_students = 0;
    // size distribution of schools
    for(int c = 0; c < 20; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_schools; ++p) {
      int s = get_school(p)->get_size();
      tot_students += s;
      int n = s / 50;
      if(n < 20) {
        count[n]++;
      } else {
        count[19]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "\nSchool size distribution: %d schools %d students\n", total, tot_students);
    for(int c = 0; c < 20; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n", (c + 1) * 50, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    // age distribution in schools
    fprintf(Global::Statusfp, "\nSchool age distribution:\n");
    int count[100];
    for(int c = 0; c < 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_schools; ++p) {
      for(int c = 0; c < 100; ++c) {
        count[c] += Place::get_school(p)->get_size_by_age(c);
      }
    }
    for(int c = 0; c < 100; ++c) {
      fprintf(Global::Statusfp, "age = %2d  students = %6d\n", c, count[c]);
    }
    fprintf(Global::Statusfp, "\n");
  }

  if(Global::Verbose > 0) {
    int count[101];
    int small_employees = 0;
    int med_employees = 0;
    int large_employees = 0;
    int xlarge_employees = 0;
    int total_employees = 0;
    int total = 0;
    // size distribution of workplaces
    for(int c = 0; c <= 100; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_workplaces; ++p) {
      int s = get_workplace(p)->get_size();
      if(s <= 100) {
        count[s]++;
      } else {
        count[100]++;
      }
      if(s < 50) {
        small_employees += s;
      } else if (s < 100) {
        med_employees += s;
      } else if (s < 500) {
        large_employees += s;
      } else {
        xlarge_employees += s;
      }
      total_employees += s;
      total++;
    }
    for(int p = 0; p < number_of_schools; ++p) {
      int s = get_school(p)->get_staff_size();
      if(s <= 100) {
        count[s]++;
      } else {
        count[100]++;
      }
      if(s < 50) {
        small_employees += s;
      } else if (s < 100) {
        med_employees += s;
      } else if (s < 500) {
        large_employees += s;
      } else {
        xlarge_employees += s;
      }
      total_employees += s;
      total++;
    }
    fprintf(Global::Statusfp, "\nWorkplace size distribution: %d workplaces\n", total);
    for(int c = 0; c <= 100; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
	      (c+1)*1, count[c], (100.0*count[c])/total);
    }
    fprintf(Global::Statusfp, "\n\n");

    fprintf(Global::Statusfp, "employees at small workplaces (1-49): ");
    fprintf(Global::Statusfp, "%d\n", small_employees);

    fprintf(Global::Statusfp, "employees at medium workplaces (50-99): ");
    fprintf(Global::Statusfp, "%d\n", med_employees);

    fprintf(Global::Statusfp, "employees at large workplaces (100-499): ");
    fprintf(Global::Statusfp, "%d\n", large_employees);

    fprintf(Global::Statusfp, "employees at xlarge workplaces (500-up): ");
    fprintf(Global::Statusfp, "%d\n", xlarge_employees);

    fprintf(Global::Statusfp, "total employees: %d\n\n", total_employees);
  }

  if(Global::Verbose > 0) {
    int count[60];
    int total = 0;
    // size distribution of offices
    for(int c = 0; c < 60; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < number_of_workplaces; ++p) {
      Place* w = get_workplace(p);
      for(int off = 0; off < w->get_number_of_partitions(); ++off) {
        int s = w->get_partition(off)->get_size();
        int n = s;
        if(n < 60) {
          count[n]++;
        } else {
          count[60 - 1]++;
        }
        total++;
      }
    }
    fprintf(Global::Statusfp, "\nOffice size distribution: %d offices\n", total);
    for(int c = 0; c < 60; ++c) {
      fprintf(Global::Statusfp, "%3d: %6d (%.2f%%)\n",
          c, count[c], (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "\n");
  }
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "places quality control finished\n");
    fflush(Global::Statusfp);
  }
}

void Place::setup_partitions(int partition_type_id, int partition_capacity, string partition_basis, int min_age_partition, int max_age_partition) {

  if(partition_type_id < 0) {
    return;
  }

  if(partition_capacity == 0) {
    return;
  }

  long long int sp_id = 0;
  int size = this->get_size();

  if(partition_basis == "age") {

    // find size of each age group
    this->original_size_by_age = new int [Demographics::MAX_AGE + 1];
    this->partitions_by_age = new place_vector_t[Demographics::MAX_AGE + 1];
    int next_partition_by_age[Demographics::MAX_AGE+1];
    for(int a = 0; a <= Demographics::MAX_AGE; ++a) {
      this->original_size_by_age[a] = 0;
      this->partitions_by_age[a].clear();
      next_partition_by_age[a] = 0;
    }
    for(int i = 0; i < size; ++i) {
      this->original_size_by_age[get_member(i)->get_age()]++;
    }

    // create each partition
    for(int a = 0; a <= max_age_partition; ++a) {
      int n = this->original_size_by_age[a];
      if(n == 0) {
        continue;
      }
      int rooms = n / partition_capacity;
      if(n % partition_capacity) {
        rooms++;
      }
      FRED_STATUS(1, "place %d %s age %d number %d rooms %d\n", this->get_id(), this->get_label(), a, n, rooms);
      for(int c = 0; c < rooms; ++c) {
        char label[FRED_STRING_SIZE];
        sprintf(label, "%s-%02d-%02d", this->get_label(), a, c + 1);
        Place* partition = Place::add_place(label, partition_type_id, Place::SUBTYPE_NONE, this->get_longitude(), this->get_latitude(),
            this->get_elevation(), this->get_admin_code());
        sp_id = this->sp_id * 1000000 + 10000 * a + (c + 1);
        partition->set_sp_id(sp_id);
        this->partitions.push_back(partition);
        this->partitions_by_age[a].push_back(partition);
        partition->set_container(this);
        Global::Neighborhoods->add_place(partition);
        FRED_VERBOSE(1, "CREATE PARTITIONS place %d %s added partition %s %d\n", this->get_id(), this->get_label(),
            partition->get_label(), partition->get_id());
      }
    }

    // assign partition to each member, round robin
    for(int i = 0; i < size; ++i) {
      Person* person = get_member(i);
      int age = person->get_age();
      int room = next_partition_by_age[age];
      if(room < (int)this->partitions_by_age[age].size() - 1) {
        next_partition_by_age[age]++;
      } else {
        next_partition_by_age[age] = 0;
      }
      FRED_VERBOSE(1, "room = %d %s %d\n", room,
          this->partitions_by_age[age][room]->get_label(),
          this->partitions_by_age[age][room]->get_id());
      person->set_place_of_type(partition_type_id, this->partitions_by_age[age][room]);
    }
  }

  if(partition_basis == "random") {
    // determine number of partitions
    int parts = size / partition_capacity;
    if(size % partition_capacity) {
      parts++;
    }
    if(parts == 0) {
      ++parts;
    }
    FRED_VERBOSE(1, "CREATE PARTITIONS Place %d %s number %d partitions %d  partition_type_id %d\n",
        this->get_id(), this->get_label(), size, parts, partition_type_id);

    // create each partition
    for(int i = 0; i < parts; ++i) {
      char label[FRED_STRING_SIZE];
      sprintf(label, "%s-%03d", this->get_label(), i + 1);
      Place* partition = Place::add_place(label, partition_type_id, Place::SUBTYPE_NONE,
          this->get_longitude(), this->get_latitude(),
          this->get_elevation(), this->get_admin_code());
      sp_id = this->sp_id * 10000 + (i + 1);
      partition->set_sp_id(sp_id);
      this->partitions.push_back(partition);
      partition->set_container(this);
      Global::Neighborhoods->add_place(partition);
      FRED_VERBOSE(1, "CREATE PARTITIONS place %d %s added partition %s %d\n", this->get_id(), this->get_label(), partition->get_label(), partition->get_id());
    }

    // assign each member to a random partition
    for(int i = 0; i < size; ++i) {
      Person* person = get_member(i);
      select_partition(person);
    }
  }

  return;
}


Place* Place::select_partition(Person* person) {
  string partition_basis = Place_Type::get_place_type(this->type_id)->get_partition_basis();
  int partition_type_id = Place_Type::get_place_type(this->type_id)->get_partition_type_id();
  Place* partition = NULL;
  int room;
  if(partition_basis == "age") {
    int age = person->get_age();
    room = Random::draw_random_int(0, this->partitions_by_age[age].size()-1);
    partition = this->partitions_by_age[age][room];
  }
  if(partition_basis == "random") {
    room = Random::draw_random_int(0, this->partitions.size()-1);
    partition = this->partitions[room];
  }
  FRED_VERBOSE(1, "room = %d %s %d\n", room,
      partition->get_label(), partition->get_id());
  person->set_place_of_type(partition_type_id, partition);
  return(partition);
}


bool Place::is_open(int day) {

  FRED_VERBOSE(1, "is_open: check place %s on day %d\n", this->get_label(), day);

  // place is closed if container is closed:
  if(this->container != NULL) {
    if(this->container->is_open(day) == false) {
      FRED_STATUS(1, "day %d place %s is closed because container %s is closed\n", day, this->get_label(), this->container->get_label());
      return false;
    } else {
      FRED_STATUS(1, "day %d place %s container %s is open\n", day, this->get_label(), this->container->get_label());
    }
  }

  // see if base class is open
  return Group::is_open();
}

bool Place::has_admin_closure() {

  int day = Global::Simulation_Day;

  FRED_VERBOSE(1, "has_admin_closure: check place %s on day %d\n", this->get_label(), day);

  // place is closed if container is closed:
  if(this->container != NULL) {
    if(this->container->has_admin_closure()) {
      FRED_STATUS(1, "day %d place %s is closed because container %s is closed\n", day, this->get_label(), this->container->get_label());
      return true;
    } else {
      FRED_STATUS(1, "day %d place %s container %s is open\n", day, this->get_label(), this->container->get_label());
    }
  }

  // see if base class has a closure
  return Group::has_admin_closure();
}

void Place::update_elevations() {

  if (Place::Update_elevation) {

    // add elevation sites to the appropriate neighborhood patches
    Place::get_elevation_data();

    // get elevation info for each place
    Neighborhood_Patch* patch;
    for(int p = 0; p < get_number_of_households(); ++p) {
      Place* place = get_household(p);
      fred::geo lat = place->get_latitude();
      fred::geo lon = place->get_longitude();
      patch = Global::Neighborhoods->get_patch(lat, lon);
      if(patch != NULL) {
        place->set_elevation(patch->get_elevation(lat, lon));
      }
      // printf("household %d %s has elevation %f\n", p, place->get_label(), place->get_elevation());
    }
    for(int p = 0; p < Place::get_number_of_schools(); ++p) {
      Place* place = get_school(p);
      fred::geo lat = place->get_latitude();
      fred::geo lon = place->get_longitude();
      patch = Global::Neighborhoods->get_patch(lat, lon);
      if(patch != NULL) {
        place->set_elevation(patch->get_elevation(lat, lon));
      }
      // printf("school %s has elevation %f\n", place->get_label(), place->get_elevation());
    }
    for(int p = 0; p < get_number_of_workplaces(); ++p) {
      Place* place = get_workplace(p);
      fred::geo lat = place->get_latitude();
      fred::geo lon = place->get_longitude();
      patch = Global::Neighborhoods->get_patch(lat, lon);
      if(patch != NULL) {
        place->set_elevation(patch->get_elevation(lat, lon));
      }
      // printf("workplace %s has elevation %f\n", place->get_label(), place->get_elevation());
    }
    for(int p = 0; p < Place::get_number_of_hospitals(); ++p) {
      Place* place = get_hospital(p);
      fred::geo lat = place->get_latitude();
      fred::geo lon = place->get_longitude();
      patch = Global::Neighborhoods->get_patch(lat, lon);
      if(patch != NULL) {
        place->set_elevation(patch->get_elevation(lat, lon));
      }
      // printf("hospital %s has elevation %f\n", place->get_label(), place->get_elevation());
    }

    // update input files for each specified location
    int locs = get_number_of_location_ids();
    for(int i = 0; i < locs; ++i) {
      char loc_id[FRED_STRING_SIZE];
      char loc_dir[FRED_STRING_SIZE];
      char location_file[FRED_STRING_SIZE];
      strcpy(loc_id, get_location_id(i));
      sprintf(loc_dir, "%s/%s/%s/%s", Place::Population_directory, Place::Country, Place::Population_version, loc_id);

      // update household locations
      sprintf(location_file, "%s/households.txt", loc_dir);
      Place::update_household_file(location_file);
      
      // update school locations
      sprintf(location_file, "%s/schools.txt", loc_dir);
      Place::update_school_file(location_file);
      
      // update workplace locations
      sprintf(location_file, "%s/workplaces.txt", loc_dir);
      Place::update_workplace_file(location_file);
      
      // update gq locations
      sprintf(location_file, "%s/gq.txt", loc_dir);
      Place::update_gq_file(location_file);

      sprintf(location_file, "touch %s/UPDATED", loc_dir);
      system(location_file);
    }

    Utils::fred_print_lap_time("Places.update_elevations");

    // terminate
    exit(0);
  }
}

void Place::set_partition_elevation(double elev) {
  // set elevation of partitions
  int rooms = this->partitions.size();
  for(int i = 0; i < rooms; ++i) {
    this->partitions[i]->set_elevation(elev);
    this->partitions[i]->set_partition_elevation(elev);
  }
}

bool Place::is_low_vaccination_place() {
  return this->vaccination_rate < Place_Type::get_place_type(this->type_id)->get_default_vaccination_rate();
}


void Place::prepare_vaccination_rates() {

  Place_Type* place_type = Place_Type::get_place_type(this->type_id);
  place_type->prepare_vaccination_rates();

  // set vaccination rate for this place
  if(place_type->is_vaccination_rate_enabled()) {

    if(this->vaccination_rate < 0.0) {
      this->vaccination_rate = place_type->get_default_vaccination_rate();
    }

    // randomize the order of processing the members
    std::vector<int> shuffle_index;
    shuffle_index.reserve(get_size());
    for(int i = 0; i < get_size(); ++i) {
      shuffle_index.push_back(i);
    }
    FYShuffle<int>(shuffle_index);
    
    int ineligibles = place_type->get_medical_vacc_exempt_rate() * get_size();
    for(int i = 0; i < ineligibles; ++i) {
      Person* person = this->members[shuffle_index[i]];
      person->set_ineligible_for_vaccine();
    }
    
    int refusers = ((1.0 - place_type->get_medical_vacc_exempt_rate()) - this->vaccination_rate) * get_size();
    if(refusers < 0) {
      refusers = 0;
    }
    for(int i = ineligibles; i < get_size() && i < ineligibles + refusers; ++i) {
      Person* person = this->members[shuffle_index[i]];
      person->set_vaccine_refusal();
      // printf("PREP_VAX: refuser person %d status %d place %s size %d\n", person->get_id(), person->refuses_vaccines(), this->label, get_size());
    }
    
    int receivers = 0;
    for(int i = ineligibles + refusers; i < get_size(); ++i) {
      Person* person = this->members[shuffle_index[i]];
      person->set_received_vaccine();
      receivers++;
    }      
    
    printf("PREP_VAX: place %s coverage %0.2f size %d ineligibles = %d refusers = %d received = %d\n", 
        this->label, this->vaccination_rate, get_size(), ineligibles, refusers, receivers);
  }
  
}

Place* Place::get_place_from_sp_id(long long int n) {
  return static_cast<Place*>(Group::get_group_from_sp_id(n));
}
