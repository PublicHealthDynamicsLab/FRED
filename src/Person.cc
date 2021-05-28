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
// File: Person.cc
//

#include "Person.h"

#include "Census_Tract.h"
#include "Clause.h"
#include "Condition.h"
#include "County.h"
#include "Date.h"
#include "Demographics.h"
#include "Epidemic.h"
#include "Factor.h"
#include "Global.h"
#include "Expression.h"
#include "Geo.h"
#include "Group.h"
#include "Group_Type.h"
#include "Household.h"
#include "Link.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include "Network.h"
#include "Network_Type.h"
#include "Property.h"
#include "Place.h"
#include "Place_Type.h"
#include "Preference.h"
#include "Random.h"
#include "Rule.h"
#include "Travel.h"
#include "Utils.h"

#include <cstdio>
#include <vector>
#include <sstream>
#include <unordered_set>

// static variables
person_vector_t Person::people;
person_vector_t Person::admin_agents;
person_vector_t Person::death_list;
person_vector_t Person::migrant_list;
person_vector_t Person::report_person;
std::vector<int> Person::id_map;
std::vector<report_t*> Person::report_vec;
int Person::max_reporting_agents = 100;
int Person::pop_size = 0;
int Person::next_id = 0;
int Person::next_meta_id = -1;
Person* Person::Import_agent = NULL;
std::unordered_map<Person*, Group*> Person::admin_group_map;
bool Person::record_location = false;
  
// personal variables
std::vector<std::string> Person::var_name;
int Person::number_of_vars = 0;
std::vector<std::string> Person::list_var_name;
int Person::number_of_list_vars = 0;
double* Person::var_init_value = NULL;

// global variables
std::vector<std::string> Person::global_var_name;
int Person::number_of_global_vars = 0;
std::vector<std::string> Person::global_list_var_name;
int Person::number_of_global_list_vars = 0;
double* Person::global_var;
double_vector_t* Person::global_list_var;

// used during input
bool Person::load_completed = false;
int Person::enable_copy_files = 0;
  
  // output data
int Person::report_initial_population = 0;
int Person::output_population = 0;
char Person::pop_outfile[FRED_STRING_SIZE];
char Person::output_population_date_match[FRED_STRING_SIZE];
int Person::Popsize_by_age [Demographics::MAX_AGE+1];

// static variables
bool Person::is_initialized = false;
double Person::health_insurance_distribution[Insurance_assignment_index::UNSET];
int Person::health_insurance_cdf_size = 0;

Person::Person() {
  this->id = -1;
  this->index = -1;
  this->eligible_to_migrate = true;
  this->native = true;
  this->original = false;
  this->vaccine_refusal = false;
  this->ineligible_for_vaccine = false;
  this->received_vaccine = false;
  this->init_age = -1;
  this->sex = 'n';
  this->birthday_sim_day = -1;
  this->deceased = false;
  this->household_relationship = -1;
  this->race = -1;
  this->number_of_children = -1;
  this->alive = true;
  this->previous_infection_serotype = 0;
  this->insurance_type = Insurance_assignment_index::UNSET;
  this->condition = NULL;
  this->var = NULL;
  this->home_neighborhood = NULL;
  this->profile = Activity_Profile::UNDEFINED;
  this->schedule_updated = -1;
  this->stored_activity_groups = NULL;
  this->primary_healthcare_facility = NULL;
  this->is_traveling = false;
  this->is_traveling_outside = false;
  this->is_hospitalized = false;
  this->sim_day_hospitalization_ends = -1;
  this->last_school = NULL;
  this->return_from_travel_sim_day = -1;
  this->in_parents_home = false;
  this->link = new Link [Group_Type::get_number_of_group_types()];
}

Person::~Person() {
}

void Person::setup(int _index, int _id, int _age, char _sex,
		   int _race, int rel, Place* house, Place* school, Place* work,
		   int day, bool today_is_birthday) {
  FRED_VERBOSE(1, "Person::setup() id %d age %d house %s school %s work %s\n",
	       _id, _age, house ? house->get_label():"NULL", school? school->get_label():"NULL", work?work->get_label():"NULL");
  this->index = _index;
  this->id = _id;

  // adjust age for those over 89 (due to binning in the synthetic pop)
  if(_age > 89) {
    _age = 90;
    while(_age < Demographics::MAX_AGE && Random::draw_random() < 0.6) _age++;
    // FRED_VERBOSE(0, "AGE ADJUSTED: person %d age %d\n", get_id(), _age);
  }

  // set demographic variables
  this->init_age = _age;
  this->sex = _sex;
  this->race = _race;
  this->household_relationship = rel;
  this->deceased = false;
  this->number_of_children = 0;

  if(today_is_birthday) {
    this->birthday_sim_day = day;
  } else {
    // set the agent's birthday relative to simulation day
    this->birthday_sim_day = day - 365 * _age;
    // adjust for leap years:
    this->birthday_sim_day -= (_age / 4);
    // pick a random birthday in the previous year
    this->birthday_sim_day -= Random::draw_random_int(1,365);
  }
  /*
  if(today_is_birthday) {
    FRED_VERBOSE(0, "Baby index %d id %d age %d born on day %d household %s  new_size %d original_size %d\n",
		 _index, this->id, age, day, house->get_label(), house->get_size(), house->get_original_size());
  }
  */
  setup_conditions();
  // meta_agents have no activities
  if (0 <= this->id) {
    setup_activities(house, school, work);
  }

}

void Person::print(FILE* fp, int condition_id) {
  if(fp == NULL) {
    return;
  }
  fprintf(fp, "id %7d  age %3d  sex %c  race %d\n",
          id,
          get_age(),
          get_sex(),
          get_race());
  fflush(fp);
}

int Person::get_number_of_people_in_group_in_state(int type_id, int condition_id, int state_id) {
  Group* group = get_activity_group(type_id);
  if(group != NULL) {
    int count = 0;
    int size = group->get_size();
    for (int i = 0; i < size; i++) {
      Person* person = group->get_member(i);
      if (person->get_state(condition_id) == state_id) {
	count++;
      }
    }
    return count;
  }
  return 0;
}

int Person::get_number_of_other_people_in_group_in_state(int type_id, int condition_id, int state_id) {
  Group* group = get_activity_group(type_id);
  if(group != NULL) {
    int count = 0;
    int size = group->get_size();
    for (int i = 0; i < size; i++) {
      Person* person = group->get_member(i);
      if (person != this && person->get_state(condition_id) == state_id) {
	count++;
      }
    }
    return count;
  }
  return 0;
}

Person* Person::give_birth(int day) {
  int age = 0;
  char sex = (Random::draw_random(0.0, 1.0) < 0.5 ? 'M' : 'F');
  int race = get_race();
  int rel = Household_Relationship::CHILD;
  Place* house = get_household();
  /*
    if (house == NULL) {
    printf("Mom %d has no household\n", this->get_id()); fflush(stdout);
    }
  */
  assert(house != NULL);
  Place* school = NULL;
  Place* work = NULL;
  bool today_is_birthday = true;
  Person* baby = Person::add_person_to_population(age, sex, race, rel,
					house, school, work, day, today_is_birthday);

  baby->initialize_conditions(day);

  this->number_of_children++;
  if(Global::Birthfp != NULL) {
    // report births
    fprintf(Global::Birthfp, "day %d mother %d age %d\n",
	    day, get_id(), get_age());
    fflush(Global::Birthfp);
  }

  FRED_VERBOSE(1, "mother %d baby %d\n", this->get_id(), baby->get_id());

  return baby;
}

std::string Person::to_string() {

  stringstream tmp_string_stream;
  tmp_string_stream << this->id << " " << this->get_age() << " " <<  this->get_sex() << " " ;
  tmp_string_stream << this->get_race() << " " ;
  tmp_string_stream << Place::get_place_label(this->get_household()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_school()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_classroom()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_workplace()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_office()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_neighborhood()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_hospital()) << " ";
  tmp_string_stream << this->get_household_relationship();

  return tmp_string_stream.str();
}

void Person::terminate(int day) {
  FRED_VERBOSE(1, "terminating person %d\n", id);
  terminate_conditions(day);
  terminate_activities();
  Demographics::terminate(this);

}

double Person::get_x() {
  Place* hh = this->get_household();
  if(hh == NULL) {
    return 0.0;
  } else {
    return hh->get_x();
  }
}

double Person::get_y() {
  Place* hh = this->get_household();
  if(hh == NULL) {
    return 0.0;
  } else {
    return hh->get_y();
  }
}

char* Person::get_household_structure_label() {
  return get_household()->get_household_structure_label();
}

int Person::get_income() {
  return get_household()->get_income();
}

int Person::get_place_elevation(int type) {
  Place* place = get_place_of_type(type);
  if (place != NULL) {
    return place->get_elevation();
  }
  else {
    return 0;
  }
}

int Person::get_place_income(int type) {
  Place* place = get_place_of_type(type);
  if (place != NULL) {
    return place->get_income();
  }
  else {
    return 0;
  }
}

void Person::quit_place_of_type(int place_type_id) {

  Place* place = get_place_of_type(place_type_id);
  int size = place==NULL ? -1 : place->get_size();
  FRED_VERBOSE(1, "person %d QUIT PLACE %s size %d\n",
	       this->id,
	       place ? place->get_label() : "NULL",
	       size);

  // decrement the group_state_counts associated with this place
  if (place!=NULL) {
    for (int cond_id = 0; cond_id < Condition::get_number_of_conditions(); cond_id++) {
      int state = get_state(cond_id);
      // printf("decrementing_group_state_count for person %d for cond %d state %d place %s\n",
      // this->id, cond_id, state, place->get_label());
      Condition::get_condition(cond_id)->decrement_group_state_count(place_type_id, place, state);
    }
  }

  set_activity_group(place_type_id, NULL);
  FRED_VERBOSE(1,"HEALTH RECORD: %s %s day %d person %d QUITS PLACE type %s label %s new size = %d\n",
	       Date::get_date_string().c_str(),
	       Date::get_12hr_clock().c_str(),
	       Global::Simulation_Day,
	       this->get_id(),
	       Place_Type::get_place_type_name(place_type_id).c_str(),
	       place == NULL ? "NULL" : place->get_label(),
	       size);
  if (Global::Enable_Records) {
    fprintf(Global::Recordsfp,
	    "HEALTH RECORD: %s %s day %d person %d QUITS PLACE type %s label %s new size = %d\n",
	    Date::get_date_string().c_str(),
	    Date::get_12hr_clock().c_str(),
	    Global::Simulation_Day,
	    this->get_id(),
	    Place_Type::get_place_type_name(place_type_id).c_str(),
	    place == NULL ? "NULL" : place->get_label(),
	    size);
  }

  size = place==NULL ? -1 : place->get_size();
  FRED_VERBOSE(1, "AFTER person %d QUIT PLACE %s size %d\n",
	       this->id,
	       place ? place->get_label() : "NULL",
	       size);

}

void Person::select_place_of_type(int place_type_id) {
  // FRED_VERBOSE(0, "person %d select_place_of_type %d entered\n", get_id(), place_type_id);
  Place* place = Place_Type::select_place_of_type(place_type_id, this);
  int size = place==NULL ? -1 : place->get_size();
  FRED_VERBOSE(1, "person %d JOIN PLACE %s size %d\n",
	       this->id,
	       place ? place->get_label() : "NULL",
	       size);

  set_activity_group(place_type_id, place);

  // increment the group_state_counts associated with this place
  if (place!=NULL) {
    for (int cond_id = 0; cond_id < Condition::get_number_of_conditions(); cond_id++) {
      int state = get_state(cond_id);
      // printf("incrementing_group_state_count for person %d for cond %d state %d place %s\n",
      // this->id, cond_id, state, place->get_label());
      Condition::get_condition(cond_id)->increment_group_state_count(place_type_id, place, state);
    }
  }
  FRED_VERBOSE(1,"HEALTH RECORD: %s %s day %d person %d JOINS PLACE type %s label %s new size = %d\n",
	       Date::get_date_string().c_str(),
	       Date::get_12hr_clock().c_str(),
	       Global::Simulation_Day,
	       this->get_id(),
	       Place_Type::get_place_type_name(place_type_id).c_str(),
	       place == NULL ? "NULL" : place->get_label(),
	       size);
  if (Global::Enable_Records) {
    fprintf(Global::Recordsfp,
	    "HEALTH RECORD: %s %s day %d person %d JOINS PLACE type %s label %s new size = %d\n",
	    Date::get_date_string().c_str(),
	    Date::get_12hr_clock().c_str(),
	    Global::Simulation_Day,
	    this->get_id(),
	    Place_Type::get_place_type_name(place_type_id).c_str(),
	    place == NULL ? "NULL" : place->get_label(),
	    size);
  }

  size = place==NULL ? -1 : place->get_size();
  FRED_VERBOSE(1, "AFTER person %d JOIN PLACE %s size %d\n",
	       this->id,
	       place ? place->get_label() : "NULL",
	       size);

}

void Person::join_network(int network_type_id) {
  // FRED_VERBOSE(0,"join_network type %d\n", network_type_id);
  Network* network = Network_Type::get_network_type(network_type_id)->get_network();
  join_network(network);
}

void Person::quit_network(int network_type_id) {
  FRED_VERBOSE(0,"quit_network type %d\n", network_type_id);
  this->link[network_type_id].remove_from_network(this);
}

void Person::report_place_size(int place_type_id) {
  Place* place = get_place_of_type(place_type_id);
  if (place!=NULL) {
    place->start_reporting_size();
    Place_Type::report_place_size(place_type_id);
  }
}

void Person::set_place_of_type(int type, Place* place) {
  set_activity_group(type, place);
}

Place* Person::get_place_of_type(int type) {
  if (type == Group_Type::HOSTED_GROUP) {
    return Place_Type::get_place_hosted_by(this);
  }
  if (type < Place_Type::get_number_of_place_types()) {
    return this->link[type].get_place();
  }
  return NULL;
}

Group* Person::get_group_of_type(int type) {
  if (type < 0) {
    return NULL;
  }
  if (type < Group_Type::get_number_of_group_types()) {
    return this->link[type].get_group();
  }
  return NULL;
}

Network* Person::get_network_of_type(int type) {
  if (type < Place_Type::get_number_of_place_types() + Network_Type::get_number_of_network_types()) {
    return this->link[type].get_network();
  }
  return NULL;
}

int Person::get_adi_state_rank() {
  return get_household()->get_adi_state_rank();
}


int Person::get_adi_national_rank() {
  return get_household()->get_adi_national_rank();
}

person_vector_t Person::get_placemates(int place_type_id, int maxn) {
  person_vector_t result;
  result.clear();
  Place* place = get_place_of_type(place_type_id);
  if (place != NULL) {
    int size = place->get_size();
    /*
    printf("placemates place %s size %d ", place->get_label(),size);
    for (int i = 0; i < size; i++) {
      printf("%d ", place->get_member(i)->get_id());
    }
    printf("\n");
    */
    if (size <= maxn) {
      for (int i = 0; i < size; i++) {
	Person* per2 = place->get_member(i);
	if (per2 != this) {
	  result.push_back(per2);
	}
      }
    }
    else {
      // randomize the order of processing the members
      std::vector<int> shuffle_index;
      shuffle_index.clear();
      shuffle_index.reserve(size);
      for(int i = 0; i < size; ++i) {
	shuffle_index.push_back(i);
      }
      FYShuffle<int>(shuffle_index);
      /*
      printf("shuffle: ");
      for(int i = 0; i < size; ++i) {
	printf("%d ", shuffle_index[i]);
      }
      printf("\n");
      */

      // return the first maxn
      for (int i = 0; i < maxn; i++) {
	result.push_back(place->get_member(shuffle_index[i]));
      }
    }
  }
  return result;
}

int Person::get_place_size(int type_id) {
  return get_group_size(type_id);
  /*
  Place* place = get_place_of_type(type_id);
  if (place == NULL) {
    return 0;
  }
  else {
    return place->get_size();
  }
  */
}


int Person::get_network_size(int type_id) {
  return get_group_size(type_id);
}


double Person::get_age_in_years() const {
  return double(Global::Simulation_Day - this->birthday_sim_day) / 365.25;
}

int Person::get_age_in_days() const {
  return Global::Simulation_Day - this->birthday_sim_day;
}

int Person::get_age_in_weeks() const {
  return get_age_in_days() / 7;
}

int Person::get_age_in_months() const {
  return (int) (get_age_in_years() / 12.0);
}

double Person::get_real_age() const {
  return double(Global::Simulation_Day - this->birthday_sim_day) / 365.25;
}

