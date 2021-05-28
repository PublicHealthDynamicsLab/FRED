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

#include "Condition.h"
#include "Clause.h"
#include "Date.h"
#include "Global.h"
#include "Expression.h"
#include "Household.h"
#include "Natural_History.h"
#include "Network_Type.h"
#include "Property.h"
#include "Person.h"
#include "Place.h"
#include "Predicate.h"
#include "Preference.h"
#include "Random.h"
#include "Rule.h"
#include "State_Space.h"
#include "Utils.h"

Natural_History::Natural_History() {
  this->transition_day = NULL;
  this->transition_date = NULL;
  this->transition_days = NULL;
  this->transition_hour = NULL;
  this->state_space = NULL;
  this->update_vars_externally = NULL;
  this->enable_external_update = false;
  this->R0 = -1.0;
  this->R0_a = -1.0;
  this->R0_b = -1.0;
  this->action_rules = NULL;
  this->wait_rule = NULL;
  this->exposure_rule = NULL;
  this->next_rules = NULL;
  this->default_rule = NULL;
  this->import_count_rule = NULL;
  this->import_per_capita_rule = NULL;
  this->import_ages_rule = NULL;
  this->import_location_rule = NULL;
  this->import_admin_code_rule = NULL;
  this->count_all_import_attempts = NULL;
}


Natural_History::~Natural_History() {
}

void Natural_History::setup(Condition * _condition) {

  FRED_VERBOSE(0, "Natural_History::setup for condition %s\n",
	       _condition->get_name());

  this->condition = _condition;
  this->id = this->condition->get_id();
  strcpy(this->name, this->condition->get_name());
  this->state_space = new State_Space(this->name);

}


