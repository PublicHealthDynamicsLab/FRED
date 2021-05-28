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
// File: Place_Type.cc
//

#include "Condition.h"
#include "Group_Type.h"
#include "Neighborhood_Layer.h"
#include "Property.h"
#include "Person.h"
#include "Place.h"
#include "Place_Type.h"
#include "Random.h"
#include "Utils.h"

//////////////////////////
//
// STATIC VARIABLES
//
//////////////////////////

std::vector <Place_Type*> Place_Type::place_types;
std::vector<std::string> Place_Type::names;
std::unordered_map<Person*, Place*> Place_Type::host_place_map;


Place_Type::Place_Type(int _id, string _name) : Group_Type(_name) {
  this->places.clear();
  this->base_type_id = -1;
  this->enable_visualization = 0;
  this->max_dist = 0.0;
  this->report_size = -1;
  this->partition_name = "NONE";
  this->partition_type_id = -1;
  this->partition_capacity = 999999999;
  this->partition_basis = "none";
  this->min_age_partition = 0;
  this->max_age_partition = Demographics::MAX_AGE;
  this->medical_vacc_exempt_rate = 0.0;
  this->enable_vaccination_rates = 0;
  this->default_vaccination_rate = 95.0;
  this->need_to_get_vaccination_rates = 0;
  this->elevation_cutoffs = {0,0,0,0,0,0,0};
  this->size_cutoffs = {0,0,0,0,0,0,0};
  this->income_cutoffs = {0,0,0,0,0,0,0};
  this->next_sp_id = 700000000 + _id * 1000000 + 1;
  Group_Type::add_group_type(this);
}

Place_Type::~Place_Type() {
}

void Place_Type::get_properties(){

  // first get the base class properties
  Group_Type::get_properties();

  FRED_STATUS(0, "place_type %s read_properties entered\n", this->name.c_str());
  
  // property name to fill in
  char property_name[FRED_STRING_SIZE];

  // optional properties:
  Property::disable_abort_on_failure();

  this->enable_visualization = 0;
  sprintf(property_name, "%s.enable_visualization", this->name.c_str());
  if(Property::does_property_exist(property_name)) {
    Property::get_property(property_name, &this->enable_visualization);
  } else {
    sprintf(property_name, "%s_enable_visualization", this->name.c_str());
    Property::get_property(property_name, &this->enable_visualization);
  }

  this->report_size = 0;
  /*
  sprintf(property_name, "%s.report_size", this->name.c_str());
  if (Property::does_property_exist(property_name)) {
    Property::get_property(property_name, &this->report_size);
  }
  */

  this->base_type_id = 0;
  string base_type = "Household";
  sprintf(property_name, "%s.base_type", this->name.c_str());
  if(Property::does_property_exist(property_name)) {
    Property::get_property(property_name, &base_type);
    this->base_type_id = Place_Type::get_type_id(base_type);
  }

  // control for gravity model
  this->max_dist = 99.0;
  sprintf(property_name, "%s.max_dist", this->name.c_str());
  if(Property::does_property_exist(property_name)) {
    Property::get_property(property_name, &this->max_dist);
  } else {
    sprintf(property_name, "%s_max_dist", this->name.c_str());
    Property::get_property(property_name, &this->max_dist);
  }

  // partition place_type
  char type_str[FRED_STRING_SIZE];

  sprintf(type_str, "NONE");
  sprintf(property_name, "%s.partition", this->name.c_str());
  if(Property::does_property_exist(property_name)) {
    Property::get_property(property_name, type_str);
  } else {
    sprintf(property_name, "%s_partition", this->name.c_str());
    Property::get_property(property_name, type_str);
  }
  this->partition_name = string(type_str);

  sprintf(property_name, "%s.partition_basis", this->name.c_str());
  if(Property::does_property_exist(property_name)) {
    Property::get_property(property_name, &this->partition_basis);
  } else {
    sprintf(property_name, "%s_partition_basis", this->name.c_str());
    Property::get_property(property_name, &this->partition_basis);
  }

  sprintf(property_name, "%s.partition_min_age", this->name.c_str());
  if(Property::does_property_exist(property_name)) {
    Property::get_property(property_name, &this->min_age_partition);
  } else {
    sprintf(property_name, "%s_%s_min_age", this->name.c_str(), type_str);
    Property::get_property(property_name, &this->min_age_partition);
  }

  sprintf(property_name, "%s.partition_max_age", this->name.c_str());
  if(Property::does_property_exist(property_name)) {
    Property::get_property(property_name, &this->max_age_partition);
  } else {
    sprintf(property_name, "%s_%s_max_age", this->name.c_str(), type_str);
    Property::get_property(property_name, &this->max_age_partition);
  }

  sprintf(property_name, "%s.partition_size", this->name.c_str());
  if(Property::does_property_exist(property_name)) {
    Property::get_property(property_name, &this->partition_capacity);
  } else {
    sprintf(property_name, "%s_%s_size", this->name.c_str(), type_str);
    Property::get_property(property_name, &this->partition_capacity);
  }

  sprintf(property_name, "%s.partition_capacity", this->name.c_str());
  if(Property::does_property_exist(property_name)) {
    Property::get_property(property_name, &this->partition_capacity);
  } else {
    sprintf(property_name, "%s_%s_capacity", this->name.c_str(), type_str);
    Property::get_property(property_name, &this->partition_capacity);
  }

  // vaccination rates
  sprintf(property_name, "enable_%s_vaccination_rates", this->name.c_str());
  Property::get_property(property_name, &this->enable_vaccination_rates);
  if(this->enable_vaccination_rates) {
    sprintf(property_name, "%s_vaccination_rate_file",  this->name.c_str());
    Property::get_property(property_name, this->vaccination_rate_file);
    this->need_to_get_vaccination_rates = 1;
  } else {
    strcpy(this->vaccination_rate_file, "none");
    this->need_to_get_vaccination_rates = 0;
  }

  sprintf(property_name, "default_%s_vaccination_rate", this->name.c_str());
  Property::get_property(property_name, &this->default_vaccination_rate);

  sprintf(property_name, "medical_vacc_exempt_rate");
  Property::get_property(property_name, &this->medical_vacc_exempt_rate);

  Property::set_abort_on_failure();

  FRED_STATUS(0, "place_type %s read_properties finished\n", this->name.c_str());
}