int Person::get_age() const {
  return (int) get_real_age();
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

///// STATIC METHODS

void Person::get_population_properties() {

  FRED_VERBOSE(0, "get_population_properties entered\n");
  
  // optional properties:
  Property::disable_abort_on_failure();

  Property::get_property("report_initial_population", &Person::report_initial_population);
  Property::get_property("output_population", &Person::output_population);
  Property::get_property("pop_outfile", Person::pop_outfile);
  Property::get_property("output_population_date_match",
		    Person::output_population_date_match);
  Property::get_property("max_reporting_agents", &Person::max_reporting_agents);

  // restore requiring properties
  Property::set_abort_on_failure();

  FRED_VERBOSE(0, "get_population_properties finish\n");

}

void Person::initialize_static_variables() {

  FRED_VERBOSE(0, "initialize_static_variables entered\n");

  FRED_VERBOSE(0, "initialize_static_variables entered\n");

  //Setup the static variables if they aren't already initialized
  if(!Person::is_initialized) {

    // read optional properties
    Property::disable_abort_on_failure();
  
    Person::record_location = false;
    int tmp;
    Property::get_property("record_location", &tmp);
    Person::record_location = tmp;

    if(Global::Enable_Health_Insurance) {

      Person::health_insurance_cdf_size = Property::get_property_vector((char*)"health_insurance_distribution", Person::health_insurance_distribution);

      // convert to cdf
      double stotal = 0;
      for(int i = 0; i < Person::health_insurance_cdf_size; ++i) {
        stotal += Person::health_insurance_distribution[i];
      }
      if(stotal != 100.0 && stotal != 1.0) {
        Utils::fred_abort("Bad distribution health_insurance_distribution must sum to 1.0 or 100.0\n");
      }
      double cumm = 0.0;
      for(int i = 0; i < Person::health_insurance_cdf_size; ++i) {
        Person::health_insurance_distribution[i] /= stotal;
        Person::health_insurance_distribution[i] += cumm;
        cumm = Person::health_insurance_distribution[i];
      }
    }

    // restore requiring properties
    Property::set_abort_on_failure();

    Person::is_initialized = true;
  }

  FRED_VERBOSE(0, "initialize_static_variables finished\n");
}


/*
 * All Persons in the population must be created using add_person_to_population
 */
Person* Person::add_person_to_population(int age, char sex, int race, int rel,
					     Place* house, Place* school, Place* work,
					     int day, bool today_is_birthday) {

  Person* person = new Person;
  int id = Person::next_id++;
  int idx = Person::people.size();
  // FRED_VERBOSE(0, "setup:\n");
  person->setup(idx, id, age, sex, race, rel, house, school, work, day, today_is_birthday);
  // FRED_VERBOSE(0, "setup finished\n");
  Person::people.push_back(person);
  Person::pop_size = Person::people.size();
  Person::id_map.push_back(idx);
  return person;
}

Person* Person::create_admin_agent() {
  Person* agent = create_meta_agent();
  Person::admin_agents.push_back(agent);
  return agent;
}

Person* Person::create_meta_agent() {
  Person* agent = new Person;
  int id = Person::next_meta_id--;
  agent->setup(id, id, 999, 'M', -1, 0, NULL, NULL, NULL, 0, true);
  return agent;
}


void Person::prepare_to_die(int day, Person* person) {
  if (person->is_deceased() == false) {
    // add person to daily death_list
    Person::death_list.push_back(person);
    FRED_VERBOSE(1, "prepare_to_die PERSON: %d\n", person->get_id());
    person->set_deceased();
  }
}

void Person::prepare_to_migrate(int day, Person* person) {
  if (person->is_eligible_to_migrate() && person->is_deceased() == false) {
    // add person to daily migrant_list
    Person::migrant_list.push_back(person);
    FRED_VERBOSE(1, "prepare_to_migrate PERSON: %d\n", person->get_id());
    person->unset_eligible_to_migrate();
    person->set_deceased();
  }
}

void Person::setup() {
  FRED_STATUS(0, "setup population entered\n", "");

  Person::people.clear();
  Person::pop_size = 0;
  Person::death_list.clear();
  Person::migrant_list.clear();
  Person::number_of_vars = Person::var_name.size();
  Person::number_of_list_vars = Person::list_var_name.size();
  Person::number_of_global_vars = Person::global_var_name.size();
  Person::number_of_global_list_vars = Person::global_list_var_name.size();

  // read optional properties
  Property::disable_abort_on_failure();
  
  if (Person::number_of_global_vars > 0) {
    Person::global_var = new double [Person::number_of_global_vars];
    for (int i = 0; i < Person::number_of_global_vars; i++) {
      Person::global_var[i] = 0.0;
      Property::get_property(Person::global_var_name[i], &Person::global_var[i]);
    }
  }

  if (Person::number_of_vars > 0) {
    Person::var_init_value = new double [Person::number_of_vars];
    for (int i = 0; i < Person::number_of_vars; i++) {
      Person::var_init_value[i] = 0.0;
      Property::get_property(Person::var_name[i], &Person::var_init_value[i]);
    }
  }

  // restore requiring properties
  Property::set_abort_on_failure();


  int number_of_list_vars = Person::get_number_of_global_list_vars();
  if (number_of_list_vars > 0) {
    Person::global_list_var = new double_vector_t [number_of_list_vars];
    for (int i = 0; i < number_of_list_vars; i++) {
      Person::global_list_var[i].clear();
      FRED_VERBOSE(0, "Creating global_list_var %s\n", Person::global_list_var_name[i].c_str());
    }
  }

  // define the Import_agent
  Person::Import_agent = Person::create_meta_agent();

  read_all_populations();

  if(Global::Enable_Health_Insurance) {
    // select insurance coverage
    // try to make certain that everyone in a household has same coverage
    initialize_health_insurance();
  }

  Person::load_completed = true;
  
  initialize_activities();

  // record age-specific popsize
  for(int age = 0; age <= Demographics::MAX_AGE; ++age) {
    Person::Popsize_by_age[age] = 0;
  }
  for(int p = 0; p < Person::get_population_size(); ++p) {
    Person* person = get_person(p);
    int age = person->get_age();
    if(age > Demographics::MAX_AGE) {
      age = Demographics::MAX_AGE;
    }
    Person::Popsize_by_age[age]++;
  }

  // print initial demographics if requested
  if (Person::report_initial_population) {
    char pfilename [FRED_STRING_SIZE];
    sprintf(pfilename, "%s/population.txt", Global::Simulation_directory);
    FILE* pfile = fopen(pfilename, "w");
    for(int p = 0; p < Person::get_population_size(); ++p) {
      Person* person = get_person(p);
      fprintf(pfile, "%d,%d,%c,%d,%f,%f\n",
	      person->get_id(),
	      person->get_age(),
	      person->get_sex(),
	      person->get_race(),
	      person->get_household()->get_latitude(),
	      person->get_household()->get_longitude()
	      );
    }
    fclose(pfile);
  }

  FRED_STATUS(0, "population setup finished\n", "");
}

void Person::get_person_data(char* line, bool gq) {

  // FRED_STATUS(0, "get_person_data %s\n", line);
  int day = Global::Simulation_Day;

  // person data
  char label[FRED_STRING_SIZE];
  Place* house = NULL;
  Place* work = NULL;
  Place* school = NULL;
  int age = -1;
  int race = -1;
  int household_relationship = -1;
  char sex = 'X';
  bool today_is_birthday = false;

  // place labels
  char house_label[FRED_STRING_SIZE];
  char school_label[FRED_STRING_SIZE];
  char work_label[FRED_STRING_SIZE];
  char gq_label[FRED_STRING_SIZE];
  char tmp_house_label[FRED_STRING_SIZE];
  char tmp_school_label[FRED_STRING_SIZE];
  char tmp_work_label[FRED_STRING_SIZE];

  strcpy(label, "X");
  strcpy(house_label, "X");
  strcpy(school_label, "X");
  strcpy(work_label, "X");

  if (gq) {
    sscanf(line, "%s %s %d %c", label, gq_label, &age, &sex);
    sprintf(house_label, "GH-%s", gq_label);
    sprintf(work_label, "GW-%s", gq_label);
  }
  else {
    sscanf(line, "%s %s %d %c %d %d %s %s", label, tmp_house_label, &age, &sex, &race, &household_relationship, tmp_school_label, tmp_work_label);
    sprintf(house_label, "H-%s", tmp_house_label);
    sprintf(work_label, "W-%s", tmp_work_label);
    sprintf(school_label, "S-%s", tmp_school_label);
  }

  // FRED_VERBOSE(0,"GET_PERSON_DATA %s %s %d %c %d %d %s %s\n", label, house_label, age, (char) sex, race, household_relationship, school_label, work_label); fflush(stdout);

  if (strcmp(tmp_school_label,"X") != 0 && Global::GRADES <= age) {
    // person is too old for schools in FRED!
    FRED_VERBOSE(0, "WARNING: person %s age %d is too old to attend school %s\n",
		 label,
		 age,
		 school_label);
    // reset school _label
    strcpy(school_label, "X");
  }

  // set pointer to primary places in init data object
  house = Place::get_household_from_label(house_label);
  // FRED_VERBOSE(0, "get_household_from_label ok\n");
  work =  Place::get_workplace_from_label(work_label);
  school = Place::get_school_from_label(school_label);

  if(house == NULL) {
    // we need at least a household (homeless people not yet supported), so
    // skip this person
    FRED_VERBOSE(0, "WARNING: skipping person %s -- %s %s\n", label,
		 "no household found for label =", house_label);
    return;
  }

  // warn if we can't find workplace
  if(strcmp(tmp_work_label, "X") != 0 && work == NULL) {
    FRED_VERBOSE(2, "WARNING: person %s -- no workplace found for label = %s\n", label,
		 work_label);
    if(Global::Enable_Local_Workplace_Assignment) {
      work = Place::get_random_workplace();
      if (work != NULL) {
	FRED_VERBOSE(0, "WARNING: person %s assigned to workplace %s\n",
		     label, work->get_label());
      }
      else {
	FRED_VERBOSE(0, "WARNING: no workplace available for person %s\n", label);
      }
    }
  }

  // warn if we can't find school.
  if (strcmp(tmp_school_label,"X") != 0 && school == NULL) {
    FRED_VERBOSE(0, "WARNING: person %s -- no school found for label = %s\n", label, school_label);
  }

  // FRED_VERBOSE(0, "add_person_to_pop:\n");
  add_person_to_population(age, sex, race, household_relationship, house,
			   school, work, day, today_is_birthday);
}


void Person::read_all_populations() {

  // process each specified location
  int locs = Place::get_number_of_location_ids();
  for (int i = 0; i < locs; ++i) {
    char pop_dir[FRED_STRING_SIZE];
    Place::get_population_directory(pop_dir, i);
    read_population(pop_dir, "people");
    if(Global::Enable_Group_Quarters) {
      read_population(pop_dir, "gq_people");
    }
  }

  // mark all original people as original
  for (int i = 0; i < people.size(); i++) {
    people[i]->set_original();
  }

  // report on time take to read populations
  Utils::fred_print_lap_time("reading populations");

}

void Person::read_population(const char* pop_dir, const char* pop_type) {

  FRED_STATUS(0, "read population entered\n");

  char population_file[FRED_STRING_SIZE];
  bool is_group_quarters_pop = (strcmp(pop_type, "gq_people") == 0);
  FILE* fp = NULL;

  sprintf(population_file, "%s/%s.txt", pop_dir, pop_type);

  // verify that the file exists
  fp = Utils::fred_open_file(population_file);
  if(fp == NULL) {
    Utils::fred_abort("population_file %s not found\n", population_file);
  }
  fclose(fp);

  if (Global::Compile_FRED) {
    return;
  }

  std::ifstream stream(population_file, ifstream::in);

  char line[FRED_STRING_SIZE];
  // discard header line
  stream.getline(line, FRED_STRING_SIZE);
  while(stream.good()) {
    stream.getline(line, FRED_STRING_SIZE);
    // skip empty lines and header lines ...
    if((line[0] == '\0') || strncmp(line, "sp_id", 6) == 0 || strncmp(line, "per_id", 7) == 0) {
      continue;
    }
    get_person_data(line, is_group_quarters_pop);
  }
  FRED_VERBOSE(0, "finished reading population, pop_size = %d\n", Person::pop_size);
}

void Person::remove_dead_from_population(int day) {
  FRED_VERBOSE(1, "remove_dead_from_population\n");
  size_t deaths = Person::death_list.size();
  for(size_t i = 0; i < deaths; ++i) {
    Person* person = Person::death_list[i];
    delete_person_from_population(day, person);
  }
  // clear the death list
  Person::death_list.clear();
  FRED_VERBOSE(1, "remove_dead_from_population finished\n");
}

void Person::remove_migrants_from_population(int day) {
  FRED_VERBOSE(1, "remove_migrant_from_population\n");
  size_t migrants = Person::migrant_list.size();
  for(size_t i = 0; i < migrants; ++i) {
    Person* person = Person::migrant_list[i];
    delete_person_from_population(day, person);
  }
  // clear the migrant list
  Person::migrant_list.clear();
  FRED_VERBOSE(1, "remove_migrant_from_population finished\n");
}

void Person::delete_person_from_population(int day, Person* person) {
  FRED_VERBOSE(1, "DELETING PERSON: %d\n", person->get_id());

  person->terminate(day);

  // delete from population data structure
  int id = person->get_id();
  int idx = person->get_pop_index();
  Person::id_map[id] = -1;

  if (Person::pop_size > 1) {
    // move last person in vector to this person's position
    Person::people[idx] = Person::people[Person::pop_size-1];

    // inform the move person of its new index
    Person::people[idx]->set_pop_index(idx);

    // update the id_map
    Person::id_map[Person::people[idx]->get_id()] = idx;
  }

  // remove last element in vector
  Person::people.pop_back();

  // record new population_size
  Person::pop_size = Person::people.size();

  // call Person's destructor directly!!!
  person->~Person();
}

void Person::report(int day) {

  // FRED_VERBOSE(0, "report on day %d\n", day);

  // Write out the population if the output_population property is set.
  // Will write only on the first day of the simulation, on days
  // matching the date pattern in the program file, and the on
  // the last day of the simulation
  if(Person::output_population > 0) {
    int month;
    int day_of_month;
    sscanf(Person::output_population_date_match,"%d-%d", &month, &day_of_month);
    if((day == 0)
       || (month == Date::get_month() && day_of_month == Date::get_day_of_month())) {
      Person::write_population_output_file(day);
    }
  }


  if(Global::Enable_Population_Dynamics) {
    int year = Date::get_year();
    if (2010 <= year && Date::get_month() == 6 && Date::get_day_of_month()==30) {
      int males[18];
      int females[18];
      int male_count = 0;
      int female_count = 0;
      int natives = 0;
      int originals = 0;
      std::vector<double>ages;
      ages.clear();
      ages.reserve(Person::get_population_size());

      for (int i = 0; i < 18; i++) {
	males[i] = 0;
	females[i] = 0;
      }
      for(int p = 0; p < Person::get_population_size(); ++p) {
	Person* person = get_person(p);
	int age = person->get_age();
	ages.push_back(person->get_real_age());
	int age_group = age / 5;
	if (age_group > 17) { 
	  age_group = 17;
	}
	if (person->get_sex()=='M') {
	  males[age_group]++;
	  male_count++;
	}
	else {
	  females[age_group]++;
	  female_count++;
	}
	if (person->is_native()) {
	  natives++;
	}
	if (person->is_original()) {
	  originals++;
	}
      }
      std::sort(ages.begin(), ages.end());
      double median = ages[Person::get_population_size()/2];

      char filename[FRED_STRING_SIZE];
      sprintf(filename, "%s/pop-%d.txt",
	      Global::Simulation_directory,
	      Global::Simulation_run_number);
      FILE *fp = NULL;
      if (year == 2010) {
	fp = fopen(filename,"w");
      }
      else {
	fp = fopen(filename,"a");
      }
      assert(fp != NULL);
      fprintf(fp, "%d total %d males %d females %d natives %d %f orig %d %f median_age %0.2f\n",
	      Date::get_year(),
	      Person::get_population_size(),
	      male_count, female_count,
	      natives, (double) natives / Person::get_population_size(),
	      originals, (double) originals / Person::get_population_size(),
	      median);
      fclose(fp);

      if (year % 5 == 0) {
	sprintf(filename, "%s/pop-ages-%d-%d.txt",
		Global::Simulation_directory,
		year,
		Global::Simulation_run_number);
	fp = fopen(filename,"w");
	assert(fp != NULL);
	for (int i = 0; i < 18; i++) {
	  int lower = 5*i;
	  char label[16];
	  if (lower < 85) {
	    sprintf(label,"%d-%d",lower,lower+4);
	  }
	  else {
	    sprintf(label,"85+");
	  }
	  fprintf(fp, "%d %s %d %d %d %d\n",
		  Date::get_year(), label, lower,
		  males[i], females[i], males[i]+females[i]);
	}
	fclose(fp);
      }
    }
  }

  int report_size = Person::report_vec.size();
  // FRED_VERBOSE(0, "report_size = %d\n", report_size);
  for (int i = 0; i < report_size; i++) {
    // FRED_VERBOSE(0, "report_size = %d report_vec %d\n", report_size, i);
    report_t* report = Person::report_vec[i];
    // FRED_VERBOSE(0, "expression = |%s|\n", report->expression->get_name().c_str());
    double value = report->expression->get_value(report->person);
    //FRED_VERBOSE(0, "expression = |%s| value = |%f|\n", report->expression->get_name().c_str(), value);
    int vec_size = report->value_on_day.size();
    if (vec_size == 0 || value!=report->value_on_day[vec_size-1]) {
      report->change_day.push_back(day);
      report->value_on_day.push_back(value);
    }
  }
}

void Person::finish() {
  // Write the population to the output file if the property is set
  // Will write only on the first day of the simulation, days matching
  // the date pattern in the program file, and the last day of the
  // simulation
  if(Person::output_population > 0) {
    Person::write_population_output_file(Global::Simulation_Days);
  }

  // write final reports
  if(Person::report_vec.size()>0) {

    char dir[FRED_STRING_SIZE];
    char outfile[FRED_STRING_SIZE];
    FILE* fp;

    sprintf(dir, "%s/RUN%d/DAILY",
	    Global::Simulation_directory,
	    Global::Simulation_run_number);
    Utils::fred_make_directory(dir);

    for (int i = 0; i < Person::report_vec.size(); i++) {
      int person_index = Person::report_vec[i]->person_index;
      std::string expression_str = Person::report_vec[i]->expression->get_name();

      sprintf(outfile, "%s/PERSON.Person%d_%s.txt", dir, person_index, expression_str.c_str());
      fp = fopen(outfile, "w");
      if (fp == NULL) {
	Utils::fred_abort("Fred: can't open file %s\n", outfile);
      }
      for (int day = 0; day < Global::Simulation_Days; day++) {
	double value = 0.0;
	int vec_size = Person::report_vec[i]->value_on_day.size();
	for (int j = 0; j < vec_size; j++) {
	  if (Person::report_vec[i]->change_day[j] > day) {
	    break;
	  }
	  else {
	    value = Person::report_vec[i]->value_on_day[j];
	  }
	}
	fprintf(fp, "%d %f\n", day, value);
      }
      fclose(fp);
    }

    // create a csv file for PERSON

    // this joins two files with same value in column 1, from
    // https://stackoverflow.com/questions/14984340/using-awk-to-process-input-from-multiple-files
    char awkcommand[FRED_STRING_SIZE];
    sprintf(awkcommand, "awk 'FNR==NR{a[$1]=$2 FS $3;next}{print $0, a[$1]}' ");

    char command[FRED_STRING_SIZE];
    char dailyfile[FRED_STRING_SIZE];

    sprintf(outfile, "%s/RUN%d/%s.csv", Global::Simulation_directory, Global::Simulation_run_number, "PERSON");

    for (int i = 0; i < Person::report_vec.size(); i++) {
      int person_index = Person::report_vec[i]->person_index;
      std::string expression_str = Person::report_vec[i]->expression->get_name();
      sprintf(dailyfile, "%s/PERSON.Person%d_%s.txt", dir, person_index, expression_str.c_str());
      if (i==0) {
	sprintf(command, "cp %s %s", dailyfile, outfile);
      }
      else {
	sprintf(command, "%s %s %s > %s.tmp; mv %s.tmp %s",
		awkcommand, dailyfile, outfile, outfile, outfile, outfile);
      }
      system(command);
    }  

    // create a header line for the csv file
    char headerfile[FRED_STRING_SIZE];
    sprintf(headerfile, "%s/RUN%d/%s.header", Global::Simulation_directory, Global::Simulation_run_number, "PERSON");
    fp = fopen(headerfile, "w");
    fprintf(fp, "Day ");
    for (int i = 0; i < Person::report_vec.size(); i++) {
      int person_index = Person::report_vec[i]->person_index;
      std::string expression_str = Person::report_vec[i]->expression->get_name();
      fprintf(fp, "PERSON.Person%d_%s ", person_index, expression_str.c_str()); 
    }
    fprintf(fp, "\n");
    fclose(fp);

    // concatenate header line
    sprintf(command, "cat %s %s > %s.tmp; mv %s.tmp %s; unlink %s",
	    headerfile, outfile, outfile, outfile, outfile, headerfile);
    system(command);

    // replace spaces with commas
    sprintf(command, "sed -E 's/ +/,/g' %s | sed -E 's/,$//' | sed -E 's/,/ /' > %s.tmp; mv %s.tmp %s",
	    outfile, outfile, outfile, outfile);
    system(command);

  }
}

void Person::quality_control() {
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "population quality control check\n");
    fflush(Global::Statusfp);
  }

  // check population
  for(int p = 0; p < Person::get_population_size(); ++p) {
    Person* person = get_person(p);
    if(person->get_household() == NULL) {
      fprintf(Global::Statusfp, "Help! Person %d has no home.\n", person->get_id());
      person->print(Global::Statusfp, 0);
    }
  }

  if(Global::Verbose > 0) {
    int n0, n5, n18, n50, n65;
    int count[20];
    int total = 0;
    n0 = n5 = n18 = n50 = n65 = 0;
    // age distribution
    for(int c = 0; c < 20; ++c) {
      count[c] = 0;
    }
    for(int p = 0; p < Person::get_population_size(); ++p) {
      Person* person = get_person(p);
      int a = person->get_age();

      if(a < 5) {
        n0++;
      } else if(a < 18) {
        n5++;
      } else if(a < 50) {
        n18++;
      } else if(a < 65) {
        n50++;
      }else {
        n65++;
      }
      int n = a / 5;
      if(n < 20) {
        count[n]++;
      } else {
        count[19]++;
      }
      total++;
    }
    fprintf(Global::Statusfp, "\nAge distribution: %d people\n", total);
    for(int c = 0; c < 20; ++c) {
      fprintf(Global::Statusfp, "age %2d to %d: %6d (%.2f%%)\n", 5 * c, 5 * (c + 1) - 1, count[c],
	      (100.0 * count[c]) / total);
    }
    fprintf(Global::Statusfp, "AGE 0-4: %d %.2f%%\n", n0, (100.0 * n0) / total);
    fprintf(Global::Statusfp, "AGE 5-17: %d %.2f%%\n", n5, (100.0 * n5) / total);
    fprintf(Global::Statusfp, "AGE 18-49: %d %.2f%%\n", n18, (100.0 * n18) / total);
    fprintf(Global::Statusfp, "AGE 50-64: %d %.2f%%\n", n50, (100.0 * n50) / total);
    fprintf(Global::Statusfp, "AGE 65-100: %d %.2f%%\n", n65, (100.0 * n65) / total);
    fprintf(Global::Statusfp, "\n");

  }
  FRED_STATUS(0, "population quality control finished\n");
}

