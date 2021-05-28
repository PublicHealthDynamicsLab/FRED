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

#include "Factor.h"

#include "Condition.h"
#include "Date.h"
#include "Global.h"
#include "Person.h"
#include "Place.h"
#include "Network.h"
#include "Group.h"
#include "Group_Type.h"
#include "Network_Type.h"
#include "Place_Type.h"
#include "Random.h"
#include "Utils.h"

Factor::Factor(string s) {
  this->name = s;
  this->number_of_args = 0;
  this->number = 0.0;
  this->f0 = NULL;
  this->f1 = NULL;
  this->f2 = NULL;
  this->f3 = NULL;
  this->f4 = NULL;
  this->f5 = NULL;
  this->f6 = NULL;
  this->f7 = NULL;
  this->f8 = NULL;
  this->F2 = NULL;
  this->F3 = NULL;
  this->arg2 = 0;
  this->arg3 = 0;
  this->arg4 = 0;
  this->arg5 = 0;
  this->arg6 = 0;
  this->arg7 = 0;
  this->arg8 = 0;
  this->is_constant = false;
  this->warning = false;
}


string Factor::get_name() {
  return this->name;
}


/////////////////////////////////////////////////////////
//
// 

/// Factors drawn from a statistical distribution

double Factor::get_random() {
  return Random::draw_random(-1.0,1.0);
}

double Factor::get_normal() {
  return Random::draw_normal(0.0,1.0);
}

double Factor::get_exponential() {
  return Random::draw_exponential(1.0);
}

/// Factors based on time and dates

double Factor::get_sim_day() {
  return Global::Simulation_Day;
}

double Factor::get_sim_week() {
  return (int) (Global::Simulation_Day / 7);
}

double Factor::get_sim_month() {
  return (int) (Global::Simulation_Day / 30);
}

double Factor::get_sim_year() {
  return (int) (Global::Simulation_Day / 365);
}

double Factor::get_day_of_week() {
  return Date::get_day_of_week();
}

double Factor::get_day_of_month() {
  return Date::get_day_of_month();
}

double Factor::get_day_of_year() {
  return Date::get_day_of_year();
}

double Factor::get_year() {
  return Date::get_year();
}

double Factor::get_month() {
  return Date::get_month();
}

double Factor::get_date() {
  // note: transform Month-Day into integer to allow comparisons
  int month = Date::get_month();
  int day = Date::get_day_of_month();
  return 100*month + day;
}

double Factor::get_hour() {
  return Global::Simulation_Hour;
}

double Factor::get_epi_week() {
  return Date::get_epi_week();
}

double Factor::get_epi_year() {
  return Date::get_epi_year();
}

/// Factors based on the agent's demographics

double Factor::get_id(Person* person) {
  return person->get_id();
}

double Factor::get_birth_year(Person* person) {
  return person->get_birth_year();
}

double Factor::get_age_in_days(Person* person) {
  return person->get_age_in_days();
}

double Factor::get_age_in_weeks(Person* person) {
  return person->get_age_in_weeks();
}

double Factor::get_age_in_months(Person* person) {
  return person->get_age_in_months();
}

double Factor::get_age_in_years(Person* person) {
  return person->get_age_in_years();
}

double Factor::get_age(Person* person) {
  return person->get_age();
}

double Factor::get_sex(Person* person) {
  return (person->get_sex()=='M');
}

double Factor::get_race(Person* person) {
  return person->get_race();
}

double Factor::get_profile(Person* person) {
  return person->get_profile();
}

double Factor::get_household_relationship(Person* person) {
  return person->get_household_relationship();
}

double Factor::get_number_of_children(Person* person) {
  return person->get_number_of_children(); 
}

/// Factors based on agent's current state

double Factor::get_current_state(Person* person, int condition_id) {
  return person->get_state(condition_id);
}

double Factor::get_time_since_entering_state(Person* person, int condition_id, int state) {
  int result;
  int entered = person->get_time_entered(condition_id, state);
  if (entered < 0) {
    result = entered;
  }
  else {
    result = 24*Global::Simulation_Day + Global::Simulation_Hour - entered;
  }
  /*
  printf("TIME SINCE person %d ENTERING %s.%s = %d\n",
	 person->get_id(),
	 Condition::get_condition(condition_id)->get_name(),
	 Condition::get_condition(condition_id)->get_state_name(state).c_str(),
	 result);
  */
  return result;
}

double Factor::get_susceptibility(Person* person, int condition_id) {
  return person->get_susceptibility(condition_id);
}

double Factor::get_transmissibility(Person* person, int condition_id) {
  if (0 <= person->get_id()) {
    return person->get_transmissibility(condition_id);
  }
  else {
    return Condition::get_condition(condition_id)->get_transmissibility();
  }
}

double Factor::get_transmissions(Person* person, int condition_id) {
  return person->get_transmissions(condition_id);
}

double Factor::get_var(Person* person, int var_index, int is_global) {
  if (is_global) {
    return Person::get_global_var(var_index);
  }
  else {
    return person->get_var(var_index);
  }
}

double Factor::get_list_size(Person* person, int list_var_index, int is_global) {
  double value = 0.0;
  if (is_global) {
    value = Person::get_global_list_size(list_var_index);
    // FRED_VERBOSE(0, "FACTOR get_list_size person %d global_list_var_id %d value %f\n", person->get_id(), list_var_index, value);
  }
  else {
    value = person->get_list_size(list_var_index);
    // FRED_VERBOSE(0, "FACTOR get_list_size person %d list_var_id %d value %f\n", person->get_id(), list_var_index, value);
  }
  return value;
}