void Place_Type::prepare_place_types() {
  for(int type_id = 0; type_id < Place_Type::place_types.size(); ++type_id) {
    Place_Type::place_types[type_id]->prepare(); 
  }
}


void Place_Type::add_place(Place* place) {
  place->set_index(this->places.size());
  this->places.push_back(place);
}

void Place_Type::prepare() {

  FRED_STATUS(0, "place_type %s prepare entered\n", this->name.c_str());

  // this needs to be done after all places are available

  int number = get_number_of_places();
  double* values = new double [number];

  // update size boundaries
  sprintf(this->size_cutoffs.name, "%s_%s", this->name.c_str(), "size");
  for(int p = 0; p < number; ++p) {
    values[p] = get_place(p)->get_size();
    // printf("GET_SIZE %s value[%d] = %.1f\n", this->name.c_str(), p, values[p]);
  }
  Place_Type::set_cutoffs(&this->size_cutoffs, values, number);

  // update elevation boundaries
  sprintf(this->elevation_cutoffs.name, "%s_%s", this->name.c_str(), "elevation");
  for(int p = 0; p < number; ++p) {
    values[p] = get_place(p)->get_elevation();
    // printf("GET_ELEV value[%d] = %.1f\n", p, values[p]);
  }
  Place_Type::set_cutoffs(&this->elevation_cutoffs, values, number);

  // update income boundaries
  sprintf(this->income_cutoffs.name, "%s_%s", this->name.c_str(), "income");
  for(int p = 0; p < number; ++p) {
    values[p] = get_place(p)->get_income();
    // printf("GET_INC value[%d] = %.1f\n", p, values[p]);
  }

  Place_Type::set_cutoffs(&this->income_cutoffs, values, number);
  if(this->has_admin) {
    for(int p = 0; p < number; ++p) {
      get_place(p)->create_administrator();
    }
  }

  FRED_STATUS(0, "place_type %s prepare finished\n", this->name.c_str());
}


//////////////////////////
//
// STATIC METHODS
//
//////////////////////////