void Person::assign_primary_healthcare_facilities() {
  assert(Place::is_load_completed());
  assert(Person::is_load_completed());
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "assign primary healthcare entered\n");
    fflush(Global::Statusfp);
  }
  for(int p = 0; p < Person::get_population_size(); ++p) {
    Person* person = get_person(p);
    person->assign_primary_healthcare_facility();

  }
  FRED_VERBOSE(0,"assign primary healthcare finished\n");
}

void Person::get_network_stats(char* directory) {
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "get_network_stats entered\n");
    fflush(Global::Statusfp);
  }
  char filename[FRED_STRING_SIZE];
  sprintf(filename, "%s/degree.csv", directory);
  FILE* fp = fopen(filename, "w");
  fprintf(fp, "id,age,deg,h,n,s,c,w,o\n");
  for(int p = 0; p < Person::get_population_size(); ++p) {
    Person* person = get_person(p);
    fprintf(fp, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n", person->get_id(), person->get_age(), person->get_degree(),
	    person->get_household_size(), person->get_neighborhood_size(), person->get_school_size(),
	    person->get_classroom_size(), person->get_workplace_size(), person->get_office_size());
  }
  fclose(fp);
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "get_network_stats finished\n");
    fflush(Global::Statusfp);
  }
}

void Person::print_age_distribution(char* dir, char* date_string, int run) {
  FILE* fp;
  int count[Demographics::MAX_AGE + 1];
  double pct[Demographics::MAX_AGE + 1];
  char filename[FRED_STRING_SIZE];
  sprintf(filename, "%s/age_dist_%s.%02d", dir, date_string, run);
  printf("print_age_dist entered, filename = %s\n", filename);
  fflush(stdout);
  for(int i = 0; i < 21; ++i) {
    count[i] = 0;
  }
  for(int p = 0; p < Person::get_population_size(); ++p) {
    Person* person = get_person(p);
    int age = person->get_age();
    assert(age >= 0);
    if(age > Demographics::MAX_AGE) {
      age = Demographics::MAX_AGE;
    }
    count[age]++;
  }
  fp = fopen(filename, "w");
  for(int i = 0; i < 21; ++i) {
    pct[i] = 100.0 * count[i] / Person::pop_size;
    fprintf(fp, "%d  %d %f\n", i * 5, count[i], pct[i]);
  }
  fclose(fp);
}

Person* Person::select_random_person() {
  int i = Random::draw_random_int(0, get_population_size() - 1);
  return get_person(i);
}

void Person::write_population_output_file(int day) {

  //Loop over the whole population and write the output of each Person's to_string to the file
  char population_output_file[FRED_STRING_SIZE];
  sprintf(population_output_file, "%s/%s_%s.txt", Global::Output_directory, Person::pop_outfile,
	  Date::get_date_string().c_str());
  FILE* fp = fopen(population_output_file, "w");
  if(fp == NULL) {
    Utils::fred_abort("Help! population_output_file %s not found\n", population_output_file);
  }

  for(int p = 0; p < Person::get_population_size(); ++p) {
    Person* person = get_person(p);
    fprintf(fp, "%s\n", person->to_string().c_str());
  }
  fflush(fp);
  fclose(fp);
}

void Person::get_age_distribution(int* count_males_by_age, int* count_females_by_age) {
  for(int i = 0; i <= Demographics::MAX_AGE; ++i) {
    count_males_by_age[i] = 0;
    count_females_by_age[i] = 0;
  }
  for(int p = 0; p < Person::get_population_size(); ++p) {
    Person* person = get_person(p);
    int age = person->get_age();
    if(age > Demographics::MAX_AGE) {
      age = Demographics::MAX_AGE;
    }
    if(person->get_sex() == 'F') {
      count_females_by_age[age]++;
    } else {
      count_males_by_age[age]++;
    }
  }
}

void Person::initialize_activities() {
  for(int p = 0; p < Person::get_population_size(); ++p) {
    Person* person = get_person(p);
    person->prepare_activities();
  }
}

void Person::update_population_demographics(int day) {
  if(!Global::Enable_Population_Dynamics) {
    return;
  }
  Demographics::update(day);

  // end_membership all student on July 31 each year
  if(Date::get_month() == 7 && Date::get_day_of_month() == 31) {
    for(int p = 0; p < Person::get_population_size(); ++p) {
      Person* person = get_person(p);
      if (person->is_student()) {
	person->change_school(NULL);
      }
    }
  }

  // update everyone's demographic profile based on age on Aug 1 each year.
  // this re-begin_memberships all school-age student in a school.
  if(Date::get_month() == 8 && Date::get_day_of_month() == 1) {
    for(int p = 0; p < Person::get_population_size(); ++p) {
      Person* person = get_person(p);
      person->update_profile_based_on_age();
    }
  }

}

void Person::initialize_health_insurance() {
  for(int p = 0; p < Person::get_population_size(); ++p) {
    Person* person = get_person(p);
    set_health_insurance(person);
  }
}


void Person::set_health_insurance(Person* p) {

  //  assert(Place::is_load_completed());
  //
  //  //65 or older will use Medicare
  //  if(p->get_real_age() >= 65.0) {
  //    p->set_insurance_type(Insurance_assignment_index::MEDICARE);
  //  } else {
  //    //Get the household of the agent to see if anyone already has insurance
  //    Household* hh = p->get_household();
  //    if(hh == NULL) {
  //      if(Global::Enable_Hospitals && p->person_is_hospitalized() && p->get_permanent_household() != NULL) {
  //        hh = p->get_permanent_household();
  //      }
  //    }
  //    assert(hh != NULL);
  //
  //    double low_income_threshold = 2.0 * Household::get_min_hh_income();
  //    double hh_income = hh->get_household_income();
  //    if(hh_income <= low_income_threshold && Random::draw_random() < (1.0 - (hh_income / low_income_threshold))) {
  //      p->set_insurance_type(Insurance_assignment_index::MEDICAID);
  //    } else {
  //      person_vector_t inhab_vec = hh->get_inhabitants();
  //      for(person_vector_t::iterator itr = inhab_vec.begin();
  //          itr != inhab_vec.end(); ++itr) {
  //        Insurance_assignment_index::e insr = (*itr)->get_insurance_type();
  //        if(insr != Insurance_assignment_index::UNSET && insr != Insurance_assignment_index::MEDICARE) {
  //          //Set this agent's insurance to the same one
  //          p->set_insurance_type(insr);
  //          return;
  //        }
  //      }
  //
  //      //No one had insurance, so set to insurance from distribution
  //      Insurance_assignment_index::e insr = Person::get_health_insurance_from_distribution();
  //      p->set_insurance_type(insr);
  //    }
  //  }

  //If agent already has insurance set (by another household agent), then return
  if(p->get_insurance_type() != Insurance_assignment_index::UNSET) {
    return;
  }

  //Get the household of the agent to see if anyone already has insurance
  Household* hh = p->get_household();
  assert(hh != NULL);
  person_vector_t inhab_vec = hh->get_inhabitants();
  for(person_vector_t::iterator itr = inhab_vec.begin();
      itr != inhab_vec.end(); ++itr) {
    Insurance_assignment_index::e insr = (*itr)->get_insurance_type();
    if(insr != Insurance_assignment_index::UNSET) {
      //Set this agent's insurance to the same one
      p->set_insurance_type(insr);
      return;
    }
  }

  //No one had insurance, so set everyone in household to the same insurance
  Insurance_assignment_index::e insr = Person::get_health_insurance_from_distribution();
  for(person_vector_t::iterator itr = inhab_vec.begin();
      itr != inhab_vec.end(); ++itr) {
    (*itr)->set_insurance_type(insr);
  }

}


void Person::get_external_updates(int day) {

  FILE* fp = NULL;
  char filename [FRED_STRING_SIZE];

  // create the API directory, if necessary
  char dirname [FRED_STRING_SIZE];
  sprintf(dirname, "%s/RUN%d/API", Global::Simulation_directory, Global::Simulation_run_number);
  Utils::fred_make_directory(dirname);

  person_vector_t updates;
  updates.clear();

  char requests_file[FRED_STRING_SIZE];
  sprintf(requests_file, "%s/requests", dirname);
  FILE* reqfp = fopen(requests_file, "w");

  int requests = 0;
  for(int p = 0; p < Person::get_population_size(); ++p) {
    Person* person = get_person(p);
    bool update = false;
    int number_of_conditions = Condition::get_number_of_conditions();
    for (int condition_id = 0; update==false && condition_id < number_of_conditions; condition_id++) {
      Condition* condition = Condition::get_condition(condition_id);
      if (condition->is_external_update_enabled()) {
	int state = person->get_state(condition_id);
	update = condition->state_gets_external_updates(state);
      }
    }
    if (update) {
      updates.push_back(person);
      sprintf(filename, "%s/request.%d", dirname, person->get_id());
      fp = fopen(filename, "w");
      person->request_external_updates(fp, day);
      fclose(fp);

      // add filename to the list of requests
      fprintf(reqfp, "request.%d\n", person->get_id());
      requests++;
    }
  }
  fclose(reqfp);

  if (requests > 0) {
    char command [FRED_STRING_SIZE];
    sprintf(command, "%s/bin/FRED_API %s", getenv("FRED_HOME"), dirname);
    system(command);

    sprintf(filename, "%s/results_ready", dirname);
    int tries = 0;
    while (tries < 1000 && NULL == (fp = fopen(filename, "r"))) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      tries++;
    }
    if (fp != NULL) {
      fclose(fp);
      unlink(filename);
      
      for (int p = 0; p < updates.size(); p++) {
	Person* person = updates[p];
	sprintf(filename, "%s/results.%d", dirname, person->get_id());
	fp = fopen(filename, "r");
	person->get_external_updates(fp, day);
	fclose(fp);
	
	// clean up files
	// unlink(filename);
	// sprintf(filename, "%s/request.%d", dirname, person->get_id());
	// unlink(filename);
      }
    }
  }

  sprintf(filename, "%s/log", dirname);
  fp = NULL;
  if (day == 0) {
    fp = fopen(filename, "w");
  }
  else {
    fp = fopen(filename, "a");
  }
  fprintf(fp, "day %d requests %d\n", day, requests);
  fclose(fp);
}

void Person::set_admin_group(Group* group) {
  if (is_meta_agent() && group != NULL) {
    Person::admin_group_map[this] = group;
    set_activity_group(group->get_type_id(), group);
  }
}

Group* Person::get_admin_group() {
  if (is_meta_agent()) {
    std::unordered_map<Person*,Group*>::const_iterator found = Person::admin_group_map.find(this);
    if (found != admin_group_map.end()) {
      return found->second;
    }
    else {
      return NULL;
    }
  }
  else {
    return NULL;
  }
}

bool Person::has_closure() {
  if (this->is_meta_agent()) {
    Group* group = get_admin_group();
    // FRED_VERBOSE(0, "meta person %d admin for group %s\n", this->id, group->get_label());
    int group_type_id = group->get_type_id();
    for (int cond_id = 0; cond_id < this->number_of_conditions; cond_id++) {
      int state = get_state(cond_id);
      if (Condition::get_condition(cond_id)->is_closed(state,group_type_id)) {
	FRED_VERBOSE(1, "meta person %d admin CLOSES group %s in state %s.%s\n",
		     this->id, group->get_label(), Condition::get_name(cond_id).c_str(),
		     Condition::get_condition(cond_id)->get_state_name(state).c_str());
	return true;
      }
      else {
	FRED_VERBOSE(1, "meta person %d admin does not close group %s in state %s.%s\n",
		     this->id, group->get_label(), Condition::get_name(cond_id).c_str(),
		     Condition::get_condition(cond_id)->get_state_name(state).c_str());
      }
    }
    FRED_VERBOSE(1, "meta person %d admin does not close group %s\n",
		 this->id, group->get_label());
  }
  return false;
}