double Factor::get_id_of_transmission_source(Person* person, int condition_id) {
  Person* source = person->get_source(condition_id);
  if (source == NULL) {
    // FRED_VERBOSE(0, "get_id_of_transmission_source person %d id = -999999\n", person->get_id());
    return -999999;
  }
  else {
    // FRED_VERBOSE(0, "get_id_of_transmission_source person %d source id = %d\n", person->get_id(), source->get_id());
    return source->get_id();
  }
}

/// Factors based on other agents

double Factor::get_state_count(Person* person, int verb, int is_count,
		       int group_type_id, int condition_id, int state, int except_me) {

  FRED_VERBOSE(1, "GET_CURRENT_COUNT person %d cond %s state %s verb %d is_count %d group_type %d %s\n",
	       person->get_id(), Condition::get_condition(condition_id)->get_name(),
	       Condition::get_condition(condition_id)->get_state_name(state).c_str(),
	       verb, is_count, group_type_id, Group_Type::get_group_type_name(group_type_id).c_str());
  double value = 0.0;
  if (group_type_id < 0) {
    int count = 0;
    if (verb == 1) {
      count = Condition::get_condition(condition_id)->get_incidence_count(state);
      FRED_VERBOSE(1, "GET_CURRENT_COUNT cond %s state %s count = %d\n",
		   Condition::get_condition(condition_id)->get_name(),
		   Condition::get_condition(condition_id)->get_state_name(state).c_str(),
		   count);
    }
    if (verb == 2) {
      count = Condition::get_condition(condition_id)->get_current_count(state);
    }
    if (verb == 3) {
      count = Condition::get_condition(condition_id)->get_total_count(state);
    }
    if (except_me && (person->get_state(condition_id) == state)) {
      count--;
    }
    if (is_count) {
      value = count;
    }
    else {
      value = (double) count * 100.0 / (double) Person::get_population_size();
    }
    FRED_VERBOSE(1, "GET_CURRENT_COUNT cond %s state %s value = %f\n",
		 Condition::get_condition(condition_id)->get_name(),
		 Condition::get_condition(condition_id)->get_state_name(state).c_str(),
		 value);
  }
  else {
    if (Group::is_a_place(group_type_id)) {
      Place* place = NULL;
      place = person->get_place_of_type(group_type_id);
      if (place == NULL) {
	FRED_VERBOSE(1, "get_current_count cond %s state %s place %s = %d\n",
		     Condition::get_condition(condition_id)->get_name(),
		     Condition::get_condition(condition_id)->get_state_name(state).c_str(),
		     "NULL", 0);
	return 0;
      }
      FRED_VERBOSE(1, "get_current_count cond %s state %s place %s\n",
		   Condition::get_condition(condition_id)->get_name(),
		     Condition::get_condition(condition_id)->get_state_name(state).c_str(),
		   place->get_label());
      int count = 0;
      if (verb == 1) {
	count = Condition::get_condition(condition_id)->get_incidence_group_state_count(place, state);
      }
      if (verb == 2) {
	count = Condition::get_condition(condition_id)->get_current_group_state_count(place,state);
	FRED_VERBOSE(1, "get_current_count cond %s state %s place %s = %d\n",
		     Condition::get_condition(condition_id)->get_name(),
		     Condition::get_condition(condition_id)->get_state_name(state).c_str(),
		     place->get_label(), count);
      }
      if (verb == 3) {
	count = Condition::get_condition(condition_id)->get_total_group_state_count(place, state);
	FRED_VERBOSE(1, "get_total_count cond %s state %s place %s = %d\n",
		     Condition::get_condition(condition_id)->get_name(),
		     Condition::get_condition(condition_id)->get_state_name(state).c_str(),
		     place->get_label(), count);
      }
      if (except_me && (person->get_state(condition_id) == state)) {
	count--;
      }
      if (is_count) {
	value = count;
      }
      else {
	if (place->get_size() > 0) {
	  value = (double) count * 100.0 / (double) place->get_size();
	  FRED_VERBOSE(1, "get_current_percent cond %d state %d place %s size %d = %f\n",
		       condition_id, state, place->get_label(), place->get_size(), value);
	}
	else {
	  value = 0.0;
	}
      }
    }
    else {
      Network* network = NULL;
      network = person->get_network_of_type(group_type_id);
      if (network == NULL) {
	return 0;
      }
      int count = 0;
      if (verb == 1) {
	count = Condition::get_condition(condition_id)->get_incidence_group_state_count(network, state);
      }
      if (verb == 2) {
	count = Condition::get_condition(condition_id)->get_current_group_state_count(network,state);
	FRED_VERBOSE(1, "get_current_count cond %d state %d network %s = %d\n",
		     condition_id, state, network->get_label(), count);
      }
      if (verb == 3) {
	count = Condition::get_condition(condition_id)->get_total_group_state_count(network, state);
      }
      if (except_me && (person->get_state(condition_id) == state)) {
	count--;
      }
      if (is_count) {
	value = count;
      }
      else {
	if (network->get_size() > 0) {
	  value = (double) count * 100.0 / (double) network->get_size();
	  FRED_VERBOSE(1, "get_current_percent cond %d state %d network %s size %d = %f\n",
		       condition_id, state, network->get_label(), network->get_size(), value);
	}
	else {
	  value = 0.0;
	}
      }
    }
  }
  FRED_VERBOSE(1, "GET_STATE_COUNT day %d person %d verb %d group_type %d cond_id %d state %d except_me %d value %f\n",
	       Global::Simulation_Day, person->get_id(), verb, group_type_id, condition_id, state, except_me, value);

  return value;
}