void Place_Type::get_place_type_properties() {

  Place_Type::place_types.clear();

  // read in the list of place_type names
  char property_name[FRED_STRING_SIZE];
  char property_value[FRED_STRING_SIZE];

  for(int type_id = 0; type_id < Place_Type::names.size(); ++type_id) {

    // create new Place_Type object
    Place_Type * place_type = new Place_Type(type_id, Place_Type::names[type_id]);

    // add to place_type list
    Place_Type::place_types.push_back(place_type);

    // get properties for this place type
    place_type->get_properties();

    printf("CREATED_PLACE_TYPE place_type %d = %s\n", type_id, Place_Type::names[type_id].c_str());

  }

  // setup partitions
  for(int type_id = 0; type_id < Place_Type::place_types.size(); ++type_id) {
    Place_Type* place_type = Place_Type::place_types[type_id];
    place_type->partition_type_id = Place_Type::get_type_id(place_type->partition_name);
    if(place_type->partition_type_id > -1) {
      FRED_VERBOSE(0, "PARTITION for %s name = %s p_id = %d\n",
          place_type->name.c_str(), place_type->partition_name.c_str(), place_type->partition_type_id);
    }
  }
}

void Place_Type::read_places(const char* pop_dir) {

  FRED_VERBOSE(0, "read_places from %s entered\n", pop_dir);

  for(int type_id = 0; type_id < Place_Type::place_types.size(); ++type_id) {
    Place_Type* place_type = Place_Type::get_place_type(type_id);
    if(is_predefined(place_type->name)) {
      continue;
    }

    if(place_type->file_available) {
      string filename = Utils::str_tolower(place_type->name);
      char location_file[FRED_STRING_SIZE];
      sprintf(location_file, "%s/%ss.txt", pop_dir, filename.c_str());
      FRED_VERBOSE(0, "place_type name %s filename %s location_file %s\n",
          place_type->name.c_str(),
          filename.c_str(),
          location_file);
      FRED_VERBOSE(0,"read_place_file %s\n", location_file);
      Place::read_place_file(location_file, type_id);
    } else {
      FRED_VERBOSE(0,"place_type name no location_file available\n", place_type->name.c_str());

      // read any location for this place_type from the FRED program
      string prop_name = place_type->name + ".add";
      string value = "";
      int n = Property::get_next_property(prop_name, &value, 0);
      while(0 <= n) {
        long long int sp_id;
        double lat, lon, elevation;
        char label[FRED_STRING_SIZE];
        char place_subtype = Place::SUBTYPE_NONE;
        sscanf(value.c_str(), "%lld %lf %lf %lf", &sp_id, &lat, &lon, &elevation);
        if(sp_id == 0) {
          sp_id = place_type->get_next_sp_id();
        }
        if(!Group::sp_id_exists(sp_id)) {
          sprintf(label, "%s-%lld", place_type->name.c_str(), sp_id);
          FRED_VERBOSE(0, "ADD_PLACE %s |%s| %s, sp_id %lld lat %f lon %f elev %f\n", place_type->name.c_str(),
              value.c_str(), label, sp_id, lat, lon, elevation);
          Place* place = Place::add_place(label, type_id, place_subtype, lon, lat, elevation, 0);
          place->set_sp_id(sp_id);
        }

        // get next place location
        n = Property::get_next_property(prop_name, &value, n + 1);
      }
    }
  }

  FRED_VERBOSE(0, "read_places from %s finished\n", pop_dir);
  return;
}


void Place_Type::add_places_to_neighborhood_layer() {
  FRED_VERBOSE(1, "add_place_to_neighborhood_layer entered place_types %d\n", (int) place_types.size());

  for(int type_id = 0; type_id < place_types.size(); ++type_id) {
    int size = Place_Type::place_types[type_id]->places.size();
    FRED_VERBOSE(0, "add_place_to_neighborhood_layer entered place_type %d size %d\n", type_id, size);
    for(int p = 0; p < size; ++p) {
      Place* place = Place_Type::place_types[type_id]->places[p];
      if(place == NULL) {
        // FRED_VERBOSE(0, "add_place_to_neighborhood_layer place %d is NULL\n", p);
      } else {
        // FRED_VERBOSE(0, "add_place_to_neighborhood_layer place %d is %s\n", p, place->get_label());
        Global::Neighborhoods->add_place(place);
      }
    }
  }

  FRED_VERBOSE(1, "add_place_to_neighborhood_layer finished\n");
}

