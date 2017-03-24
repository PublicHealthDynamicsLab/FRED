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
// File: Place_List.cc
//
#include <algorithm>
#include <iostream>
#include <limits>
#include <math.h>
#include <new>
#include <set>
#include <string>
#include <typeinfo>
#include <unistd.h>

#include "Census_Tract.h"
#include "Classroom.h"
#include "Condition.h"
#include "County.h"
#include "Date.h"
#include "Geo.h"
#include "Global.h"
#include "Hospital.h"
#include "Household.h"
#include "Neighborhood.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include "Office.h"
#include "Params.h"
#include "Person.h"
#include "Place.h"
#include "Place_List.h"
#include "Population.h"
#include "Regional_Layer.h"
#include "Regional_Patch.h"
#include "Random.h"
#include "School.h"
#include "Seasonality.h"
#include "Tracker.h"
#include "Travel.h"
#include "Utils.h"
#include "Visualization_Layer.h"
#include "Workplace.h"

// Place_List::quality_control implementation is very large,
// include from separate .cc file:
#include "Place_List_Quality_Control.cc"

using namespace std;

typedef std::map<int, int> HospitalIDCountMapT;

bool Place_List::Static_variables_set = false;
char Place_List::Population_directory[FRED_STRING_SIZE];
char Place_List::Country[FRED_STRING_SIZE];
char Place_List::Population_version[FRED_STRING_SIZE];

// mean size of "household" associated with group quarters
double Place_List::College_dorm_mean_size = 3.5;
double Place_List::Military_barracks_mean_size = 12;
double Place_List::Prison_cell_mean_size = 1.5;
double Place_List::Nursing_home_room_mean_size = 1.5;

// non-resident staff for group quarters
int Place_List::College_fixed_staff = 0;
double Place_List::College_resident_to_staff_ratio = 0;
int Place_List::Prison_fixed_staff = 0;
double Place_List::Prison_resident_to_staff_ratio = 0;
int Place_List::Nursing_home_fixed_staff = 0;
double Place_List::Nursing_home_resident_to_staff_ratio = 0;
int Place_List::Military_fixed_staff = 0;
double Place_List::Military_resident_to_staff_ratio = 0;
int Place_List::School_fixed_staff = 0;
double Place_List::School_student_teacher_ratio = 0;

bool Place_List::Household_hospital_map_file_exists = false;
int Place_List::Hospital_fixed_staff = 1.0;
double Place_List::Hospital_worker_to_bed_ratio = 1.0;
double Place_List::Hospital_outpatients_per_day_per_employee = 0.0;
double Place_List::Healthcare_clinic_outpatients_per_day_per_employee = 0;
int Place_List::Hospital_min_bed_threshold = 0;
double Place_List::Hospitalization_radius = 0.0;
int Place_List::Hospital_overall_panel_size = 0;

vector<string> Place_List::location_id;

HospitalIDCountMapT Place_List::Hospital_ID_total_assigned_size_map;
HospitalIDCountMapT Place_List::Hospital_ID_current_assigned_size_map;

double distance_between_places(Place* p1, Place* p2) {
  return Geo::xy_distance(p1->get_latitude(), p1->get_longitude(), p2->get_latitude(), p2->get_longitude());
}

Place_List::~Place_List() {
  delete_place_label_map();
}

void Place_List::init_place_type_name_lookup_map() {
  this->place_type_name_lookup_map[Place::TYPE_NEIGHBORHOOD] = "NEIGHBORHOOD";
  this->place_type_name_lookup_map[Place::TYPE_HOUSEHOLD] = "HOUSEHOLD";
  this->place_type_name_lookup_map[Place::TYPE_SCHOOL] = "SCHOOL";
  this->place_type_name_lookup_map[Place::TYPE_CLASSROOM] = "CLASSROOM";
  this->place_type_name_lookup_map[Place::TYPE_WORKPLACE] = "WORKPLACE";
  this->place_type_name_lookup_map[Place::TYPE_OFFICE] = "OFFICE";
  this->place_type_name_lookup_map[Place::TYPE_HOSPITAL] = "HOSPITAL";
  this->place_type_name_lookup_map[Place::TYPE_COMMUNITY] = "COMMUNITY";
}

void Place_List::get_parameters() {

  if(!Place_List::Static_variables_set) {

    // get static parameters for all place subclasses
    Household::get_parameters();
    Neighborhood::get_parameters();
    School::get_parameters();
    Classroom::get_parameters();
    Workplace::get_parameters();
    Office::get_parameters();
    Hospital::get_parameters();

    // population parameters
    Params::get_param("population_directory", Place_List::Population_directory);
    Params::get_param("country", Place_List::Country);
    this->country_is_usa = strcmp(Place_List::Country,"usa")==0;
    this->country_is_colombia = strcmp(Place_List::Country,"colombia")==0;
    this->country_is_india = strcmp(Place_List::Country,"india")==0;

    Params::get_param("population_version", Place_List::Population_version);
    location_id.clear();
    char locations_filename[FRED_STRING_SIZE];
    Params::get_param("locations_file", locations_filename);
    FILE * loc_fp = Utils::fred_open_file(locations_filename);
    if(loc_fp != NULL) {
      char loc_id[FRED_STRING_SIZE];
      // int n = 0;
      while (fscanf(loc_fp, "%s", loc_id)==1) {
	// printf("loc_id = |%s|\n", loc_id);
	string str;
	str.assign(loc_id);
	location_id.push_back(str);
	// printf("push_back |%s|\n", get_location_id(n)); fflush(stdout); n++;
      }
      assert(location_id.size() > 0);
      fclose(loc_fp);
      for (int i = 0; i < location_id.size(); ++i) {
	FRED_VERBOSE(0, "location_id[%d] = %s\n", i, get_location_id(i));
      }
    }
    else {
      Utils::fred_abort("Can't find locations_file |%s|\n", locations_filename);
    }

    // school staff size
    Params::get_param("school_fixed_staff", &Place_List::School_fixed_staff);
    Params::get_param("school_student_teacher_ratio", &Place_List::School_student_teacher_ratio);

    if(Global::Enable_Group_Quarters) {
      // group quarter parameters
      Params::get_param("college_dorm_mean_size", &Place_List::College_dorm_mean_size);
      Params::get_param("military_barracks_mean_size", &Place_List::Military_barracks_mean_size);
      Params::get_param("prison_cell_mean_size", &Place_List::Prison_cell_mean_size);
      Params::get_param("nursing_home_room_mean_size", &Place_List::Nursing_home_room_mean_size);

      Params::get_param("college_fixed_staff", &Place_List::College_fixed_staff);
      Params::get_param("college_resident_to_staff_ratio", &Place_List::College_resident_to_staff_ratio);
      Params::get_param("prison_fixed_staff", &Place_List::Prison_fixed_staff);
      Params::get_param("prison_resident_to_staff_ratio", &Place_List::Prison_resident_to_staff_ratio);
      Params::get_param("nursing_home_fixed_staff", &Place_List::Nursing_home_fixed_staff);
      Params::get_param("nursing_home_resident_to_staff_ratio", &Place_List::Nursing_home_resident_to_staff_ratio);
      Params::get_param("military_fixed_staff", &Place_List::Military_fixed_staff);
      Params::get_param("military_resident_to_staff_ratio", &Place_List::Military_resident_to_staff_ratio);
    }

    // household evacuation parameters
    if(Global::Enable_Hospitals) {
      Params::get_param("hospital_worker_to_bed_ratio", &Place_List::Hospital_worker_to_bed_ratio);
      Place_List::Hospital_worker_to_bed_ratio = (Place_List::Hospital_worker_to_bed_ratio == 0.0 ? 1.0 : Place_List::Hospital_worker_to_bed_ratio);
      Params::get_param("hospital_outpatients_per_day_per_employee", &Place_List::Hospital_outpatients_per_day_per_employee);
      Params::get_param("healthcare_clinic_outpatients_per_day_per_employee", &Place_List::Healthcare_clinic_outpatients_per_day_per_employee);
      Params::get_param("hospital_min_bed_threshold", &Place_List::Hospital_min_bed_threshold);
      Params::get_param("hospitalization_radius", &Place_List::Hospitalization_radius);
      Params::get_param("hospital_fixed_staff", &Place_List::Hospital_fixed_staff);
    }
  }
  Place_List::Static_variables_set = true;

  if(Global::Enable_Hospitals) {
    char hosp_file_dir[FRED_STRING_SIZE];
    char hh_hosp_map_file_name[FRED_STRING_SIZE];

    Params::get_param("household_hospital_map_file_directory", hosp_file_dir);
    Params::get_param("household_hospital_map_file", hh_hosp_map_file_name);

    if(strcmp(hh_hosp_map_file_name, "none") == 0) {
      Place_List::Household_hospital_map_file_exists = false;
    } else {
      //If there is a file mapping Households to Hospitals, open it
      FILE* hospital_household_map_fp = NULL;

      char filename[FRED_STRING_SIZE];

      sprintf(filename, "%s%s", hosp_file_dir, hh_hosp_map_file_name);

      hospital_household_map_fp = Utils::fred_open_file(filename);
      if(hospital_household_map_fp != NULL) {
        Place_List::Household_hospital_map_file_exists = true;
        enum column_index {
          hh_label = 0, hospital_label = 1
        };
        char line_str[255];
        Utils::Tokens tokens;
        for(char* line = line_str; fgets(line, 255, hospital_household_map_fp); line = line_str) {
          tokens = Utils::split_by_delim(line, ',', tokens, false);
          // skip header line
          if(strcmp(tokens[hh_label], "hh_id") != 0 && strcmp(tokens[hh_label], "sp_id") != 0) {
            char s[80];

            sprintf(s, "%s", tokens[hh_label]);
            string hh_label_str(s);
            sprintf(s, "%s", tokens[hospital_label]);
            string hosp_label_str(s);
            this->hh_label_hosp_label_map.insert(std::pair<string, string>(hh_label_str, hosp_label_str));
          }
          tokens.clear();
        }
        fclose(hospital_household_map_fp);
      }
    }
  }

}