double Factor::get_sum_of_vars_in_group(Person* person, int var_id, int group_type_id) {
  if (group_type_id < Place_Type::get_number_of_place_types()) {
    Place* place = NULL;
    place = person->get_place_of_type(group_type_id);
    if (place == NULL) {
      return 0;
    }
    double value = place->get_sum_of_var(var_id);
    return value;
  }
  else {
    Network* network = NULL;
    network = person->get_network_of_type(group_type_id);
    if (network == NULL) {
      return 0;
    }
    double value = network->get_sum_of_var(var_id);
    return value;
  }
}

double Factor::get_ave_of_vars_in_group(Person* person, int var_id, int group_type_id) {
  if (group_type_id < Place_Type::get_number_of_place_types()) {
    Place* place = NULL;
    place = person->get_place_of_type(group_type_id);
    if (place == NULL) {
      return 0;
    }
    double value = place->get_sum_of_var(var_id);
    int size = place->get_size();
    if (size > 0) {
      value = value / size;
    }
    return value;
  }
  else {
    Network* network = NULL;
    network = person->get_network_of_type(group_type_id);
    if (network == NULL) {
      return 0;
    }
    double value = network->get_sum_of_var(var_id);
    int size = network->get_size();
    if (size > 0) {
      value = value / size;
    }
    return value;
  }
}

double Factor::get_block_group_admin_code(Person* person, int place_type_id) {
  Place* place = NULL;
  place = person->get_place_of_type(place_type_id);
  if (place == NULL) {
    return 0;
  }
  return place->get_block_group_admin_code();
}

double Factor::get_census_tract_admin_code(Person* person, int place_type_id) {
  Place* place = NULL;
  place = person->get_place_of_type(place_type_id);
  if (place == NULL) {
    return 0;
  }
  return place->get_census_tract_admin_code();
}

double Factor::get_county_admin_code(Person* person, int place_type_id) {
  Place* place = NULL;
  place = person->get_place_of_type(place_type_id);
  if (place == NULL) {
    return 0;
  }
  return place->get_county_admin_code();
}

double Factor::get_state_admin_code(Person* person, int place_type_id) {
  Place* place = NULL;
  place = person->get_place_of_type(place_type_id);
  if (place == NULL) {
    return 0;
  }
  return place->get_state_admin_code();
}

/// Factors based on groups

double Factor::get_group_id(Person* person, int group_type_id) {
  // FRED_VERBOSE(0, "get_group_id person %d type_id %d\n", person->get_id(), group_type_id);
  if (group_type_id < 0) {
    return -1;
  }
  Group* group = NULL;
  group = person->get_group_of_type(group_type_id);
  if (group == NULL) {
    // FRED_VERBOSE(0, "get_group_id person %d type_id %d result -1\n", person->get_id(), group_type_id);
    return -1;
  }
  else {
    // FRED_VERBOSE(0, "get_group_id person %d type_id %d result %d\n", person->get_id(), group_type_id, group->get_sp_id());
    return group->get_sp_id();
  }
}

double Factor::get_admin_id(Person* person, int group_type_id) {
  if (Group::is_a_place(group_type_id)) {
    Place* place = NULL;
    place = person->get_place_of_type(group_type_id);
    if (place == NULL) {
      // FRED_VERBOSE(0, "GET_ADMIN_ID person %d group %s is NULL\n", person->get_id(), Group_Type::get_group_type(group_type_id)->get_name());
      return -1;
    }
    else {
      Person* admin = place->get_administrator();
      if (admin) {
	// FRED_VERBOSE(0, "GET_ADMIN_ID person %d group %s admin %d\n", person->get_id(), Group_Type::get_group_type(group_type_id)->get_name(), admin->get_id());
	return admin->get_id();
      }
      else {
	// FRED_VERBOSE(0, "GET_ADMIN_ID person %d group %s admin NULL\n", person->get_id(), Group_Type::get_group_type(group_type_id)->get_name());
	return -1;
      }
    }
  }
  if (Group::is_a_network(group_type_id)) {
    Network* network = NULL;
    network = person->get_network_of_type(group_type_id);
    if (network == NULL) {
      return -1;
    }
    else {
      Person* admin = network->get_administrator();
      if (admin) {
	return admin->get_id();
      }
      else {
	return -1;
      }
    }
  }
  return -1;
}


double Factor::get_group_level(Person* person, int selection, int place_type_id) {

  FRED_VERBOSE(1, "GET_PLACE_LEVEL day %d person %d place_type %d\n",
	       Global::Simulation_Day, person->get_id(), place_type_id);

  if (selection==1) {
    Group* group = person->get_group_of_type(place_type_id);
    if (group) {
      return group->get_size();
    }
    else {
      return 0;
    }
  }

  if (selection==2) {
    Group* group = person->get_group_of_type(place_type_id);
    if (group) {
      return group->get_income();
    }
    else {
      return 0;
    }
  }

  Place* place = NULL;
  place = person->get_place_of_type(place_type_id);
  if (place == NULL) {
    FRED_VERBOSE(1, "GET_PLACE_LEVEL day %d person %d place_type %d NULL PLACE RETURN 0\n",
		 Global::Simulation_Day, person->get_id(), place_type_id);
    return 0;
  }

  double value;
  switch (selection) {
    /*
      case 1:
      value = place->get_size();
      break;
      case 2:
      value = place->get_income();
      break;
    */
  case 3:
    value = place->get_elevation();
    break;
  case 4:
    value = Place_Type::get_place_type(place_type_id)->get_size_quartile(place->get_size());
    break;
  case 5:
    value = Place_Type::get_place_type(place_type_id)->get_income_quartile(place->get_income());
    break;
  case 6:
    value = Place_Type::get_place_type(place_type_id)->get_elevation_quartile(place->get_elevation());
    break;
  case 7:
    value = Place_Type::get_place_type(place_type_id)->get_size_quintile(place->get_size());
    break;
  case 8:
    value = Place_Type::get_place_type(place_type_id)->get_income_quintile(place->get_income());
    break;
  case 9:
    value = Place_Type::get_place_type(place_type_id)->get_elevation_quintile(place->get_elevation());
    break;
  case 10:
    value = place->get_latitude();
    break;
  case 11:
    value = place->get_longitude();
    break;
  }

  FRED_VERBOSE(1, "GET_PLACE_LEVEL day %d person %d place_type %d VALUE %f\n",
	       Global::Simulation_Day, person->get_id(), place_type_id, value);
  return value;
}