void Person::start_hosting(int place_type_id) {
  FRED_VERBOSE(1, "START_HOSTING person %d place_type %s\n",
	       get_id(), Place_Type::get_place_type_name(place_type_id).c_str());
  Place* place = get_place_of_type(place_type_id);
  if (place == NULL) {
    place = Place_Type::generate_new_place(place_type_id, this);
    set_place_of_type(place_type_id, place); 
    FRED_VERBOSE(0, "START_HOSTING finished person %d place_type %s place %s\n",
		 get_id(), Place_Type::get_place_type_name(place_type_id).c_str(), place ? place->get_label() : "NULL");
  }
  else {
    FRED_VERBOSE(0, "START_HOSTING person %d place_type %s -- current place not NULL\n",
		 get_id(), Place_Type::get_place_type_name(place_type_id).c_str());
  }
}



std::string Person::get_household_relationship_name(int rel) {
  switch(rel) {

  case Household_Relationship::HOUSEHOLDER :
    return "householder";
    break;

  case Household_Relationship::SPOUSE :
    return "spouse";
    break;

  case Household_Relationship::CHILD :
    return "child";
    break;

  case Household_Relationship::SIBLING :
    return "sibling";
    break;

  case Household_Relationship::PARENT :
    return "parent";
    break;

  case Household_Relationship::GRANDCHILD :
    return "grandchild";
    break;

  case Household_Relationship::IN_LAW :
    return "in_law";
    break;

  case Household_Relationship::OTHER_RELATIVE :
    return "other_relative";
    break;

  case Household_Relationship::BOARDER :
    return "boarder";
    break;

  case Household_Relationship::HOUSEMATE :
    return "housemate";
    break;

  case Household_Relationship::PARTNER :
    return "partner";
    break;

  case Household_Relationship::FOSTER_CHILD :
    return "foster_child";
    break;

  case Household_Relationship::OTHER_NON_RELATIVE :
    return "other_non_relative";
    break;

  case Household_Relationship::INSTITUTIONALIZED_GROUP_QUARTERS_POP :
    return "institutionalized_group_quarters_pop";
    break;

  case Household_Relationship::NONINSTITUTIONALIZED_GROUP_QUARTERS_POP :
    return "noninstitutionalized_group_quarters_pop";
    break;

  default:
    return "unknown";
  }
}

int Person::get_household_relationship_from_name(std::string name) {
  for (int i = 0; i < Household_Relationship::HOUSEHOLD_RELATIONSHIPS; i++) {
    if (name == Person::get_household_relationship_name(i)) {
      return i;
    }
  }
  return -1;
}

std::string Person::get_race_name(int n) {
  switch(n) {

  case Race::UNKNOWN_RACE :
    return "unknown_race";
    break;

  case Race::WHITE :
    return "white";
    break;

  case Race::AFRICAN_AMERICAN :
    return "african_american";
    break;

  case Race::AMERICAN_INDIAN :
    return "american_indian";
    break;

  case Race::ALASKA_NATIVE :
    return "alaska_native";
    break;

  case Race::TRIBAL :
    return "tribal";
    break;

  case Race::ASIAN :
    return "asian";
    break;

  case Race::HAWAIIAN_NATIVE :
    return "hawaiian_native";
    break;

  case Race::OTHER_RACE :
    return "other_race";
    break;

  case Race::MULTIPLE_RACE :
    return "multiple_race";
    break;

  default:
    return "unknown";
  }
}



int Person::get_race_from_name(std::string name) {
  for (int i = -1; i < Race::RACES; i++) {
    if (name == Person::get_race_name(i)) {
      return i;
    }
  }
  return -1;
}



string Person::get_var_name(int id) {
  if (Person::number_of_vars <= id) {
    return "";
  }
  return Person::var_name[id];
}

int Person::get_var_id(string vname) {
  // printf("VAR looking for %s among %d vars\n", vname.c_str(), Person::number_of_vars);
  for (int i = 0; i < Person::number_of_vars; i++) {
    // printf("VAR %d = %s\n", i, Person::var_name[i].c_str());
    if (vname == Person::var_name[i]) {
      // printf("VAR %s id %d\n", vname.c_str(), i);
      return i;
    }
  }
  // printf("VAR %s NOT FOUND\n", vname.c_str());
  return -1;
}


string Person::get_list_var_name(int id) {
  if (Person::number_of_list_vars <= id) {
    return "";
  }
  return Person::list_var_name[id];
}

int Person::get_list_var_id(string vname) {
  // printf("LIST_VAR looking for %s among %d list_vars\n", vname.c_str(), Person::number_of_list_vars);
  for (int i = 0; i < Person::number_of_list_vars; i++) {
    // printf("VAR %d = %s\n", i, Person::list_var_name[i].c_str());
    if (vname == Person::list_var_name[i]) {
      // printf("LIST_VAR %s id %d\n", vname.c_str(), i);
      return i;
    }
  }
  return -1;
}


string Person::get_global_var_name(int id) {
  if (Person::number_of_global_vars <= id) {
    return "";
  }
  return Person::global_var_name[id];
}

int Person::get_global_var_id(string vname) {
  // printf("VAR looking for %s among %d vars\n", vname.c_str(), Person::number_of_vars);
  for (int i = 0; i < Person::number_of_global_vars; i++) {
    // printf("VAR %d = %s\n", i, Person::var_name[i].c_str());
    if (vname == Person::global_var_name[i]) {
      // printf("VAR %s id %d\n", vname.c_str(), i);
      return i;
    }
  }
  // printf("VAR %s NOT FOUND\n", vname.c_str());
  return -1;
}


string Person::get_global_list_var_name(int id) {
  if (Person::number_of_global_list_vars <= id) {
    return "";
  }
  return Person::global_list_var_name[id];
}

int Person::get_global_list_var_id(string vname) {
  // printf("LIST_VAR looking for %s among %d list_vars\n", vname.c_str(), Person::number_of_list_vars);
  for (int i = 0; i < Person::number_of_global_list_vars; i++) {
    // printf("VAR %d = %s\n", i, Person::list_var_name[i].c_str());
    if (vname == Person::global_list_var_name[i]) {
      // printf("LIST_VAR %s id %d\n", vname.c_str(), i);
      return i;
    }
  }
  return -1;
}


void Person::start_reporting(Rule *rule) {

  Expression* expression = rule->get_expression();
  for (int i = 0; i < Person::report_vec.size(); i++) {
    report_t* report = Person::report_vec[i];
    if (report->person==this && report->expression->get_name() == expression->get_name()) {
      return;
    }
  }
  
  // find index for this person in the reporting list
  int index = -1;
  int report_person_size = Person::report_person.size();
  for (int i = 0; i < report_person_size; i++) {
    if (Person::report_person[i] == this) {
      index = i;
      break;
    }
  }

  if (index < 0) {
    if (Person::max_reporting_agents < Person::report_vec.size()) {
      Person::report_person.push_back(this);
      index = report_person_size;
    }
    else {
      // no room for more reporters
      return;
    }
  }

  // insert new report
  Person::report_vec.push_back(new report_t);
  int n = Person::report_vec.size()-1;
  Person::report_vec[n]->person_index = index;
  Person::report_vec[n]->person_id = this->id;
  Person::report_vec[n]->person = this;
  Person::report_vec[n]->expression = expression;
}


//////////////////////////////////////////////

// FROM HEALTH

#include "Date.h"
#include "Condition.h"
#include "Household.h"
#include "Natural_History.h"
#include "Group.h"
#include "Property.h"
#include "Person.h"
#include "Place.h"
#include "Random.h"
#include "Utils.h"

Insurance_assignment_index::e Person::get_health_insurance_from_distribution() {
  if(Global::Enable_Health_Insurance && Person::is_initialized) {
    int i = Random::draw_from_distribution(Person::health_insurance_cdf_size, Person::health_insurance_distribution);
    return Person::get_insurance_type_from_int(i);
  } else {
    return Insurance_assignment_index::UNSET;
  }
}

void Person::setup_conditions() {
  FRED_VERBOSE(1, "Person::setup for person %d\n", get_id());
  this->alive = true;
  
  this->number_of_conditions = Condition::get_number_of_conditions();
  // FRED_VERBOSE(0, "Person::setup person %d conditions %d\n", get_id(), this->number_of_conditions);

  this->condition = new condition_t [this->number_of_conditions];

  for(int condition_id = 0; condition_id < this->number_of_conditions; ++condition_id) {
    this->condition[condition_id].state = -1;
    this->condition[condition_id].susceptibility = 0;
    this->condition[condition_id].transmissibility = 0;
    this->condition[condition_id].last_transition_step = -1;
    this->condition[condition_id].next_transition_step = -1;
    this->condition[condition_id].exposure_day = -1;
    this->condition[condition_id].is_fatal = false;
    this->condition[condition_id].source = NULL;
    this->condition[condition_id].group = NULL;
    this->condition[condition_id].number_of_hosts = 0;
    int states = get_natural_history(condition_id)->get_number_of_states();
    this->condition[condition_id].entered = new int [states];
    for (int i = 0; i < states; i++) {
      this->condition[condition_id].entered[i] = -1;
    }
  }
  this->previous_infection_serotype = -1;
  int number_of_vars = Person::get_number_of_vars();
  if (number_of_vars > 0) {
    this->var = new double [number_of_vars];
    for (int i = 0; i < number_of_vars; i++) {
      this->var[i] = Person::var_init_value[i];
    }
  }
  if (this->id==-1) {
    FRED_VERBOSE(0, "setup_conditions: person %d number_of_vars %d\n", this->id, number_of_vars);
  }

  int number_of_list_vars = Person::get_number_of_list_vars();
  if (number_of_list_vars > 0) {
    this->list_var = new double_vector_t [number_of_list_vars];
    for (int i = 0; i < number_of_list_vars; i++) {
      this->list_var[i].clear();
    }
  }
}

void Person::initialize_conditions(int day) {
  for(int condition_id = 0; condition_id < this->number_of_conditions; ++condition_id) {
    Condition::get_condition(condition_id)->initialize_person(this, day);
  }
}

void Person::become_exposed(int condition_id, Person* source, Group* group, int day, int hour) {
  
  FRED_VERBOSE(1, "HEALTH: become_exposed: person %d is exposed to condition %d day %d hour %d\n",
	       get_id(), condition_id, day, hour);
  
  if (Global::Enable_Records) {
    fprintf(Global::Recordsfp,
	    "HEALTH RECORD: %s %s day %d person %d age %d is %s to %s%s%s",
	    Date::get_date_string().c_str(),
	    Date::get_12hr_clock(hour).c_str(),
	    day,
	    get_id(), get_age(),
	    source == Person::get_import_agent() ? "an IMPORTED EXPOSURE" : "EXPOSED",
	    Condition::get_name(condition_id).c_str(),
	    group == NULL ? "" : " at ",
	    group == NULL ? "" : group->get_label());
    if (source == Person::get_import_agent()) {
      fprintf(Global::Recordsfp, "\n");
    }
    else {
      fprintf(Global::Recordsfp, " from person %d age %d\n", source->get_id(), source->get_age());
    }
  }

  set_source(condition_id, source);
  set_group(condition_id, group);
  set_exposure_day(condition_id, day);

  if (Condition::get_condition(condition_id)->get_transmission_network()!=NULL) {
    Network* network = Condition::get_condition(condition_id)->get_transmission_network();
    if (network) {
      join_network(network);
      source->join_network(network);
      source->add_edge_to(this, network);
      add_edge_from(source, network);
    }
  }

  int place_type_id = Condition::get_condition(condition_id)->get_place_type_to_transmit();
  if (0 <= place_type_id) {
    Place* place = NULL;
    if (source == Person::get_import_agent()) {
      FRED_VERBOSE(1, "PLACE_TRANSMISSION generate new place\n");
      place = Place_Type::generate_new_place(place_type_id, this);
    }
    else {
      place = source->get_place_of_type(place_type_id);
      FRED_VERBOSE(1, "PLACE TRANSMISSION inherit place %s from source %d\n", place? place->get_label(): "NULL", source->get_id());
    }
    set_place_of_type(place_type_id, place); 

    // increment the group_state_counts for all conditions for this place
    for (int cond_id = 0; cond_id < Condition::get_number_of_conditions(); cond_id++) {
      int state = get_state(cond_id);
      Condition::get_condition(cond_id)->increment_group_state_count(place_type_id, place, state);
    }

    if (Global::Enable_Records) {
      fprintf(Global::Recordsfp,
	      "HEALTH RECORD: %s %s day %d person %d GETS TRANSMITTED PLACE type %s label %s from person %d size = %d\n",
	      Date::get_date_string().c_str(),
	      Date::get_12hr_clock(hour).c_str(),
	      day,
	      get_id(),
	      Place_Type::get_place_type_name(place_type_id).c_str(),
	      place == NULL ? "NULL" : place->get_label(),
	      source->get_id(),
	      place == NULL ? 0 : place->get_size());
    }
  }

  FRED_VERBOSE(1, "HEALTH: become_exposed FINISHED: person %d is exposed to condition %d day %d hour %d\n",
	       get_id(), condition_id, day, hour);
  
}


void Person::become_case_fatality(int condition_id, int day) {
  FRED_VERBOSE(1, "CONDITION %s STATE %s is FATAL: day %d person %d\n",
	       Condition::get_name(condition_id).c_str(),
	       Condition::get_condition(condition_id)->get_state_name(get_state(condition_id)).c_str(),
	       day, get_id());

  if (Global::Enable_Records) {
    fprintf(Global::Recordsfp,
	    "HEALTH RECORD: %s %s day %d person %d age %d sex %c race %d income %d is CASE_FATALITY for %s.%s\n",
	    Date::get_date_string().c_str(),
	    Date::get_12hr_clock(Global::Simulation_Hour).c_str(),
	    Global::Simulation_Day,
	    get_id(), get_age(), get_sex(), get_race(),get_income(),
	    Condition::get_name(condition_id).c_str(),
	    Condition::get_condition(condition_id)->get_state_name(get_state(condition_id)).c_str());
  }

  // set status flags
  set_case_fatality(condition_id);

  // queue removal from population
  Person::prepare_to_die(day, this);
  FRED_VERBOSE(1, "become_case_fatality finished for person %d\n",
	       get_id());
}

void Person::update_condition(int day, int condition_id) {
} // end Person::update_condition //


double Person::get_susceptibility(int condition_id) const {
  return this->condition[condition_id].susceptibility;
}

double Person::get_transmissibility(int condition_id) const {
  return this->condition[condition_id].transmissibility;
}


int Person::get_transmissions(int condition_id) const {
  return this->condition[condition_id].number_of_hosts;
}


void Person::expose(Person* host, int source_condition_id, int condition_id, Group* group, int day, int hour) {

  /*
  FRED_STATUS(0, "person %d with cond %d EXPOSES person %d to cond %d group = %s day = %d  hour = %d\n",
	      get_id(), source_condition_id,
	      host->get_id(), condition_id,
	      group ? group->get_label() : "NULL", day, hour);
  */

  host->become_exposed(condition_id, this, group, day, hour);

  this->condition[source_condition_id].number_of_hosts++;
  
  int exp_day = get_exposure_day(source_condition_id);

  if (0 <= exp_day) {
    Condition* condition = Condition::get_condition(source_condition_id);
    condition->increment_cohort_host_count(exp_day);
  }

  /*
  FRED_STATUS(0, "person %d with cond %d EXPOSES person %d to cond %d hosts = %d\n",
	      get_id(), source_condition_id,
	      host->get_id(), condition_id,
	      get_number_of_hosts(source_condition_id));
  */

}

void Person::terminate_conditions(int day) {
  FRED_VERBOSE(1, "TERMINATE CONDITIONS for person %d day %d\n",
	       get_id(), day);

  for(int condition_id = 0; condition_id < Condition::get_number_of_conditions(); ++condition_id) {
    Condition::get_condition(condition_id)->terminate_person(this, day);
  }
  this->alive = false;
}

void Person::set_state(int condition_id, int state, int day) {
  this->condition[condition_id].state = state;
  int current_time = 24*Global::Simulation_Day + Global::Simulation_Hour;
  this->condition[condition_id].entered[state] = current_time;
  set_last_transition_step(condition_id, current_time);
  FRED_VERBOSE(1, "set_state person %d cond %d state %d\n",
	       get_id(), condition_id, state);

}

int Person::get_group_id(int condtion_id) const {
  return get_group(condtion_id) ? get_group(condtion_id)->get_id() : -1;
}

char* Person::get_group_label(int condtion_id) const {
  return get_group(condtion_id) ? get_group(condtion_id)->get_label() : (char*) "X";
}

int Person::get_group_type_id(int condition_id) const {
  return get_group(condition_id) ? get_group(condition_id)->get_type_id() : -1 ;
}


double Person::get_var(int index) {
  int number_of_vars = Person::get_number_of_vars();
  // FRED_VERBOSE(0, "get_var person %d index %d number of vars %d\n", this->id, index, number_of_vars);
  if (index < number_of_vars) {
    return this->var[index];
  }
  else {
    printf("ERR: Can't find variable person %d index = %d vars = %d\n", this->id, index, number_of_vars);
    // exit(0);
    return 0.0;
  }
}