void Place_List::read_all_places() {

  // clear the vectors and maps
  this->households.clear();
  this->neighborhoods.clear();
  this->schools.clear();
  this->workplaces.clear();
  this->hospitals.clear();
  this->counties.clear();
  this->census_tracts.clear();
  this->fips_to_county_map.clear();
  this->fips_to_census_tract_map.clear();
  this->hosp_label_hosp_id_map.clear();
  this->hh_label_hosp_label_map.clear();

  // to compute the region's bounding box
  this->min_lat = this->min_lon = 999;
  this->max_lat = this->max_lon = -999;

  // process each specified location
  int locs = get_number_of_location_ids();
  for (int i = 0; i < locs; ++i) {
    char loc_id[FRED_STRING_SIZE];
    char loc_dir[FRED_STRING_SIZE];
    strcpy(loc_id, get_location_id(i));
    sprintf(loc_dir, "%s/%s/%s/%s", Place_List::Population_directory,
	    Place_List::Country, Place_List::Population_version, loc_id);
    read_places(loc_dir);
  }

  for(int i = 0; i < this->counties.size(); ++i) {
    int fips = this->counties[i]->get_fips();
    if (this->country_is_usa) {
      FRED_VERBOSE(0, "COUNTIES[%d] = %05d\n", i, fips);
    }
    else {
      FRED_VERBOSE(0, "COUNTIES[%d] = %d\n", i, fips);
    }
  }
  for(int i = 0; i < this->census_tracts.size(); ++i) {
    long int fips = this->census_tracts[i]->get_fips();
    FRED_VERBOSE(0, "CENSUS_TRACTS[%d] = %011ld\n", i, fips);
  }

  FRED_STATUS(0, "finished total places = %d\n", next_place_id);

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

  // Initialize global seasonality object
  if(Global::Enable_Seasonality) {
    Global::Clim = new Seasonality(Global::Simulation_Region);
  }

  // layer containing neighborhoods
  Global::Neighborhoods = new Neighborhood_Layer();

  // add households to the Neighborhoods Layer
  FRED_VERBOSE(0, "adding %d households to neighborhoods\n", this->households.size());
  for(int i = 0; i < this->households.size(); ++i) {
    Household* h = this->get_household(i);
    int row = Global::Neighborhoods->get_row(h->get_latitude());
    int col = Global::Neighborhoods->get_col(h->get_longitude());
    Neighborhood_Patch* patch = Global::Neighborhoods->get_patch(row, col);

    FRED_CONDITIONAL_VERBOSE(0, patch == NULL, "Help: household %d has bad patch,  lat = %f  lon = %f\n", h->get_id(),
			     h->get_latitude(), h->get_longitude());

    assert(patch != NULL);

    patch->add_household(h);
    h->set_patch(patch);
    // FRED_VERBOSE(0, "set household %s to patch row %d col %d\n", h->get_label(), patch->get_row(), patch->get_col());
  }

  int number_of_neighborhoods = Global::Neighborhoods->get_number_of_neighborhoods();

  // Neighborhood_Layer::setup call Neighborhood_Patch::make_neighborhood
  Global::Neighborhoods->setup();
  FRED_VERBOSE(0, "Created %d neighborhoods\n", this->neighborhoods.size());

  // add workplaces to Regional grid (for worker reassignment)
  int number_places = static_cast<int>(this->workplaces.size());
  for(int p = 0; p < number_places; ++p) {
    Global::Simulation_Region->add_workplace(this->workplaces[p]);
  }

  // add hospitals to Regional grid (for household hospital assignment)
  number_places = static_cast<int>(this->hospitals.size());
  for(int p = 0; p < number_places; ++p) {
    // printf("ADD HOSP %d %s\n", p, this->hospitals[p]->get_label());
    Global::Simulation_Region->add_hospital(this->hospitals[p]);
  }

  this->load_completed = true;
  number_places = get_number_of_households() + get_number_of_neighborhoods()
    + get_number_of_schools() + get_number_of_workplaces() + get_number_of_hospitals();

  FRED_STATUS(0, "read places finished: Places = %d\n", number_places);
}

void Place_List::read_places(const char* pop_dir) {

  FRED_STATUS(0, "read places entered\n", "");

  // record the actual synthetic population in the log file
  Utils::fred_log("POPULATION_FILE: %s\n", pop_dir);

  // read household locations
  char location_file[FRED_STRING_SIZE];
  sprintf(location_file, "%s/households.txt", pop_dir);
  read_household_file(location_file);
  Utils::fred_print_lap_time("Places.read_household_file");
  // FRED_VERBOSE(0, "after %s num_households = %d\n", location_file, this->households.size());

  // log county info
  for(int i = 0; i < this->counties.size(); ++i) {
    fprintf(Global::Statusfp, "COUNTIES[%d] = %05d\n", i, this->counties[i]->get_fips());
  }

  // read school locations
  sprintf(location_file, "%s/schools.txt", pop_dir);
  read_school_file(location_file);

  // read workplace locations
  sprintf(location_file, "%s/workplaces.txt", pop_dir);
  read_workplace_file(location_file);

  // read hospital locations
  if(Global::Enable_Hospitals) {
    sprintf(location_file, "%s/hospitals.txt", pop_dir);
    read_hospital_file(location_file);
  }

  if(Global::Enable_Group_Quarters) {
    // read group quarters locations (a new workplace and household is created 
    // for each group quarters)
    sprintf(location_file, "%s/gq.txt", pop_dir);
    read_group_quarters_file(location_file);
    Utils::fred_print_lap_time("Places.read_group_quarters_file");
    // FRED_VERBOSE(0, "after %s num_households = %d\n", location_file, this->households.size());

    // log county info
    fprintf(Global::Statusfp, "COUNTIES AFTER READING GQ\n");
    for(int i = 0; i < this->counties.size(); ++i) {
      if (this->country_is_usa) {
	fprintf(Global::Statusfp, "COUNTIES[%d] = %05d\n", i, this->counties[i]->get_fips());
      }
      else {
	fprintf(Global::Statusfp, "COUNTIES[%d] = %d\n", i, this->counties[i]->get_fips());
      }
    }
  }
  FRED_STATUS(0, "read places finished\n", "");
}


void Place_List::read_household_file(char* location_file) {

  // data to fill in from input file
  char place_type = Place::TYPE_HOUSEHOLD;
  char place_subtype = Place::SUBTYPE_NONE;
  char label[FRED_STRING_SIZE];
  char fips_str[FRED_STRING_SIZE];
  long int census_tract_fips = 0;
  int county_fips;
  double lat;
  double lon;
  int race;
  int income;

  FILE* fp = Utils::fred_open_file(location_file);

  // skip header line
  fgets(label, FRED_STRING_SIZE, fp);

  while (fscanf(fp, "%s %s %d %d %lf %lf ", label, fips_str, &race, &income, &lat, &lon) == 6) {
    // printf("%s %s %d %d %lf %lf\n", label, fips_str, race, income, lat, lon); fflush(stdout);

    update_geo_boundaries(lat, lon);

    if (this->country_is_usa) {
      // census tract
      // use the first eleven (state and county + six) digits of fips_field to get the census tract
      // e.g 090091846001 StateCo = 09009, 184600 is the census tract, throw away the 1
      fips_str[11] = '\0';
    }

    sscanf(fips_str, "%ld", &census_tract_fips);

    Household* place = static_cast<Household*>(add_place(label, place_type, place_subtype, lon, lat, census_tract_fips));
      
    // if this is a new census_tracts fips code, create a Census_tract object
    std::map<long int,int>::iterator itr_tract;
    itr_tract = this->fips_to_census_tract_map.find(census_tract_fips);
    if (itr_tract == this->fips_to_census_tract_map.end()) {
      Census_Tract* new_census_tract = new Census_Tract(census_tract_fips);
      this->census_tracts.push_back(new_census_tract);
      this->fips_to_census_tract_map[census_tract_fips] = this->census_tracts.size() - 1;
    }

    // add the household to the census_tract's list
    Census_Tract* census_tract = get_census_tract(census_tract_fips);
    census_tract->add_household(place);

    // county fips code

    if (this->country_is_usa || this->country_is_india) {
      // use the first five digits of fips_field to get the county fips code
      fips_str[5] = '\0';
    }

    sscanf(fips_str, "%d", &county_fips);

    if (this->country_is_india) {
      // use the last three digits to get the county (district) fips code
      county_fips = (county_fips % 1000);
    }

    // if this is a new county fips code, create a County object
    std::map<int,int>::iterator itr;
    itr = this->fips_to_county_map.find(county_fips);
    if (itr == this->fips_to_county_map.end()) {
      County* new_county = new County(county_fips);
      this->counties.push_back(new_county);
      this->fips_to_county_map[county_fips] = this->counties.size() - 1;
    }

    // add the household to the county list
    County* county = get_county(county_fips);
    county->add_household(place);
      
    // printf("county = %d census_tract = %ld\n", county_fips, census_tract_fips); exit(0);

    // household race and income
    place->set_household_race(race);
    place->set_household_income(income);
  }
  fclose(fp);
  return;
}

void Place_List::read_workplace_file(char* location_file) {
  // data to fill in from input file
  char place_type = Place::TYPE_WORKPLACE;
  char place_subtype = Place::SUBTYPE_NONE;
  char label[FRED_STRING_SIZE];
  double lat;
  double lon;

  FILE* fp = Utils::fred_open_file(location_file);

  // skip header line
  fgets(label, FRED_STRING_SIZE, fp);

  while (fscanf(fp, "%s %lf %lf ", label, &lat, &lon) == 3) {
    // printf("%s %lf %lf\n", label, lat, lon); fflush(stdout);
    Place* place = add_place(label, place_type, place_subtype, lon, lat, 0);
  }
  fclose(fp);
  return;
}