double Factor::get_adi_state_rank(Person* person, int place_type_id) { 
  Place* place = person->get_place_of_type(place_type_id);
  int rank = 0;
  if (place != NULL) {
    rank = place->get_adi_state_rank();
  }
  return rank;
}

double Factor::get_adi_national_rank(Person* person, int place_type_id) {
  Place* place = person->get_place_of_type(place_type_id);
  int rank = 0;
  if (place != NULL) {
    rank = place->get_adi_national_rank();
  }
  return rank;
}

/// Factors based on network

double Factor::get_network_in_degree(Person* person, int network_type_id) {
  Network_Type* network_type = Network_Type::get_network_type(network_type_id);
  if (network_type==NULL) {
    return 0;
  }
  Network* network = network_type->get_network();
  int degree = person->get_in_degree(network);
  /*
    printf("GET_IN_DEGREE net_type %d person %d in_deg %d\n",
    network_type_id, person->get_id(), d);
  */
  return degree;
}

double Factor::get_network_out_degree(Person* person, int network_type_id) {
  Network_Type* network_type = Network_Type::get_network_type(network_type_id);
  if (network_type==NULL) {
    return 0;
  }
  Network* network = network_type->get_network();
  int degree = person->get_out_degree(network);
  return degree;
}

double Factor::get_network_degree(Person* person, int network_type_id) {
  if (Network_Type::get_network_type(network_type_id)->is_undirected()) {
    return Factor::get_network_in_degree(person, network_type_id);
  }
  else {
    return Factor::get_network_in_degree(person, network_type_id)
      + Factor::get_network_out_degree(person, network_type_id);
  }
}

double Factor::is_connected(Person* person1, Person* person2, int network_type_id) {
  double result = 0.0;
  Network* network = person1->get_network_of_type(network_type_id);
  if (network!=NULL) {
    result = person1->is_connected_to(person2, network);
  }
  return result;
}

double Factor::get_network_weight(Person* person1, Person* person2, int network_type_id) {
  double result = 0.0;
  Network* network = person1->get_network_of_type(network_type_id);
  if (network!=NULL) {
    result = person1->get_weight_to(person2, network);
  }
  return result;
}

double Factor::get_network_timestamp(Person* person1, Person* person2, int network_type_id) {
  double result = -1.0;
  Network* network = person1->get_network_of_type(network_type_id);
  if (network!=NULL) {
    result = person1->get_timestamp_to(person2, network);
  }
  return result;
}

double Factor::get_id_of_max_weight_inward_edge_in_network(Person* person, int network_type_id) {
  double result = -999999;
  Network* network = person->get_network_of_type(network_type_id);
  if (network!=NULL) {
    result = person->get_id_of_max_weight_inward_edge_in_network(network);
  }
  return result;
}

double Factor::get_id_of_max_weight_outward_edge_in_network(Person* person, int network_type_id) {
  double result = -999999;
  Network* network = person->get_network_of_type(network_type_id);
  if (network!=NULL) {
    result = person->get_id_of_max_weight_outward_edge_in_network(network);
  }
  return result;
}


double Factor::get_id_of_min_weight_inward_edge_in_network(Person* person, int network_type_id) {
  double result = -999999;
  Network* network = person->get_network_of_type(network_type_id);
  if (network!=NULL) {
    result = person->get_id_of_min_weight_inward_edge_in_network(network);
  }
  return result;
}

double Factor::get_id_of_min_weight_outward_edge_in_network(Person* person, int network_type_id) {
  double result = -999999;
  Network* network = person->get_network_of_type(network_type_id);
  if (network!=NULL) {
    result = person->get_id_of_min_weight_outward_edge_in_network(network);
  }
  return result;
}


double Factor::get_id_of_last_inward_edge_in_network(Person* person, int network_type_id) {
  double result = -999999;
  Network* network = person->get_network_of_type(network_type_id);
  if (network!=NULL) {
    result = person->get_id_of_last_inward_edge_in_network(network);
  }
  return result;
}

double Factor::get_id_of_last_outward_edge_in_network(Person* person, int network_type_id) {
  double result = -999999;
  Network* network = person->get_network_of_type(network_type_id);
  if (network!=NULL) {
    result = person->get_id_of_last_outward_edge_in_network(network);
  }
  return result;
}


/////////////////////////////////////////////////////////


double Factor::get_value(Person* person) {
  double value = 0.0;
  if (this->is_constant) {
    return this->number;
  }
  switch (this->number_of_args) {
  case 0:
    value = f0();
    break;
  case 1:
    value = f1(person);
    break;
  case 2:
    value = f2(person,arg2);
    break;
  case 3:
    value = f3(person,arg2,arg3);
    break;
  case 4:
    value = f4(person,arg2,arg3,arg4);
    break;
  case 5:
    value = f5(person,arg2,arg3,arg4,arg5);
    break;
  case 6:
    value = f6(person,arg2,arg3,arg4,arg5,arg6);
    break;
  case 7:
    value = f7(person,arg2,arg3,arg4,arg5,arg6,arg7);
    break;
  case 8:
    value = f8(person,arg2,arg3,arg4,arg5,arg6,arg7,arg8);
    break;
  }
  // FRED_VERBOSE(0,"FACTOR %s get_value = %f\n", this->name.c_str(), value);
  return value;
}
  