void Person::set_var(int index, double value) {
  int number_of_vars = Person::get_number_of_vars();
  FRED_VERBOSE(0, "set_var person %d index %d number of vars %d\n", this->id, index, number_of_vars);
  if (index < number_of_vars) {
    this->var[index] = value;
  }
}

double Person::get_global_var(int index) {
  int number_of_vars = Person::get_number_of_global_vars();
  if (index < number_of_vars) {
    return Person::global_var[index];
  }
  else {
    printf("ERR: Can't find global var index = %d vars = %d\n", index, number_of_vars);
    // assert(0);
    return 0.0;
  }
}

int Person::get_list_size(int list_var_id) {
  if (0 <= list_var_id && list_var_id < Person::get_number_of_list_vars()) {
    return this->list_var[list_var_id].size();
  }
  else {
    return -1;
  }
}

int Person::get_global_list_size(int list_var_id) {
  if (0 <= list_var_id && list_var_id < Person::get_number_of_global_list_vars()) {
    return Person::global_list_var[list_var_id].size();
  }
  else {
    return -1;
  }
}

double_vector_t Person::get_list_var(int index) {
  int number_of_list_vars = Person::get_number_of_list_vars();
  if (index < number_of_list_vars) {
    return this->list_var[index];
  }
  else {
    printf("ERR: index = %d vars = %d\n", index, number_of_vars);
    assert(0);
    double_vector_t empty;
    return empty;
  }
}

double_vector_t Person::get_global_list_var(int index) {
  int number_of_list_vars = Person::get_number_of_global_list_vars();
  if (index < number_of_list_vars) {
    return Person::global_list_var[index];
  }
  else {
    printf("ERR: index = %d vars = %d\n", index, number_of_vars);
    assert(0);
    double_vector_t empty;
    return empty;
  }
}

void Person::push_back_global_list_var(int list_var_id, double value) {
  Person::global_list_var[list_var_id].push_back(value);
}

void Person::request_external_updates(FILE* fp, int day) {
  fprintf(fp, "day = %d\n", day);
  fprintf(fp, "person = %d\n", get_id());
  fprintf(fp, "age = %d\n", get_age());
  fprintf(fp, "race = %d\n", get_race());
  fprintf(fp, "sex = %c\n", get_sex());
  for (int condition_id = 0; condition_id < this->number_of_conditions; condition_id++) {
    Condition* condition = Condition::get_condition(condition_id);
    int state = get_state(condition_id);
    fprintf(fp, "%s = %s\n",
	   condition->get_name(),
	   condition->get_state_name(state).c_str());
  }
  int number_of_vars = Person::get_number_of_vars();
  for (int i = 0; i < number_of_vars; i++) {
    fprintf(fp, "%s = %f\n",
	    Person::get_var_name(i).c_str(),
	    this->var[i]);
  }  
  fflush(fp);
}

void Person::get_external_updates(FILE* fp, int day) {
  int n;
  int ival;
  char cval;
  double fval;
  char vstr[FRED_STRING_SIZE];
  char vstr2[FRED_STRING_SIZE];

  fscanf(fp, "day = %d ", &ival);
  if (ival!=day) {
    printf("Error: day out of sync %d %d\n", day, ival);
    assert(ival==day);
  }

  fscanf(fp, "person = %d ", &ival);
  if (ival!=get_id()) {
    printf("Error: id %d %d\n", get_id(), ival);
    assert(ival==get_id());
  }

  fscanf(fp, "age = %d ", &ival);
  if (ival!=get_age()) {
    printf("Error: age %d %d\n", get_age(), ival);
    assert(ival==get_age());
  }

  fscanf(fp, "race = %d ", &ival);
  if (ival!=get_race()) {
    printf("Error: race %d %d\n", get_race(), ival);
    assert(ival==get_race());
  }

  fscanf(fp, "sex = %c ", &cval);
  if (cval!=get_sex()) {
    printf("Error: sex %c %c\n", get_sex(), cval);
    assert(cval==get_sex());
  }

  for (int condition_id = 0; condition_id < this->number_of_conditions; condition_id++) {
    Condition* condition = Condition::get_condition(condition_id);
    int state = get_state(condition_id);

    fscanf(fp, "%s = %s ", vstr, vstr2);
    assert(strcmp(vstr,condition->get_name())==0);
    assert(strcmp(vstr2,condition->get_state_name(state).c_str())==0);

  }
  int number_of_vars = Person::get_number_of_vars();
  for (int i = 0; i < number_of_vars; i++) {
    fscanf(fp, "%s = %lf ", vstr, &fval);
    sprintf(vstr2, "%s", Person::get_var_name(i).c_str());
    assert(strcmp(vstr,vstr2)==0);
    this->var[i] = fval;
  }  
}

Natural_History* Person::get_natural_history(int condition_id) const {
  return Condition::get_condition(condition_id)->get_natural_history();
}


///////////////////////////////////////////

//// ACTIVITIES

bool Person::is_weekday = false;
int Person::day_of_week = 0;

const char* get_label_for_place(Place* place) {
  return (place != NULL ? place->get_label() : "NULL");
}

/// static method called in main (Fred.cc)
void Person::get_activity_properties() {
  FRED_STATUS(0, "Person::get_properties() entered\n", "");

  // read optional properties
  Property::disable_abort_on_failure();
  
  // restore requiring properties
  Property::set_abort_on_failure();

}

void Person::setup_activities(Place* house, Place* school, Place* work) {

  FRED_VERBOSE(1, "ACTIVITIES_SETUP: person %d age %d household %s\n",
	       get_id(), get_age(), house->get_label());

  clear_activity_groups();

  // FRED_VERBOSE(0, "set household %s\n", get_label_for_place(house));
  set_household(house);

  // FRED_VERBOSE(0, "set school %s\n", get_label_for_place(school));
  set_school(school);

  // FRED_VERBOSE(0, "set workplace %s\n", get_label_for_place(work));
  set_workplace(work);
  // FRED_VERBOSE(0, "set workplace %s\n", get_label_for_place(work));

  // get the neighborhood from the household
  // FRED_VERBOSE(0, "get_patch hh %s\n", get_household() ? get_household()->get_label() : "null");
  set_neighborhood(get_household()->get_patch()->get_neighborhood());
  FRED_VERBOSE(1, "ACTIVITIES_SETUP: person %d neighborhood %d %s\n", get_id(),
	       get_neighborhood()->get_id(), get_neighborhood()->get_label());

  if (get_neighborhood() == NULL) {
    FRED_VERBOSE(0,
		 "Help! NO NEIGHBORHOOD for person %d house %d \n",
		 get_id(), get_household()->get_id());
  }
  this->home_neighborhood = get_neighborhood();

  // assign profile
  assign_initial_profile();
  // FRED_VERBOSE(0,"set profile ok\n");

  // need to set the daily schedule
  this->schedule_updated = -1;
  this->is_traveling = false;
  this->is_traveling_outside = false;

  if (Global::Verbose > 1) {
    printf("ACTIVITY::SETUP finished for person %d ", get_id());
    for (int n = 0; n < Place_Type::get_number_of_place_types(); n++) {
      printf("%s %s ", Place_Type::get_place_type_name(n).c_str(), get_place_of_type(n) ? get_place_of_type(n)->get_label() : "NULL");
    }
    printf("\n"); fflush(stdout);
  }

}

void Person::assign_initial_profile() {
  int age = get_age();
  if(age == 0) {
    this->profile = Activity_Profile::PRESCHOOL;
    this->in_parents_home = true;
  } else if(get_school() != NULL) {
    this->profile = Activity_Profile::STUDENT;
    this->in_parents_home = true;
  } else if(age < Global::SCHOOL_AGE) {
    this->profile = Activity_Profile::PRESCHOOL;		// child at home
    this->in_parents_home = true;
  } else if(age < Global::ADULT_AGE) {
    this->profile = Activity_Profile::STUDENT;		// school age
    this->in_parents_home = true;
  } else if(get_workplace() != NULL) {
    this->profile = Activity_Profile::WORKER;		// worker
  } else if(age < Global::RETIREMENT_AGE) {
    this->profile = Activity_Profile::WORKER;		// worker
  } else if(Global::RETIREMENT_AGE <= age) {
    if(Random::draw_random()< 0.5 ) {
      this->profile = Activity_Profile::RETIRED;		// retired
    }
    else {
      this->profile = Activity_Profile::WORKER;		// older worker
    }
  } else {
    this->profile = Activity_Profile::UNEMPLOYED;
  }

  if(this->profile == Activity_Profile::WORKER || this->profile == Activity_Profile::UNEMPLOYED) {
    // determine if I am living in parents house
    int rel = get_household_relationship();
    if (rel == Household_Relationship::CHILD || rel == Household_Relationship::GRANDCHILD || rel == Household_Relationship::FOSTER_CHILD) {
      this->in_parents_home = true;
    }
  }  

  // weekend worker
  if((this->profile == Activity_Profile::WORKER && Random::draw_random() < 0.2)) {  // 20% weekend worker
    this->profile = Activity_Profile::WEEKEND_WORKER;
  }

  // profiles for group quarters residents
  if(get_household()->is_college()) {
    this->profile = Activity_Profile::COLLEGE_STUDENT;
    update_profile_after_changing_household();
    this->in_parents_home = false;
    return;
  }
  if(get_household()->is_military_base()) {
    this->profile = Activity_Profile::MILITARY;
    update_profile_after_changing_household();
    this->in_parents_home = false;
    return;
  }
  if(get_household()->is_prison()) {
    this->profile = Activity_Profile::PRISONER;
    FRED_VERBOSE(2, "INITIAL PROFILE AS PRISONER ID %d AGE %d SEX %c HOUSEHOLD %s\n",
		 get_id(), age, get_sex(), get_household()->get_label());
    update_profile_after_changing_household();
    this->in_parents_home = false;
    return;
  }
  if(get_household()->is_nursing_home()) {
    this->profile = Activity_Profile::NURSING_HOME_RESIDENT;
    update_profile_after_changing_household();
    this->in_parents_home = false;
    return;
  }
}


void Person::update(int sim_day) {
  FRED_STATUS(1, "Activities update entered\n");
  // decide if this is a weekday:
  Person::is_weekday = Date::is_weekday();
  FRED_STATUS(1, "Activities update completed\n");
}


void Person::update_activities(int sim_day) {

  FRED_VERBOSE(1,"update_activities for person %d day %d\n", get_id(), sim_day);

  // update activity schedule only once per day
  if(sim_day <= this->schedule_updated) {
    return;
  }

  this->schedule_updated = sim_day;

  // clear the schedule
  this->on_schedule.reset();

  // reset neighborhood
  set_neighborhood(get_household()->get_patch()->get_neighborhood());

  // normally participate in household activities
  this->on_schedule[Place_Type::get_type_id("Household")] = true;

  // non-built-in activities
  for(int i = Place_Type::get_type_id("Hospital")+1; i < Group_Type::get_number_of_group_types(); ++i) {
    if(this->link[i].is_member()) {
      this->on_schedule[i] = 1;
    }
    else {
      this->on_schedule[i] = 1;
    }
  }

  if(this->profile == Activity_Profile::PRISONER || this->profile == Activity_Profile::NURSING_HOME_RESIDENT) {
    // prisoners and nursing home residents stay indoors
    this->on_schedule[Place_Type::get_type_id("Workplace")] = true;
    this->on_schedule[Place_Type::get_type_id("Office")] = true;
    return;
  }

  // normally visit the neighborhood
  this->on_schedule[Place_Type::get_type_id("Neighborhood")] = true;

  // decide which neighborhood to visit today
  if (is_transmissible()) {
    Place* destination_neighborhood = Global::Neighborhoods->select_destination_neighborhood(this->home_neighborhood);
    // FRED_VERBOSE(0, "SELECT DEST NEIGHBOHOOD person %d old %s new %s\n", this->id, this->home_neighborhood->get_label(), destination_neighborhood->get_label());
    set_neighborhood(destination_neighborhood);
  }
  else {
    set_neighborhood(get_household()->get_patch()->get_neighborhood());
  }
  // FRED_VERBOSE(0,"update_activities for person %d day %d nbhd %s\n",
  // get_id(), sim_day, get_activity_group(Place_Type::get_type_id("Neighborhood"))->get_label());

  // attend school only on weekdays
  if(Person::is_weekday) {
    if(get_school() != NULL) {
      this->on_schedule[Place_Type::get_type_id("School")] = true;
      if(get_classroom() != NULL) {
	this->on_schedule[Place_Type::get_type_id("Classroom")] = true;
      }
    }
  }

  // normal worker work only on weekdays
  if(Person::is_weekday) {
    if(get_workplace() != NULL) {
      this->on_schedule[Place_Type::get_type_id("Workplace")] = true;
      if(get_office() != NULL) {
	this->on_schedule[Place_Type::get_type_id("Office")] = true;
      }
    }
  }
  else {
    // students with jobs and weekend worker work on weekends
    if(this->profile == Activity_Profile::WEEKEND_WORKER || this->profile == Activity_Profile::STUDENT) {
      if(get_workplace() != NULL) {
	this->on_schedule[Place_Type::get_type_id("Workplace")] = true;
	if(get_office() != NULL) {
	  this->on_schedule[Place_Type::get_type_id("Office")] = true;
	}
      }
    }

  }

  FRED_STATUS(1, "update_activities on day %d\n%s\n", sim_day, schedule_to_string(sim_day).c_str());
}


void Person::start_hospitalization(int sim_day, int length_of_stay) {
}

void Person::end_hospitalization() {
}

void Person::print_schedule(int day) {
  FRED_STATUS(0, "%s\n", this->schedule_to_string(day).c_str());
}

void Person::print_activities() {
  FRED_STATUS(0, "%s\n", this->to_string().c_str());
}

void Person::assign_school() {
  int day = Global::Simulation_Day;
  Place* old_school = this->last_school;
  int grade = get_age();
  FRED_VERBOSE(1, "assign_school entered for person %d age %d grade %d\n",
	       get_id(), get_age(), grade);

  Household* hh = get_household();
  assert(hh != NULL);

  Place* school = NULL;
  school = Census_Tract::get_census_tract_with_admin_code(hh->get_census_tract_admin_code())->select_new_school(grade);
  if (school == NULL) {
    school = County::get_county_with_admin_code(hh->get_county_admin_code())->select_new_school(grade);
    FRED_VERBOSE(1, "DAY %d ASSIGN_SCHOOL FROM COUNTY %s selected for person %d age %d\n",
		 day, school->get_label(), get_id(), get_age());
  }
  else {
    FRED_VERBOSE(1, "DAY %d ASSIGN_SCHOOL FROM CENSUS_TRACT %s selected for person %d age %d\n",
		 day, school->get_label(), get_id(), get_age());
  }
  if (school == NULL) {
    school = Place::get_random_school(grade);
  }
  assert(school != NULL);
  set_school(school);
  set_classroom(NULL);
  assign_classroom();
  FRED_VERBOSE(1, "assign_school finished for person %d age %d: school %s classroom %s\n",
               get_id(), get_age(),
	       get_school()->get_label(), get_classroom()->get_label());
}

void Person::assign_classroom() {
  assert(get_school() != NULL && get_classroom() == NULL);
  FRED_VERBOSE(1, "assign classroom entered\n");

  Place* school = get_school();
  Place* place = school->select_partition(this);
  if (place == NULL) {
    FRED_VERBOSE(0, "CLASSROOM_WARNING: assign classroom returns NULL: person %d age %d school %s\n",
		 get_id(), get_age(), school->get_label());
  }
  set_classroom(place);
  FRED_VERBOSE(1,"assign classroom finished\n");
}

void Person::assign_workplace() {
  Household* hh;
  hh = get_household();
  Place* p = NULL;
  p = Census_Tract::get_census_tract_with_admin_code(hh->get_census_tract_admin_code())->select_new_workplace();
  if (p == NULL) {
    p = County::get_county_with_admin_code(hh->get_county_admin_code())->select_new_workplace();
    FRED_VERBOSE(1, "ASSIGN_WORKPLACE FROM COUNTY %s selected for person %d age %d\n",
		 p->get_label(), get_id(), get_age());
  }
  else {
    FRED_VERBOSE(1, "ASSIGN_WORKPLACW FROM CENSUS_TRACT %s selected for person %d age %d\n",
		 p->get_label(), get_id(), get_age());
  }
  change_workplace(p);
}

void Person::assign_office() {
  if(get_workplace() != NULL && get_office() == NULL && get_workplace()->is_workplace()
     && Place_Type::get_place_type("Workplace")->get_partition_capacity() > 0) {
    Place* place = get_workplace()->select_partition(this);
    if(place == NULL) {
      FRED_VERBOSE(0, "OFFICE WARNING: No office assigned for person %d workplace %d\n", get_id(),
		   get_workplace()->get_id());
    }
    set_office(place);
  }
}

void Person::assign_primary_healthcare_facility() {
}

void Person::assign_hospital(Place* place) {
}

