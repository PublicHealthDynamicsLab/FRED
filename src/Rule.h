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

#ifndef _FRED_RULE_H
#define _FRED_RULE_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

using namespace std;

class Person;
class Clause;
class Expression;
class Preference;

namespace Rule_Action {
  enum e { NONE = -1,
	   WAIT,
	   WAIT_UNTIL,
	   GIVE_BIRTH,
	   DIE, 
	   DIE_OLD, 
	   SUS,
	   TRANS,
	   JOIN,
	   QUIT,
	   ADD_EDGE_FROM,
	   ADD_EDGE_TO,
	   DELETE_EDGE_FROM,
	   DELETE_EDGE_TO,
	   SET,
	   SET_LIST,
	   SET_STATE,
	   CHANGE_STATE,
	   SET_WEIGHT, 
	   SET_SUS,
	   SET_TRANS,
	   REPORT,
	   ABSENT,
	   PRESENT, 
	   CLOSE,
	   RANDOMIZE_NETWORK,
	   IMPORT_COUNT,
	   IMPORT_PER_CAPITA,
	   IMPORT_LOCATION,
	   IMPORT_CENSUS_TRACT,
	   IMPORT_AGES,
	   COUNT_ALL_IMPORT_ATTEMPTS,
	   IMPORT_LIST,
	   RULE_ACTIONS };
};

class Rule {
public:

  Rule(std::string s);

  std::string get_name() {
    return this->name;
  }

  std::string get_cond() {
    return this->cond;
  }

  int get_cond_id() {
    return this->cond_id;
  }

  std::string get_state() {
    return this->state;
  }

  int get_state_id() {
    return this->state_id;
  }

  std::string get_clause_str() {
    return this->clause_str;
  }

  Clause* get_clause() {
    return this->clause;
  }

  std::string get_next_state() {
    return this->next_state;
  }

  int get_next_state_id() {
    return this->next_state_id;
  }

  std::string get_action() {
    return this->action;
  }

  int get_action_id() {
    return this->action_id;
  }

  std::string get_expression_str() {
    return this->expression_str;
  }

  Expression* get_expression() {
    return this->expression;
  }

  std::string get_expression_str2() {
    return this->expression_str2;
  }

  Expression* get_expression2() {
    return this->expression2;
  }

  std::string get_expression_str3() {
    return this->expression_str3;
  }

  Expression* get_expression3() {
    return this->expression3;
  }

  std::string get_var() {
    return this->var;
  }

  int get_var_id() {
    return this->var_id;
  }

  std::string get_list_var() {
    return this->list_var;
  }

  int get_list_var_id() {
    return this->list_var_id;
  }

  std::string get_source_cond() {
    return this->source_cond;
  }

  int get_source_cond_id() {
    return this->source_cond_id;
  }

  std::string get_source_state() {
    return this->source_state;
  }

  int get_source_state_id() {
    return this->source_state_id;
  }

  std::string get_dest_state() {
    return this->dest_state;
  }

  int get_dest_state_id() {
    return this->dest_state_id;
  }

  std::string get_network() {
    return this->network;
  }

  int get_network_id() {
    return this->network_id;
  }

  std::string get_group() {
    return this->group;
  }

  int get_group_type_id() {
    return this->group_type_id;
  }

  void set_err_msg(string msg) {
    this->err = msg;
  }

  std::string get_err_msg() {
    return this->err;
  }

  bool is_warning() {
    return this->warning;
  }

  bool is_join_rule() {
    return this->action == "join";
  }

  bool is_wait_rule() {
    return this->wait_rule;
  }

  bool is_exposure_rule() {
    return this->exposure_rule;
  }

  bool is_default_rule() {
    return this->default_rule;
  }

  bool is_next_rule() {
    return this->next_rule;
  }

  bool is_schedule_rule() {
    return this->schedule_rule;
  }

  bool is_action_rule() {
    return this->action_rule;
  }

  bool is_global() {
    return this->global;
  }

  void parse(std::string str);

  bool parse();

  bool parse_action_rule();
  bool parse_wait_rule();
  bool parse_exposure_rule();
  bool parse_next_rule();
  bool parse_default_rule();
  bool parse_state();

  void print();

  double get_value(Person* person, Person* other = NULL);

  void mark_as_used() {
    this->used = true;
  }

  void mark_as_unused() {
    this->used = false;
  }

  bool is_used() {
    return this->used;
  }

  bool compile();
  bool compile_action_rule();

  void set_hidden_by_rule(Rule* rule);

  Rule* get_hidden_by_rule() {
    return this->hidden_by;
  }

  static void print_warnings();

  static void add_rule_line(std::string line);

  static void prepare_rules();

  static int get_number_of_rules() {
    return Rule::rules.size();
  }

  static Rule* get_rule(int n) {
    return Rule::rules[n];
  }

  static int get_number_of_compiled_rules() {
    return Rule::compiled_rules.size();
  }

  static Rule* get_compiled_rule(int n) {
    return Rule::compiled_rules[n];
  }

private:
  static int first_rule;
  static std::vector<std::string> rule_list;
  static std::vector<Rule*> rules;
  static std::vector<Rule*> compiled_rules;
  static std::vector<std::string> action_string;

  std::string name;
  std::string cond;
  int cond_id;
  std::string state;
  int state_id;
  std::string clause_str;
  Clause* clause;
  std::string next_state;
  int next_state_id;
  std::string action;
  int action_id;
  std::string expression_str;
  Expression* expression;
  std::string expression_str2;
  Expression* expression2;
  std::string expression_str3;
  Expression* expression3;
  std::string var;
  int var_id;
  std::string list_var;
  int list_var_id;
  std::string source_cond;
  int source_cond_id;
  std::string source_state;
  int source_state_id;
  std::string dest_state;
  int dest_state_id;
  std::string network;
  int network_id;
  std::string group;
  int group_type_id;

  std::string err;
  std::vector<std::string>parts;

  bool used;
  bool warning;
  bool global;

  // Rule Types:
  bool action_rule;
  bool wait_rule;
  bool exposure_rule;
  bool next_rule;
  bool default_rule;
  bool schedule_rule;

  Preference* preference;

  Rule* hidden_by;
};

#endif