double Factor::get_value(Person* person1, Person* person2) {
  double value = 0.0;
  if (this->is_constant) {
    return this->number;
  }
  switch (this->number_of_args) {
  case 2:
    value = F2(person1, person2);
    break;
  case 3:
    value = F3(person1, person2, arg3);
    break;
  default:
    value = get_value(person1);
  }
  // FRED_VERBOSE(0,"FACTOR %s get_value = %f\n", this->name.c_str(), value);
  return value;
}


bool Factor::parse() {

  // printf("FACTOR: parsing factor |%s|\n", this->name.c_str()); fflush(stdout);

  if (this->name == "") {
    this->number_of_args = 0;
    return true;
  }

  if (Utils::is_number(this->name)) {
    this->is_constant = true;
    this->number = strtod(this->name.c_str(), NULL);
    return true;
  }

  int var_id = Person::get_var_id(this->name);
  if (0 <= var_id) {
    this->arg2 = var_id;
    this->arg3 = 0;
    this->number_of_args = 3;
    this->f3 = get_var;
    return true;
  }

  var_id = Person::get_global_var_id(this->name);
  if (0 <= var_id) {
    this->arg2 = var_id;
    this->arg3 = 1;
    this->number_of_args = 3;
    this->f3 = get_var;
    return true;
  }

  int group_type_id = Group_Type::get_type_id(this->name);
  // printf("FACTOR: parsing factor |%s| group_type_id %d\n", this->name.c_str(), group_type_id); fflush(stdout);
  if (0 <= group_type_id) {
    this->f2 = &get_group_id;
    this->arg2 = group_type_id;
    this->number_of_args = 2;
    return true;
  }

  if (this->name.find("list_size_of_") == 0) {
    // get list_var _id
    int pos1 = strlen("list_size_of_");
    string list_var_name = this->name.substr(pos1);
    int list_var_id = Person::get_list_var_id(list_var_name);
    if (0 <= list_var_id) {
      this->f3 = get_list_size;
      this->arg2 = list_var_id;
      this->arg3 = 0;
      this->number_of_args = 3;
      return true;
    }
    list_var_id = Person::get_global_list_var_id(list_var_name);
    if (0 <= list_var_id) {
      this->f3 = get_list_size;
      this->arg2 = list_var_id;
      this->arg3 = 1;
      this->number_of_args = 3;
      return true;
    }
    return false;
  }    

  // random distributions
  if (this->name == "random") {
    f0 = &get_random;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "normal") {
    f0 = &get_normal;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "exponential") {
    f0 = &get_exponential;
    this->number_of_args = 0;
    return true;
  }

  // time and dates
  if (this->name == "sim_day") {
    f0 = &get_sim_day;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "sim_week") {
    f0 = &get_sim_week;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "sim_month") {
    f0 = &get_sim_month;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "sim_year") {
    f0 = &get_sim_year;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "day_of_week") {
    f0 = &get_day_of_week;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "day_of_month") {
    f0 = &get_day_of_month;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "day_of_year") {
    f0 = &get_day_of_year;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "month") {
    f0 = &get_month;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "year") {
    f0 = &get_year;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "date") {
    f0 = &get_date;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "hour") {
    f0 = &get_hour;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "epi_week") {
    f0 = &get_epi_week;
    this->number_of_args = 0;
    return true;
  }

  if (this->name == "epi_year") {
    f0 = &get_epi_year;
    this->number_of_args = 0;
    return true;
  }

  // the agent's demographics
  if (this->name == "id") {
    f1 = &get_id;
    this->number_of_args = 1;
    return true;
  }

  if (this->name == "birth_year") {
    f1 = &get_birth_year;
    this->number_of_args = 1;
    return true;
  }

  if (this->name == "age_in_days") {
    f1 = &get_age_in_days;
    this->number_of_args = 1;
    return true;
  }

  if (this->name == "age_in_weeks") {
    f1 = &get_age_in_weeks;
    this->number_of_args = 1;
    return true;
  }

  if (this->name == "age_in_months") {
    f1 = &get_age_in_months;
    this->number_of_args = 1;
    return true;
  }

  if (this->name == "age_in_years") {
    f1 = &get_age_in_years;
    this->number_of_args = 1;
    return true;
  }

  if (this->name == "age") {
    f1 = &get_age;
    this->number_of_args = 1;
    return true;
  }

  if (this->name == "sex") {
    f1 = &get_sex;
    this->number_of_args = 1;
    return true;
  }

  if (this->name == "race") {
    f1 = &get_race;
    this->number_of_args = 1;
    return true;
  }

  if (this->name == "profile") {
    f1 = &get_profile;
    this->number_of_args = 1;
    return true;
  }

  if (this->name == "household_relationship") {
    f1 = &get_household_relationship;
    this->number_of_args = 1;
    return true;
  }

  if (this->name == "number_of_children") {
    f1 = &get_number_of_children;
    this->number_of_args = 1;
    return true;
  }

  // the agent's current state

  if (this->name.find("current_state_in_") == 0) {
    // get condition _id
    int pos1 = strlen("current_state_in_");
    string cond_name = this->name.substr(pos1);
    int cond_id = Condition::get_condition_id(cond_name);
    if (cond_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED CONDITION = |%s|\n", this->name.c_str());
      this->warning = true;
      return false;
    }
    f2 = &get_current_state;
    this->arg2 = cond_id;
    this->number_of_args = 2;
    return true;
  }

  if (this->name.find("time_since") == 0) {
    // FRED_VERBOSE(0, "PARSING SINCE FACTOR = |%s|\n", this->name.c_str());
    // get condition _id
    int pos1 = strlen("time_since_entering_");
    int pos2 = this->name.find(".",pos1);
    if (pos1==string::npos || pos2==string::npos) {
      FRED_VERBOSE(1, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }
    string cond_name = this->name.substr(pos1,pos2-pos1);
    int cond_id = Condition::get_condition_id(cond_name);
    if (cond_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED CONDITION = |%s|\n", this->name.c_str());
      this->warning = true;
      return false;
    }
    FRED_VERBOSE(1, "PARSING SINCE FACTOR = |%s| cond %s %d\n", this->name.c_str(), cond_name.c_str(), cond_id);
    string state_name = this->name.substr(pos2+1);
    int state_id = Condition::get_condition(cond_id)->get_state_from_name(state_name);
    if (state_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED STATE = |%s|\n", this->name.c_str());
      this->warning = true;
      return false;
    }
    f3 = &get_time_since_entering_state;
    this->arg2 = cond_id;
    this->arg3 = state_id;
    this->number_of_args = 3;
    return true;
  }

  if (this->name.find("susceptibility") == 0) {
    // get condition _id
    string cond_name = this->name.substr(strlen("susceptibility_to_"));
    int cond_id = Condition::get_condition_id(cond_name);
    if (cond_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED CONDITION = |%s|\n", this->name.c_str());
      this->warning = true;
      return false;
    }
    f2 = &get_susceptibility;
    this->arg2 = cond_id;
    this->number_of_args = 2;
    return true;
  }

  if (this->name.find("transmissibility") == 0) {
    // get condition _id
    string cond_name = this->name.substr(strlen("transmissibility_for_"));
    int cond_id = Condition::get_condition_id(cond_name);
    if (cond_id < 0) {
      cond_name = this->name.substr(strlen("transmissibility_of_"));
      cond_id = Condition::get_condition_id(cond_name);
      if (cond_id < 0) {
	FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED CONDITION = |%s|\n", this->name.c_str());
	this->warning = true;
	return false;
      }
    }
    f2 = &get_transmissibility;
    this->arg2 = cond_id;
    this->number_of_args = 2;
    return true;
  }

  if (this->name.find("transmissions_of_") == 0) {
    // get condition _id
    string cond_name = this->name.substr(strlen("transmissions_of_"));
    int cond_id = Condition::get_condition_id(cond_name);
    if (cond_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED CONDITION = |%s|\n", this->name.c_str());
      this->warning = true;
      return false;
    }
    f2 = &get_transmissions;
    this->arg2 = cond_id;
    this->number_of_args = 2;
    return true;
  }

  if (this->name.find("id_of_source_of_") == 0) {
    // get condition _id
    string cond_name = this->name.substr(strlen("id_of_source_of_"));
    int cond_id = Condition::get_condition_id(cond_name);
    if (cond_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED CONDITION = |%s|\n", this->name.c_str());
      this->warning = true;
      return false;
    }
    f2 = &get_id_of_transmission_source;
    this->arg2 = cond_id;
    this->number_of_args = 2;
    return true;
  }

  // factors based on other agents

  if ((this->name.find("incidence_")!=string::npos || this->name.find("current_")!=string::npos || this->name.find("total_")!=string::npos) &&
      (this->name.find("_count_")!=string::npos || this->name.find("_percent_")!=string::npos)) {

    // find verb
    int verb = 0;
    if (this->name.find("incidence_")!=string::npos) {
      verb = 1;
    }
    else if (this->name.find("current_")!=string::npos) {
      verb = 2;
    }
    else if (this->name.find("total_")!=string::npos) {
      verb = 3;
    }
    if (verb == 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }

    // see if we're looking at count or percent
    int is_count = (this->name.find("_count_") != string::npos);

    // get condition name
    int pos = this->name.find("_of_") + 4;
    int next = this->name.find(".", pos);
    string cond_name = this->name.substr(pos, next-pos);
    int cond_id = Condition::get_condition_id(cond_name);
    if (cond_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED CONDITION = |%s|\n", this->name.c_str());
      this->warning = true;
      return false;
    }
    // FRED_VERBOSE(0, "SET COUNT |%s| condition |%s| id %d\n", this->name.c_str(), cond_name.c_str(), cond_id);

    // get state name
    pos = this->name.find("_", next);
    string state_name = this->name.substr(next+1, pos-next-1);
    // FRED_VERBOSE(0, "SET COUNT |%s| state |%s|\n", this->name.c_str(), state_name.c_str());
    int state_id = Condition::get_condition(cond_id)->get_state_from_name(state_name);
    if (state_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED STATE = |%s|\n", this->name.c_str());
      this->warning = true;
      return false;
    }

    // get group_type_name if any
    string group_type_name = "";
    int group_type_id = -1;
    if (this->name.find("_in_") != std::string::npos) {
      pos = name.find("_in_") + 4;
      next = this->name.find("_", pos);
      group_type_name = this->name.substr(pos, next-pos);
      group_type_id = Group_Type::get_type_id(group_type_name);
      if (group_type_id < 0) {
	FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED PLACE OR NETWORK NAME = |%s|\n", this->name.c_str());
	return false;
      }
    }

    // looking at others
    int except_me = (this->name.find("_excluding_me") != string::npos);

    if (0 <= group_type_id && group_type_id < Place_Type::get_number_of_place_types() + Network_Type::get_number_of_network_types()) {
      Condition::get_condition(cond_id)->track_group_state_counts(group_type_id, state_id);
    }

    this->arg2 = verb;
    this->arg3 = is_count;
    this->arg4 = group_type_id;
    this->arg5 = cond_id;
    this->arg6 = state_id;
    this->arg7 = except_me;
    this->number_of_args = 7;
    this->f7 = &get_state_count;
    return true;
  }

  if (this->name.find("sum_of_")==0 || this->name.find("ave_of_")==0) {

    // get verb: 0 = "sum_of", 1 = "ave_of"
    int verb = (this->name.find("ave_of_")==0);

    // get var name
    int pos = this->name.find("_of_") + 4;
    int next = this->name.find("_", pos);
    string var_name = this->name.substr(pos, next-pos);

    FRED_VERBOSE(1,"SET GET_VAR_IN_PLACE var |%s|\n", var_name.c_str());

    int var_id = Person::get_var_id(var_name);
    if (var_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }

    // get group_type_name
    string group_type_name = "";
    if (this->name.find("_in_") != std::string::npos) {
      pos = this->name.find("_in_") + 4;
      next = this->name.find("_", pos);
      group_type_name = this->name.substr(pos, next-pos);
    }
    int group_type_id = Group_Type::get_type_id(group_type_name);
    if (group_type_id < 0) {
      group_type_id = Group_Type::get_type_id(group_type_name);
    }
    if (group_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED PLACE OR NETWORK NAME = |%s|\n", this->name.c_str());
      return false;
    }

    this->arg2 = var_id;
    this->arg3 = group_type_id;
    this->number_of_args = 3;
    if (verb == 0) {
      this->f3 = get_sum_of_vars_in_group;
    }
    else {
      this->f3 = get_ave_of_vars_in_group;
    }
    return true;
  }

  // factors based on the agent's places

  // admin id
  if (this->name.find("admin_of_")==0) {

    // find place type
    int pos = this->name.find("_of_") + 4;
    string name = this->name.substr(pos);
    int group_type_id = Group_Type::get_type_id(name);
    if (group_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }
    
    this->f2 = &get_admin_id;
    this->arg2 = group_type_id;
    this->number_of_args = 2;
    return true;
  }

  // place size, income or elevation
  if (this->name.find("size_")==0 ||
      this->name.find("latitude_")==0 ||
      this->name.find("longitude_")==0 ||
      this->name.find("income_")==0 ||
      this->name.find("elevation_")==0) {
  
    // find verb
    int verb = 0;
    if (this->name.find("size_of_")==0) {
      verb = 1;
    }
    else if (this->name.find("income_of_")==0) {
      verb = 2;
    }
    else if (this->name.find("elevation_of_")==0) {
      verb = 3;
    }
    if (this->name.find("size_quartile_of_")==0) {
      verb = 4;
    }
    else if (this->name.find("income_quartile_of_")==0) {
      verb = 5;
    }
    else if (this->name.find("elevation_quartile_of_")==0) {
      verb = 6;
    }
    if (this->name.find("size_quintile_of_")==0) {
      verb = 7;
    }
    else if (this->name.find("income_quintile_of_")==0) {
      verb = 8;
    }
    else if (this->name.find("elevation_quintile_of_")==0) {
      verb = 9;
    }
    else if (this->name.find("latitude_of_")==0) {
      verb = 10;
    }
    else if (this->name.find("longitude_of_")==0) {
      verb = 11;
    }

    // find place type
    int pos = this->name.find("_of_") + 4;
    string name = this->name.substr(pos);
    int place_type_id = Group_Type::get_type_id(name);
    if (place_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }
    
    this->f3 = &get_group_level;
    this->arg2 = verb;
    this->arg3 = place_type_id;
    this->number_of_args = 3;
    return true;
  }

  // adi rank
  if (this->name.find("adi_state_rank_")==0) {
    int pos = this->name.find("_of_") + 4;
    string name = this->name.substr(pos);
    int place_type_id = Group_Type::get_type_id(name);
    if (place_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }
    this->number_of_args = 2;
    this->arg2 = place_type_id;
    this->f2 = &get_adi_state_rank;
    return true;
  }

  if (this->name.find("adi_national_rank_")==0) {
    int pos = this->name.find("_of_") + 4;
    string name = this->name.substr(pos);
    int place_type_id = Group_Type::get_type_id(name);
    if (place_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }
    this->number_of_args = 2;
    this->arg2 = place_type_id;
    this->f2 = &get_adi_national_rank;
    return true;
  }

  // admin_code of block_group
  if (this->name.find("block_group")==0) {
    int pos = this->name.find("_of_") + 4;
    string name = this->name.substr(pos);
    int place_type_id = Group_Type::get_type_id(name);
    if (place_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }
    this->number_of_args = 2;
    this->arg2 = place_type_id;
    this->f2 = &get_block_group_admin_code;
    return true;
  }

  // admin_code of census_tract
  if (this->name.find("census_tract")==0) {
    int pos = this->name.find("_of_") + 4;
    string name = this->name.substr(pos);
    int place_type_id = Group_Type::get_type_id(name);
    if (place_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }
    this->number_of_args = 2;
    this->arg2 = place_type_id;
    this->f2 = &get_census_tract_admin_code;
    return true;
  }

  // admin_code of county
  if (this->name.find("county")==0) {
    int pos = this->name.find("_of_") + 4;
    string name = this->name.substr(pos);
    int place_type_id = Group_Type::get_type_id(name);
    if (place_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }
    this->number_of_args = 2;
    this->arg2 = place_type_id;
    this->f2 = &get_county_admin_code;
    return true;
  }

  // admin_code of state
  if (this->name.find("state")==0) {
    int pos = this->name.find("_of_") + 4;
    string name = this->name.substr(pos);
    int place_type_id = Group_Type::get_type_id(name);
    if (place_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }
    this->number_of_args = 2;
    this->arg2 = place_type_id;
    this->f2 = &get_state_admin_code;
    return true;
  }

  // network in_degree
  if (this->name.find("in_degree")==0) {
    int pos = this->name.find("_of_") + 4;
    string name = this->name.substr(pos);
    int network_type_id = Group_Type::get_type_id(name);
    if (network_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }
    this->number_of_args = 2;
    this->arg2 = network_type_id;
    this->f2 = &get_network_in_degree;
    return true;
  }

  // network out_degree
  if (this->name.find("out_degree")==0) {
    int pos = this->name.find("_of_") + 4;
    string net_name = this->name.substr(pos);
    int network_type_id = Group_Type::get_type_id(net_name);
    if (network_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }
    this->number_of_args = 2;
    this->arg2 = network_type_id;
    this->f2 = &get_network_out_degree;
    return true;
  }

  // network degree
  if (this->name.find("degree_of_")==0) {
    int pos = this->name.find("_of_") + 4;
    string net_name = this->name.substr(pos);
    int network_type_id = Group_Type::get_type_id(net_name);
    if (network_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| net_name %s\n", this->name.c_str(), net_name.c_str());
      return false;
    }
    this->number_of_args = 2;
    this->arg2 = network_type_id;
    this->f2 = &get_network_degree;
    return true;
  }

  if (this->name.find("id_of_max_weight_inward_edge_in_")==0) {
    int pos = this->name.find("_in_") + 4;
    string net_name = this->name.substr(pos);
    int network_type_id = Group_Type::get_type_id(net_name);
    if (network_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| net_name %s\n", this->name.c_str(), net_name.c_str());
      return false;
    }
    if (Group::is_a_network(network_type_id)) {
      this->number_of_args = 2;
      this->arg2 = network_type_id;
      this->f2 = &get_id_of_max_weight_inward_edge_in_network;
      return true;
    }
    else {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| group %s is not a network\n", this->name.c_str(), net_name.c_str());
      return false;
    }
  }

  if (this->name.find("id_of_max_weight_outward_edge_in_")==0) {
    int pos = this->name.find("_in_") + 4;
    string net_name = this->name.substr(pos);
    int network_type_id = Group_Type::get_type_id(net_name);
    if (network_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| net_name %s\n", this->name.c_str(), net_name.c_str());
      return false;
    }
    if (Group::is_a_network(network_type_id)) {
      this->number_of_args = 2;
      this->arg2 = network_type_id;
      this->f2 = &get_id_of_max_weight_outward_edge_in_network;
      return true;
    }
    else {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| group %s is not a network\n", this->name.c_str(), net_name.c_str());
      return false;
    }
  }

  if (this->name.find("id_of_min_weight_inward_edge_in_")==0) {
    int pos = this->name.find("_in_") + 4;
    string net_name = this->name.substr(pos);
    int network_type_id = Group_Type::get_type_id(net_name);
    if (network_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| net_name %s\n", this->name.c_str(), net_name.c_str());
      return false;
    }
    if (Group::is_a_network(network_type_id)) {
      this->number_of_args = 2;
      this->arg2 = network_type_id;
      this->f2 = &get_id_of_min_weight_inward_edge_in_network;
      return true;
    }
    else {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| group %s is not a network\n", this->name.c_str(), net_name.c_str());
      return false;
    }
  }

  if (this->name.find("id_of_min_weight_outward_edge_in_")==0) {
    int pos = this->name.find("_in_") + 4;
    string net_name = this->name.substr(pos);
    int network_type_id = Group_Type::get_type_id(net_name);
    if (network_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| net_name %s\n", this->name.c_str(), net_name.c_str());
      return false;
    }
    if (Group::is_a_network(network_type_id)) {
      this->number_of_args = 2;
      this->arg2 = network_type_id;
      this->f2 = &get_id_of_min_weight_outward_edge_in_network;
      return true;
    }
    else {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| group %s is not a network\n", this->name.c_str(), net_name.c_str());
      return false;
    }
  }

  if (this->name.find("id_of_last_inward_edge_in_")==0) {
    int pos = this->name.find("_in_") + 4;
    string net_name = this->name.substr(pos);
    int network_type_id = Group_Type::get_type_id(net_name);
    if (network_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| net_name %s\n", this->name.c_str(), net_name.c_str());
      return false;
    }
    if (Group::is_a_network(network_type_id)) {
      this->number_of_args = 2;
      this->arg2 = network_type_id;
      this->f2 = &get_id_of_last_inward_edge_in_network;
      return true;
    }
    else {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| group %s is not a network\n", this->name.c_str(), net_name.c_str());
      return false;
    }
  }

  if (this->name.find("id_of_last_outward_edge_in_")==0) {
    int pos = this->name.find("_in_") + 4;
    string net_name = this->name.substr(pos);
    int network_type_id = Group_Type::get_type_id(net_name);
    if (network_type_id < 0) {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| net_name %s\n", this->name.c_str(), net_name.c_str());
      return false;
    }
    if (Group::is_a_network(network_type_id)) {
      this->number_of_args = 2;
      this->arg2 = network_type_id;
      this->f2 = &get_id_of_last_outward_edge_in_network;
      return true;
    }
    else {
      FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s| group %s is not a network\n", this->name.c_str(), net_name.c_str());
      return false;
    }
  }

  // unrecognized factor
  FRED_VERBOSE(0, "HELP: FACTOR UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
  return false;
}