int Person::get_degree() {
  int degree;
  int n;
  degree = 0;
  n = get_group_size(Place_Type::get_type_id("Neighborhood"));
  if(n > 0) {
    degree += (n - 1);
  }
  n = get_group_size(Place_Type::get_type_id("School"));
  if(n > 0) {
    degree += (n - 1);
  }
  n = get_group_size(Place_Type::get_type_id("Workplace"));
  if(n > 0) {
    degree += (n - 1);
  }
  n = get_group_size(Place_Type::get_type_id("Hospital"));
  if(n > 0) {
    degree += (n - 1);
  }
  return degree;
}

int Person::get_group_size(int index) {
  int size = 0;
  if(get_activity_group(index) != NULL) {
    size = get_activity_group(index)->get_size();
  }
  return size;
}

bool Person::is_hospital_staff() {
  bool ret_val = false;
  return ret_val;
}

bool Person::is_prison_staff() {
  bool ret_val = false;

  if(this->profile == Activity_Profile::WORKER || this->profile == Activity_Profile::WEEKEND_WORKER) {
    if(get_workplace() != NULL && get_household() != NULL) {
      if(get_workplace()->is_prison() &&
         !get_household()->is_prison()) {
        ret_val = true;
      }
    }
  }

  return ret_val;
}

bool Person::is_college_dorm_staff() {
  bool ret_val = false;

  if(this->profile == Activity_Profile::WORKER || this->profile == Activity_Profile::WEEKEND_WORKER) {
    if(get_workplace() != NULL && get_household() != NULL) {
      if(get_workplace()->is_college() &&
         !get_household()->is_college()) {
        ret_val = true;
      }
    }
  }

  return ret_val;
}

bool Person::is_military_base_staff() {
  bool ret_val = false;

  if(this->profile == Activity_Profile::WORKER || this->profile == Activity_Profile::WEEKEND_WORKER) {
    if(get_workplace() != NULL && get_household() != NULL) {
      if(get_workplace()->is_military_base() &&
         !get_household()->is_military_base()) {
        ret_val = true;
      }
    }
  }

  return ret_val;
}

bool Person::is_nursing_home_staff() {
  bool ret_val = false;

  if(this->profile == Activity_Profile::WORKER || this->profile == Activity_Profile::WEEKEND_WORKER) {
    if(get_workplace() != NULL && get_household() != NULL) {
      if(get_workplace()->is_nursing_home() &&
         !get_household()->is_nursing_home()) {
        ret_val = true;
      }
    }
  }

  return ret_val;
}

void Person::update_profile_after_changing_household() {
  int age = get_age();
  int day = Global::Simulation_Day;

  // printf("person %d house %s subtype %c\n", get_id(), get_household()->get_label(), get_household()->get_subtype());

  // profiles for new group quarters residents
  if(get_household()->is_college()) {
    if(this->profile != Activity_Profile::COLLEGE_STUDENT) {
      this->profile = Activity_Profile::COLLEGE_STUDENT;
      change_school(NULL);
      change_workplace(get_household()->get_group_quarters_workplace());
      FRED_VERBOSE(1, "AFTER_MOVE CHANGING PROFILE TO COLLEGE_STUDENT: person %d age %d DORM %s\n",
		   get_id(), age, get_household()->get_label());
    }
    this->in_parents_home = false;
    return;
  }

  if(get_household()->is_military_base()) {
    if(this->profile != Activity_Profile::MILITARY) {
      this->profile = Activity_Profile::MILITARY;
      change_school(NULL);
      change_workplace(get_household()->get_group_quarters_workplace());
      FRED_VERBOSE(1, "AFTER_MOVE CHANGING PROFILE TO MILITARY: person %d age %d BARRACKS %s\n",
		   get_id(), age, get_household()->get_label());
    }
    this->in_parents_home = false;
    return;
  }

  if(get_household()->is_prison()) {
    if(this->profile != Activity_Profile::PRISONER) {
      this->profile = Activity_Profile::PRISONER;
      change_school(NULL);
      change_workplace(get_household()->get_group_quarters_workplace());
      FRED_VERBOSE(1,"AFTER_MOVE CHANGING PROFILE TO PRISONER: person %d age %d PRISON %s\n",
		   get_id(), age, get_household()->get_label());
    }
    this->in_parents_home = false;
    return;
  }

  if(get_household()->is_nursing_home()) {
    if(this->profile != Activity_Profile::NURSING_HOME_RESIDENT) {
      this->profile = Activity_Profile::NURSING_HOME_RESIDENT;
      change_school(NULL);
      change_workplace(get_household()->get_group_quarters_workplace());
      FRED_VERBOSE(1,"AFTER_MOVE CHANGING PROFILE TO NURSING HOME: person %d age %d NURSING_HOME %s\n",
		   get_id(), age, get_household()->get_label());
    }
    this->in_parents_home = false;
    return;
  }

  // former military
  if(this->profile == Activity_Profile::MILITARY && get_household()->is_military_base() == false) {
    change_school(NULL);
    change_workplace(NULL);
    this->profile = Activity_Profile::WORKER;
    assign_workplace();
    FRED_VERBOSE(1, "AFTER_MOVE CHANGING PROFILE FROM MILITRAY TO WORKER: person %d age %d sex %c WORKPLACE %s OFFICE %s\n",
		 get_id(), age, get_sex(),
		 get_workplace()->get_label(), get_office()->get_label());
    return;
  }

  // former prisoner
  if(this->profile == Activity_Profile::PRISONER && get_household()->is_prison() == false) {
    change_school(NULL);
    change_workplace(NULL);
    this->profile = Activity_Profile::WORKER;
    assign_workplace();
    FRED_VERBOSE(1, "AFTER_MOVE CHANGING PROFILE FROM PRISONER TO WORKER: person %d age %d sex %c WORKPLACE %s OFFICE %s\n",
		 get_id(), age, get_sex(),
		 get_workplace()->get_label(), get_office()->get_label());
    return;
  }

  // former college student
  if(this->profile == Activity_Profile::COLLEGE_STUDENT && get_household()->is_college() == false) {
    if(Random::draw_random() < 0.25) {
      // time to leave college for good
      change_school(NULL);
      change_workplace(NULL);
      // get a job
      this->profile = Activity_Profile::WORKER;
      assign_workplace();
      FRED_VERBOSE(1, "AFTER_MOVE CHANGING PROFILE FROM COLLEGE STUDENT TO WORKER: id %d age %d sex %c HOUSE %s WORKPLACE %s OFFICE %s\n",
		   get_id(), age, get_sex(), get_household()->get_label(), get_workplace()->get_label(), get_office()->get_label());
    }
    return;
  }

  // school age => select a new school
  if(this->profile == Activity_Profile::STUDENT && age < Global::ADULT_AGE) {
    Place* school = get_school();
    Place* old_school = this->last_school;
    int grade = get_age();
    Household* hh = get_household();
    assert(hh != NULL);

    // FRED_VERBOSE(0, "DAY %d AFTER_MOVE checking school status\n", day);
    // stay in current school if this grade is available and is attended by student in this census tract
    Census_Tract* ct = Census_Tract::get_census_tract_with_admin_code(hh->get_census_tract_admin_code());
    if (school != NULL && ct->is_school_attended(school->get_id(), grade)) {
      set_classroom(NULL);
      assign_classroom();
      assert(get_school() && get_classroom());
      FRED_VERBOSE(1, "DAY %d AFTER_MOVE STAY IN CURRENT SCHOOL: person %d age %d LAST_SCHOOL %s SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
		   day, get_id(), age, 
		   old_school == NULL ? "NULL" : old_school->get_label(),
		   get_school()->get_label(), get_school()->get_size(),
		   get_school()->get_original_size(), get_classroom()->get_label());
    }
    else {
      // select a new school
      change_school(NULL);
      change_workplace(NULL);
      assign_school();
      assert(get_school() && get_classroom());
      FRED_VERBOSE(1, "DAY %d AFTER_MOVE SELECT NEW SCHOOL: person %d age %d LAST_SCHOOL %s SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
		   day, get_id(), age, 
		   old_school == NULL ? "NULL" : old_school->get_label(),
		   get_school()->get_label(), get_school()->get_size(),
		   get_school()->get_original_size(), get_classroom()->get_label());
    }
    return;
  }


  // worker => select a new workplace
  if(this->profile == Activity_Profile::WORKER || this->profile == Activity_Profile::WEEKEND_WORKER) {
    change_school(NULL);
    Place* old_workplace = get_workplace();
    change_workplace(NULL);
    assign_workplace();
    FRED_VERBOSE(1, "AFTER_MOVE SELECT NEW WORKPLACE: person %d age %d sex %c OLD WORKPLACE %s NEW WORKPLACE %s OFFICE %s\n",
		 get_id(), age, get_sex(),
		 old_workplace == NULL ? "NULL" : old_workplace->get_label(),
		 get_workplace()->get_label(), get_office()->get_label());
    return;
  }

}

void Person::update_profile_based_on_age() {
  int age = get_age();
  int day = Global::Simulation_Day;

  // pre-school children entering school
  if(this->profile == Activity_Profile::PRESCHOOL && Global::SCHOOL_AGE <= age && age < Global::ADULT_AGE) {
    this->profile = Activity_Profile::STUDENT;
    change_school(NULL);
    change_workplace(NULL);
    assign_school();
    assert(get_school() && get_classroom());
    FRED_VERBOSE(1, "AGE_UP CHANGING PROFILE FROM PRESCHOOL TO STUDENT: person %d age %d SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
                 get_id(), age, get_school()->get_label(), get_school()->get_size(),
		 get_school()->get_original_size(), get_classroom()->get_label());
    return;
  }

  // school age
  if(this->profile == Activity_Profile::STUDENT && age < Global::ADULT_AGE) {
    Place* old_school = this->last_school;
    Place* school = get_school();
    int grade = get_age();
    // FRED_VERBOSE(0, "DAY %d AGE_UP checking school status, age = %d\n", day, age);
    // stay in current school if this grade is available
    if (school != NULL && school->get_number_of_partitions_by_age(grade) > 0) {
      FRED_VERBOSE(1, "DAY %d AGE_UP checking school status, age = %d classroms %d\n", day, age,
		   get_school()->get_number_of_partitions_by_age(grade));
      set_classroom(NULL);
      assign_classroom();
      assert(get_school() && get_classroom());
      FRED_VERBOSE(1, "DAY %d AGE_UP STAY IN SCHHOL: person %d age %d LAST_SCHOOL %s SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
		   day, get_id(), age, 
		   old_school == NULL ? "NULL" : old_school->get_label(),
		   get_school()->get_label(), get_school()->get_size(),
		   get_school()->get_original_size(), get_classroom()->get_label());
    }
    else {
      change_school(NULL);
      change_workplace(NULL);
      assign_school();
      assert(get_school() && get_classroom());
      FRED_VERBOSE(1, "DAY %d AGE_UP KEEPING STUDENT PROFILE: person %d age %d LAST_SCHOOL %s SCHOOL %s SIZE %d ORIG %d CLASSROOM %s\n",
		   day,  get_id(), age, 
		   old_school == NULL ? "NULL" : old_school->get_label(),
		   get_school()->get_label(), get_school()->get_size(),
		   get_school()->get_original_size(), get_classroom()->get_label());
    }
    return;
  }


  // graduating from school
  if(this->profile == Activity_Profile::STUDENT && Global::ADULT_AGE <= age) {
    Place* old_school = this->last_school;
    change_school(NULL);
    change_workplace(NULL);
    this->profile = Activity_Profile::WORKER;
    assign_workplace();
    FRED_VERBOSE(1, "DAY %d AGE_UP CHANGING PROFILE FROM STUDENT TO WORKER: person %d age %d LAST_SCHOOL %s sex %c WORKPLACE %s OFFICE %s\n",
                 day, get_id(), age,
		 old_school == NULL ? "NULL" : old_school->get_label(),
		 get_sex(), get_workplace()->get_label(), get_office()->get_label());
    return;
  }


  // unemployed worker re-entering workplace
  if(this->profile == Activity_Profile::WORKER || this->profile == Activity_Profile::WEEKEND_WORKER) {
    if(get_workplace() == NULL) {
      assign_workplace();
      FRED_VERBOSE(1, "AGE_UP CHANGING PROFILE FROM UNEMPLOYED TO WORKER: person %d age %d sex %c WORKPLACE %s OFFICE %s\n",
		   get_id(), age, get_sex(), get_workplace()->get_label(), get_office()->get_label());
    }
    // NOTE: no return;
  }

  // possibly entering retirement
  if(this->profile != Activity_Profile::RETIRED && Global::RETIREMENT_AGE <= age && get_household()->is_group_quarters()==false) {
    if(Random::draw_random()< 0.5 ) {
      // quit working
      if(is_teacher()) {
        change_school(NULL);
      }
      change_workplace(NULL);
      this->profile = Activity_Profile::RETIRED;
      FRED_VERBOSE(1, "AGE_UP CHANGING PROFILE TO RETIRED: person %d age %d sex\n",
		   get_id(), age, get_sex());
    }
    return;
  }

}


void Person::start_traveling(Person* visited) {

  if(visited == NULL) {
    this->is_traveling_outside = true;
  } else {
    store_activity_groups();
    clear_activity_groups();
    set_household(visited->get_household());
    set_neighborhood(visited->get_neighborhood());
    if(this->profile == Activity_Profile::WORKER) {
      set_workplace(visited->get_workplace());
      set_office(visited->get_office());
    }
  }
  this->is_traveling = true;
  FRED_STATUS(1, "start traveling: id = %d\n", get_id());
}

void Person::stop_traveling() {
  if(!this->is_traveling_outside) {
    restore_activity_groups();
  }
  this->is_traveling = false;
  this->is_traveling_outside = false;
  this->return_from_travel_sim_day = -1;
  FRED_STATUS(1, "stop traveling: id = %d\n", get_id());
}

bool Person::become_a_teacher(Place* school) {
  bool success = false;
  FRED_VERBOSE(1, "become_a_teacher: person %d age %d\n", get_id(), get_age());
  // print(this);
  if(get_school() != NULL) {
    if(Global::Verbose > 1) {
      FRED_WARNING("become_a_teacher: person %d age %d ineligible -- already goes to school %d %s\n",
		   get_id(), get_age(), get_school()->get_id(), get_school()->get_label());
    }
    this->profile = Activity_Profile::STUDENT;
  } else {
    // set profile
    this->profile = Activity_Profile::TEACHER;
    // join the school
    FRED_VERBOSE(1, "set school to %s\n", school->get_label());
    set_school(school);
    set_classroom(NULL);
    success = true;
  }
  
  // withdraw from this workplace and any associated office
  Place* workplace = get_workplace();
  FRED_VERBOSE(1, "leaving workplace %d %s\n", workplace->get_id(), workplace->get_label());
  change_workplace(NULL);
  if (success) {
    FRED_VERBOSE(1, "become_a_teacher finished for person %d age %d  school %s\n",
		 get_id(),
		 get_age(),
		 school->get_label());
    // print(this);
  }
  return success;
}

void Person::change_school(Place* place) {
  FRED_VERBOSE(1, "person %d set school %s\n", get_id(), place ? place->get_label() : "NULL");
  set_school(place);
  FRED_VERBOSE(1,"set classroom to NULL\n");
  set_classroom(NULL);
  if(place != NULL) {
    FRED_VERBOSE(1, "assign classroom\n");
    assign_classroom();
  }
}

void Person::change_workplace(Place* place, int include_office) {
  FRED_VERBOSE(1, "person %d set workplace %s\n", get_id(), place ? place->get_label() : "NULL");
  set_workplace(place);
  set_office(NULL);
  if(place != NULL) {
    if(include_office) {
      assign_office();
    }
  }
}

std::string Person::schedule_to_string(int day) {
  std::stringstream ss;
  ss << "day " << day << " schedule for person " << get_id() << "  ";
  for(int p = 0; p < Place_Type::get_number_of_place_types(); ++p) {
    if(get_activity_group(p)) {
      ss << Place_Type::get_place_type_name(p) << ": ";
      ss << (this->on_schedule[p] ? "+" : "-");
      ss << get_activity_group_label(p) << " ";
    }
  }
  return ss.str();
}

std::string Person::activities_to_string() {
  std::stringstream ss;
  ss << "Activities for person " << get_id() << ": ";
  for(int p = 0; p < Place_Type::get_number_of_place_types(); ++p) {
    if(get_activity_group(p)) {
      ss << Place_Type::get_place_type_name(p) << ": ";
      ss << get_activity_group_label(p) << " ";
    }
  }
  return ss.str();
}

void Person::change_household(Place* house) {

  // household can not be NULL
  assert(house != NULL); 

  FRED_VERBOSE(1, "move_to_new_house start person %d house %s subtype %c\n",
	       get_id(), house->get_label(), house->get_subtype());

  // change household
  set_household(house);

  // set neighborhood
  set_neighborhood(house->get_patch()->get_neighborhood());

  // possibly update the profile depending on new household
  update_profile_after_changing_household();

  FRED_VERBOSE(1, "move_to_new_house finished person %d house %s subtype %c profile %d\n",
	       get_id(), get_household()->get_label(),
	       get_household()->get_subtype(), this->profile);
}

void Person::terminate_activities() {
  if(get_travel_status()) {
    if(this->is_traveling && !this->is_traveling_outside) {
      restore_activity_groups();
    }
    Travel::terminate_person(this);
  }

  // withdraw from society
  end_membership_in_activity_groups();
}