void Place_Type::setup_partitions() {
  FRED_VERBOSE(0, "setup_partitions entered for type %s partition_type %d basis %s\n",
      this->name.c_str(), this->partition_type_id, this->partition_basis.c_str());
  if(this->partition_type_id > -1) {
    int number_of_places = get_number_of_places();
    for(int p = 0; p < number_of_places; ++p) {
      this->places[p]->setup_partitions(this->partition_type_id, this->partition_capacity,
          this->partition_basis,
          this->min_age_partition,
          this->max_age_partition);
    }
  }
}

void Place_Type::prepare_vaccination_rates() {

  if(this->need_to_get_vaccination_rates) {

    // do this just once for each place type
    this->need_to_get_vaccination_rates = 0;

    if(strcmp(this->vaccination_rate_file, "none") != 0) {
      FILE *fp = Utils::fred_open_file(this->vaccination_rate_file);
      if(fp != NULL) {
        char label[FRED_STRING_SIZE];
        double rate;
        while(fscanf(fp, "%s %lf", label, &rate) == 2) {
          // printf("VAX: %s %f\n", label, rate);
          // find place and set it vaccination rates
          Place* place = Place::get_school_from_label(label);
          if(place != NULL) {
            place->set_vaccination_rate(rate);
            printf("VAX: school %s %s rate %f %f size %d\n",
                place->get_label(), label,
                place->get_vaccination_rate(), rate,
                place->get_size());
          } else {
            printf("VAX: %s %f -- label not found\n", label, rate);
          }
        }	fclose(fp);
      }
    }
  }
}

Place* Place_Type::generate_new_place(int place_type_id, Person* person) {
  // FRED_VERBOSE(0, "generate_new_place type %d\n", place_type_id);
  if(place_type_id < 0 || Place_Type::place_types.size() <= place_type_id) {
    return NULL;
  } else {
    return Place_Type::place_types[place_type_id]->generate_new_place(person);
  }
}

Place* Place_Type::generate_new_place(Person* person) {
  // FRED_VERBOSE(0, "generate_new_place type %s person %d\n", this->name.c_str(), person->get_id());
  Place* place = Place_Type::get_place_hosted_by(person);
  if(place != NULL) {
    // FRED_VERBOSE(0, "Already hosts place %s\n", place->get_label());
    return place;
  }

  Place* source = person->get_place_of_type(this->base_type_id);
  if(source == NULL) {
    // FRED_VERBOSE(0, "base_type_id %d source is NULL\n", this->base_type_id);
    return NULL;
  }

  double lon = source->get_longitude();
  double lat = source->get_latitude();
  double elevation = source->get_elevation();
  long long int census_tract_admin_code = source->get_census_tract_admin_code();
  char label[FRED_STRING_SIZE];
  long long int sp_id = get_next_sp_id();
  sprintf(label, "%s-%lld", this->name.c_str(), sp_id);
      
  // create a new place
  place = Place::add_place(label, Group_Type::get_type_id(this->name), 'x', lon, lat, elevation, census_tract_admin_code);
  place->set_sp_id(sp_id);
  place->set_host(person);
  Place_Type::host_place_map[person] = place;
  // create an administrator if needed
  if(this->has_admin) {
    place->create_administrator();
    // setup admin agents in epidemics
    Condition::initialize_person(place->get_administrator());
  }
  FRED_VERBOSE(1, "GENERATE_NEW_PLACE place %s type %d %d lat %f lon %f elev %f admin_code %lu  age of host = %d\n",
      place->get_label(),
      get_type_id(this->name), place->get_type_id(),
      place->get_latitude(),
      place->get_longitude(),
      place->get_elevation(),
      place->get_census_tract_admin_code(),
      place->get_host()->get_age());

  return place;
}

void Place_Type::report_contacts() {
  for(int id = 0; id < Place_Type::get_number_of_place_types(); ++id) {
    Place_Type* place_type = Place_Type::get_place_type(id);
    place_type->report_contacts_for_place_type();
  }
}

void Place_Type::report_contacts_for_place_type() {
  char filename[FRED_STRING_SIZE];
  int size = get_number_of_places();
  sprintf(filename, "%s/age-age-%s.txt", Global::Simulation_directory, get_name());
  FILE* fp = fopen(filename, "w");
  for(int i = 0; i < size; ++i) {
    Place* place = get_place(i);
    int n = place->get_size();
    int ages[n];
    for(int p = 0; p < n; ++p) {
      ages[p] = place->get_member(p)->get_age();
    }
    for(int j = 0; j < n; ++j) {
      for(int k = 0; k < n; ++k) {
        if(j != k) {
          fprintf(fp, "%d %d\n", ages[j], ages[k]);
        }
      }
    }
  }
  fclose(fp);
}