void Natural_History::get_properties() {
  char property_name[FRED_STRING_SIZE];

  FRED_VERBOSE(0, "Natural_History::get_properties for condition %d\n", this->condition->get_id());

  // this sets the number and names of the states:
  this->state_space->get_properties();
  this->number_of_states = this->state_space->get_number_of_states();

  // read optional properties
  Property::disable_abort_on_failure();

  ///////// TRANSMISSIBILITY

  this->transmissibility = 0.0;
  Property::get_property(get_name(), "transmissibility", &this->transmissibility);

  // convenience R0 setting properties
  Property::get_property(get_name(), "R0", &this->R0);
  Property::get_property(get_name(), "R0_a", &this->R0_a);
  Property::get_property(get_name(), "R0_b", &this->R0_b);

  if(this->R0 > 0) {
    this->transmissibility = this->R0_a * this->R0 * this->R0 + this->R0_b * this->R0;
    FRED_VERBOSE(0, "R0 = %f so setting transmissibility to %f\n", this->R0, this->transmissibility);
  }

  ///////// TRANSITION MODEL

  // The transition model determines how to decide whether to change
  // state, and which state to change to.

  // STATE DURATION
  this->wait_rule = new Rule* [this->number_of_states];
  this->duration_expression = new Expression* [this->number_of_states];
  this->transition_day = new int [this->number_of_states]; 
  this->transition_date = new std::string [this->number_of_states]; 
  this->transition_days = new int [this->number_of_states]; 
  this->transition_hour = new int [this->number_of_states]; 
  for (int state = 0; state < this->number_of_states; ++state) {
    this->wait_rule[state] = NULL;
  }

  // STATE ACTIONS
  this->transmissibility_rule = new Rule* [this->number_of_states];
  this->susceptibility_rule = new Rule* [this->number_of_states];
  this->edge_expression = new Expression* [this->number_of_states];
  this->condition_to_transmit = new int [this->number_of_states]; 
  this->place_type_to_join = new int [this->number_of_states]; 
  this->place_type_to_quit = new int [this->number_of_states]; 
  this->network_action = new int [this->number_of_states]; 
  this->network_type = new int [this->number_of_states]; 
  this->network_mean_degree = new double [this->number_of_states]; 
  this->network_max_degree = new int [this->number_of_states]; 
  this->start_hosting = new int [this->number_of_states]; 
  this->maternity_state = new bool [this->number_of_states]; 
  this->fatal_state = new bool [this->number_of_states]; 

  // STATE CONTACT RESTRICTIONS
  this->absent_groups = new bool* [this->number_of_states];
  this->close_groups = new bool* [this->number_of_states];

  // IMPORT STATE
  this->import_count = new int [this->number_of_states];
  this->import_per_capita_transmissions = new double [this->number_of_states];
  this->import_latitude = new double [this->number_of_states];
  this->import_longitude = new double [this->number_of_states];
  this->import_radius = new double [this->number_of_states];
  this->import_admin_code = new longint [this->number_of_states];
  this->import_min_age = new double [this->number_of_states];
  this->import_max_age = new double [this->number_of_states];
  this->import_count_rule = new Rule* [this->number_of_states];
  this->import_per_capita_rule = new Rule* [this->number_of_states];
  this->import_ages_rule = new Rule* [this->number_of_states];
  this->import_location_rule = new Rule* [this->number_of_states];
  this->import_admin_code_rule = new Rule* [this->number_of_states];
  this->import_list_rule = new Rule* [this->number_of_states];
  this->count_all_import_attempts = new bool [this->number_of_states];

  // TRANSITIONS
  this->state_is_dormant = new int [this->number_of_states]; 
  this->default_next_state = new int [this->number_of_states]; 
  this->default_rule = new Rule* [this->number_of_states]; 

  // INITIALIZE
  for (int i = 0; i < this->number_of_states; i++) {
    this->duration_expression[i] = NULL;
    this->transition_day[i] = -1;
    this->transition_date[i] = "";
    this->transition_days[i] = -1;
    this->transition_hour[i] = 0;
    this->transmissibility_rule[i] = NULL;
    this->susceptibility_rule[i] = NULL;
    this->edge_expression[i] = NULL;
    this->condition_to_transmit[i] = this->id;
    this->place_type_to_join[i] = -1;
    this->place_type_to_quit[i] = -1;
    this->network_action[i] = Network_Action::NONE;
    this->network_type[i] = -1;
    this->network_mean_degree[i] = 0;
    this->network_max_degree[i] = 999999;
    this->start_hosting[i] = 0;
    int n = Group_Type::get_number_of_group_types();
    this->absent_groups[i] = new bool [n];
    this->close_groups[i] = new bool [n];
    for (int j = 0; j < n; j++) {
      this->absent_groups[i][j] = false;
      this->close_groups[i][j] = false;
    }
    this->state_is_dormant[i] = 0;
    this->default_next_state[i] = i;
    this->default_rule[i] = NULL;
    this->import_count[i] = 0;
    this->import_per_capita_transmissions[i] = 0;
    this->import_latitude[i] = 0;
    this->import_longitude[i] = 0;
    this->import_radius[i] = 0;
    this->import_admin_code[i] = 0;
    this->import_min_age[i] = 0;
    this->import_max_age[i] = 999;
    this->maternity_state[i] = false;
    this->fatal_state[i] = false;

    this->import_count_rule[i] = NULL;
    this->import_per_capita_rule[i] = NULL;
    this->import_ages_rule[i] = NULL;
    this->import_location_rule[i] = NULL;
    this->import_admin_code_rule[i] = NULL;
    this->import_list_rule[i] = NULL;
    this->count_all_import_attempts[i] = false;
  }

  // GET PROPERTY VALUES
  for (int i = 0; i < this->number_of_states; i++) {

    int is_dormant = 0;
    sprintf(property_name, "%s.%s.is_dormant", get_name(), get_state_name(i).c_str());
    if (Property::does_property_exist(property_name)) {
      Property::get_property(property_name, &is_dormant);
      this->state_is_dormant[i] = is_dormant;
    }

    char next_state_name[FRED_STRING_SIZE];
    int next_state = i;

    // IMPORT STATE

    sprintf(property_name, "%s.%s.import_max_cases", get_name(), get_state_name(i).c_str());
    this->import_count[i] = 0;
    Property::get_property(property_name, &this->import_count[i]);
    if (this->import_count[i]) {
      FRED_VERBOSE(0, "SETTING %s.%s.import_max_cases = %d\n",
		   get_name(), get_state_name(i).c_str(), this->import_count[i]);
    }
    
    sprintf(property_name, "%s.%s.import_per_capita", get_name(), get_state_name(i).c_str());
    this->import_per_capita_transmissions[i] = 0;
    Property::get_property(property_name, &this->import_per_capita_transmissions[i]);
    if (this->import_per_capita_transmissions[i]) {
      FRED_VERBOSE(0, "SETTING %s.%s.import_per_capita = %f\n",
		   get_name(), get_state_name(i).c_str(), this->import_per_capita_transmissions[i]);
    }
    
    sprintf(property_name, "%s.%s.import_latitude", get_name(), get_state_name(i).c_str());
    this->import_latitude[i] = 0;
    Property::get_property(property_name, &this->import_latitude[i]);
    if (this->import_latitude[i]) {
      FRED_VERBOSE(0, "SETTING %s.%s.import_latitude = %f\n",
		   get_name(), get_state_name(i).c_str(), this->import_latitude[i]);
    }
    
    sprintf(property_name, "%s.%s.import_longitude", get_name(), get_state_name(i).c_str());
    this->import_longitude[i] = 0;
    Property::get_property(property_name, &this->import_longitude[i]);
    if (this->import_longitude[i]) {
      FRED_VERBOSE(0, "SETTING %s.%s.import_longitude = %f\n",
		   get_name(), get_state_name(i).c_str(), this->import_longitude[i]);
    }
    
    sprintf(property_name, "%s.%s.import_radius", get_name(), get_state_name(i).c_str());
    this->import_radius[i] = 0;
    Property::get_property(property_name, &this->import_radius[i]);
    if (this->import_radius[i]) {
      FRED_VERBOSE(0, "SETTING %s.%s.import_radius = %f\n",
		   get_name(), get_state_name(i).c_str(), this->import_radius[i]);
    }

    sprintf(property_name, "%s.%s.import_min_age", get_name(), get_state_name(i).c_str());
    this->import_min_age[i] = 0;
    Property::get_property(property_name, &this->import_min_age[i]);
    if (this->import_min_age[i]) {
      FRED_VERBOSE(0, "SETTING %s.%s.import_min_age = %f\n",
		   get_name(), get_state_name(i).c_str(), this->import_min_age[i]);
    }

    sprintf(property_name, "%s.%s.import_max_age", get_name(), get_state_name(i).c_str());
    this->import_max_age[i] = 999;
    Property::get_property(property_name, &this->import_max_age[i]);
    if (this->import_max_age[i]) {
      FRED_VERBOSE(0, "SETTING %s.%s.import_max_age = %f\n",
		   get_name(), get_state_name(i).c_str(), this->import_max_age[i]);
    }

    sprintf(property_name, "%s.%s.import_admin_code", get_name(), get_state_name(i).c_str());
    this->import_admin_code[i] = 0;
    Property::get_property(property_name, &this->import_admin_code[i]);
    if (this->import_admin_code[i]) {
      FRED_VERBOSE(0, "SETTING %s.%s.import_admin_code = %lld\n",
		   get_name(), get_state_name(i).c_str(), this->import_admin_code[i]);
    }

  }

  // get state to enter upon exposure
  this->exposed_state = -1;
  /*
  char exp_state_name[FRED_STRING_SIZE];
  strcpy(exp_state_name, "");
  Property::get_property(get_name(), "exposed_state", exp_state_name);
  if (strcmp(exp_state_name,"") != 0) {
    this->exposed_state = this->state_space->get_state_from_name(exp_state_name);
  }
  FRED_VERBOSE(0, "exposed state = %d\n", this->exposed_state);
  */

  // get start state for import
  this->import_start_state = -1;
  char import_state_name[FRED_STRING_SIZE];
  strcpy(import_state_name, "none");
  Property::get_property(get_name(), "import_start_state", import_state_name);
  if (strcmp(import_state_name,"none") != 0) {
    this->import_start_state = this->state_space->get_state_from_name(import_state_name);
  }
  FRED_VERBOSE(0, "%s.import_start_state = %s\n", get_name(), import_state_name);

  // RULES

  this->action_rules = new rule_vector_t [this->number_of_states];
  for (int state = 0; state < this->number_of_states; state++) {
    this->action_rules[state].clear();
  }
  this->next_rules = new rule_vector_t* [this->number_of_states];
  for (int state = 0; state < this->number_of_states; state++) {
    this->next_rules[state] = new rule_vector_t [this->number_of_states];
    for (int state2 = 0; state2 < this->number_of_states; state2++) {
      this->next_rules[state][state2].clear();
    }
  }

  // PERSONAL VARIABLES

  this->update_vars_externally = new bool [this->number_of_states];
  for (int state = 0; state < this->number_of_states; state++) {
    this->update_vars_externally[state] = false;
  }
  this->update_vars = new bool [this->number_of_states];
  for (int state = 0; state < this->number_of_states; state++) {
    this->update_vars[state] = false;
  }

  for (int state = 0; state < this->number_of_states; state++) {

    // decide if we update vars externally in this state
    sprintf(property_name, "%s.%s.update_vars_externally", get_name(), get_state_name(state).c_str());
    int check = 0;
    Property::get_property(property_name, &check);
    this->update_vars_externally[state] = check;
    if (check) {
      this->enable_external_update = true;
      // set global external update flag
      Global::Enable_External_Updates = true;
    }
  }

  // restore requiring properties
  Property::set_abort_on_failure();

  FRED_VERBOSE(0, "Natural_History::setup finished for condition %s\n",
	       this->condition->get_name());
}