void Place_List::read_hospital_file(char* location_file) {

  // data to fill in from input file
  char place_type = Place::TYPE_HOSPITAL;
  char place_subtype = Place::SUBTYPE_NONE;
  char label[FRED_STRING_SIZE];
  double lat;
  double lon;
  int workers;
  int physicians;
  int beds;

  FILE* fp = Utils::fred_open_file(location_file);

  // skip header line
  fgets(label, FRED_STRING_SIZE, fp);

  int new_hospitals = 0;
  while (fscanf(fp, "%s %d %d %d %lf %lf ", label, &workers, &physicians, &beds, &lat, &lon) == 6) {
    // printf("%s %d %d %d %lf %lf ", label, workers, physicians, beds, lat, lon);

    update_geo_boundaries(lat, lon);
    Hospital* place = static_cast<Hospital*>(add_place(label, place_type, place_subtype, lon, lat, 0));
    place->set_employee_count(workers);
    place->set_physician_count(physicians);
    place->set_bed_count(beds);

    string hosp_label_str(label);
    int hosp_id = this->hospitals.size() - 1;
    this->hosp_label_hosp_id_map.insert(std::pair<string, int>(hosp_label_str, hosp_id));
    new_hospitals++;
    // printf("READ HOSP %s hosp_id %d\n", place->get_label(), hosp_id);
  }
  fclose(fp);
  FRED_VERBOSE(0, "read_hospital_file: found %d hospitals\n", new_hospitals);
  return;
}


void Place_List::read_school_file(char* location_file) {
  // data to fill in from input file
  char place_type = Place::TYPE_SCHOOL;
  char place_subtype = Place::SUBTYPE_NONE;
  char label[FRED_STRING_SIZE];
  char code[FRED_STRING_SIZE];
  long int census_tract_fips = 0;
  char county_fips_str[8];
  double lat;
  double lon;

  FILE* fp = Utils::fred_open_file(location_file);

  // skip header line
  fgets(label, FRED_STRING_SIZE, fp);

  while (fscanf(fp, "%s %s %lf %lf ", label, code, &lat, &lon) == 4) {
    // printf("%s %s %lf %lf\n", label, code, lat, lon); fflush(stdout);

    // census tract fips code
    if (this->country_is_usa) {
      strncpy(county_fips_str, code, 5);
      county_fips_str[5] = '\0';
      sscanf(county_fips_str, "%ld", &census_tract_fips);
      census_tract_fips *= 1000000;
    }

    Place* place = add_place(label, place_type, place_subtype, lon, lat, census_tract_fips);
  }
  fclose(fp);
  return;
}


void Place_List::read_group_quarters_file(char* location_file) {

  // data to fill in from input file
  char place_type = Place::TYPE_HOUSEHOLD;
  char place_subtype = Place::SUBTYPE_NONE;
  char gq_type;
  char id[FRED_STRING_SIZE];
  char label[FRED_STRING_SIZE];
  char fips_str[FRED_STRING_SIZE];
  long int census_tract_fips = 0;
  int county_fips;
  double lat;
  double lon;
  int capacity;

  FILE* fp = Utils::fred_open_file(location_file);

  // skip header line
  fgets(label, FRED_STRING_SIZE, fp);

  while (fscanf(fp, "%s %c %s %d %lf %lf ", id, &gq_type, fips_str, &capacity, &lat, &lon) == 6) {
    // printf("read_gq_file: %s %c %s %d %lf %lf\n", id, gq_type, fips_str, capacity, lat, lon); fflush(stdout);

    update_geo_boundaries(lat, lon);

    // census tract
    // use the first eleven (state and county + six) digits of fips_field to get the census tract
    // e.g 090091846001 StateCo = 09009, 184600 is the census tract, throw away the 1
    fips_str[11] = '\0';
    sscanf(fips_str, "%ld", &census_tract_fips);

    // if this is a new census_tract fips code, create a Census_tract object
    std::map<long int,int>::iterator itr_tract;
    itr_tract = this->fips_to_census_tract_map.find(census_tract_fips);
    if (itr_tract == this->fips_to_census_tract_map.end()) {
      Census_Tract* new_census_tract = new Census_Tract(census_tract_fips);
      this->census_tracts.push_back(new_census_tract);
      this->fips_to_census_tract_map[census_tract_fips] = this->census_tracts.size() - 1;
    }

    // county fips code
    // use the first five digits of fips_field to get the county fips code
    fips_str[5] = '\0';
    sscanf(fips_str, "%d", &county_fips);

    // if this is a new county fips code, create a County object
    std::map<int,int>::iterator itr;
    itr = this->fips_to_county_map.find(county_fips);
    if (itr == this->fips_to_county_map.end()) {
      County* new_county = new County(county_fips);
      this->counties.push_back(new_county);
      this->fips_to_county_map[county_fips] = this->counties.size() - 1;
    }

    // set number of units and subtype for this group quarters
    int number_of_units = 0;
    if(gq_type == 'C') {
      number_of_units = capacity / Place_List::College_dorm_mean_size;
      place_subtype = Place::SUBTYPE_COLLEGE;
    }
    if(gq_type == 'M') {
      number_of_units = capacity / Place_List::Military_barracks_mean_size;
      place_subtype = Place::SUBTYPE_MILITARY_BASE;
    }
    if(gq_type == 'P') {
      number_of_units = capacity / Place_List::Prison_cell_mean_size;
      place_subtype = Place::SUBTYPE_PRISON;
    }
    if(gq_type == 'N') {
      number_of_units = capacity / Place_List::Nursing_home_room_mean_size;
      place_subtype = Place::SUBTYPE_NURSING_HOME;
    }
    if(number_of_units == 0) {
      number_of_units = 1;
    }

    // add a workplace for this group quarters
    place_type = Place::TYPE_WORKPLACE;
    // sprintf(label, "%c%s", place_type, id);
    strcpy(label, id);
    FRED_VERBOSE(1, "Adding GQ Workplace %s subtype %c\n", label, place_subtype);
    Place* workplace = add_place(label, place_type, place_subtype, lon, lat, census_tract_fips);
    
    // add as household
    place_type = Place::TYPE_HOUSEHOLD;
    // sprintf(label, "%c%s", place_type, id);
    strcpy(label, id);

    FRED_VERBOSE(1, "Adding GQ Household %s subtype %c\n", label, place_subtype);
    Household *place = static_cast<Household *>(add_place(label, place_type, place_subtype, lon, lat, census_tract_fips));
    place->set_group_quarters_units(number_of_units);
    place->set_group_quarters_workplace(workplace);
    
    // add the household to the census_tract's list
    Census_Tract* census_tract = get_census_tract(census_tract_fips);
    census_tract->add_household(place);

    // add the household to the county list
    County* county = get_county(county_fips);
    county->add_household(place);
      
    // generate additional household units associated with this group quarters
    for(int i = 1; i < number_of_units; ++i) {
      sprintf(label, "%c%s-%03d", place_type, id, i);
      Household *place = static_cast<Household *>(add_place(label, place_type, place_subtype, lon, lat, census_tract_fips));
      FRED_VERBOSE(1, "Adding GQ Household %s subtype %c out of %d units\n", label, place_subtype, number_of_units);

      // add the household to the census_tract's list
      Census_Tract* census_tract = get_census_tract(census_tract_fips);
      census_tract->add_household(place);

      // add the household to the county list
      County* county = get_county(county_fips);
      county->add_household(place);
    }
  }
  fclose(fp);
  return;

  ////////////////////////////////////////////////

  /*
  // location of fields in input file
  int id_field = 0;
  int type_field = 1;
  int size_field = 2;
  int fips_field = 3;
  int lat_field = 4;
  int lon_field = 5;

  // data to fill in from input file
  char place_type = Place::TYPE_HOUSEHOLD;
  char place_subtype = Place::SUBTYPE_NONE;
  char label[80];
  char fips_str[12];
  long int census_tract_fips = 0;
  int county_fips;
  double lat;
  double lon;
  int capacity;

  char line_str[10*FRED_STRING_SIZE];
  Utils::Tokens tokens;
  FILE* fp = Utils::fred_open_file(location_file);

  for(char* line = line_str; fgets(line, 10*FRED_STRING_SIZE, fp); line = line_str) {
    tokens.clear();
    tokens = Utils::split_by_delim(line, ',', tokens, false);

    // skip header line
    if(strcmp(tokens[id_field], "sp_id") == 0) {
      continue;
    }

    // lat/lon
    sscanf(tokens[lat_field], "%lf", &lat); 
    sscanf(tokens[lon_field], "%lf", &lon); 
    update_geo_boundaries(lat, lon);

    // census tract
    // use the first eleven (state and county + six) digits of fips_field to get the census tract
    // e.g 090091846001 StateCo = 09009, 184600 is the census tract, throw away the 1
    strncpy(fips_str, tokens[fips_field], 11);
    fips_str[11] = '\0';
    sscanf(fips_str, "%ld", &census_tract_fips);

    // if this is a new census_tracts fips code, create a Census_tract object
    std::map<long int,int>::iterator itr_tract;
    itr_tract = this->fips_to_census_tract_map.find(census_tract_fips);
    if (itr_tract == this->fips_to_census_tract_map.end()) {
      Census_Tract* new_census_tract = new Census_Tract(census_tract_fips);
      this->census_tracts.push_back(new_census_tract);
      this->fips_to_census_tract_map[census_tract_fips] = this->census_tracts.size() - 1;
    }

    // county fips code
    // use the first five digits of fips_field to get the county fips code
    strncpy(fips_str, tokens[fips_field], 5);
    fips_str[5] = '\0';
    sscanf(fips_str, "%d", &county_fips);

    // if this is a new county fips code, create a County object
    std::map<int,int>::iterator itr;
    itr = this->fips_to_county_map.find(county_fips);
    if (itr == this->fips_to_county_map.end()) {
      County* new_county = new County(county_fips);
      this->counties.push_back(new_county);
      this->fips_to_county_map[county_fips] = this->counties.size() - 1;
    }

    // size
    sscanf(tokens[size_field], "%d", &capacity); 

    // set number of units and subtype for this group quarters
    int number_of_units = 0;
    if(strcmp(tokens[type_field], "C") == 0) {
      number_of_units = capacity / Place_List::College_dorm_mean_size;
      place_subtype = Place::SUBTYPE_COLLEGE;
    }
    if(strcmp(tokens[type_field], "M") == 0) {
      number_of_units = capacity / Place_List::Military_barracks_mean_size;
      place_subtype = Place::SUBTYPE_MILITARY_BASE;
    }
    if(strcmp(tokens[type_field], "P") == 0) {
      number_of_units = capacity / Place_List::Prison_cell_mean_size;
      place_subtype = Place::SUBTYPE_PRISON;
    }
    if(strcmp(tokens[type_field], "N") == 0) {
      number_of_units = capacity / Place_List::Nursing_home_room_mean_size;
      place_subtype = Place::SUBTYPE_NURSING_HOME;
    }
    if(number_of_units == 0) {
      number_of_units = 1;
    }

    // add a workplace for this group quarters
    place_type = Place::TYPE_WORKPLACE;
    sprintf(label, "%c%s", place_type, tokens[id_field]);
    FRED_VERBOSE(1, "Adding GQ Workplace %s subtype %c\n", label, place_subtype);
    Place* workplace = add_place(label, place_type, place_subtype, lon, lat, census_tract_fips);
    
    // add as household
    place_type = Place::TYPE_HOUSEHOLD;
    sprintf(label, "%c%s", place_type, tokens[id_field]);

    FRED_VERBOSE(1, "Adding GQ Household %s subtype %c\n", label, place_subtype);
    Household *place = static_cast<Household *>(add_place(label, place_type, place_subtype, lon, lat, census_tract_fips));
    place->set_group_quarters_units(number_of_units);
    place->set_group_quarters_workplace(workplace);
    
    // add the household to the census_tract's list
    Census_Tract* census_tract = get_census_tract(census_tract_fips);
    census_tract->add_household(place);

    // add the household to the county list
    County* county = get_county(county_fips);
    county->add_household(place);
      
    // generate additional household units associated with this group quarters
    for(int i = 1; i < number_of_units; ++i) {
      sprintf(label, "%c%s-%03d", place_type, tokens[id_field], i);
      Household *place = static_cast<Household *>(add_place(label, place_type, place_subtype, lon, lat, census_tract_fips));
      FRED_VERBOSE(1, "Adding GQ Household %s subtype %c out of %d units\n", label, place_subtype, number_of_units);

      // add the household to the census_tract's list
      Census_Tract* census_tract = get_census_tract(census_tract_fips);
      census_tract->add_household(place);

      // add the household to the county list
      County* county = get_county(county_fips);
      county->add_household(place);
    }
  }
  fclose(fp);
  */
}