int Person::get_visiting_health_status(Place* place, int day, int condition_id) {

  // assume we are not visiting this place today
  int status = 0;

  // traveling abroad?
  if(this->is_traveling_outside) {
    return status;
  }

  if(day > this->schedule_updated) {
    // get list of places to visit today
    update_activities(day);
  }

  // see if the given place is on my schedule today
  int place_type_id = place->get_type_id();
  if(this->on_schedule[place_type_id] && get_activity_group(place_type_id) == place) {
    if(is_susceptible(condition_id)) {
      status = 1;
    } else if(is_transmissible(condition_id)) {
      status = 2;
    } else {
      status = 3;
    }
  }

  return status;
}

void Person::update_member_index(Group* group, int new_index) {
  int type = group->get_type_id();
  if(group == get_activity_group(type)) {
    FRED_VERBOSE(1, "update_member_index for person %d type %d new_index %d\n", get_id(), type, new_index);
    this->link[type].update_member_index(new_index);
    return;
  }
  else {
    FRED_VERBOSE(0, "update_member_index: person %d group %s not found at pos %d in daily activity locations: ",
		 get_id(), group->get_label(), type);
    for(int i = 0; i < Group_Type::get_number_of_group_types(); ++i) {
      printf("%s ", this->link[i].get_group() == NULL ? "NULL" : this->link[i].get_group()->get_label()); 
    }
    printf("\n");
    fflush(stdout);
    assert(0);
  }
}

///////////////////////////////////

void Person::clear_activity_groups() {
  FRED_VERBOSE(1, "clear_activity_groups entered group_types = %d\n", Group_Type::get_number_of_group_types());
  for(int i = 0; i < Group_Type::get_number_of_group_types(); ++i) {
    if(this->link[i].is_member()) {
      this->link[i].end_membership(this);
    }
    assert(this->link[i].get_place() == NULL);
  }
  FRED_VERBOSE(1, "clear_activity_groups finished group_types = %d\n", Group_Type::get_number_of_group_types());
}
  
void Person::begin_membership_in_activity_group(int i) {
  Group* group = get_activity_group(i);
  if(group != NULL) {
    this->link[i].begin_membership(this, group);
  }
}

void Person::begin_membership_in_activity_groups() {
  for(int i = 0; i < Group_Type::get_number_of_group_types(); ++i) {
    begin_membership_in_activity_group(i);
  }
}

void Person::end_membership_in_activity_group(int i) {
  Group* group = get_activity_group(i);
  if(group != NULL) {
    this->link[i].end_membership(this);
  }
}

void Person::end_membership_in_activity_groups() {
  for(int i = 0; i < Group_Type::get_number_of_group_types(); ++i) {
    end_membership_in_activity_group(i);
  }
  clear_activity_groups();
}

void Person::store_activity_groups() {
  int group_types = Group_Type::get_number_of_group_types();
  this->stored_activity_groups = new Group* [group_types];
  for(int i = 0; i < group_types; ++i) {
    this->stored_activity_groups[i] = get_activity_group(i);
  }
}

void Person::restore_activity_groups() {
  int group_types = Group_Type::get_number_of_group_types();
  for(int i = 0; i < group_types; ++i) {
    set_activity_group(i, this->stored_activity_groups[i]);
  }
  delete[] this->stored_activity_groups;
}

int Person::get_activity_group_id(int p) {
  return get_activity_group(p) == NULL ? -1 : get_activity_group(p)->get_id();
}

const char* Person::get_activity_group_label(int p) {
  return (get_activity_group(p) == NULL) ? "NULL" : get_activity_group(p)->get_label();
}


void Person::set_activity_group(int i, Group* group) {

  Group* old_group = get_activity_group(i);

  FRED_VERBOSE(1, "person %d SET ACTIVITY GROUP %d group %d %s size %d\n",
	       this->id, i, 
	       group ? group->get_id() : -1, 
	       group? group->get_label(): "NULL",
	       group? group->get_size() :-1);

  // update link if necessary
  // FRED_VERBOSE(0, "old group %s\n", old_group? old_group->get_label():"NULL");
  if(group != old_group) {
    if(old_group != NULL) {
      // remove old link
      // printf("remove old link\n");
      if (this->is_meta_agent()) {
	this->link[i].unlink(this);
      }
      else {
	this->link[i].end_membership(this);
      }
    }
    if(group != NULL) {
      if (this->is_meta_agent()) {
	this->link[i].link(this, group);
      }
      else {
	// printf("begin membership in link %d\n", i);
	this->link[i].begin_membership(this, group);
	// FRED_VERBOSE(0, "test link %s\n", this->link[i].get_group()->get_label());
      }
    }
  }
  FRED_VERBOSE(1, "person %d SET ACTIVITY done GROUP %d group %d %s size %d\n",
	       this->id, i, 
	       group ? group->get_id() : -1, 
	       group? group->get_label(): "NULL",
	       group? group->get_size() :-1);

}

bool Person::is_present(int sim_day, Group* group) {

  if (group==NULL) {
    return false;
  }
  if (this->is_meta_agent()) {
    return false;
  }

  int type_id = group->get_type_id();

  // FRED_VERBOSE(0, "IS_PRESENT entered day %d person %d group %s\n", sim_day, get_id(), group->get_label());

  /*
  printf("SCHEDULE 1: ");
  for (int n = 0; n < Group_Type::get_number_of_group_types(); n++) {
    printf("%s %s ", Group_Type::get_group_type_name(n).c_str(), this->on_schedule[n] ? "Y" : "N");
  }
  printf("\n");
  */

  // not here if traveling abroad
  if(this->is_traveling_outside) {
    return false;
  }

  // update list of groups to visit today if not already done
  if(sim_day > this->schedule_updated) {
    update_activities(sim_day);
  }

  // see if this group is on the list
  if (this->on_schedule[group->get_type_id()]) {
    for (int cond_id = 0; cond_id < Condition::get_number_of_conditions(); cond_id++) {
      int state = get_state(cond_id);
      if (Condition::get_condition(cond_id)->is_absent(state, type_id)) {
	// printf("absent due to condition %d\n", cond_id);
	return false;
      }
    }
    // printf("is on schedule\n");
    return true;
  }
  else {
    // printf("not on schedule\n");
    return false;
  }

}

void Person::join_network(Network* network) {
  int network_type_id = network->get_type_id();
  if (this->link[network_type_id].is_member()) {
    return;
  }
  this->link[network_type_id].begin_membership(this, network);
  FRED_VERBOSE(1, "JOINED NETWORK: person %d network %s type_id %d size %d\n",
	       get_id(), network->get_label(), network_type_id, network->get_size());
}

void Person::quit_network(Network* network) {
  FRED_VERBOSE(1, "UNENROLL NETWORK: id = %d\n", get_id());
  int network_type_id = network->get_type_id();
  this->link[network_type_id].remove_from_network(this);
}


void Person::add_edge_to(Person* other, Network* network) {
  // FRED_VERBOSE(0, "add_edge_to entered network %s person %d\n", network->get_name(), other? other->get_id(): -9999);
  if (other==NULL) {
    return;
  }
  int n = network->get_type_id();
  if (0 <= n) {
    join_network(network);
    this->link[n].add_edge_to(other);
  }
}

void Person::add_edge_from(Person* other, Network* network) {
  if (other==NULL) {
    return;
  }
  int n = network->get_type_id();
  if (0 <= n) {
    join_network(network);
    this->link[n].add_edge_from(other);
  }
}

void Person::delete_edge_to(Person* person, Network* network) {
  if (person==NULL) {
    return;
  }
  int n = network->get_type_id();
  if (0 <= n) {
    this->link[n].delete_edge_to(person);
  }
}

void Person::delete_edge_from(Person* person, Network* network) {
  if (person==NULL) {
    return;
  }
  int n = network->get_type_id();
  if (0 <= n) {
    this->link[n].delete_edge_from(person);
  }
}

bool Person::is_member_of_network(Network* network) {
  int n = network->get_type_id();
  return this->link[n].get_network() != NULL;
}

bool Person::is_connected_to(Person* person, Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].is_connected_to(person);
  }
  return false;
}


bool Person::is_connected_from(Person* person, Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].is_connected_from(person);
  }
  return false;
}

int Person::get_id_of_max_weight_inward_edge_in_network(Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_id_of_max_weight_inward_edge();
  }
  else {
    return -99999999;
  }
}

int Person::get_id_of_max_weight_outward_edge_in_network(Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_id_of_max_weight_outward_edge();
  }
  else {
    return -99999999;
  }
}

int Person::get_id_of_min_weight_inward_edge_in_network(Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_id_of_min_weight_inward_edge();
  }
  else {
    return -99999999;
  }
}

int Person::get_id_of_min_weight_outward_edge_in_network(Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_id_of_min_weight_outward_edge();
  }
  else {
    return -99999999;
  }
}

int Person::get_id_of_last_inward_edge_in_network(Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_id_of_last_inward_edge();
  }
  else {
    return -99999999;
  }
}

int Person::get_id_of_last_outward_edge_in_network(Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_id_of_last_outward_edge();
  }
  else {
    return -99999999;
  }
}


double Person::get_weight_to(Person* person, Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_weight_to(person);
  }
  return 0.0;
}

void Person::set_weight_to(Person* person, Network* network, double value) {
  int n = network->get_type_id();
  if (0 <= n) {
    this->link[n].set_weight_to(person, value);
  }
}

void Person::set_weight_from(Person* person, Network* network, double value) {
  int n = network->get_type_id();
  if (0 <= n) {
    this->link[n].set_weight_from(person, value);
  }
}

double Person::get_weight_from(Person* person, Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_weight_from(person);
  }
  return 0.0;
}

double Person::get_timestamp_to(Person* person, Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_timestamp_to(person);
  }
  return 0.0;
}

double Person::get_timestamp_from(Person* person, Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_timestamp_from(person);
  }
  return 0.0;
}

int Person::get_out_degree(Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_out_degree();
  }
  else {
    return 0;
  }
}

int Person::get_in_degree(Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_in_degree();
  }
  else {
    return 0;

  }
}

int Person::get_degree(Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    if (network->is_undirected()) {
      return this->link[n].get_in_degree();
    }
    else {
      return this->link[n].get_in_degree() + this->link[n].get_out_degree();
    }
  }
  else {
    return 0;

  }
}


void Person::clear_network(Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    this->link[n].clear();
  }
}

person_vector_t Person::get_outward_edges(Network* network, int max_dist) {
  FRED_VERBOSE(1, "get_outward_edges person %d network %s max_dist %d\n",
	       get_id(), network ? network->get_label() : "NULL", max_dist);
  std::unordered_set<int> found = {};
  person_vector_t results;
  results.clear();
  assert(network != NULL);
  if (network == NULL) {
    return results;
  }
  int n = network->get_type_id();
  if (1 <= max_dist) {
    person_vector_t tmp = this->link[n].get_outward_edges();
    for (int k = 0; k < tmp.size(); k++) {
      if (tmp[k] != this && found.insert(tmp[k]->get_id()).second) {
	results.push_back(tmp[k]);
	FRED_VERBOSE(1, "add direct link to person %d result = %lu\n", tmp[k]->get_id(), results.size());
      }
    }
    if (max_dist > 1) {
      int size = results.size();
      for (int i = 0; i < size; i++) {
	Person* other = results[i];
	tmp = other->get_outward_edges(network, max_dist-1);
	for (int k = 0; k < tmp.size(); k++) {
	  if (tmp[k] != this && found.insert(tmp[k]->get_id()).second) {
	    results.push_back(tmp[k]);
	    FRED_VERBOSE(1, "add indirect link thru person %d to person %d result = %lu\n",
			 other->get_id(),
			 tmp[k]->get_id(), results.size());
	  }
	}
      }
    }
  }
  // sort the results by id
  std::sort(results.begin(), results.end(), Utils::compare_id);

  /*
  for (int i = 0; i < results.size(); i++) {
    printf("get_outward_edges network %s person %d other %d\n",
	   network->get_label(), this->id, results[i]->get_id());
  }
  */

  FRED_VERBOSE(1, "get_outward_edges finished person %d network %s max_dist %d\n\n",
	       get_id(), network ? network->get_label() : "NULL", max_dist);

  return results;
}

person_vector_t Person::get_inward_edges(Network* network, int max_dist) {
  FRED_VERBOSE(1, "get_linked_people person %d network %s max_dist %d\n",
	       get_id(), network ? network->get_label() : "NULL", max_dist);
  std::unordered_set<int> found = {};
  person_vector_t results;
  results.clear();
  assert(network != NULL);
  if (network == NULL) {
    return results;
  }
  int n = network->get_type_id();
  if (1 <= max_dist) {
    person_vector_t tmp = this->link[n].get_inward_edges();
    for (int k = 0; k < tmp.size(); k++) {
      if (tmp[k] != this && found.insert(tmp[k]->get_id()).second) {
	results.push_back(tmp[k]);
	FRED_VERBOSE(1, "add direct link to person %d result = %lu\n", tmp[k]->get_id(), results.size());
      }
    }
    if (max_dist > 1) {
      int size = results.size();
      for (int i = 0; i < size; i++) {
	Person* other = results[i];
	tmp = other->get_inward_edges(network, max_dist-1);
	for (int k = 0; k < tmp.size(); k++) {
	  if (tmp[k] != this && found.insert(tmp[k]->get_id()).second) {
	    results.push_back(tmp[k]);
	    FRED_VERBOSE(1, "add indirect link thru person %d to person %d result = %lu\n",
			 other->get_id(),
			 tmp[k]->get_id(), results.size());
	  }
	}
      }
    }
  }
  // sort the results by id
  std::sort(results.begin(), results.end(), Utils::compare_id);

  /*
  for (int i = 0; i < results.size(); i++) {
    printf("get_inward_edges network %s person %d other %d\n",
	   network->get_label(), this->id, results[i]->get_id());
  }
  */

  FRED_VERBOSE(1, "get_inward_edges finished person %d network %s max_dist %d\n\n",
	       get_id(), network ? network->get_label() : "NULL", max_dist);

  return results;
}

Person* Person::get_outward_edge(int k, Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_outward_edge(k);
  }
  else {
    return NULL;
  }
}

Person* Person::get_inward_edge(int k, Network* network) {
  int n = network->get_type_id();
  if (0 <= n) {
    return this->link[n].get_inward_edge(k);
  }
  else {
    return NULL;
  }
}


/**
 * @return a pointer to this agent's Household
 */
Household* Person::get_household() {
  int i = Place_Type::get_type_id("Household");
  Group* group = this->link[i].get_group();
  group = get_activity_group(i);
  Household* hh = static_cast<Household*>(group);
  return hh;
}

Household* Person::get_stored_household() {
  return static_cast<Household*>(this->stored_activity_groups[Place_Type::get_type_id("Household")]);
}


/**
 * @return a pointer to this agent's Hospital
 */
Hospital* Person::get_hospital() {
  return static_cast<Hospital*>( get_activity_group(Place_Type::get_type_id("Hospital")));
}

void Person::set_last_school(Place* school) {
  this->last_school = school;
}


void Person::select_activity_of_type(int place_type_id) {
  Place_Type* place_type = Place_Type::get_place_type(place_type_id);
  if (place_type != NULL) {
    Place *place = Place_Type::select_place_of_type(place_type_id, this);
    set_activity_group(place_type_id, place);
  }
}

void Person::schedule_activity(int day, int group_type_id) {
  Group* group = get_activity_group(group_type_id);
  this->on_schedule[group_type_id] = true;
  if (group != NULL && is_present(day, group)==false) {
    this->on_schedule[group_type_id] = true;
  }
}


void Person::cancel_activity(int day, int group_type_id) {
  Group* group = get_activity_group(group_type_id);
  // FRED_VERBOSE(0, "CANCEL person %d day %d group_type %d\n", get_id(), day, group_type_id);
  FRED_VERBOSE(1, "CANCEL group %s\n", group==NULL ? "NULL" : group->get_label());
  if (group != NULL && is_present(day, group)) {
    printf("CANCEL_ACTIVITY person %d day %d group_type %s\n", get_id(), day, Group_Type::get_group_type(group_type_id)->get_name());
    this->on_schedule[group_type_id] = false;
  }
}

std::string Person::get_profile_name(int prof) {
  switch(prof) {
  case Activity_Profile::INFANT :
    return "infant";
    break;

  case Activity_Profile::PRESCHOOL :
    return "preschool";
    break;

  case Activity_Profile::STUDENT :
    return "student";
    break;

  case Activity_Profile::TEACHER :
    return "teacher";
    break;

  case Activity_Profile::WORKER :
    return "worker";
    break;

  case Activity_Profile::WEEKEND_WORKER :
    return "weekend_worker";
    break;

  case Activity_Profile::UNEMPLOYED :
    return "unemployed";
    break;

  case Activity_Profile::RETIRED :
    return "retired";
    break;

  case Activity_Profile::PRISONER :
    return "prisoner";
    break;

  case Activity_Profile::COLLEGE_STUDENT :
    return "college_student";
    break;

  case Activity_Profile::MILITARY :
    return "military";
    break;

  case Activity_Profile::NURSING_HOME_RESIDENT :
    return "nursing_home_resident";
    break;

  case Activity_Profile::UNDEFINED :
    return "undefined";
    break;

  default:
    return "unknown";
  }
}