Place* Place_Type::select_place(Person* person) {
  int size = get_number_of_places();
  int n = Random::draw_random_int(0, size - 1);
  // FRED_VERBOSE(0, "select_place: size %d n %d label %s\n", size,n,get_place(n)? get_place(n)->get_label(): "NULL");
  return get_place(n);
}

Place* Place_Type::select_place_of_type(int place_type_id, Person* person) {
  return Place_Type::get_place_type(place_type_id)->select_place(person);
}

void Place_Type::report(int day) {
  if(this->report_size) {
    int number = get_number_of_places();
    for(int p = 0; p < number; ++p) {
      if(get_place(p)->is_reporting_size()) {
        get_place(p)->report_size(day);
      }
    }
  }
}

void Place_Type::finish_place_types() {
  for(int type_id = 0; type_id < Place_Type::place_types.size(); ++type_id) {
    Place_Type::place_types[type_id]->finish(); 
  }
}

void Place_Type::finish() {

  if(this->report_size == 0) {
    return;
  }

  char dir[FRED_STRING_SIZE];
  char outfile[FRED_STRING_SIZE];
  FILE* fp;

  sprintf(dir, "%s/RUN%d/DAILY", Global::Simulation_directory, Global::Simulation_run_number);
  Utils::fred_make_directory(dir);

  for (int i = 0; i < get_number_of_places(); ++i) {
    sprintf(outfile, "%s/%s.SizeOf%s%03d.txt", dir, this->name.c_str(), this->name.c_str(), i);
    fp = fopen(outfile, "w");
    if(fp == NULL) {
      Utils::fred_abort("Fred: can't open file %s\n", outfile);
    }
    for(int day = 0; day < Global::Simulation_Days; ++day) {
      fprintf(fp, "%d %d\n", day, get_place(i)->get_size_on_day(day));
    }
    fclose(fp);
  }

  // create a csv file for this place_type

  // this joins two files with same value in column 1, from
  // https://stackoverflow.com/questions/14984340/using-awk-to-process-input-from-multiple-files
  char awkcommand[FRED_STRING_SIZE];
  sprintf(awkcommand, "awk 'FNR==NR{a[$1]=$2 FS $3;next}{print $0, a[$1]}' ");

  char command[FRED_STRING_SIZE];
  char dailyfile[FRED_STRING_SIZE];
  sprintf(outfile, "%s/RUN%d/%s.csv", Global::Simulation_directory, Global::Simulation_run_number, this->name.c_str());

  sprintf(dailyfile, "%s/%s.SizeOf%s000.txt", dir, this->name.c_str(), this->name.c_str());
  sprintf(command, "cp %s %s", dailyfile, outfile);
  system(command);

  for(int i = 0; i < get_number_of_places(); ++i) {
    if(i > 0) {
      sprintf(dailyfile, "%s/%s.SizeOf%s%03d.txt", dir, this->name.c_str(), this->name.c_str(), i);
      sprintf(command, "%s %s %s > %s.tmp; mv %s.tmp %s", awkcommand, dailyfile, outfile, outfile, outfile, outfile);
      system(command);
    }
  }  
  
  // create a header line for the csv file
  char headerfile[FRED_STRING_SIZE];
  sprintf(headerfile, "%s/RUN%d/%s.header", Global::Simulation_directory, Global::Simulation_run_number, this->name.c_str());
  fp = fopen(headerfile, "w");
  fprintf(fp, "Day ");
  for(int i = 0; i < get_number_of_places(); ++i) {
    fprintf(fp, "%s.SizeOf%s%03d ", this->name.c_str(), this->name.c_str(), i);
  }
  fprintf(fp, "\n");
  fclose(fp);
  
  // concatenate header line
  sprintf(command, "cat %s %s > %s.tmp; mv %s.tmp %s; unlink %s", headerfile, outfile, outfile, outfile, outfile, headerfile);
  system(command);
  
  // replace spaces with commas
  sprintf(command, "sed -E 's/ +/,/g' %s | sed -E 's/,$//' | sed -E 's/,/ /' > %s.tmp; mv %s.tmp %s", outfile, outfile, outfile, outfile);
  system(command);
  
}