void Natural_History::prepare() {

  char property_name[FRED_STRING_SIZE];
  char src_string[FRED_STRING_SIZE];
  char dest_string[FRED_STRING_SIZE];

  FRED_VERBOSE(0, "Natural_History::prepare entered for condition %s\n",
	       this->condition->get_name());

  prepare_rules();

  // read optional properties
  Property::disable_abort_on_failure();

  for (int state = 0; state < this->number_of_states; ++state) {

    sprintf(property_name, "%s.%s.condition_to_transmit", get_name(), get_state_name(state).c_str());
    string condition_name = string(get_name());
    Property::get_property(property_name, condition_name);
    int cond_id = Condition::get_condition_id(condition_name);
    this->condition_to_transmit[state] = cond_id;
 

    sprintf(property_name, "%s.%s.start_hosting", get_name(), get_state_name(state).c_str());
    this->start_hosting[state] = 0;
    Property::get_property(property_name, &this->start_hosting[state]);
  }

  // transmitted place type
  char new_place_type [FRED_STRING_SIZE];
  sprintf(property_name, "%s.place_type_to_transmit", get_name());
  sprintf(new_place_type, "");
  Property::get_property(property_name, new_place_type);
  this->place_type_to_transmit = Place_Type::get_type_id(new_place_type);

  // restore requiring properties
  Property::set_abort_on_failure();

  print();

  FRED_VERBOSE(0, "Natural_History::prepare finished for condition %s\n",
	       this->condition->get_name());

}