void Place_List::setup_counties() {
  // set each county's school and workplace attendance probabilities
  for(int i = 0; i < this->counties.size(); ++i) {
    this->counties[i]->setup();
  }
}


void Place_List::setup_census_tracts() {
  // set each census tract's school and workplace attendance probabilities
  for(int i = 0; i < this->census_tracts.size(); ++i) {
    this->census_tracts[i]->setup();
  }
}


void Place_List::prepare() {

  FRED_STATUS(0, "prepare places entered\n", "");

  for(int p = 0; p < get_number_of_households(); ++p) {
    this->households[p]->prepare();
  }

  for(int p = 0; p < get_number_of_schools(); ++p) {
    this->schools[p]->prepare();
  }

  for(int p = 0; p < get_number_of_workplaces(); ++p) {
    this->workplaces[p]->prepare();
  }

  for(int p = 0; p < get_number_of_hospitals(); ++p) {
    this->hospitals[p]->prepare();
  }

  Global::Neighborhoods->prepare();

  // create lists of school by grade
  int number_of_schools = this->schools.size();
  for(int p = 0; p < number_of_schools; ++p) {
    School* school = get_school(p);
    for(int grade = 0; grade < Global::GRADES; ++grade) {
      if(school->get_orig_students_in_grade(grade) > 0) {
	this->schools_by_grade[grade].push_back(school);
      }
    }
  }

  if(Global::Verbose > 1) {
    // check the schools by grade lists
    printf("\n");
    for(int grade = 0; grade < Global::GRADES; ++grade) {
      int size = this->schools_by_grade[grade].size();
      printf("GRADE = %d SCHOOLS = %d: ", grade, size);
      for(int i = 0; i < size; ++i) {
        printf("%s ", this->schools_by_grade[grade][i]->get_label());
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
    for(int i = 0; i < this->counties.size(); ++i) {
      if (this->country_is_usa) {
	fprintf(fp, "%05d\n", this->counties[i]->get_fips());
      }
      else {
	fprintf(fp, "%d\n", this->counties[i]->get_fips());
      }
    }
    fclose(fp);
    
    // add list of census_tracts to visualization data directory
    sprintf(filename, "%s/CENSUS_TRACTS", Global::Visualization_directory);
    fp = fopen(filename, "w");
    for(int i = 0; i < this->census_tracts.size(); ++i) {
      long int fips = this->census_tracts[i]->get_fips();
      fprintf(fp, "%011ld\n", fips);
    }
    fclose(fp);
    
    // add geographical bounding box to visualization data directory
    sprintf(filename, "%s/BBOX", Global::Visualization_directory);
    fp = fopen(filename, "w");
    fprintf(fp, "ymin = %0.6f\n", this->min_lat);
    fprintf(fp, "xmin = %0.6f\n", this->min_lon);
    fprintf(fp, "ymax = %0.6f\n", this->max_lat);
    fprintf(fp, "xmax = %0.6f\n", this->max_lon);
    fclose(fp);
  }
}


void Place_List::print_status_of_schools(int day) {
  int students_per_grade[Global::GRADES];
  for(int i = 0; i < Global::GRADES; ++i) {
    students_per_grade[i] = 0;
  }

  int number_of_schools = this->schools.size();
  for(int p = 0; p < number_of_schools; ++p) {
    School *school = get_school(p);
    for(int grade = 0; grade < Global::GRADES; ++grade) {
      int total = school->get_orig_number_of_students();
      int orig = school->get_orig_students_in_grade(grade);
      int now = school->get_students_in_grade(grade);
      students_per_grade[grade] += now;
      if(0 && total > 1500 && orig > 0) {
	printf("%s GRADE %d ORIG %d NOW %d DIFF %d\n", school->get_label(), grade,
	       school->get_orig_students_in_grade(grade),
	       school->get_students_in_grade(grade),
	       school->get_students_in_grade(grade)
	       - school->get_orig_students_in_grade(grade));
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

void Place_List::update(int day) {

  FRED_STATUS(1, "update places entered\n", "");

  if(Global::Enable_Seasonality) {
    Global::Clim->update(day);
  }

  if(Global::Enable_Vector_Transmission) {
    for(int p = 0; p < get_number_of_households(); ++p) {
      get_household(p)->update_vector_population(day);
    }

    for(int p = 0; p < get_number_of_schools(); ++p) {
      School* school = get_school(p);
      school->update_vector_population(day);
      for(int c = 0; c < school->get_number_of_classrooms(); ++c) {
	school->get_classroom(c)->update_vector_population(day);
      }
    }
    
    for(int p = 0; p < get_number_of_workplaces(); ++p) {
      Workplace* work = get_workplace(p);
      work->update_vector_population(day);
      for(int c = 0; c < work->get_number_of_offices(); ++c) {
	work->get_office(c)->update_vector_population(day);
      }
    }
    
    for(int p = 0; p < get_number_of_hospitals(); ++p) {
      this->hospitals[p]->update_vector_population(day);
    }
  }

  for(int p = 0; p < get_number_of_households(); ++p) {
    get_household(p)->reset_healthcare_info();
  }
  
  for(int p = 0; p < get_number_of_hospitals(); ++p) {
    get_hospital(p)->reset_current_daily_patient_count();
  }

  FRED_STATUS(1, "update places finished\n", "");
}

void Place_List::setup_household_childcare() {
  assert(this->is_load_completed());
  assert(Global::Pop.is_load_completed());
  if(Global::Report_Childhood_Presenteeism) {
    int number_places = this->households.size();
    for(int p = 0; p < number_places; ++p) {
      Household* hh = get_household(p);
      hh->prepare_person_childcare_sickleave_map();
    }
  }
}

void Place_List::setup_school_income_quartile_pop_sizes() {
  assert(this->is_load_completed());
  assert(Global::Pop.is_load_completed());
  if(Global::Report_Childhood_Presenteeism) {
    int number_places = this->schools.size();
    for(int p = 0; p < number_places; ++p) {
      School* school = get_school(p);
      school->prepare_income_quartile_pop_size();
    }
  }
}

void Place_List::setup_household_income_quartile_sick_days() {
  assert(this->is_load_completed());
  assert(Global::Pop.is_load_completed());
  if(Global::Report_Childhood_Presenteeism) {
    typedef std::multimap<double, Household*> HouseholdMultiMapT;

    HouseholdMultiMapT* household_income_hh_mm = new HouseholdMultiMapT();
    int number_households = this->households.size();
    for(int p = 0; p < number_households; ++p) {
      Household* hh = get_household(p);
      double hh_income = hh->get_household_income();
      std::pair<double, Household*> my_insert(hh_income, hh);
      household_income_hh_mm->insert(my_insert);
    }

    int total = static_cast<int>(household_income_hh_mm->size());
    int q1 = total / 4;
    int q2 = q1 * 2;
    int q3 = q1 * 3;

    FRED_STATUS(0, "\nPROBABILITY WORKERS HAVE PAID SICK DAYS BY HOUSEHOLD INCOME QUARTILE:\n");
    double q1_sick_leave = 0.0;
    double q1_count = 0.0;
    double q2_sick_leave = 0.0;
    double q2_count = 0.0;
    double q3_sick_leave = 0.0;
    double q3_count = 0.0;
    double q4_sick_leave = 0.0;
    double q4_count = 0.0;
    int counter = 0;
    for(HouseholdMultiMapT::iterator itr = household_income_hh_mm->begin(); itr != household_income_hh_mm->end(); ++itr) {
      double hh_sick_leave_total = 0.0;
      double hh_employee_total = 0.0;

      for(int i = 0; i < static_cast<int>((*itr).second->enrollees.size()); ++i) {
        Person* per = (*itr).second->enrollees[i];
        if(per->is_adult() && !per->is_student()
	   && (per->get_activities()->is_teacher() || per->get_activities()->get_profile() == WORKER_PROFILE
	       || per->get_activities()->get_profile() == WEEKEND_WORKER_PROFILE)) {
          hh_sick_leave_total += (per->get_activities()->is_sick_leave_available() ? 1.0 : 0.0);
          hh_employee_total += 1.0;
        }
      }

      if(counter < q1) {
        (*itr).second->set_income_quartile(Global::Q1);
        q1_sick_leave += hh_sick_leave_total;
        q1_count += hh_employee_total;
      } else if(counter < q2) {
        (*itr).second->set_income_quartile(Global::Q2);
        q2_sick_leave += hh_sick_leave_total;
        q2_count += hh_employee_total;
      } else if(counter < q3) {
        (*itr).second->set_income_quartile(Global::Q3);
        q3_sick_leave += hh_sick_leave_total;
        q3_count += hh_employee_total;
      } else {
        (*itr).second->set_income_quartile(Global::Q4);
        q4_sick_leave += hh_sick_leave_total;
        q4_count += hh_employee_total;
      }

      counter++;
    }

    FRED_STATUS(0, "HOUSEHOLD INCOME QUARITLE[%d]: %.2f\n", Global::Q1,
		(q1_count == 0.0 ? 0.0 : (q1_sick_leave / q1_count)));
    FRED_STATUS(0, "HOUSEHOLD INCOME QUARITLE[%d]: %.2f\n", Global::Q2,
		(q2_count == 0.0 ? 0.0 : (q2_sick_leave / q2_count)));
    FRED_STATUS(0, "HOUSEHOLD INCOME QUARITLE[%d]: %.2f\n", Global::Q3,
		(q3_count == 0.0 ? 0.0 : (q3_sick_leave / q3_count)));
    FRED_STATUS(0, "HOUSEHOLD INCOME QUARITLE[%d]: %.2f\n", Global::Q4,
		(q4_count == 0.0 ? 0.0 : (q4_sick_leave / q4_count)));

    delete household_income_hh_mm;
  }
}

int Place_List::get_min_household_income_by_percentile(int percentile) {
  assert(this->is_load_completed());
  assert(Global::Pop.is_load_completed());
  assert(percentile > 0);
  assert(percentile <= 100);
  return -1;
}

Place* Place_List::get_household_from_label(const char* s) const {
  assert(this->household_label_map != NULL);
  if(s[0] == '\0' || strcmp(s, "X") == 0) {
    return NULL;
  }
  LabelMapT::iterator itr;
  string str(s);
  itr = this->household_label_map->find(str);
  if(itr != this->household_label_map->end()) {
    return this->households[itr->second];
  } else {
    FRED_VERBOSE(1, "Help!  can't find household with label = %s\n", str.c_str());
    return NULL;
  }
}

Place* Place_List::get_school_from_label(const char* s) const {
  assert(this->school_label_map != NULL);
  if(s[0] == '\0' || strcmp(s, "X") == 0) {
    return NULL;
  }
  LabelMapT::iterator itr;
  string str(s);
  itr = this->school_label_map->find(str);
  if(itr != this->school_label_map->end()) {
    return this->schools[itr->second];
  } else {
    FRED_VERBOSE(1, "Help!  can't find school with label = %s\n", str.c_str());
    return NULL;
  }
}

Place* Place_List::get_workplace_from_label(const char* s) const {
  assert(this->workplace_label_map != NULL);
  if(s[0] == '\0' || strcmp(s, "X") == 0) {
    return NULL;
  }
  LabelMapT::iterator itr;
  string str(s);
  itr = this->workplace_label_map->find(str);
  if(itr != this->workplace_label_map->end()) {
    return this->workplaces[itr->second];
  } else {
    FRED_VERBOSE(1, "Help!  can't find workplace with label = %s\n", str.c_str());
    return NULL;
  }
}

Place* Place_List::add_place(char* label, char type, char subtype, fred::geo lon, fred::geo lat, long int census_tract_fips) {

  string label_str;
  label_str.assign(label);

  if (type == 'H') {
    if(this->household_label_map->find(label_str) != this->household_label_map->end()) {
      if (Global::Verbose > 0) {
	FRED_WARNING("duplicate household label found: %s\n", label);
      }
      return get_household_from_label(label);
    }
  }
  if (type == 'S') {
    if(this->school_label_map->find(label_str) != this->school_label_map->end()) {
      if (Global::Verbose > 0) {
	FRED_WARNING("duplicate school label found: %s\n", label);
      }
      return get_school_from_label(label);
    }
  }
  if (type == 'W') {
    if(this->workplace_label_map->find(label_str) != this->workplace_label_map->end()) {
      if (Global::Verbose > 0) {
	FRED_WARNING("duplicate workplace label found: %s\n", label);
      }
      return get_workplace_from_label(label);
    }
  }


  Place* place = NULL;
  switch(type) {
  case 'H':
    place = new Household(label, subtype, lon, lat);
    this->households.push_back(place);
    this->household_label_map->insert(std::make_pair(label_str, this->households.size()-1));
    break;

  case 'W':
    place = new Workplace(label, subtype, lon, lat);
    this->workplaces.push_back(place);
    this->workplace_label_map->insert(std::make_pair(label_str, this->workplaces.size()-1));
    break;
    
  case 'O':
    place = new Office(label, subtype, lon, lat);
    break;
    
  case 'N':
    place = new Neighborhood(label, subtype, lon, lat);
    this->neighborhoods.push_back(place);
    break;

  case 'S':
    place = new School(label, subtype, lon, lat);
    this->schools.push_back(place);
    this->school_label_map->insert(std::make_pair(label_str, this->schools.size()-1));
    break;
    
  case 'C':
    place = new Classroom(label, subtype, lon, lat);
    break;
    
  case 'M':
    place = new Hospital(label, subtype, lon, lat);
    this->hospitals.push_back(place);
    break;
  }

  int id = get_new_place_id();
  place->set_id(id);
  place->set_census_tract_fips(census_tract_fips);
    
  FRED_VERBOSE(1, "add_place %d lab %s type %c sub %c lat %f lon %f\n",
		 place->get_id(), place->get_label(), place->get_type(), place->get_subtype(), place->get_latitude(), place->get_longitude());

  return place;
}


void Place_List::setup_group_quarters() {

  FRED_STATUS(0, "setup group quarters entered\n", "");

  // reset household indexes
  int num_households = this->households.size();
  for(int i = 0; i < num_households; ++i) {
    this->get_household(i)->set_index(i);
  }

  int p = 0;
  int units = 0;
  while(p < num_households) {
    Household* house = this->get_household(p++);
    Household* new_house;
    if(house->is_group_quarters()) {
      int gq_size = house->get_size();
      int gq_units = house->get_group_quarters_units();
      FRED_VERBOSE(1, "GQ_setup: house %d label %s subtype %c initial size %d units %d\n", p, house->get_label(),
		   house->get_subtype(), gq_size, gq_units);
      int units_filled = 1;
      if(gq_units > 1) {
	vector<Person*> housemates;
	housemates.clear();
	for(int i = 0; i < gq_size; ++i) {
	  Person* person = house->get_enrollee(i);
	  housemates.push_back(person);
	}
	int min_per_unit = gq_size / gq_units;
	int larger_units = gq_size - min_per_unit * gq_units;
        int smaller_units = gq_units - larger_units;
        FRED_VERBOSE(1, "GQ min_per_unit %d smaller = %d  larger = %d total = %d  orig = %d\n", min_per_unit,
		     smaller_units, larger_units, smaller_units*min_per_unit + larger_units*(min_per_unit+1), gq_size);
        int next_person = min_per_unit;
        for(int i = 1; i < smaller_units; ++i) {
          // assert(units_filled < gq_units);
          new_house = this->get_household(p++);
          // printf("GQ smaller new_house %s subtype %c\n", new_house->get_label(), new_house->get_subtype()); fflush(stdout);
          for(int j = 0; j < min_per_unit; ++j) {
            Person* person = housemates[next_person++];
            person->change_household(new_house);
          }
          // printf("GQ smaller new_house %s subtype %c size %d\n", new_house->get_label(), new_house->get_subtype(), new_house->get_size()); fflush(stdout);
          units_filled++;
          // printf("GQ size of smaller unit %s = %d remaining in main house %d\n",
	  // new_house->get_label(), new_house->get_size(), house->get_size());
        }
        for(int i = 0; i < larger_units; ++i) {
          new_house = this->get_household(p++);
          // printf("GQ larger new_house %s\n", new_house->get_label()); fflush(stdout);
          for(int j = 0; j < min_per_unit + 1; ++j) {
            Person* person = housemates[next_person++];
            person->change_household(new_house);
          }
          // printf("GQ larger new_house %s subtype %c size %d\n", new_house->get_label(), new_house->get_subtype(), new_house->get_size()); fflush(stdout);
          units_filled++;
          // printf("GQ size of larger unit %s = %d -- remaining in main house %d\n",
	  // new_house->get_label(), new_house->get_size(), house->get_size());
        }
      }
      units += units_filled;
    }
  }
  FRED_STATUS(0, "setup group quarters finished, units = %d\n", units);
}

// Comparison used to sort households by income below (resolve ties by place id)
static bool compare_household_incomes(Place* h1, Place* h2) {
  int inc1 = (static_cast<Household*>(h1))->get_household_income();
  int inc2 = (static_cast<Household*>(h2))->get_household_income();
  return ((inc1 == inc2) ? (h1->get_id() < h2->get_id()) : (inc1 < inc2));
}

void Place_List::setup_households() {

  FRED_STATUS(0, "setup households entered\n", "");

  int num_households = this->households.size();
  for(int p = 0; p < num_households; ++p) {
    Household* house = this->get_household(p);
    house->set_index(p);
    if(house->get_size() == 0) {
      FRED_VERBOSE(0, "Warning: house %d label %s has zero size.\n", house->get_id(), house->get_label());
      continue;
    }

    // ensure that each household has an identified householder
    Person* person_with_max_age = NULL;
    Person* head_of_household = NULL;
    int max_age = -99;
    for(int j = 0; j < house->get_size() && head_of_household == NULL; ++j) {
      Person* person = house->get_enrollee(j);
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
      Person* person = house->get_enrollee(j);
      if(person != head_of_household && person->is_householder()) {
        person->set_relationship(Global::HOUSEMATE);
      }
    }
    assert(head_of_household != NULL);
    FRED_VERBOSE(1, "HOLDER: house %d label %s is_group_quarters %d householder %d age %d\n", house->get_id(),
		 house->get_label(), house->is_group_quarters()?1:0, head_of_household->get_id(), head_of_household->get_age());

    // setup household structure type
    house->set_household_structure();
    house->set_orig_household_structure();
  }

  // NOTE: the following sorts households from lowest income to highest
  std::sort(this->households.begin(), this->households.end(), compare_household_incomes);

  // reset household indexes
  for(int i = 0; i < num_households; ++i) {
    this->get_household(i)->set_index(i);
  }

  report_household_incomes();

  FRED_STATUS(0, "setup households finished\n", "");
}


void Place_List::setup_classrooms() {
  FRED_STATUS(0, "setup classrooms entered\n");
  int number_classrooms = 0;
  int number_schools = this->schools.size();
  for(int p = 0; p < number_schools; ++p) {
    School* school = get_school(p);
    school->setup_classrooms();
  }
  FRED_STATUS(0, "setup classrooms finished\n");
}


void Place_List::reassign_workers() {

  if(Global::Assign_Teachers) {
    reassign_workers_to_schools();
  }

  if(Global::Enable_Hospitals) {
    reassign_workers_to_hospitals();
  }

  if(Global::Enable_Group_Quarters) {
    reassign_workers_to_group_quarters(Place::SUBTYPE_COLLEGE, Place_List::College_fixed_staff,
				       Place_List::College_resident_to_staff_ratio);
    reassign_workers_to_group_quarters(Place::SUBTYPE_PRISON, Place_List::Prison_fixed_staff,
				       Place_List::Prison_resident_to_staff_ratio);
    reassign_workers_to_group_quarters(Place::SUBTYPE_MILITARY_BASE, Place_List::Military_fixed_staff,
				       Place_List::Military_resident_to_staff_ratio);
    reassign_workers_to_group_quarters(Place::SUBTYPE_NURSING_HOME, Place_List::Nursing_home_fixed_staff,
				       Place_List::Nursing_home_resident_to_staff_ratio);
  }

  Utils::fred_print_lap_time("reassign workers");
}

void Place_List::reassign_workers_to_schools() {

  // staff data from:
  // http://www.statemaster.com/graph/edu_ele_sec_pup_rat-elementary-secondary-pupil-teacher-ratio
  int fixed_staff = Place_List::School_fixed_staff;
  double staff_ratio = Place_List::School_student_teacher_ratio;

  int number_of_schools = get_number_of_schools();

  Utils::fred_log("reassign workers to schools entered. schools = %d fixed_staff = %d staff_ratio = %f \n",
		  number_of_schools, fixed_staff, staff_ratio);

  for(int p = 0; p < number_of_schools; p++) {
    School* school = get_school(p);
    fred::geo lat = school->get_latitude();
    fred::geo lon = school->get_longitude();
    double x = Geo::get_x(lon);
    double y = Geo::get_y(lat);
    FRED_VERBOSE(1, "Reassign teachers to school %s in county %d at (%f,%f) \n",
		 school->get_label(), school->get_county_fips(), x, y);
    
    // ignore school if it is outside the region
    Regional_Patch* regional_patch = Global::Simulation_Region->get_patch(lat, lon);
    if(regional_patch == NULL) {
      FRED_VERBOSE(0, "school %s OUTSIDE_REGION lat %f lon %f \n",
		   school->get_label(), lat, lon);
      continue;
    }

    // target staff size
    int n = school->get_orig_number_of_students();
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
		   school->get_label(), school->get_county_fips(), lat, lon);
    }
  }
}

void Place_List::reassign_workers_to_hospitals() {

  int number_places = get_number_of_hospitals();
  Utils::fred_log("reassign workers to hospitals entered. places = %d\n", number_places);

  int fixed_staff = Place_List::Hospital_fixed_staff;
  double staff_ratio = (1.0 / Place_List::Hospital_worker_to_bed_ratio);

  for(int p = 0; p < number_places; p++) {
    Hospital* hosp = get_hospital(p);
    fred::geo lat = hosp->get_latitude();
    fred::geo lon = hosp->get_longitude();
    double x = Geo::get_x(lon);
    double y = Geo::get_y(lat);
    FRED_VERBOSE(0, "Reassign workers to hospital %s in county %d at (%f,%f) \n",
		 hosp->get_label(), hosp->get_county_fips(), x, y);

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
		   hosp->get_label(), hosp->get_county_fips(), lat, lon);
    }
  }
}

void Place_List::reassign_workers_to_group_quarters(char subtype, int fixed_staff, double resident_to_staff_ratio) {
  int number_places = this->workplaces.size();
  Utils::fred_log("reassign workers to group quarters subtype %c entered. total workplaces = %d\n", subtype, number_places);
  for(int p = 0; p < number_places; ++p) {
    Workplace* place = get_workplace(p);
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

      FRED_VERBOSE(0, "REASSIGN WORKERS to GQ %s subtype %c target staff %d at (%f,%f) \n",
		   place->get_label(), subtype, staff, lat, lon);

      // ignore place if it is outside the region
      Regional_Patch* regional_patch = Global::Simulation_Region->get_patch(lat, lon);
      if(regional_patch == NULL) {
        FRED_VERBOSE(0, "REASSIGN WORKERS to place GQ %s subtype %c FAILED -- OUTSIDE_REGION lat %f lon %f \n",
		     place->get_label(), subtype, lat, lon);
        continue;
      }

      Place* nearby_workplace = regional_patch->get_nearby_workplace(place, staff);
      if(nearby_workplace != NULL) {
        // make all the workers in selected workplace as workers in the target place
        FRED_VERBOSE(0, "REASSIGN WORKERS: NEARBY_WORKPLACE FOUND %s for GQ %s subtype %c at lat %f lon %f \n",
		     nearby_workplace->get_label(),
		     place->get_label(), subtype, lat, lon);
        nearby_workplace->reassign_workers(place);
      }
      else {
        FRED_VERBOSE(0, "REASSIGN WORKERS: NO NEARBY_WORKPLACE FOUND for GQ %s subtype %c at lat %f lon %f \n",
		     place->get_label(), subtype, lat, lon);
      }
    }
  }
}


void Place_List::setup_offices() {
  FRED_STATUS(0, "setup offices entered\n");
  int number_workplaces = this->workplaces.size();
  for(int p = 0; p < number_workplaces; ++p) {
    Workplace* workplace = get_workplace(p);
    workplace->setup_offices();
  }
  FRED_STATUS(0, "setup offices finished\n");
}


Place* Place_List::get_random_workplace() {
  int size = static_cast<int>(this->workplaces.size());
  if(size > 0) {
    return this->workplaces[Random::draw_random_int(0, size - 1)];
  } else {
    return NULL;
  }
}

Place* Place_List::get_random_school(int grade) {
  int size = static_cast<int>(this->schools_by_grade[grade].size());
  if(size > 0) {
    return this->schools_by_grade[grade][Random::draw_random_int(0, size - 1)];
  } else {
    return NULL;
  }
}


void Place_List::assign_hospitals_to_households() {
  if(Global::Enable_Hospitals) {

    FRED_STATUS(0, "assign_hospitals_to_household entered\n");

    int number_hh = (int)this->households.size();
    for(int i = 0; i < number_hh; ++i) {
      Household* hh = static_cast<Household*>(this->households[i]);
      Hospital* hosp = static_cast<Hospital*>(this->get_hospital_assigned_to_household(hh));
      assert(hosp != NULL);
      if(hosp != NULL) {
        hh->set_household_visitation_hospital(hosp);
        string hh_label_str(hh->get_label());
        string hosp_label_str(hosp->get_label());

        this->hh_label_hosp_label_map.insert(std::pair<string, string>(hh_label_str, hosp_label_str));
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
      if(this->hosp_label_hosp_id_map.find(hosp_label_str) != this->hosp_label_hosp_id_map.end()) {
	hosp_id = this->hosp_label_hosp_id_map.find(hosp_label_str)->second;
      }
      assert(0 <= hosp_id && hosp_id < number_hospitals);
      // printf("CATCH house %s hosp_id %d %s\n", hh->get_label(), hosp_id, hosp->get_label());
      catchment_count[hosp_id] += hh->get_size();
      catchment_dist[hosp_id] += hh->get_size()*(distance_between_places(hh,hosp));
      for (int j = 0; j < hh->get_size(); j++) {
	double age = hh->get_enrollee(j)->get_real_age();
	catchment_age[hosp_id] += age;
      }
    }

    for (int i = 0; i < number_hospitals; i++) {
      if (catchment_count[i] > 0) {
	catchment_dist[i] /= catchment_count[i];
	catchment_age[i] /= catchment_count[i];
      }
      FRED_STATUS(0,
		  "HOSPITAL CATCHMENT %d %s beds %d count %d age %f dist %f\n",
		  i, this->hospitals[i]->get_label(),
		  static_cast<Hospital*>(this->hospitals[i])->get_bed_count(0),
		  catchment_count[i],
		  catchment_age[i],
		  catchment_dist[i]);
    }

    //Write the mapping file if it did not already exist (or if it was incomplete)
    if(!Place_List::Household_hospital_map_file_exists) {

      char map_file_dir[FRED_STRING_SIZE];
      char map_file_name[FRED_STRING_SIZE];
      Params::get_param("household_hospital_map_file_directory", map_file_dir);
      Params::get_param("household_hospital_map_file", map_file_name);

      if(strcmp(map_file_name, "none") == 0) {
        this->hh_label_hosp_label_map.clear();
        return;
      }

      char filename[FRED_STRING_SIZE];
      sprintf(filename, "%s%s", map_file_dir, map_file_name);

      Utils::get_fred_file_name(filename);
      FILE* hh_label_hosp_label_map_fp = fopen(filename, "w");
      if(hh_label_hosp_label_map_fp == NULL) {
        Utils::fred_abort("Can't open %s\n", filename);
      }

      for(std::map<std::string, string>::iterator itr = this->hh_label_hosp_label_map.begin(); itr != this->hh_label_hosp_label_map.end(); ++itr) {
        fprintf(hh_label_hosp_label_map_fp, "%s,%s\n", itr->first.c_str(), itr->second.c_str());
      }

      fflush(hh_label_hosp_label_map_fp);
      fclose(hh_label_hosp_label_map_fp);
    }

    this->hh_label_hosp_label_map.clear();
    FRED_STATUS(0, "assign_hospitals_to_household finished\n");
  }
}

void Place_List::prepare_primary_care_assignment() {

  if(this->is_primary_care_assignment_initialized) {
    return;
  }

  if(Global::Enable_Hospitals && this->is_load_completed() && Global::Pop.is_load_completed()) {
    int tot_pop_size = Global::Pop.get_population_size();
    assert(Place_List::Hospital_overall_panel_size > 0);
    //Determine the distribution of population that should be assigned to each hospital location
    for(int i = 0; i < this->hospitals.size(); ++i) {
      Hospital* hosp = this->get_hospital(i);
      double proprtn_of_total_panel = 0;
      if(hosp->get_subtype() != Place::SUBTYPE_MOBILE_HEALTHCARE_CLINIC) {
        proprtn_of_total_panel = static_cast<double>(hosp->get_daily_patient_capacity(0))
	  / static_cast<double>(Place_List::Hospital_overall_panel_size);
      }
      Place_List::Hospital_ID_total_assigned_size_map.insert(std::pair<int, int>(hosp->get_id(), ceil(proprtn_of_total_panel * tot_pop_size)));
      Place_List::Hospital_ID_current_assigned_size_map.insert(std::pair<int, int>(hosp->get_id(), 0));
    }
    this->is_primary_care_assignment_initialized = true;
  }
}

Hospital* Place_List::get_random_open_hospital_matching_criteria(int sim_day, Person* per, bool check_insurance) {
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
    double distance = distance_between_places(hh, hospital);
    double cur_prob = 0.0;
    int increment = 0;
    overnight_cap = hospital->get_bed_count(sim_day);
    //Need to make sure place is not a healthcare clinic && there are beds available
    if(distance > 0.0 && !hospital->is_healthcare_clinic() && !hospital->is_mobile_healthcare_clinic()
       && hospital->should_be_open(sim_day)
       && (hospital->get_occupied_bed_count() < overnight_cap)) {
      if(check_insurance) {
        Insurance_assignment_index::e per_insur = per->get_health()->get_insurance_type();
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
}


Hospital* Place_List::get_random_open_healthcare_facility_matching_criteria(int sim_day, Person* per, bool check_insurance, bool use_search_radius_limit) {
  if(!Global::Enable_Hospitals) {
    return NULL;
  }

  if(check_insurance) {
    assert(Global::Enable_Health_Insurance);
  }
  assert(per != NULL);

  int daily_hosp_cap = 0;
  Hospital* assigned_hospital = NULL;
  int number_hospitals = this->hospitals.size();
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
    double distance = distance_between_places(hh, hospital);
    double cur_prob = 0.0;
    int increment = 0;

    //Need to make sure place is open and not over capacity
    if(distance > 0.0 && hospital->should_be_open(sim_day)
       && hospital->get_current_daily_patient_count() < daily_hosp_cap) {
      if(use_search_radius_limit) {
        if(distance <= Place_List::Hospitalization_radius) {
          if(check_insurance) {
            Insurance_assignment_index::e per_insur = per->get_health()->get_insurance_type();
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
          Insurance_assignment_index::e per_insur = per->get_health()->get_insurance_type();
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
}


Hospital* Place_List::get_random_primary_care_facility_matching_criteria(Person* per, bool check_insurance, bool use_search_radius_limit) {
  if(!Global::Enable_Hospitals) {
    return NULL;
  }

  if(check_insurance) {
    assert(Global::Enable_Health_Insurance);
  }
  assert(per != NULL);

  //This is the initial primary care assignment
  if(!this->is_primary_care_assignment_initialized) {
    this->prepare_primary_care_assignment();
  }

  int daily_hosp_cap = 0;
  Hospital* assigned_hospital = NULL;
  int number_hospitals = this->hospitals.size();
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
    double distance = distance_between_places(hh, hospital);
    double cur_prob = 0.0;
    int increment = 0;

    //Need to make sure place is open and not over capacity
    if(distance > 0.0 && hospital->should_be_open(0)) {
      if(use_search_radius_limit) {
        if(distance <= Place_List::Hospitalization_radius) {
          if(check_insurance) {
            Insurance_assignment_index::e per_insur = per->get_health()->get_insurance_type();
            if(hospital->accepts_insurance(per_insur)) {
              //Hospital accepts the insurance so can check further
              if(Place_List::Hospital_ID_current_assigned_size_map.at(hospital->get_id())
		 < Place_List::Hospital_ID_total_assigned_size_map.at(hospital->get_id())) {
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
            if(Place_List::Hospital_ID_current_assigned_size_map.at(hospital->get_id())
	       < Place_List::Hospital_ID_total_assigned_size_map.at(hospital->get_id())) {
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
          Insurance_assignment_index::e per_insur = per->get_health()->get_insurance_type();
          if(hospital->accepts_insurance(per_insur)) {
            //Hospital accepts the insurance so can check further
            if(Place_List::Hospital_ID_current_assigned_size_map.at(hospital->get_id())
	       < Place_List::Hospital_ID_total_assigned_size_map.at(hospital->get_id())) {
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
          if(Place_List::Hospital_ID_current_assigned_size_map.at(hospital->get_id())
	     < Place_List::Hospital_ID_total_assigned_size_map.at(hospital->get_id())) {
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
}


void Place_List::print_household_size_distribution(char* dir, char* date_string, int run) {
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
  int number_households = (int)households.size();
  for(int p = 0; p < number_households; ++p) {
    int n = this->households[p]->get_size();
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

void Place_List::delete_place_label_map() {
  if(this->household_label_map) {
    delete this->household_label_map;
    this->household_label_map = NULL;
  }
  if(this->school_label_map) {
    delete this->school_label_map;
    this->school_label_map = NULL;
  }
  if(this->workplace_label_map) {
    delete this->workplace_label_map;
    this->workplace_label_map = NULL;
  }
}

void Place_List::get_initial_visualization_data_from_households() {
  int num_households = this->households.size();
  for(int i = 0; i < num_households; ++i) {
    Household* h = this->get_household(i);
    Global::Visualization->initialize_household_data(h->get_latitude(), h->get_longitude(), h->get_size());
    // printf("%f %f %3d %s\n", h->get_latitude(), h->get_longitude(), h->get_size(), h->get_label());
  }
}

void Place_List::get_visualization_data_from_households(int day, int condition_id, int output_code) {
  int num_households = this->households.size();
  for(int i = 0; i < num_households; ++i) {
    Household* h = this->get_household(i);
    int count = h->get_visualization_counter(day, condition_id, output_code);
    int popsize = h->get_size();
    // update appropriate visualization patch
    Global::Visualization->update_data(h->get_latitude(), h->get_longitude(), count, popsize);
  }
}

void Place_List::get_census_tract_data_from_households(int day, int condition_id, int output_code) {
  int num_households = this->households.size();
  for(int i = 0; i < num_households; ++i) {
    Household* h = this->get_household(i);
    int count = h->get_visualization_counter(day, condition_id, output_code);
    int popsize = h->get_size();
    long int census_tract_fips = h->get_census_tract_fips();
    Global::Visualization->update_data(census_tract_fips, count, popsize);
  }
}

void Place_List::report_household_incomes() {

  // initialize household income stats
  this->min_household_income = 0;
  this->max_household_income = 0;
  this->median_household_income = 0;
  this->first_quartile_household_income = 0;
  this->third_quartile_household_income = 0;

  int num_households = this->households.size();
  if(num_households > 0) {
    this->min_household_income = this->get_household(0)->get_household_income();
    this->max_household_income = this->get_household(num_households - 1)->get_household_income();
    this->first_quartile_household_income = this->get_household(num_households / 4)->get_household_income();
    this->median_household_income = this->get_household(num_households / 2)->get_household_income();
    this->third_quartile_household_income = this->get_household((3 * num_households) / 4)->get_household_income();
  }

  // print household incomes to LOG file
  if(Global::Verbose > 1) {
    for(int i = 0; i < num_households; ++i) {
      Household* h = this->get_household(i);
      int h_county = h->get_county_fips();
      FRED_VERBOSE(0, "INCOME: %s %c %f %f %d %d\n", h->get_label(), h->get_type(), h->get_latitude(),
		   h->get_longitude(), h->get_household_income(), h_county);
    }
  }
  FRED_VERBOSE(0, "INCOME_STATS: households: %d  min %d  first_quartile %d  median %d  third_quartile %d  max %d\n",
	       num_households, min_household_income, first_quartile_household_income, median_household_income,
	       third_quartile_household_income, max_household_income);

}

void Place_List::end_of_run() {

  if(Global::Verbose > 1) {

    for(int p = 0; p < get_number_of_households(); ++p) {
      Place* place = get_household(p);
      fprintf(Global::Statusfp,
	      "PLACE REPORT: id %d type %c size %d inf %d attack_rate %5.2f first_day %d last_day %d\n",
	      place->get_id(), place->get_type(), place->get_size(),
	      place->get_total_infections(0),
	      100.0 * place->get_attack_rate(0),
	      place->get_first_day_infectious(),
	      place->get_last_day_infectious());
    }
    
    for(int p = 0; p < get_number_of_schools(); ++p) {
      Place* place = get_school(p);
      fprintf(Global::Statusfp,
	      "PLACE REPORT: id %d type %c size %d inf %d attack_rate %5.2f first_day %d last_day %d\n",
	      place->get_id(), place->get_type(), place->get_size(),
	      place->get_total_infections(0),
	      100.0 * place->get_attack_rate(0),
	      place->get_first_day_infectious(),
	      place->get_last_day_infectious());
    }
    
    for(int p = 0; p < get_number_of_workplaces(); ++p) {
      Place* place = get_workplace(p);
      fprintf(Global::Statusfp,
	      "PLACE REPORT: id %d type %c size %d inf %d attack_rate %5.2f first_day %d last_day %d\n",
	      place->get_id(), place->get_type(), place->get_size(),
	      place->get_total_infections(0),
	      100.0 * place->get_attack_rate(0),
	      place->get_first_day_infectious(),
	      place->get_last_day_infectious());
    }
    
    for(int p = 0; p < get_number_of_hospitals(); ++p) {
      Place* place = get_hospital(p);
      fprintf(Global::Statusfp,
	      "PLACE REPORT: id %d type %c size %d inf %d attack_rate %5.2f first_day %d last_day %d\n",
	      place->get_id(), place->get_type(), place->get_size(),
	      place->get_total_infections(0),
	      100.0 * place->get_attack_rate(0),
	      place->get_first_day_infectious(),
	      place->get_last_day_infectious());
    }
    
  }

}

int Place_List::get_housing_data(int* target_size, int* current_size) {
  int num_households = this->households.size();
  for(int i = 0; i < num_households; ++i) {
    Household* h = this->get_household(i);
    current_size[i] = h->get_size();
    target_size[i] = h->get_orig_size();
  }
  return num_households;
}

void Place_List::swap_houses(int house_index1, int house_index2) {

  Household* h1 = this->get_household(house_index1);
  Household* h2 = this->get_household(house_index2);
  if(h1 == NULL || h2 == NULL)
    return;

  FRED_VERBOSE(1, "HOUSING: swapping house %s with %d beds and %d occupants with %s with %d beds and %d occupants\n",
	       h1->get_label(), h1->get_orig_size(), h1->get_size(), h2->get_label(), h2->get_orig_size(), h2->get_size());

  // get pointers to residents of house h1
  vector<Person*> temp1;
  temp1.clear();
  vector<Person*> housemates1 = h1->get_inhabitants();
  for(std::vector<Person*>::iterator itr = housemates1.begin(); itr != housemates1.end(); ++itr) {
    temp1.push_back(*itr);
  }

  // get pointers to residents of house h2
  vector<Person*> temp2;
  temp2.clear();
  vector<Person *> housemates2 = h2->get_inhabitants();
  for(std::vector<Person*>::iterator itr = housemates2.begin(); itr != housemates2.end(); ++itr) {
    temp2.push_back(*itr);
  }

  // move first group into house h2
  for(std::vector<Person*>::iterator itr = temp1.begin(); itr != temp1.end(); ++itr) {
    (*itr)->change_household(h2);
  }

  // move second group into house h1
  for(std::vector<Person*>::iterator itr = temp2.begin(); itr != temp2.end(); ++itr) {
    (*itr)->change_household(h1);
  }

  FRED_VERBOSE(1, "HOUSING: swapped house %s with %d beds and %d occupants with %s with %d beds and %d occupants\n",
	       h1->get_label(), h1->get_orig_size(), h1->get_size(), h2->get_label(), h2->get_orig_size(), h2->get_size());
}

void Place_List::swap_houses(Household* h1, Household* h2) {

  if(h1 == NULL || h2 == NULL)
    return;

  FRED_VERBOSE(0, "HOUSING: swapping house %s with %d beds and %d occupants with %s with %d beds and %d occupants\n",
	       h1->get_label(), h1->get_orig_size(), h1->get_size(), h2->get_label(), h2->get_orig_size(), h2->get_size());

  // get pointers to residents of house h1
  vector<Person*> temp1;
  temp1.clear();
  vector<Person*> housemates1 = h1->get_inhabitants();
  for(std::vector<Person*>::iterator itr = housemates1.begin(); itr != housemates1.end(); ++itr) {
    temp1.push_back(*itr);
  }

  // get pointers to residents of house h2
  vector<Person*> temp2;
  temp2.clear();
  vector<Person *> housemates2 = h2->get_inhabitants();
  for(std::vector<Person*>::iterator itr = housemates2.begin(); itr != housemates2.end(); ++itr) {
    temp2.push_back(*itr);
  }

  // move first group into house h2
  for(std::vector<Person*>::iterator itr = temp1.begin(); itr != temp1.end(); ++itr) {
    (*itr)->change_household(h2);
  }

  // move second group into house h1
  for(std::vector<Person*>::iterator itr = temp2.begin(); itr != temp2.end(); ++itr) {
    (*itr)->change_household(h1);
  }

  FRED_VERBOSE(1, "HOUSING: swapped house %s with %d beds and %d occupants with %s with %d beds and %d occupants\n",
	       h1->get_label(), h1->get_orig_size(), h1->get_size(), h2->get_label(), h2->get_orig_size(), h2->get_size());
}

void Place_List::combine_households(int house_index1, int house_index2) {

  Household* h1 = this->get_household(house_index1);
  Household* h2 = this->get_household(house_index2);
  if(h1 == NULL || h2 == NULL)
    return;

  FRED_VERBOSE(1, "HOUSING: combining house %s with %d beds and %d occupants with %s with %d beds and %d occupants\n",
	       h1->get_label(), h1->get_orig_size(), h1->get_size(), h2->get_label(), h2->get_orig_size(), h2->get_size());

  // get pointers to residents of house h2
  vector<Person*> temp2;
  temp2.clear();
  vector<Person*> housemates2 = h2->get_inhabitants();
  for(std::vector<Person*>::iterator itr = housemates2.begin(); itr != housemates2.end(); ++itr) {
    temp2.push_back(*itr);
  }

  // move into house h1
  for(std::vector<Person*>::iterator itr = temp2.begin(); itr != temp2.end(); ++itr) {
    (*itr)->change_household(h1);
  }

  printf("HOUSING: combined house %s with %d beds and %d occupants with %s with %d beds and %d occupants\n",
	 h1->get_label(), h1->get_orig_size(), h1->get_size(), h2->get_label(), h2->get_orig_size(), h2->get_size());

}

Hospital* Place_List::get_hospital_assigned_to_household(Household* hh) {
  assert(this->is_load_completed());
  if(this->hh_label_hosp_label_map.find(string(hh->get_label())) != this->hh_label_hosp_label_map.end()) {
    string hosp_label = this->hh_label_hosp_label_map.find(string(hh->get_label()))->second;
    if(this->hosp_label_hosp_id_map.find(hosp_label) != this->hosp_label_hosp_id_map.end()) {
      int hosp_id = this->hosp_label_hosp_id_map.find(hosp_label)->second;
      return static_cast<Hospital*>(this->get_hospital(hosp_id));
    } else {
      return NULL;
    }
  } else {
    if(Place_List::Household_hospital_map_file_exists) {
      //List is incomplete so set this so we can print out a new file
      Place_List::Household_hospital_map_file_exists = false;
    }

    Hospital* hosp = NULL;
    if(hh->get_size() > 0) {
      Person* per = hh->get_enrollee(0);
      assert(per != NULL);
      if(Global::Enable_Health_Insurance) {
        hosp = this->get_random_open_hospital_matching_criteria(0, per, true);
      } else {
        hosp = this->get_random_open_hospital_matching_criteria(0, per, false);
      }

      //If it still came back with nothing, ignore health insurance
      if(hosp == NULL) {
        hosp = this->get_random_open_hospital_matching_criteria(0, per, false);
      }
    }
    assert(hosp != NULL);
    return hosp;
  }
}

void Place_List::update_population_dynamics(int day) {

  if(!Global::Enable_Population_Dynamics) {
    return;
  }

  int number_counties = this->counties.size();
  for(int i = 0; i < number_counties; ++i) {
    this->counties[i]->update(day);
  }

}


///////////////////// County Methods 

int Place_List::get_fips_of_county_with_index(int index) {
  if(index < 0) {
    return 99999;
  }
  assert(index < this->counties.size());
  return this->counties[index]->get_fips();
}

int Place_List::get_population_of_county_with_index(int index) {
  if(index < 0) {
    return 0;
  }
  assert(index < this->counties.size());
  return this->counties[index]->get_current_popsize();
}

int Place_List::get_population_of_county_with_index(int index, int age) {
  if(index < 0) {
    return 0;
  }
  assert(index < this->counties.size());
  int retval = this->counties[index]->get_current_popsize(age);
  return (retval < 0 ? 0 : retval);
}

int Place_List::get_population_of_county_with_index(int index, int age, char sex) {
  if(index < 0) {
    return 0;
  }
  assert(index < this->counties.size());
  int retval = this->counties[index]->get_current_popsize(age, sex);
  return (retval < 0 ? 0 : retval);
}

int Place_List::get_population_of_county_with_index(int index, int age_min, int age_max, char sex) {
  if(index < 0) {
    return 0;
  }
  assert(index < this->counties.size());
  int retval = this->counties[index]->get_current_popsize(age_min, age_max, sex);
  return (retval < 0 ? 0 : retval);
}

void Place_List::increment_population_of_county_with_index(int index, Person* person) {
  if(index < 0) {
    return;
  }
  assert(index < this->counties.size());
  int fips = this->counties[index]->get_fips();
  bool test = this->counties[index]->increment_popsize(person);
  assert(test);
  return;
}

void Place_List::decrement_population_of_county_with_index(int index, Person* person) {
  if(index < 0) {
    return;
  }
  assert(index < this->counties.size());
  bool test = this->counties[index]->decrement_popsize(person);
  assert(test);
  return;
}

void Place_List::report_county_populations() {
  for(int index = 0; index < this->counties.size(); ++index) {
    this->counties[index]->report_county_population();
  }
}

void Place_List::update_geo_boundaries(fred::geo lat, fred::geo lon) {
  // update max and min geo coords
  if(lat != 0.0) {
    if(lat < this->min_lat) {
      this->min_lat = lat;
    }
    if(this->max_lat < lat) {
      this->max_lat = lat;
    }
  }
  if(lon != 0.0) {
    if(lon < this->min_lon) {
      this->min_lon = lon;
    }
    if(this->max_lon < lon) {
      this->max_lon = lon;
    }
  }
  return;
}