void Place_Type::report_place_size(int place_type_id) {
  Place_Type::get_place_type(place_type_id)->report_size = 1;
}

bool Place_Type::is_predefined(string value) {
  return (Utils::str_tolower(value) == "household" ||
      Utils::str_tolower(value) == "neighborhood" ||
      Utils::str_tolower(value) == "school" ||
      Utils::str_tolower(value) == "classroom" ||
      Utils::str_tolower(value) == "workplace" ||
      Utils::str_tolower(value) == "office" ||
      Utils::str_tolower(value) == "hospital");
}

void Place_Type::set_cutoffs(cutoffs_t* cutoffs, double* values, int size) {
  std::sort(values,values+size);
  if(size > 0) {
    cutoffs->first_quintile = values[int(0.2*size)];
    cutoffs->second_quintile = values[int(0.4*size)];
    cutoffs->third_quintile = values[int(0.6*size)];
    cutoffs->fourth_quintile = values[int(0.8*size)];
    cutoffs->first_quartile = values[int(0.25*size)];
    cutoffs->second_quartile = values[int(0.5*size)];
    cutoffs->third_quartile = values[int(0.75*size)];
  } else {
    cutoffs->first_quintile = 0.0;
    cutoffs->second_quintile = 0.0;
    cutoffs->third_quintile = 0.0;
    cutoffs->fourth_quintile = 0.0;
    cutoffs->first_quartile = 0.0;
    cutoffs->second_quartile = 0.0;
    cutoffs->third_quartile = 0.0;
  }
  FRED_VERBOSE(0, "CUTOFFS set_cutoffs for %s | quartiles %.1f %.1f %.1f | quintiles %.1f %.1f %.1f %.1f\n",
      cutoffs->name,
      cutoffs->first_quartile, cutoffs->second_quartile, cutoffs->third_quartile,
      cutoffs->first_quintile, cutoffs->second_quintile, cutoffs->third_quintile, cutoffs->fourth_quintile);
}

int Place_Type::get_size_quartile(int n) {
  if(n <= this->size_cutoffs.first_quartile) {
    return 1;
  }
  if(n <= this->size_cutoffs.second_quartile) {
    return 2;
  }
  if(n <= this->size_cutoffs.third_quartile) {
    return 3;
  }
  return 4;
}

int Place_Type::get_size_quintile(int n) {
  if(n <= this->size_cutoffs.first_quintile) {
    return 1;
  }
  if(n <= this->size_cutoffs.second_quintile) {
    return 2;
  }
  if(n <= this->size_cutoffs.third_quintile) {
    return 3;
  }
  if(n <= this->size_cutoffs.fourth_quintile) {
    return 4;
  }
  return 5;
}

int Place_Type::get_income_quartile(int n) {
  if(n <= this->income_cutoffs.first_quartile) {
    return 1;
  }
  if(n <= this->income_cutoffs.second_quartile) {
    return 2;
  }
  if(n <= this->income_cutoffs.third_quartile) {
    return 3;
  }
  return 4;
}

int Place_Type::get_income_quintile(int n) {
  if(n <= this->income_cutoffs.first_quintile) {
    return 1;
  }
  if(n <= this->income_cutoffs.second_quintile) {
    return 2;
  }
  if(n <= this->income_cutoffs.third_quintile) {
    return 3;
  }
  if(n <= this->income_cutoffs.fourth_quintile) {
    return 4;
  }
  return 5;
}

int Place_Type::get_elevation_quartile(double n) {
  if(n <= this->elevation_cutoffs.first_quartile) {
    return 1;
  }
  if(n <= this->elevation_cutoffs.second_quartile) {
    return 2;
  }
  if(n <= this->elevation_cutoffs.third_quartile) {
    return 3;
  }
  return 4;
}

int Place_Type::get_elevation_quintile(double n) {
  if(n <= this->elevation_cutoffs.first_quintile) {
    return 1;
  }
  if(n <= this->elevation_cutoffs.second_quintile) {
    return 2;
  }
  if(n <= this->elevation_cutoffs.third_quintile) {
    return 3;
  }
  if(n <= this->elevation_cutoffs.fourth_quintile) {
    return 4;
  }
  return 5;
}