char* Natural_History::get_name() {
  return this->state_space->get_name();
}

string Natural_History::get_state_name(int state) {
  if (state < 0) {
    return "UNSET";
  }
  return this->state_space->get_state_name(state);
}

void Natural_History::print() {
  char src_string[FRED_STRING_SIZE];
  char dest_string[FRED_STRING_SIZE];
  printf("NATURAL HISTORY OF %s\n", get_name());
  printf("NATURAL HISTORY %s.states =", get_name());
  for (int i = 0; i < this->number_of_states; i++) {
    printf(" %s", get_state_name(i).c_str());
  }
  printf("\n");
  printf("NATURAL HISTORY %s.exposed_state = %s\n", get_name(), this->exposed_state > 0 ?
	 get_state_name(this->exposed_state).c_str() : "UNSET");
  printf("NATURAL HISTORY %s.import_start_state = %s\n", get_name(), get_state_name(this->import_start_state).c_str());
  printf("NATURAL HISTORY %s.transmissibility = %f\n", get_name(), this->transmissibility);

  printf("number of states = %d\n", this->number_of_states);
  for (int i = 0; i < this->number_of_states; i++) {
    char sname[FRED_STRING_SIZE];
    strcpy(sname, get_state_name(i).c_str());

    // import state
    printf("NATURAL HISTORY %s.%s.import_max_cases = %d\n",
	   get_name(), sname, this->import_count[i]);
    printf("NATURAL HISTORY %s.%s.import_per_capita_transmissions = %f\n",
	   get_name(), sname, this->import_per_capita_transmissions[i]);
    printf("NATURAL HISTORY %s.%s.import_latitude = %f\n",
	   get_name(), sname, this->import_latitude[i]);
    printf("NATURAL HISTORY %s.%s.import_longitude = %f\n",
	   get_name(), sname, this->import_longitude[i]);
    printf("NATURAL HISTORY %s.%s.import_radius = %f\n",
	   get_name(), sname, this->import_radius[i]);
    printf("NATURAL HISTORY %s.%s.import_min_age = %f\n",
	   get_name(), sname, this->import_min_age[i]);
    printf("NATURAL HISTORY %s.%s.import_max_age = %f\n",
	   get_name(), sname, this->import_max_age[i]);
    printf("NATURAL HISTORY %s.%s.import_admin_code = %lld\n",
	   get_name(), sname, this->import_admin_code[i]);

    // transition
    for (int j = 0; j < this->number_of_states; j++) {
      for (int n = 0; n < this->next_rules[i][j].size();n++) {
	printf("NATURAL HISTORY RULE[%d][%d][%d] ",i,j,n);
	this->next_rules[i][j][n]->print();
      }
    }

    // transmission info
    string stmp = Condition::get_name(this->condition_to_transmit[i]);
    printf("NATURAL HISTORY %s.%s.condition_to_transmit = %s\n",
	   get_name(), sname, stmp.c_str());

    // transitions
    printf("NATURAL HISTORY %s.%s.state_is_dormant = %d\n",
	   get_name(), sname, this->state_is_dormant[i]);

  }

  printf("NATURAL HISTORY %s.place_type_to_transmit = %d %s\n",
	 get_name(),
	 this->place_type_to_transmit,
	 this->place_type_to_transmit < 0 ? "NONE" : Place_Type::get_place_type(this->place_type_to_transmit)->get_name());

  for (int state = 0; state < this->number_of_states; ++state) {
    printf("NATURAL HISTORY %s.%s.network_action = %d\n",
	   get_name(), get_state_name(state).c_str(), this->network_action[state]);
    printf("NATURAL HISTORY %s.%s.network_type = %d\n",
	   get_name(), get_state_name(state).c_str(), this->network_type[state]);
    printf("NATURAL HISTORY %s.%s.network_mean_degree = %f\n",
	   get_name(), get_state_name(state).c_str(), this->network_mean_degree[state]);
    printf("NATURAL HISTORY %s.%s.network_max_degree = %d\n",
	   get_name(), get_state_name(state).c_str(), this->network_max_degree[state]);
  }
}