int Person::get_profile_from_name(std::string name) {
  for (int i = 0; i < Activity_Profile::ACTIVITY_PROFILE; i++) {
    if (name == Person::get_profile_name(i)) {
      return i;
    }
  }
  return -1;
}



void Person::run_action_rules(int condition_id, int state, rule_vector_t rules) {

  int day = Global::Simulation_Day;
  int hour = Global::Simulation_Hour;

  FRED_VERBOSE(1, "run side-effect rules day %d hour %d person %d cond %d state %d rules %d\n",
	       day, hour, this->id, condition_id, state, (int) rules.size());

  for (int i = 0; i < rules.size(); i++) {

    Rule* rule = rules[i];
    int action = rule->get_action_id();
    // rule->print();
    string action_str = rule->get_action();
    int cond_id = rule->get_cond_id();
    int state_id = rule->get_state_id();
    int next_state_id = rule->get_next_state_id();
    int source_cond_id = rule->get_source_cond_id();
    int var_id = -1;

    string group_name = rule->get_group();
    int group_type_id = rule->get_group_type_id();

    string network_name = rule->get_network();
    Network* network = Network::get_network(network_name);
    int network_type_id = network ? network->get_type_id() : -1;

    Expression* expr = rule->get_expression();
    Expression* expr2 = rule->get_expression2();
    double value = 0;
    Preference* preference = NULL;
    Clause* clause = rule->get_clause();

    if (clause) {
      if (clause->get_value(this)==false) {
	// printf("action rule clause failed: ");
	// rule->print();
	continue;
      }
      else {
	// printf("action rule clause passed: ");
	// rule->print();
      }
    }
    else {
      // printf("no clause found: ");
      // rule->print();
    }

    switch (action) {

    case Rule_Action::GIVE_BIRTH :
      give_birth(day);
      break;

    case Rule_Action::DIE :
    case Rule_Action::DIE_OLD :
      this->condition[cond_id].susceptibility = 0;
      break;

    case Rule_Action::SUS :
      this->condition[cond_id].susceptibility = expr->get_value(this);
      break;

    case Rule_Action::SET_SUS :
      this->condition[rule->get_source_cond_id()].susceptibility = expr2->get_value(this);
      break;

    case Rule_Action::SET_TRANS :
      if (0 <= this->id) {
	this->condition[rule->get_source_cond_id()].transmissibility = expr2->get_value(this);
      }
      else if (this->id == -1) {
	int source_cond_id = rule->get_source_cond_id();
	double old_value = Condition::get_condition(source_cond_id)->get_transmissibility();
	double value = expr2->get_value(this);
	Condition::get_condition(rule->get_source_cond_id())->set_transmissibility(value);
	if (Global::Enable_Records && Global::Enable_Var_Records && old_value!=value) {
	  char tmp[FRED_STRING_SIZE];
	  get_record_string(tmp);
	  fprintf(Global::Recordsfp,
		  "%s state %s.%s changes %s.transmissibility from %f to %f\n",
		  tmp,
		  get_natural_history(condition_id)->get_name(),
		  get_natural_history(condition_id)->get_state_name(state).c_str(),
		  Condition::get_condition(rule->get_source_cond_id())->get_name(),
		  old_value,
		  value);
	}
      }
      break;

    case Rule_Action::TRANS :
      this->condition[cond_id].transmissibility = expr->get_value(this);
      break;

    case Rule_Action::JOIN :
      {
	if (Group::is_a_place(group_type_id)) {
	  if (expr2) {
	    long long int sp_id = expr2->get_value(this);
	    // FRED_VERBOSE(0, "JOIN specific place sp_id %lld\n", sp_id);
	    Place* place = static_cast<Place*>(Group::get_group_from_sp_id(sp_id));
	    // FRED_VERBOSE(0, "JOIN specific place sp_id %lld %s\n", sp_id, place->get_label());
	    set_activity_group(group_type_id, place);
	  }
	  else {
	    select_place_of_type(group_type_id);
	  }
	}
	else {
	  if (Group::is_a_network(group_type_id)) {
	    // FRED_VERBOSE(0, "JOIN NETWORK %s\n", network_name.c_str());
	    join_network(network_type_id);
	  }
	}
      }
      break;

    case Rule_Action::QUIT :
      if (Group::is_a_place(group_type_id)) {
	quit_place_of_type(group_type_id);
      }
      else {
	// FRED_VERBOSE(0, "QUIT NETWORK %s\n", network_name.c_str());
	quit_network(network_type_id);
      }
      break;

    case Rule_Action::ADD_EDGE_FROM :
      {
	// FRED_VERBOSE(0, "ADD_EDGE_FROM NETWORK %s\n", network_name.c_str());
	double_vector_t id_vec;
	if (expr->is_list_expression()) {
	  id_vec = expr->get_list_value(this);
	}
	else {
	  id_vec.push_back(expr->get_value(this));
	}
	int size = id_vec.size();
	for (int i = 0; i < size; i++) {
	  int other_id = id_vec[i];
	  // FRED_VERBOSE(0, "ADD_EDGE_FROM NETWORK %s other_id %d\n", network_name.c_str(), other_id);
	  Person* other = Person::get_person_with_id(other_id);
	  add_edge_from(other, network);
	  other->add_edge_to(this, network);
	  if (network->is_undirected()) {
	    add_edge_to(other, network);
	    other->add_edge_from(this, network);
	  }
	}
      }
      break;

    case Rule_Action::ADD_EDGE_TO :
      {
	// FRED_VERBOSE(0, "ADD_EDGE_TO NETWORK %s\n", network_name.c_str());
	double_vector_t id_vec;
	if (expr->is_list_expression()) {
	  id_vec = expr->get_list_value(this);
	}
	else {
	  id_vec.push_back(expr->get_value(this));
	}
	int size = id_vec.size();
	// FRED_VERBOSE(0, "ADD_EDGE_TO NETWORK %s edges %d\n", network_name.c_str(), size);
	for (int i = 0; i < size; i++) {
	  int other_id = id_vec[i];
	  // FRED_VERBOSE(0, "person %d ADD_EDGE_TO NETWORK %s other_id %d\n", this->id, network_name.c_str(), other_id);
	  Person* other = Person::get_person_with_id(other_id);
	  add_edge_to(other, network);
	  other->add_edge_from(this, network);
	  if (network->is_undirected()) {
	    add_edge_from(other, network);
	    other->add_edge_to(this, network);
	  }
	  // DEBUGGING:
	  // network->print_person(this, stdout);
	  // network->print_person(other, stdout);
	}
      }
      break;

    case Rule_Action::DELETE_EDGE_FROM :
      {
	// FRED_VERBOSE(0, "DELETE_EDGE_FROM NETWORK %s\n", network_name.c_str());
	double_vector_t id_vec;
	if (expr->is_list_expression()) {
	  id_vec = expr->get_list_value(this);
	}
	else {
	  id_vec.push_back(expr->get_value(this));
	}
	int size = id_vec.size();
	for (int i = 0; i < size; i++) {
	  int other_id = id_vec[i];
	  // FRED_VERBOSE(0, "DELETE_EDGE_FROM NETWORK %s other_id %d\n", network_name.c_str(), other_id);
	  Person* other = Person::get_person_with_id(other_id);
	  delete_edge_from(other, network);
	  other->delete_edge_to(this, network);
	  if (network->is_undirected()) {
	    delete_edge_to(other, network);
	    other->delete_edge_from(this, network);
	  }
	}
      }
      break;

    case Rule_Action::DELETE_EDGE_TO :
      {
	// FRED_VERBOSE(0, "DELETE_EDGE_TO NETWORK %s\n", network_name.c_str());
	double_vector_t id_vec;
	if (expr->is_list_expression()) {
	  id_vec = expr->get_list_value(this);
	}
	else {
	  id_vec.push_back(expr->get_value(this));
	}
	int size = id_vec.size();
	for (int i = 0; i < size; i++) {
	  int other_id = id_vec[i];
	  // FRED_VERBOSE(0, "DELETE_EDGE_TO NETWORK %s other_id %d\n", network_name.c_str(), other_id);
	  Person* other = Person::get_person_with_id(other_id);
	  delete_edge_to(other, network);
	  other->delete_edge_from(this, network);
	  if (network->is_undirected()) {
	    delete_edge_from(other, network);
	    other->delete_edge_to(this, network);
	  }
	}
      }
      break;

    case Rule_Action::SET :
      {
	var_id = rule->get_var_id();
	bool global = rule->is_global();
	Person* other = NULL;
	Expression* other_expr = rule->get_expression2();
	if (other_expr) {
	  int person_id = other_expr->get_value(this);
	  other = Person::get_person_with_id(person_id);
	}
	value = rule->get_value(this, other);
	if (global) {
	  if (Global::Enable_Records && Global::Enable_Var_Records && Person::global_var[var_id]!=value) {
	    char tmp[FRED_STRING_SIZE];
	    get_record_string(tmp);
	    fprintf(Global::Recordsfp,
		    "%s state %s.%s changes %s from %f to %f\n",
		    tmp,
		    get_natural_history(condition_id)->get_name(),
		    get_natural_history(condition_id)->get_state_name(state).c_str(),
		    Person::get_global_var_name(var_id).c_str(),
		    Person::global_var[var_id],
		    value);
	  }
	  Person::global_var[var_id] = value;
	}
	else {
	  if (other==NULL) {
	    if (Global::Enable_Records && Global::Enable_Var_Records && this->var[var_id]!=value) {
	      char tmp[FRED_STRING_SIZE];
	      get_record_string(tmp);
	      fprintf(Global::Recordsfp,
		      "%s state %s.%s changes %s from %f to %f\n",
		      tmp,
		      get_natural_history(condition_id)->get_name(),
		      get_natural_history(condition_id)->get_state_name(state).c_str(),
		      Person::get_var_name(var_id).c_str(),
		      this->var[var_id],
		      value);
	    }
	    this->var[var_id] = value;
	  }
	  else {
	    if (Global::Enable_Records && Global::Enable_Var_Records && other->get_var(var_id)!=value) {
	      char tmp[FRED_STRING_SIZE];
	      get_record_string(tmp);
	      fprintf(Global::Recordsfp,
		      "%s state %s.%s changes other %d age %d var %s from %f to %f\n",
		      tmp,
		      get_natural_history(condition_id)->get_name(),
		      get_natural_history(condition_id)->get_state_name(state).c_str(),
		      other->get_id(), other->get_age(),
		      Person::get_var_name(var_id).c_str(),
		      other->get_var(var_id),
		      value);
	    }
	    other->set_var(var_id, value);
	  }
	}
      }
      break;

    case Rule_Action::SET_LIST :
      {
	FRED_VERBOSE(1, "run SET_LIST person %d cond %d state %d rule: %s\n",
		     this->id, condition_id, state, rule->get_name().c_str());

	var_id = rule->get_list_var_id();
	bool global = rule->is_global();
	if (global) {
	  FRED_VERBOSE(1, "global_list_var %d %s\n", var_id, Person::get_global_list_var_name(var_id).c_str());
	  double_vector_t list_value = rule->get_expression()->get_list_value(this);
	  FRED_VERBOSE(1, "AFTER SET_LIST list_var %d size %d => size %d\n",
		       var_id, Person::global_list_var[var_id].size(), list_value.size());
	  Person::global_list_var[var_id] = list_value;
	  if (Global::Verbose > 1) {
	    for (int i = 0; i < Person::global_list_var[var_id].size(); i++) {
	      printf("SET_LIST person %d day %d hour %d var %s[%d] %f\n",
		     this->id, Global::Simulation_Day, Global::Simulation_Hour,
		     Person::get_global_list_var_name(var_id).c_str(), i, Person::global_list_var[var_id][i]);
	    }
	  }
	}
	else {
	  // FRED_VERBOSE(0, "list_var %d %s\n", var_id, rule->get_list_var_name(var_id).c_str());
	  double_vector_t list_value = rule->get_expression()->get_list_value(this);
	  FRED_VERBOSE(1, "AFTER SET_LIST list_var %d size %d => size %d\n",
		       var_id, this->list_var[var_id].size(), list_value.size());
	  this->list_var[var_id] = list_value;
	  if (Global::Verbose > 1) {
	    for (int i = 0; i < this->list_var[var_id].size(); i++) {
	      printf("person %d day %d hour %d var %s[%d] %f\n",
		     this->id, Global::Simulation_Day, Global::Simulation_Hour,
		     Person::get_list_var_name(var_id).c_str(), i, this->list_var[var_id][i]);
	    }
	  }
	}
      }
      break;

    case Rule_Action::SET_STATE :
      {
	int source_cond_id = rule->get_source_cond_id();
	int source_state_id = rule->get_source_state_id();
	int dest_state_id = rule->get_dest_state_id();
	if (get_state(source_cond_id)==source_state_id) {
	  int day = Global::Simulation_Day;
	  int hour = Global::Simulation_Hour;
	  if (1 && Global::Enable_Records) {
	    fprintf(Global::Recordsfp,
		    "HEALTH RECORD: %s %s day %d person %d ENTERING state %s.%s MODIFIES state %s.%s to %s.%s\n",
		    Date::get_date_string().c_str(),
		    Date::get_12hr_clock(hour).c_str(),
		    day,
		    get_id(),
		    rule->get_cond().c_str(),
		    rule->get_state().c_str(),
		    rule->get_source_cond().c_str(),
		    rule->get_source_state().c_str(),
		    rule->get_source_cond().c_str(),
		    rule->get_dest_state().c_str());
	    fflush(Global::Recordsfp);
	  }
	  Condition::get_condition(source_cond_id)->get_epidemic()->update_state(this, day, hour, dest_state_id, 0);
	}
      }
      break;

    case Rule_Action::SET_WEIGHT :
      {
	if (is_member_of_network(network)) {
	  double_vector_t id_vec;
	  if (expr->is_list_expression()) {
	    id_vec = expr->get_list_value(this);
	  }
	  else {
	    id_vec.push_back(expr->get_value(this));
	  }
	  int size = id_vec.size();
	  for (int i = 0; i < size; i++) {
	    int person_id = id_vec[i];
	    // FRED_VERBOSE(0, "SET_WEIGHT from person %d to  person %d\n", this->id, person_id);
	    Person* other = Person::get_person_with_id(person_id);
	    if (other!=NULL) {
	      double value = expr2->get_value(this,other);
	      // FRED_VERBOSE(0, "SET_WEIGHT from person %d to  person %d age %d value %f\n", this->id, other->get_id(), other->get_age(), value);
	      set_weight_to(other,network,value);
	      other->set_weight_from(this, network,value);
	      if (network->is_undirected()) {
		set_weight_from(other,network,value);
		other->set_weight_to(this, network,value);
	      }
	    }
	  }
	}
      }
      break;

    case Rule_Action::REPORT :
      start_reporting(rule);
      break;

    case Rule_Action::RANDOMIZE_NETWORK :
      if (this->is_meta_agent()) {
	// FRED_VERBOSE(0, "RANDOMIZE NETWORK %s\n", network_name.c_str());
	Group* group = get_admin_group();
	if (group && group->get_type_id()==network_type_id) {
	  double mean_degree = rule->get_expression()->get_value(this,NULL);
	  double max_degree = rule->get_expression2()->get_value(this,NULL);
	  // FRED_VERBOSE(0, "RANDOMIZE NETWORK %s mean %f max %f\n", network_name.c_str(), mean_degree, max_degree);
	  network->randomize(mean_degree, max_degree);
	}
      }
      break;

    case Rule_Action::ABSENT :
    case Rule_Action::PRESENT :
    case Rule_Action::CLOSE :
      // all schedule-related action rules are handled by Natural_History for efficiency
      break;

    case Rule_Action::IMPORT_COUNT :
    case Rule_Action::IMPORT_PER_CAPITA :
    case Rule_Action::IMPORT_LOCATION :
    case Rule_Action::IMPORT_CENSUS_TRACT :
    case Rule_Action::IMPORT_AGES :
    case Rule_Action::COUNT_ALL_IMPORT_ATTEMPTS :
    case Rule_Action::IMPORT_LIST :
      // all import-related action rules are handled by Natural_History for efficiency
      break;

    default:
      FRED_VERBOSE(0,"unknown action %d %s\n", action, rule->get_action().c_str());
      // exit(0);
      break;

    }
  }
}

void Person::get_record_string(char* result) {
  if (Person::record_location) {
    sprintf(result,"HEALTH RECORD: %s %s day %d person %d age %d sex %c race %d latitude %f longitude %f income %d",
	    Date::get_date_string().c_str(),
	    Date::get_12hr_clock(Global::Simulation_Hour).c_str(),
	    Global::Simulation_Day,
	    get_id(),
	    get_age(),
	    get_sex(),
	    get_race(),
	    get_household()? get_household()->get_latitude() : 0.0,
	    get_household()? get_household()->get_longitude() : 0.0,
	    get_household()? get_income() : 0 );
  }
  else {
    sprintf(result,"HEALTH RECORD: %s %s day %d person %d age %d sex %c race %d household %s school %s income %d",
	    Date::get_date_string().c_str(),
	    Date::get_12hr_clock(Global::Simulation_Hour).c_str(),
	    Global::Simulation_Day,
	    get_id(),
	    get_age(),
	    get_sex(),
	    get_race(),
	    get_household() ? get_household()->get_label() : "NONE",
	    get_school() ? get_school()->get_label() : "NONE",
	    get_household()? get_income() : 0 );
  }
}