int Natural_History::get_next_transition_step(Person* person, int state, int day, int hour) {
  int step = 24*day + hour;
  int transition_step = step;

  FRED_VERBOSE(1, "get_next_transition_step entered person %d state %d day %d hour %d\n",
	       person->get_id(), state, day, hour);

  if (this->duration_expression[state]) {
    double duration = this->duration_expression[state]->get_value(person);
    transition_step += round(duration);
  }
  else if (0 <= this->transition_days[state]) {
    transition_step += 24*this->transition_days[state] + (this->transition_hour[state]-hour);
  }
  else if (0 <= this->transition_day[state]) {
    int days = this->transition_day[state] - Date::get_day_of_week(day);
    if (days < 0) {
      days += 7;
    }
    else if (days == 0 && this->transition_hour[state] < hour) {
      days += 7;
    }
    transition_step += 24*days + (this->transition_hour[state] - hour);
  }
  else if (this->transition_date[state]!="") {
    int days = Date::get_sim_day(this->transition_date[state]) - day;
    transition_step += 24*days + (this->transition_hour[state] - hour);
  }

  FRED_VERBOSE(1, "get_next_transition_step finished person %d state %d trans_step %d\n",
	       person->get_id(), state, transition_step);
  return transition_step;
}



int Natural_History::get_next_state(Person* person, int state) {

  // FRED_VERBOSE(0, "get_next_state entered day %d person %d current state %s\n", Global::Simulation_Day, person->get_id(), get_state_name(state).c_str());

  double total = 0.0;
  double trans_prob [this->number_of_states];
  for(int next = 0; next < this->number_of_states; ++next) {

    trans_prob[next] = 0.0;
    int nrules = this->next_rules[state][next].size();
    if (nrules > 0) {
      // find maximum with_value for any rule that matches this agent
      double max_value = 0.0;
      for (int n = 0; n < nrules; n++) {
	Rule* rule = this->next_rules[state][next][n];
	double value = rule->get_value(person);
	// printf("value = %f from RULE ", value); rule->print();
	if (value > max_value) {
	  max_value = value;
	}
	// printf("get_next_state: person %d state %s nrules %d rule %d max %f\n", person->get_id(), get_state_name(state).c_str(), nrules, n, max_value);
      }
      // use max_value as transition prob
      trans_prob[next] = max_value;
    }

    // the following is needed to correct for round-off effects in "zero probability" logit computations
    if (trans_prob[next] < 1e-20) {
      trans_prob[next] = 0.0;
    }

    // accumulate total prob weight (will be normalized below)
    total += trans_prob[next];
  }

  if (0.999999999 <= total) {
    // normalize
    for(int next = 0; next < this->number_of_states; ++next) {
      trans_prob[next] /= total;
    }
  }
  else {
    // total of transition probs is less than 1.0.
    // In this case, the default next state is assigned the remaining probability mass.

    /*
      printf("UPDATING TRANSITION PROB FOR DEFAULT NEXT STATE. Total = %e Current: ", total);
      for(int next = 0; next < this->number_of_states; ++next) {
      printf("| %d: %e ", next, trans_prob[next]);
      }
      printf("\n");
      printf("UPDATING TRANSITION PROB FOR DEFAULT NEXT STATE by %e\n", 1.0-total);
    */
    trans_prob[this->default_next_state[state]] += 1.0 - total;
    /*
      printf("UPDATING TRANSITION PROB FOR DEFAULT NEXT STATE. New: ");
      for(int n = 0; n < this->number_of_states; ++n) {
      printf("| %d: %e ", n, trans_prob[n]);
      }
      printf("\n");
    */
  }

  // DEBUGGING
  if (0) {
    if (Global::Enable_Records) {
      fprintf(Global::Recordsfp,
	      "HEALTH RECORD: person %d COND %s TRANSITION_PROBS: ",
	      person->get_id(), get_name());
      for(int next = 0; next < this->number_of_states; ++next) {
	fprintf(Global::Recordsfp, "%d: %e |", next, trans_prob[next]);
      }
      fprintf(Global::Recordsfp,"\n");
    }
  }

  int next_state = select_next_state(state, trans_prob);

  assert(next_state > -1);
  // FRED_VERBOSE(0, "get_next_state returns person %d next state %d %s\n", person->get_id(), next_state, get_state_name(next_state).c_str());

  return next_state;
}


int Natural_History::select_next_state(int state, double* transition_prob) {

  // at this point, we assume transition_prob is a probability distribution

  // check for deterministic transition
  for(int j = 0; j < this->number_of_states; ++j) {
    if (transition_prob[j] == 1.0) {
      return j;
    }
  }

  // draw a random uniform variate
  double r = Random::draw_random();

  // find transition corresponding to this draw
  double sum = 0.0;
  for(int j = 0; j < this->number_of_states; ++j) {
    sum += transition_prob[j];
    if(r < sum) {
      return j;
    }
  }

  // should never reach this point
  Utils::fred_abort("Natural_History::select_next_state: Help! Bad result: state = %d\n",
		    state);
  return -1;
}


void Natural_History::prepare_rules() {

  this->exposed_state = -1;
  int n = Rule::get_number_of_compiled_rules();
  for (int i = 0; i < n; i++) {
    Rule* rule = Rule::get_compiled_rule(i);

    FRED_VERBOSE(0,"NH: rule = |%s|  cond %d state %d\n", 
		 rule->get_name().c_str(), 
		 rule->get_cond_id(), 
		 rule->get_state_id());

    // ACTION RULE
    if (rule->is_action_rule() && rule->get_cond_id()==this->id) {
      FRED_VERBOSE(0,"ACTION RULE\n");
      int state = rule->get_state_id();
      if (0 <= state) {
	rule->mark_as_used();
	this->action_rules[state].push_back(rule);

	// CHECK FOR FATAL RULE
	if (rule->get_action_id()==Rule_Action::DIE || rule->get_action_id()==Rule_Action::DIE_OLD) {
	  this->fatal_state[state] = true;
	}

	// CHECK FOR SUS RULES
	if (rule->get_action_id()==Rule_Action::SUS) {
	  if (this->susceptibility_rule[state]!=NULL) {
	    this->susceptibility_rule[state]->mark_as_unused();
	  }
	  this->susceptibility_rule[state] = rule;
	  rule->mark_as_used();
	  printf("SUSCEPTIBILITY RULE: "); rule->print();
	}

	// CHECK FOR TRANS RULES
	if (rule->get_action_id()==Rule_Action::TRANS) {
	  if (this->transmissibility_rule[state]!=NULL) {
	    this->transmissibility_rule[state]->mark_as_unused();
	  }
	  this->transmissibility_rule[state] = rule;
	  rule->mark_as_used();
	  printf("TRANSMISSIBILITY RULE: "); rule->print();
	}

	// CHECK FOR IMPORT RULES
	if (rule->get_action_id()==Rule_Action::IMPORT_COUNT) {
	  string property = rule->get_expression_str();
	  if (this->import_count_rule[state]!=NULL) {
	    this->import_count_rule[state]->mark_as_unused();
	  }
	  this->import_count_rule[state] = rule;
	  rule->mark_as_used();
	  printf("IMPORT RULE: "); rule->print();
	}

	if (rule->get_action_id()==Rule_Action::IMPORT_PER_CAPITA) {
	  if (this->import_per_capita_rule[state]!=NULL) {
	    this->import_per_capita_rule[state]->mark_as_unused();
	  }
	  this->import_per_capita_rule[state] = rule;
	  rule->mark_as_used();
	  printf("IMPORT RULE: "); rule->print();
	}

	if (rule->get_action_id()==Rule_Action::IMPORT_AGES) {
	  if (this->import_ages_rule[state]!=NULL) {
	    this->import_ages_rule[state]->mark_as_unused();
	  }
	  this->import_ages_rule[state] = rule;
	  rule->mark_as_used();
	  printf("IMPORT RULE: "); rule->print();
	}

	if (rule->get_action_id()==Rule_Action::IMPORT_LOCATION) {
	  if (this->import_location_rule[state]!=NULL) {
	    this->import_location_rule[state]->mark_as_unused();
	  }
	  this->import_location_rule[state] = rule;
	  rule->mark_as_used();
	  printf("IMPORT RULE: "); rule->print();
	}

	if (rule->get_action_id()==Rule_Action::IMPORT_CENSUS_TRACT) {
	  if (this->import_admin_code_rule[state]!=NULL) {
	    this->import_admin_code_rule[state]->mark_as_unused();
	  }
	  this->import_admin_code_rule[state] = rule;
	  rule->mark_as_used();
	  printf("IMPORT RULE: "); rule->print();
	}

	if (rule->get_action_id()==Rule_Action::IMPORT_LIST) {
	  if (this->import_list_rule[state]!=NULL) {
	    this->import_list_rule[state]->mark_as_unused();
	  }
	  this->import_list_rule[state] = rule;
	  rule->mark_as_used();
	  printf("IMPORT RULE: "); rule->print();
	}

	if (rule->get_action_id()==Rule_Action::COUNT_ALL_IMPORT_ATTEMPTS) {
	  this->count_all_import_attempts[state] = true;
	  rule->mark_as_used();
	  printf("IMPORT RULE: "); rule->print();
	}

	// CHECK FOR SCHEDULE RULES
	if (rule->is_schedule_rule()) {
	  string group_type_str = rule->get_expression_str();
	  printf("COMPILE_RULES: %s group_type_str = |%s|\n ", this->name, group_type_str.c_str());
	  string_vector_t group_type_vec = Utils::get_string_vector(group_type_str,',');
	  printf("COMPILE_RULES: %s group_type_vec size %lu\n ", this->name, group_type_vec.size());
	  for (int k = 0; k < group_type_vec.size(); k++) {
	    string group_name = group_type_vec[k];
	    int type_id = Group_Type::get_type_id(group_name);
	    if (rule->get_action()=="absent") {
	      this->absent_groups[state][type_id] = true;
	    }
	    if (rule->get_action()=="present") {
	      this->absent_groups[state][type_id] = false;
	    }
	    if (rule->get_action()=="close") {
	      this->close_groups[state][type_id] = true;
	    }
	    printf("COMPILE: cond %s state %s %s group_name %s type_id %d\n",
		   this->name, get_state_name(state).c_str(), rule->get_action().c_str(), group_name.c_str(), type_id);
	  }
	  // debugging:
	  if (rule->get_action()=="present") {
	    int n = Group_Type::get_number_of_group_types();
	    for (int k = 0; k < n; k++) {
	      if (this->absent_groups[state][k]) {
		string group_name = Group_Type::get_group_type_name(k);
		printf("COMPILE: cond %s state %s UPDATED ABSENT group_name %s\n",
		       this->name, get_state_name(state).c_str(), group_name.c_str());
	      }
	    }
	  }
	} ////////// SCHED
      }

    } // END ACTION RULES

    // WAIT RULE
    if (rule->is_wait_rule() && rule->get_cond_id()==this->id) {
      FRED_VERBOSE(0,"WAIT RULE\n");
      int state = rule->get_state_id();
      if (0 <= state) {
	string action = rule->get_action();
	if (action=="wait") {
	  if (this->wait_rule[state] != NULL) {
	    this->wait_rule[state]->set_hidden_by_rule(rule);
	    this->wait_rule[state]->mark_as_unused();
	  }
	  this->wait_rule[state] = rule;
	  this->wait_rule[state]->mark_as_used();
	  this->duration_expression[state] = this->wait_rule[state]->get_expression();
	}
	else { // "wait_until" rule
	  string ttime = rule->get_expression_str();
	  if (ttime.find("Today")==0) {
	    this->transition_days[state] = 0;
	  }
	  if (ttime.find("today")==0) {
	    this->transition_days[state] = 0;
	  }
	  if (ttime.find("Tomorrow")==0) {
	    this->transition_days[state] = 1;
	  }
	  if (ttime.find("tomorrow")==0) {
	    this->transition_days[state] = 1;
	  }
	  if (ttime.find("_day")!=string::npos) {
	    int pos = ttime.find("_day");
	    string dstr = ttime.substr(0,pos);
	    int d = -1;
	    sscanf(dstr.c_str(), "%d", &d);
	    this->transition_days[state] = d;
	  }
	  if (this->transition_days[state] == -1) {
	    if (ttime.find("Sun")==0) {
	      this->transition_day[state] = 0;
	    }
	    else if (ttime.find("Mon")==0) {
	      this->transition_day[state] = 1;
	    }
	    else if (ttime.find("Tue")==0) {
	      this->transition_day[state] = 2;
	    }
	    else if (ttime.find("Wed")==0) {
	      this->transition_day[state] = 3;
	    }
	    else if (ttime.find("Thu")==0) {
	      this->transition_day[state] = 4;
	    }
	    else if (ttime.find("Fri")==0) {
	      this->transition_day[state] = 5;
	    }
	    else if (ttime.find("Sat")==0) {
	      this->transition_day[state] = 6;
	    }
	    else {
	      int pos = ttime.find("_at_");
	      if (pos != string::npos) {
		this->transition_date[state] = ttime.substr(0,pos);
	      }
	      else {
		this->transition_date[state] = ttime;
	      }
	    }
	    // printf("transition_day = %d\n", this->transition_day[state]);
	    // printf("transition_date = %s\n", this->transition_date[state].c_str());
	  }
	  int hour = 0;
	  int pos = ttime.find("_at_");
	  if (pos != string::npos) {
	    string hstr = ttime.substr(pos+4);
	    sscanf(hstr.c_str(), "%d", &hour);
	    if (hour == 12 && hstr.find("am")!= string::npos) {
	      hour = 0;
	    }
	    if (hour < 12 && hstr.find("pm")!= string::npos) {
	      hour += 12;
	    }
	  }
	  this->transition_hour[state] = hour;
	  printf("transition_hour = %d\n", this->transition_hour[state]);
	  // parse the string to set the transition time
	  if (-1 < this->transition_day[state] ||
	      -1 < this->transition_days[state] ||
	      -1 < this->transition_hour[state]) {
	    if (this->wait_rule[state] != NULL) {
	      this->wait_rule[state]->set_hidden_by_rule(rule);
	      this->wait_rule[state]->mark_as_unused();
	      this->duration_expression[state] = NULL;
	    }
	    this->wait_rule[state] = rule;
	    this->wait_rule[state]->mark_as_used();
	  }
	}
      }
    }

    // EXPOSURE RULE
    if (rule->is_exposure_rule() && rule->get_cond_id()==this->id) {
      FRED_VERBOSE(0,"EXPOSURE RULE\n");
      int next_state_id = rule->get_next_state_id();
      if (0 <= next_state_id) {
	if (this->exposure_rule != NULL) {
	  this->exposure_rule->set_hidden_by_rule(rule);
	  this->exposure_rule->mark_as_unused();
	}
	this->exposure_rule = rule;
	this->exposure_rule->mark_as_used();
	this->exposed_state = next_state_id;
      }
    }

    // NEXT RULE
    if (rule->is_next_rule() && rule->get_cond_id()==this->id) {
      int state = rule->get_state_id();
      int next_state = rule->get_next_state_id();
      FRED_VERBOSE(0,"NEXT RULE cond %d state %d next_state %d\n", this->id, state,next_state);
      if (0 <= state && 0 <= next_state) {
	rule->mark_as_used();
	this->next_rules[state][next_state].push_back(rule);
      }
    }

    // DEFAULT RULE
    if (rule->is_default_rule() && rule->get_cond_id()==this->id) {
      FRED_VERBOSE(0,"DEFAULT RULE\n");
      int state_id = rule->get_state_id();
      if (0 <= state_id) {
	int next_state_id = rule->get_next_state_id();
	if (0 <= next_state_id) {
	  if (this->default_rule[state_id] != NULL) {
	    this->default_rule[state_id]->set_hidden_by_rule(rule);
	    this->default_rule[state_id]->mark_as_unused();
	  }
	  this->default_rule[state_id] = rule;
	  this->default_rule[state_id]->mark_as_used();
	  this->default_next_state[state_id] = next_state_id;
	}
      }
    }
    FRED_VERBOSE(0,"RULE %d FINISHED\n", i);
  }

  printf("\nEXPOSURE RULE:\n");
  if (this->exposure_rule)
      this->exposure_rule->print();
  
  for (int i = 0; i < this->number_of_states; i++) {

    printf("\nACTION RULES for state %d:\n", i);
    for (int n = 0; n < this->action_rules[i].size();n++) {
      this->action_rules[i][n]->print();
    }

    printf("\nWAIT RULES for state %d:\n", i);
    if (this->wait_rule[i]) {
      this->wait_rule[i]->print();
    }
    else {
      Utils::print_warning("No wait rule found for state " + string(this->name) + "." + get_state_name(i) + "\n");
      printf("No wait rule found for state %d %s.%s\n", i, this->name, get_state_name(i).c_str());
    }

    for (int j = 0; j < this->number_of_states; j++) {
      printf("\nNEXT RULES for transition %d to %d = %d:\n",i,j,(int)this->next_rules[i][j].size());
      for (int n = 0; n < this->next_rules[i][j].size();n++) {
	this->next_rules[i][j][n]->print();
      }
    }
  
    printf("\nDEFAULT RULE for state %d:\n",i);
    if (this->default_rule[i])
      this->default_rule[i]->print();

  }
}



bool Natural_History::is_absent(int state, int group_type_id) {
  if (state < 0) {
    return false;
  }
  return this->absent_groups[state][group_type_id];
}

bool Natural_History::is_closed(int state, int group_type_id) {
  if (state < 0) {
    return false;
  }
  return this->close_groups[state][group_type_id];
}

