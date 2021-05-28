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

#include "Rule.h"
#include "Condition.h"
#include "Clause.h"
#include "Expression.h"
#include "Global.h"
#include "Network.h"
#include "Person.h"
#include "Preference.h"
#include "Utils.h"

///////// STATIC MEMBERS

std::vector<std::string> Rule::rule_list;
int Rule::first_rule = 1;
std::vector<Rule*> Rule::rules;
std::vector<Rule*> Rule::compiled_rules;

std::vector<std::string> Rule::action_string =
  {
    "wait",
    "wait_until",
    "give_birth",
    "die",
    "fatal",
    "sus", 
    "trans",
    "join",
    "quit",
    "add_edge_from",
    "add_edge_to",
    "delete_edge_from",
    "delete_edge_to",
    "set",
    "set_list",
    "set_state",
    "change_state",
    "set_weight",
    "set_sus",
    "set_trans",
    "report",
    "absent",
    "present",
    "close",
    "randomize_network",
    "import_count",
    "import_per_capita",
    "import_location",
    "import_census_tract",
    "import_ages",
    "count_all_import_attempts",
    "import_list",
  };


void Rule::add_rule_line(string line) {
  if (Rule::first_rule) {
    Rule::rule_list.clear();
    Rule::first_rule = 0;
  }
  Rule::rule_list.push_back(line);
}

void Rule::prepare_rules() {

  for (int n = 0; n < Rule::rule_list.size(); n++) {
    string linestr = Rule::rule_list[n];

    Rule* rule = new Rule(linestr);
    if (rule->parse()) {
      printf("Good RULE: "); rule->print();
      Rule::rules.push_back(rule);
    }
    else {
      FRED_VERBOSE(0, "RULE parse failed: %s\n", rule->get_err_msg().c_str());
      if (rule->is_warning()) {
	Utils::print_warning(rule->get_err_msg().c_str());
      }
      else {
	Utils::print_error(rule->get_err_msg().c_str());
      }
    }
  }

  printf("RULES found = %lu\n", Rule::rules.size());
  for (int i = 0; i < Rule::rules.size(); i++) {
    printf("RULE[%d]: ",i);
    Rule::rules[i]->print();
  }
  printf("\n");

  Rule::compiled_rules.clear();

  for (int i = 0; i < Rule::rules.size(); i++) {
    Rule::rules[i]->compile();
  }

  printf("\nCOMPILED RULES size = %d:\n", (int) Rule::compiled_rules.size());
  for (int i = 0; i < Rule::compiled_rules.size(); i++) {
    printf("%d: ", i);
    Rule::compiled_rules[i]->print();
    printf("\n");
  }

}

void Rule::print_warnings() {
  string msg;
  for (int i = 0; i < Rule::rules.size(); i++) {
    if (Rule::rules[i]->is_used()==false && Rule::rules[i]->get_hidden_by_rule()==NULL) {
      msg = "Ignoring rule (check for typos):\n  " + Rule::rules[i]->get_name();
      Utils::print_warning(msg);
    }
  }
}


///////// END STATIC MEMBERS


Rule::Rule(string s) {
  this->name = s;
  this->cond = "";
  this->cond_id = -1;
  this->state = "";
  this->state_id = -1;
  this->clause_str = "";
  this->clause = NULL;
  this->next_state = "";
  this->next_state_id = -1;
  this->action = "";
  this->action_id = -1;
  this->expression_str = "";
  this->expression = NULL;
  this->expression_str2 = "";
  this->expression2 = NULL;
  this->var = "";
  this->var_id = -1;
  this->list_var = "";
  this->list_var_id = -1;
  this->source_cond = "";
  this->source_cond_id = -1;
  this->source_state = "";
  this->source_state_id = -1;
  this->dest_state = "";
  this->dest_state_id = -1;
  this->network = "";
  this->network_id = -1;
  this->group = "";
  this->group_type_id = -1;
  this->err = "";
  this->preference = NULL;
  this->used = false;
  this->warning = false;
  this->global = false;
  this->hidden_by = NULL;

  this->action_rule = false;
  this->wait_rule = false;
  this->exposure_rule = false;
  this->next_rule = false;
  this->default_rule = false;
  this->schedule_rule = false;
}


double Rule::get_value(Person* person, Person* other) {

  if (this->action_id == Rule_Action::SET) {
    double value = 0.0;
    if (this->expression != NULL) {
      value = this->expression->get_value(person, other);
      FRED_VERBOSE(1,"Rule::get_value expr = |%s| person %d value %f\n",
		   this->expression->get_name().c_str(), person->get_id(), value);
    }
    else {
      value = 0.0;
      FRED_VERBOSE(1,"Rule::get_value expr = NULL  person %d value %f\n",
		   person->get_id(), value);
    }
    return value;
  }

  // test the clause for next_rules:
  if (this->next_rule) {
    if (this->clause==NULL || this->clause->get_value(person)) {
      double value = 1.0;
      if (this->expression != NULL) {
	value = this->expression->get_value(person);
      }
      return value;
    }
    else {
      return 0.0;
    }
  }

  return 0.0;
}
  
bool Rule::parse() {

  char line[FRED_STRING_SIZE];

  printf("RULE parse? |%s|\n", this->name.c_str());

  // parse the line into separate strings
  this->parts.clear();
  this->parts = Utils::get_string_vector(this->name, ' ');
  /*
  for (int i = 0; i < this->parts.size(); i++) {
    printf("parts[%d] = ", i); fflush(stdout);
    printf("|%s|\n", this->parts[i].c_str()); fflush(stdout);
  }
  */

  if (this->name.find("then wait(")!=string::npos) {
    return parse_wait_rule();
  }

  if (this->name.find("if exposed(")==0) {
    return parse_exposure_rule();
  }

  if (this->name.find("then next(")!=string::npos) {
    return parse_next_rule();
  }

  if (this->name.find("then default(")!=string::npos) {
    return parse_default_rule();
  }

  if (this->name.find(" then ")==string::npos) {
    this->err = "No THEN clause found\n  " + this->name;
    Utils::print_error(get_err_msg().c_str());
  }

  // if not one of the above, must be a action rule
  return parse_action_rule();

}

void Rule::print() {

  // printf("PRINT %s\n", this->name.c_str());
  // printf("next_rule = %d\n", this->next_rule);

 std:string current = this->cond + "," + this->state;

  if (this->is_wait_rule()) {
    cout << "if state(" << current
	 << ") then wait("
	 << this->expression_str << ")";
    cout << "\n";
  }

  if (this->exposure_rule) {
    cout << "if exposed(" << this->cond << ") then next(" 
	 << this->next_state << ")\n";
  }

  if (this->next_rule) {
    cout << "if state(" << this->cond << "," << this->state << ") and("
	 << this->clause_str << ") then next("
	 << this->next_state << ")"
	 << " with prob(" << this->expression_str << ")\n";
  }

  if (this->is_default_rule()) {
    printf("if state(%s) then default(%s)\n",
	   current.c_str(), this->next_state.c_str());
  }

  if (this->is_action_rule()) {
    printf("%s\n", this->name.c_str());
    return;
  }

}


bool Rule::compile() {

  // FRED_VERBOSE(0, "COMPILE RULE %s\n", this->name.c_str());

  this->action_id = -1;
  for (int i = 0; i < Rule::action_string.size(); i++) {
    if (this->action == Rule::action_string[i]) {
      this->action_id = i;
      break;
    }
  }

  FRED_VERBOSE(0, "COMPILING RULE %s action |%s| action_id %d\n\n",
	       this->name.c_str(), this->action.c_str(), this->action_id);

  // get cond_id
  this->cond_id = Condition::get_condition_id(this->cond);
  if (this->cond_id < 0) {
    FRED_VERBOSE(0, "COMPILE BAD COND: RULE %s\n", this->name.c_str());
    return false;
  }

  // EXPOSURE RULE
  if (this->is_exposure_rule()) {
    this->next_state_id = Condition::get_condition(this->cond_id)->get_state_from_name(this->next_state);
    if (0 <= this->next_state_id) {
      Rule::compiled_rules.push_back(this);
      FRED_VERBOSE(0, "COMPILED EXPOSURE RULE %s\n", this->name.c_str());
      return true;
    }
    else {
      FRED_VERBOSE(0, "COMPILE BAD NEXT_STATE: EXPOSURE RULE %s\n", this->name.c_str());
      return false;
    }
  }

  // get state id for all other rules
  this->state_id = Condition::get_condition(this->cond_id)->get_state_from_name(this->state);
  if (this->state_id < 0) {
    FRED_VERBOSE(0, "COMPILE BAD STATE: RULE %s\n", this->name.c_str());
    return false;
  }

  // WAIT RULE
  if (this->is_wait_rule()) {
    if (this->action=="wait") {
      FRED_VERBOSE(0, "COMPILE WAIT RULE %s\n", this->name.c_str());
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse()) {
	Rule::compiled_rules.push_back(this);
	FRED_VERBOSE(0, "COMPILED WAIT RULE %s\n", this->name.c_str());
	return true;
      }
      else{
	this->warning = this->expression->is_warning();
	delete this->expression;
	this->err = "Expression  " + this->expression_str + " not recognized:\n  " + this->name;
	if (is_warning()) {
	  Utils::print_warning(get_err_msg().c_str());
	}
	else {
	  Utils::print_error(get_err_msg().c_str());
	}
	return false;
      }
    }
    if (this->action=="wait_until") {
      FRED_VERBOSE(0, "COMPILE WAIT_UNTIL RULE %s\n", this->name.c_str());
      Rule::compiled_rules.push_back(this);
      FRED_VERBOSE(0, "COMPILED WAIT_UNTIL RULE %s\n", this->name.c_str());
      return true;
    }
  }

  // NEXT RULE
  if (this->is_next_rule()) {
    this->next_state_id = Condition::get_condition(this->cond_id)->get_state_from_name(this->next_state);
    if (0 <= this->next_state_id) {
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse()==false) {
	FRED_VERBOSE(0, "COMPILE BAD EXPR: RULE %s\n", this->name.c_str());
	return false;
      }
      if (this->clause_str != "") {
	this->clause = new Clause(this->clause_str);
	if (this->clause->parse()==false) {
	  FRED_VERBOSE(0, "COMPILE BAD CLAUSE: RULE %s\n", this->name.c_str());
	  return false;
	}
      }
      Rule::compiled_rules.push_back(this);
      FRED_VERBOSE(0, "COMPILED NEXT RULE %s\n", this->name.c_str());
      return true;
    }
    else {
      FRED_VERBOSE(0, "COMPILE BAD NEXT_STATE: NEXT RULE %s\n", this->name.c_str());
      return false;
    }
  }

  // DEFAULT RULE
  if (this->is_default_rule()) {
    this->next_state_id = Condition::get_condition(this->cond_id)->get_state_from_name(this->next_state);
    if (0 <= this->next_state_id) {
      Rule::compiled_rules.push_back(this);
      FRED_VERBOSE(0, "COMPILED DEFAULT RULE %s\n", this->name.c_str());
      return true;
    }
    else {
      FRED_VERBOSE(0, "COMPILE BAD NEXT_STATE: DEFAULT RULE %s\n", this->name.c_str());
      return false;
    }
  }

  // ACTION RULES
  if (this->is_action_rule()) {
    return compile_action_rule();
  }

  FRED_VERBOSE(0,"COMPILE RULE UNKNOWN TYPE: |%s|\n", this->name.c_str());
  exit(0);
  return false;
}

void Rule::set_hidden_by_rule(Rule* rule) {
  this->hidden_by = rule;
  char msg[FRED_STRING_SIZE];
  sprintf(msg, "Ignoring duplicate rule:\n  %s\n     is hidden by:\n  %s",
	  get_name().c_str(),
	  rule->get_name().c_str());
  Utils::print_warning(msg);
}


bool Rule::parse_action_rule() {
  printf("ENTERED PARSE ACTION RULE: %s\n", this->name.c_str());
  if (parse_state() && this->parts[2]=="then" && this->parts.size()==4) {

    // re-write if () then sus()
    if (this->parts[3].find("sus(")==0) {
      this->parts[3].replace(0,4,"set_sus(" + this->cond + ",");
      this->name = this->parts[0] + " " + this->parts[1] + " " + this->parts[2] + " " + this->parts[3];
      printf("REWROTE RULE: |%s|\n", this->name.c_str());
    }

    // re-write if () then trans()
    if (this->parts[3].find("trans(")==0) {
      this->parts[3].replace(0,6,"set_trans(" + this->cond + ",");
      this->name = this->parts[0] + " " + this->parts[1] + " " + this->parts[2] + " " + this->parts[3];
      printf("REWROTE RULE: |%s|\n", this->name.c_str());
    }

    // re-write if () then mult_sus()
    if (this->parts[3].find("mult_sus(")==0) {
      int pos = this->parts[3].find(",");
      if (pos==string::npos) {
	return false;
      }
      std::string source = this->parts[3].substr(9,pos-9);
      this->parts[3] = "set_sus(" + source + "," + "susceptibility_to_" + source + "*" + this->parts[3].substr(pos+1);
      this->name = this->parts[0] + " " + this->parts[1] + " " + this->parts[2] + " " + this->parts[3];
      printf("REWROTE RULE: |%s|\n", this->name.c_str());
    }

    // re-write if () then mult_trans()
    if (this->parts[3].find("mult_trans(")==0) {
      int pos = this->parts[3].find(",");
      if (pos==string::npos) {
	return false;
      }
      std::string source = this->parts[3].substr(11,pos-11);
      this->parts[3] = "set_trans(" + source + "," + "transmissibility_for_" + source + "*" + this->parts[3].substr(pos+1);
      this->name = this->parts[0] + " " + this->parts[1] + " " + this->parts[2] + " " + this->parts[3];
      printf("REWROTE RULE: |%s|\n", this->name.c_str());
    }

    string str = this->parts[3];
    int pos = str.find("(");
    if (pos != string::npos && str[str.length()-1]==')') {
      this->action = str.substr(0,pos);
      this->expression_str = str.substr(pos+1,str.length()-pos-2);
      this->action_rule = true;
      return true;
    }
  }
  if (parse_state() && this->parts[3]=="then" && this->parts.size()==5) {

    // re-write if () then sus()
    if (this->parts[4].find("sus(")==0) {
      this->parts[4].replace(0,4,"set_sus(" + this->cond + ",");
      this->name = this->parts[0] + " " + this->parts[1] + " " + this->parts[2] + " " + this->parts[3] + " " + this->parts[4];
      printf("REWROTE RULE: |%s|\n", this->name.c_str());
    }

    // re-write if () then trans()
    if (this->parts[4].find("trans(")==0) {
      this->parts[4].replace(0,6,"set_trans(" + this->cond + ",");
      this->name = this->parts[0] + " " + this->parts[1] + " " + this->parts[2] + " " + this->parts[3] + " " + this->parts[4];
      printf("REWROTE RULE: |%s|\n", this->name.c_str());
    }

    // re-write if () then mult_sus()
    if (this->parts[4].find("mult_sus(")==0) {
      int pos = this->parts[4].find(",");
      if (pos==string::npos) {
	return false;
      }
      std::string source = this->parts[4].substr(9,pos-9);
      this->parts[4] = "set_sus(" + source + "," + "susceptibility_to_" + source + "*" + this->parts[4].substr(pos+1);
      this->name = this->parts[0] + " " + this->parts[1] + " " + this->parts[2] + " " + this->parts[3] + " " + this->parts[4];
      printf("REWROTE RULE: |%s|\n", this->name.c_str());
    }

    // re-write if () then mult_trans()
    if (this->parts[4].find("mult_trans(")==0) {
      int pos = this->parts[4].find(",");
      if (pos==string::npos) {
	return false;
      }
      std::string source = this->parts[4].substr(11,pos-11);
      this->parts[4] = "set_trans(" + source + "," + "transmissibility_for_" + source + "*" + this->parts[4].substr(pos+1);
      this->name = this->parts[0] + " " + this->parts[1] + " " + this->parts[2] + " " + this->parts[3] + " " + this->parts[4];
      printf("REWROTE RULE: |%s|\n", this->name.c_str());
    }

    string str = this->parts[2];
    if (str.find("and(")==0 && str[str.length()-1]==')') {
      this->clause_str = str.substr(4,str.length()-5);
    }
    else {
      printf("FAILED PARSE ACTION RULE: %s\n", this->name.c_str());
      return false;
    }

    str = this->parts[4];
    int pos = str.find("(");
    if (pos != string::npos && str[str.length()-1]==')') {
      this->action = str.substr(0,pos);
      this->expression_str = str.substr(pos+1,str.length()-pos-2);
      this->action_rule = true;
      return true;
    }
  }
  printf("FAILED PARSE ACTION RULE: %s\n", this->name.c_str());
  return false;
}

bool Rule::parse_wait_rule() {
  printf("ENTERED PARSE WAIT RULE: %s\n", this->name.c_str());
  if (parse_state() && this->parts[2]=="then") {
    string str = this->parts[3];
    if (str.find("wait(")==0 && str[str.length()-1]==')') {
      string arg = str.substr(5,str.length()-6);
      if (arg=="") {
	arg = "999999";
      }
      this->wait_rule = true;
      if (str.find("wait(until_")==0) {
	this->expression_str = arg.substr(6);
	this->action = "wait_until";
      }
      else {
	this->expression_str = arg;
	this->action = "wait";
      }
      return true;
    }
  }
  printf("FAILED WAIT RULE: %s\n", this->name.c_str());
  return false;
}

bool Rule::parse_exposure_rule() {
  printf("ENTERED PARSE EXPOSURE RULE: %s\n", this->name.c_str());
  for (int i = 0; i < this->parts.size(); i++) {
    printf("parts[%d] = ", i); fflush(stdout);
    printf("|%s|\n", this->parts[i].c_str()); fflush(stdout);
  }
  if (this->parts.size() != 4 || this->parts[0] != "if" || this->parts[2]!="then") {
    printf("parts size %d parts[0] |%s| parts[2] |%s|\n", (int) this->parts.size(), this->parts[0].c_str(), this->parts[2].c_str()); fflush(stdout);
    return false;
  }
  string str = this->parts[1];
  if (str.find("exposed(")==0 && str[str.length()-1]==')') {
    this->cond = str.substr(8,str.length()-9);
    FRED_VERBOSE(0, "exposued cond = %s\n", this->cond.c_str());

    // get next state
    str = this->parts[3];
    if (str.find("next(")==0 && str[str.length()-1]==')') {
      this->next_state = str.substr(5,str.length()-6);
      if (this->next_state=="") {
	this->err = "No Next State:\n  " + this->name;
	return false;
      }
      FRED_VERBOSE(0, "exposure next_state = %s\n", this->next_state.c_str());
      this->exposure_rule = true;
      return true;
    }
  }
  return false;
}

bool Rule::parse_next_rule() {
  printf("ENTERED PARSE NEXT RULE: %s\n", this->name.c_str());
  if (parse_state()) {
    this->expression_str = "";
    this->clause_str = "";
    int next_part = 3;
    string str = this->parts[2];
    if (str.find("and(")==0 && str[str.length()-1]==')') {
      this->clause_str = str.substr(4,str.length()-5);
      next_part++;
    }
    if (this->parts[next_part-1]!="then") {
      return false;
    }
    str = this->parts[next_part];
    if (str.find("next(")==0 && str[str.length()-1]==')') {
      this->next_state = str.substr(5,str.length()-6);
      if (this->next_state=="") {
	this->err = "No Next State:\n  " + this->name;
	return false;
      }
    }
    else {
      return false;
    }
    if (this->parts.size()==next_part+1) {
      this->expression_str = "1";
      this->next_rule = true;
      return true;
    }
    else {
      if (this->parts.size()!=next_part+3) {
	return false;
      }
      if (this->parts[next_part+1]!="with") {
	return false;
      }
      str = this->parts[next_part+2];
      if (str.find("prob(")==0 && str[str.length()-1]==')') {
	this->expression_str = str.substr(5,str.length()-6);
	if (this->expression_str =="") {
	  return false;
	}
	else {
	  this->next_rule = true;
	  return true;
	}
      }
      else {
	return false;
      }
    }
  }
  return false;
}


bool Rule::parse_default_rule() {
  printf("ENTERED PARSE DEFAULT RULE: %s\n", this->name.c_str());
  if (parse_state() && this->parts[2]=="then") {
    string str = this->parts[3];
    if (str.find("default(")==0 && str[str.length()-1]==')') {
      this->next_state = str.substr(8,str.length()-9);
      if (this->next_state=="") {
	this->err = "No Next State:\n  " + this->name;
	return false;
      }
      this->default_rule = true;
      return true;
    }
  }
  this->err = "Can't parse rule:\n  " + this->name;
  Utils::print_error(get_err_msg().c_str());
  return false;
}


bool Rule::parse_state() {
  if (this->parts.size() < 4 || this->parts[0] != "if") {
    this->err = "Can't parse rule:\n  " + this->name;
    Utils::print_error(get_err_msg().c_str());
    return false;
  }
  string str = this->parts[1];;
  if ((str.find("state(")==0 || str.find("enter(")==0)&& str[str.length()-1]==')') {
    string arg = str.substr(6,str.length()-7);
    int pos = arg.find(".");
    if (pos!=string::npos) {
      this->cond = arg.substr(0,pos);
      this->state = arg.substr(pos+1);
      return true;
    }
    pos = arg.find(",");
    if (pos!=string::npos) {
      this->cond = arg.substr(0,pos);
      this->state = arg.substr(pos+1);
      return true;
    }
  }
  this->err = "Can't parse rule:\n  " + this->name;
  Utils::print_error(get_err_msg().c_str());
  return false;
}


bool Rule::compile_action_rule() {

  FRED_VERBOSE(0, "COMPILING ACTION RULE %s action %s action_id %d\n",
	       this->name.c_str(), this->action.c_str(), this->action_id);

  if (this->clause_str != "") {
    this->clause = new Clause(this->clause_str);
    if (this->clause->parse()==false) {
      this->err = "Bad AND clause::\n  " + this->name;
      Utils::print_error(get_err_msg().c_str());
      return false;
    }
  }

  switch(this->action_id) {

  case Rule_Action::GIVE_BIRTH :
    {
      Rule::compiled_rules.push_back(this);
      FRED_VERBOSE(0, "COMPILED ACTION RULE %s\n", this->name.c_str());
      return true;
    }
    break;

  case Rule_Action::DIE :
  case Rule_Action::DIE_OLD :
    {
      Rule::compiled_rules.push_back(this);
      FRED_VERBOSE(0, "COMPILED ACTION RULE %s\n", this->name.c_str());
      return true;
    }
    break;

  case Rule_Action::JOIN :
    {
      string_vector_t args = Utils::get_top_level_parse(this->expression_str,',');
      if (args.size() > 1) {
	this->expression2 = new Expression(args[1]);
	if (this->expression2->parse()==false) {
	  this->err = "Second arg to join " + args[1] + " not recognized:\n  " + this->name;
	  Utils::print_error(get_err_msg().c_str());
	  return false;
	}
      }
      this->group = args[0];
      this->group_type_id = Group_Type::get_type_id(group);
      if (Group::is_a_network(group_type_id)) {
	this->network = this->group;
      }
      if (Group::is_a_network(group_type_id) || Group::is_a_place(group_type_id)) {
	Rule::compiled_rules.push_back(this);
	FRED_VERBOSE(0, "COMPILED ACTION RULE %s\n", this->name.c_str());
	return true;
      }
      else {
	this->err = "Group " + this->group + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
    }
    break;

  case Rule_Action::QUIT :
    {
      this->group = this->expression_str;
      this->group_type_id = Group_Type::get_type_id(group);
      if (Group::is_a_network(group_type_id)) {
	this->network = this->group;
      }
      if (Group::is_a_network(group_type_id) || Group::is_a_place(group_type_id)) {
	Rule::compiled_rules.push_back(this);
	FRED_VERBOSE(0, "COMPILED ACTION RULE %s\n", this->name.c_str());
	return true;
      }
      else {
	this->err = "Group " + this->group + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
    }
    break;

  case Rule_Action::ADD_EDGE_FROM :
  case Rule_Action::ADD_EDGE_TO :
  case Rule_Action::DELETE_EDGE_FROM :
  case Rule_Action::DELETE_EDGE_TO :
    {
      string_vector_t args = Utils::get_top_level_parse(this->expression_str,',');
      if (args.size()!=2) {
	this->err = "Needs 2 arguments:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->network = args[0];
      if (Network::get_network(this->network)==NULL) {
	this->err = "Network " + this->network + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression_str = args[1];
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse()) {
	Rule::compiled_rules.push_back(this);
	FRED_VERBOSE(0, "COMPILED ACTION RULE %s\n", this->name.c_str());
	return true;
      }
      else {
	this->err = "Expression " + this->expression_str + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
    }
    break;

  case Rule_Action::SET_LIST :
    {
      string_vector_t args = Utils::get_top_level_parse(this->expression_str,',');
      if (args.size() != 2) {
	this->err = "Needs 2 arguments:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->list_var = args[0];
      this->list_var_id = Person::get_list_var_id(list_var);
      if (this->list_var_id < 0) {
	this->list_var_id = Person::get_global_list_var_id(list_var);
	if (this->list_var_id < 0) {
	  this->err = "List_var " + this->list_var + " not recognized:\n  " + this->name;
	  Utils::print_error(get_err_msg().c_str());
	  return false;
	}
	else {
	  this->global = true;
	}
      }
      this->expression_str = args[1];
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse()) {
	if (this->expression->is_list_expression()) {
	  Rule::compiled_rules.push_back(this);
	  FRED_VERBOSE(0, "COMPILED ACTION RULE %s\n", this->name.c_str());
	  return true;
	}
	else {
	  this->err = "Need a list-valued expression: " + this->expression_str + " not recognized:\n  " + this->name;
	  Utils::print_error(get_err_msg().c_str());
	  return false;
	}
      }
      else {
	this->err = "Expression " + this->expression_str + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
    }
    break;

  case Rule_Action::SET_WEIGHT :
    {
      string_vector_t args = Utils::get_top_level_parse(this->expression_str,',');
      if (args.size() != 3) {
	this->err = "Needs 3 arguments:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->network = args[0];
      if (Network::get_network(this->network)==NULL) {
	this->err = "Network " + this->network + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression_str = args[1];
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse()==false) {
	this->err = "Person Expression " + this->expression_str + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression_str2 = args[2];
      this->expression2 = new Expression(this->expression_str2);
      if (this->expression2->parse()==false) {
	this->err = "Value Expression " + this->expression_str2 + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      Rule::compiled_rules.push_back(this);
      FRED_VERBOSE(0, "COMPILED ACTION RULE %s\n", this->name.c_str());
      return true;
    }
    break;

  case Rule_Action::REPORT :
    {
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse()) {
	Rule::compiled_rules.push_back(this);
	FRED_VERBOSE(0, "COMPILED ACTION RULE %s\n", this->name.c_str());
	return true;
      }
      else {
	this->err = "Expression " + this->expression_str + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
    }
    break;

  case Rule_Action::ABSENT :
  case Rule_Action::PRESENT :
  case Rule_Action::CLOSE :
    {
      if (this->expression_str == "") {
	// add all group types
	int types = Group_Type::get_number_of_group_types();
	for (int k = 0; k < types; k++) {
	  this->expression_str += Group_Type::get_group_type_name(k);
	  if (k < types-1) {
	    this->expression_str += ",";
	  }
	}
      }
      // verify group names in expression_str
      string_vector_t group_type_vec = Utils::get_string_vector(this->expression_str,',');
      for (int k = 0; k < group_type_vec.size(); k++) {
	string group_name = group_type_vec[k];
	int type_id = Group_Type::get_type_id(group_name);
	if (type_id < 0) {
	  this->err = "Group name " + group_name + " not recognized:\n  " + this->name;
	  Utils::print_error(get_err_msg().c_str());
	  return false;
	}
      }
      this->schedule_rule = true;
      Rule::compiled_rules.push_back(this);
      // FRED_VERBOSE(0, "COMPILED RULES size %d\n", (int) Rule::compiled_rules.size());
      return true;
    }
    break;

  case Rule_Action::RANDOMIZE_NETWORK :
    {
      string_vector_t args = Utils::get_top_level_parse(this->expression_str,',');
      if (args.size() == 3) {
	this->err = "Needs 3 arguments:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->network = args[0];
      if (Network::get_network(this->network)==NULL) {
	this->err = "Network " + this->network + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression = new Expression(args[1]);
      if (this->expression->parse()==false) {
	this->err = "Mean degree expression " + this->expression->get_name() + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression2 = new Expression(args[2]);
      if (this->expression2->parse()==false) {
	this->err = "Max degree expression " + this->expression2->get_name() + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      Rule::compiled_rules.push_back(this);
      FRED_VERBOSE(0, "COMPILED RANDOMIZE RULE %s\n", this->name.c_str());
      return true;
    }
    break;


  case Rule_Action::SET :
    {
      FRED_VERBOSE(0, "COMPILE SET RULE %s\n", this->name.c_str());
      this->global = false;
      string_vector_t args = Utils::get_top_level_parse(this->expression_str,',');
      if (args.size() < 2) {
	this->err = "Action set needs two arguments::\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->var = args[0];
      FRED_VERBOSE(0, "COMPILE SET RULE var |%s|\n", this->var.c_str());
      this->global = false;
      this->var_id = Person::get_var_id(var);
      if (this->var_id < 0) {
	this->var_id = Person::get_global_var_id(var);
	if (this->var_id < 0) {
	  this->err = "Var " + this->var + " not recognized:\n  " + this->name;
	  Utils::print_error(get_err_msg().c_str());
	  return false;
	}
	else {
	  this->global = true;
	}
      }
      this->expression_str = args[1];
      FRED_VERBOSE(0, "COMPILE SET RULE expression_str |%s|\n", this->expression_str.c_str());
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse()) {
	if (args.size()==2) {
	  Rule::compiled_rules.push_back(this);
	  FRED_VERBOSE(0, "COMPILED SET RULE %s with expression |%s|\n", this->name.c_str(), this->expression->get_name().c_str());
	  return true;
	}
	else {
	  this->expression_str2 = args[2];
	  FRED_VERBOSE(0, "COMPILE SET RULE expression_str |%s|  expression_str2 |%s|\n", this->expression_str.c_str(), this->expression_str2.c_str());
	  this->expression2 = new Expression(this->expression_str2);
	  if (this->expression2->parse()) {
	    Rule::compiled_rules.push_back(this);
	    FRED_VERBOSE(0, "COMPILED SET RULE %s with expressions |%s| |%s|\n", this->name.c_str(), this->expression->get_name().c_str(), this->expression2->get_name().c_str());
	    return true;
	  }
	  else {
	    this->warning = this->expression2->is_warning();
	    delete this->expression2;
	    this->err = "Expression  " + this->expression_str2 + " not recognized:\n  " + this->name;
	    if (is_warning()) {
	      Utils::print_warning(get_err_msg().c_str());
	    }
	    else {
	      Utils::print_error(get_err_msg().c_str());
	    }
	    return false;
	  }
	}
      }
      else {
	this->warning = this->expression->is_warning();
	delete this->expression;
	this->err = "Expression  " + this->expression_str + " not recognized:\n  " + this->name;
	if (is_warning()) {
	  Utils::print_warning(get_err_msg().c_str());
	}
	else {
	  Utils::print_error(get_err_msg().c_str());
	}
	return false;
      }
      break;
    }

  case Rule_Action::SET_STATE :
  case Rule_Action::CHANGE_STATE :
    {
      this->action_id = Rule_Action::SET_STATE;
      this->action = "set_state";
      string_vector_t args = Utils::get_top_level_parse(this->expression_str,',');
      if (args.size() != 3) {
	this->err = "Expression  " + this->expression_str + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->source_cond = args[0];
      this->source_cond_id = Condition::get_condition_id(this->source_cond);
      if (this->source_cond_id < 0) {
	this->err = "Source condition  " + source_cond + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->source_state = args[1];
      this->source_state_id = Condition::get_condition(this->source_cond_id)->get_state_from_name(this->source_state);
      if (this->source_state_id < 0) {
	this->err = "Source state  " + source_state + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->dest_state = args[2];
      this->dest_state_id = Condition::get_condition(this->source_cond_id)->get_state_from_name(this->dest_state);
      if (this->dest_state_id < 0) {
	this->err = "Destination state  " + this->dest_state + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      Rule::compiled_rules.push_back(this);
      // FRED_VERBOSE(0, "COMPILED RULES size %d\n", (int) Rule::compiled_rules.size());
      return true;
    }
    break;

  case Rule_Action::SUS :
    {
      FRED_VERBOSE(0, "COMPILE SUS RULE %s\n", this->name.c_str());
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse()) {
	Rule::compiled_rules.push_back(this);
	// FRED_VERBOSE(0, "COMPILED RULES size %d\n", (int) Rule::compiled_rules.size());
	FRED_VERBOSE(0, "COMPILED SUS RULE %s\n", this->name.c_str());
	return true;
      }
      else {
	this->warning = this->expression->is_warning();
	delete this->expression;
	this->err = "Expression  " + this->expression_str + " not recognized:\n  " + this->name;
	if (is_warning()) {
	  Utils::print_warning(get_err_msg().c_str());
	}
	else {
	  Utils::print_error(get_err_msg().c_str());
	}
	return false;
      }
    }
    break;

  case Rule_Action::SET_SUS :
    {
      FRED_VERBOSE(0, "COMPILE SET_SUS RULE %s\n", this->name.c_str());
      string_vector_t args = Utils::get_top_level_parse(this->expression_str,',');
      if (args.size() != 2) {
	return false;
      }
      this->source_cond = args[0];
      FRED_VERBOSE(0, "COMPILE SET_SUS RULE %s  cond |%s|\n", this->name.c_str(), this->source_cond.c_str());
      this->source_cond_id = Condition::get_condition_id(this->source_cond);
      if (this->source_cond_id < 0) {
	this->err = "Source condition  " + source_cond + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression_str2 = args[1];
      this->expression2 = new Expression(this->expression_str2);
      if (this->expression2->parse()) {
	Rule::compiled_rules.push_back(this);
	// FRED_VERBOSE(0, "COMPILED RULES size %d\n", (int) Rule::compiled_rules.size());
	FRED_VERBOSE(0, "COMPILED SET_SUS RULE %s\n", this->name.c_str());
	return true;
      }
      else {
	this->warning = this->expression2->is_warning();
	delete this->expression2;
	this->err = "Expression  " + this->expression_str2 + " not recognized:\n  " + this->name;
	if (is_warning()) {
	  Utils::print_warning(get_err_msg().c_str());
	}
	else {
	  Utils::print_error(get_err_msg().c_str());
	}
	return false;
      }
    }
    break;

  case Rule_Action::SET_TRANS :
    {
      FRED_VERBOSE(0, "COMPILE SET_TRANS RULE %s\n", this->name.c_str());
      string_vector_t args = Utils::get_top_level_parse(this->expression_str,',');
      if (args.size() != 2) {
	this->err = "Expression  " + this->expression_str + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->source_cond = args[0];
      FRED_VERBOSE(0, "COMPILE SET_TRANS RULE %s  cond |%s|\n", this->name.c_str(), this->source_cond.c_str());
      this->source_cond_id = Condition::get_condition_id(this->source_cond);
      if (this->source_cond_id < 0) {
	this->err = "Source condition  " + source_cond + " not recognized:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression_str2 = args[1];
      this->expression2 = new Expression(this->expression_str2);
      if (this->expression2->parse()) {
	Rule::compiled_rules.push_back(this);
	// FRED_VERBOSE(0, "COMPILED RULES size %d\n", (int) Rule::compiled_rules.size());
	FRED_VERBOSE(0, "COMPILED SET_TRANS RULE %s\n", this->name.c_str());
	return true;
      }
      else {
	this->warning = this->expression2->is_warning();
	delete this->expression2;
	this->err = "Expression  " + this->expression_str2 + " not recognized:\n  " + this->name;
	if (is_warning()) {
	  Utils::print_warning(get_err_msg().c_str());
	}
	else {
	  Utils::print_error(get_err_msg().c_str());
	}
	return false;
      }
    }
    break;

  case Rule_Action::TRANS :
    {
      FRED_VERBOSE(0, "COMPILE TRANS RULE %s\n", this->name.c_str());
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse()) {
	Rule::compiled_rules.push_back(this);
	// FRED_VERBOSE(0, "COMPILED RULES size %d\n", (int) Rule::compiled_rules.size());
	FRED_VERBOSE(0, "COMPILED TRANS RULE %s\n", this->name.c_str());
	return true;
      }
      else {
	this->warning = this->expression->is_warning();
	delete this->expression;
	this->err = "Expression  " + this->expression_str + " not recognized:\n  " + this->name;
	if (is_warning()) {
	  Utils::print_warning(get_err_msg().c_str());
	}
	else {
	  Utils::print_error(get_err_msg().c_str());
	}
	return false;
      }
    }
    break;

  case Rule_Action::IMPORT_COUNT :
    {
      FRED_VERBOSE(0, "COMPILE IMPORT RULE |%s|  expr |%s|\n", this->name.c_str(), this->expression_str.c_str());
      string_vector_t args = Utils::get_top_level_parse(this->expression_str, ',');
      if (args.size() != 1) {
	this->err = "Import_count rule needs 1 argument:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression = NULL;
      this->expression = new Expression(this->expression_str);
      this->expression_str = args[0];
      if (this->expression->parse()) {
	Rule::compiled_rules.push_back(this);
	FRED_VERBOSE(0, "COMPILED IMPORT_COUNT RULE %s\n", this->name.c_str());
	this->action = "import_count";
	return true;
      }
      else {
	this->warning = this->expression->is_warning();
	delete this->expression;
	this->err = "Expression  " + this->expression_str + " not recognized:\n  " + this->name;
	if (is_warning()) {
	  Utils::print_warning(get_err_msg().c_str());
	}
	else {
	  Utils::print_error(get_err_msg().c_str());
	}
	return false;
      }
      break;
    }

  case Rule_Action::IMPORT_PER_CAPITA :
    {
      FRED_VERBOSE(0, "COMPILE IMPORT RULE %s\n", this->name.c_str());
      string_vector_t args = Utils::get_top_level_parse(this->expression_str, ',');
      if (args.size() != 1) {
	this->err = "Import_per_capita rule needs 1 argument:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression_str = args[0];
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse()) {
	Rule::compiled_rules.push_back(this);
	FRED_VERBOSE(0, "COMPILED IMPORT_PER_CAPITA RULE %s\n", this->name.c_str());
	this->action = "import_per_capita";
	return true;
      }
      else {
	this->warning = this->expression->is_warning();
	delete this->expression;
	this->err = "Expression  " + this->expression_str + " not recognized:\n  " + this->name;
	if (is_warning()) {
	  Utils::print_warning(get_err_msg().c_str());
	}
	else {
	  Utils::print_error(get_err_msg().c_str());
	}
	return false;
      }
      break;
    }

  case Rule_Action::IMPORT_LOCATION :
    {
      FRED_VERBOSE(0, "COMPILE IMPORT RULE %s\n", this->name.c_str());
      string_vector_t args = Utils::get_top_level_parse(this->expression_str, ',');
      if (args.size() != 3) {
	this->err = "import_location rule needs 3 argument:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression_str = args[0];
      this->expression_str2 = args[1];
      this->expression_str3 = args[2];
      this->expression = new Expression(this->expression_str);
      this->expression2 = new Expression(this->expression_str2);
      this->expression3 = new Expression(this->expression_str3);
      if (this->expression->parse()) {
	if (this->expression2->parse()) {
	  if (this->expression3->parse()) {
	    Rule::compiled_rules.push_back(this);
	    FRED_VERBOSE(0, "COMPILED IMPORT_LOCATION RULE %s\n", this->name.c_str());
	    this->action = "import_location";
	    return true;
	  }
	  else {
	    this->warning = this->expression3->is_warning();
	    delete this->expression3;
	    this->err = "Expression  " + this->expression_str3 + " not recognized:\n  " + this->name;
	    if (is_warning()) {
	      Utils::print_warning(get_err_msg().c_str());
	    }
	    else {
	      Utils::print_error(get_err_msg().c_str());
	    }
	    return false;
	  }
	}
	else {
	  this->warning = this->expression2->is_warning();
	  delete this->expression2;
	  this->err = "Expression  " + this->expression_str2 + " not recognized:\n  " + this->name;
	  if (is_warning()) {
	    Utils::print_warning(get_err_msg().c_str());
	  }
	  else {
	    Utils::print_error(get_err_msg().c_str());
	  }
	  return false;
	}
      }
      else {
	this->warning = this->expression->is_warning();
	delete this->expression;
	this->err = "Expression  " + this->expression_str + " not recognized:\n  " + this->name;
	if (is_warning()) {
	  Utils::print_warning(get_err_msg().c_str());
	}
	else {
	  Utils::print_error(get_err_msg().c_str());
	}
	return false;
      }
      break;
    }

  case Rule_Action::IMPORT_CENSUS_TRACT :
    {
      FRED_VERBOSE(0, "COMPILE IMPORT RULE %s\n", this->name.c_str());
      string_vector_t args = Utils::get_top_level_parse(this->expression_str, ',');
      if (args.size() != 1) {
	this->err = "Import_census_tract rule needs 1 argument:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression_str = args[0];
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse()) {
	Rule::compiled_rules.push_back(this);
	FRED_VERBOSE(0, "COMPILED IMPORT_CENSUS_TRACT RULE %s\n", this->name.c_str());
	this->action = "import_census_tract";
	return true;
      }
      else {
	this->warning = this->expression->is_warning();
	delete this->expression;
	this->err = "Expression  " + this->expression_str + " not recognized:\n  " + this->name;
	if (is_warning()) {
	  Utils::print_warning(get_err_msg().c_str());
	}
	else {
	  Utils::print_error(get_err_msg().c_str());
	}
	return false;
      }
      break;
    }

  case Rule_Action::IMPORT_AGES :
    {
      FRED_VERBOSE(0, "COMPILE IMPORT RULE %s\n", this->name.c_str());
      string_vector_t args = Utils::get_top_level_parse(this->expression_str, ',');
      if (args.size() != 2) {
	this->err = "import_ages rule needs 2 argument:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression_str = args[0];
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse()) {
	this->expression_str2 = args[1];
	this->expression2 = new Expression(this->expression_str2);
	if (this->expression2->parse()) {
	  Rule::compiled_rules.push_back(this);
	  FRED_VERBOSE(0, "COMPILED IMPORT_AGES RULE %s\n", this->name.c_str());
	  this->action = "import_ages";
	  return true;
	}
	else {
	  this->warning = this->expression2->is_warning();
	  delete this->expression2;
	  this->err = "Expression  " + this->expression_str2 + " not recognized:\n  " + this->name;
	  if (is_warning()) {
	    Utils::print_warning(get_err_msg().c_str());
	  }
	  else {
	    Utils::print_error(get_err_msg().c_str());
	  }
	  return false;
	}
      }
      else {
	this->warning = this->expression->is_warning();
	delete this->expression;
	this->err = "Expression  " + this->expression_str + " not recognized:\n  " + this->name;
	if (is_warning()) {
	  Utils::print_warning(get_err_msg().c_str());
	}
	else {
	  Utils::print_error(get_err_msg().c_str());
	}
	return false;

      }
      break;
    }

  case Rule_Action::COUNT_ALL_IMPORT_ATTEMPTS :
    {
      FRED_VERBOSE(0, "COMPILE COUNT_ALL_IMPORT_ATTEMPTS RULE %s\n", this->name.c_str());
      if (this->expression_str=="") {
	Rule::compiled_rules.push_back(this);
	FRED_VERBOSE(0, "COMPILED COUNT_ALL_IMPORT_ATTEMPTS RULE %s\n", this->name.c_str());
	this->action = "count_all_import_attempts";
	return true;
      }
      else {
	this->err = "Count_all_import_attempts takes no arguments:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
    }

  case Rule_Action::IMPORT_LIST :
    {
      FRED_VERBOSE(0, "COMPILE IMPORT RULE %s\n", this->name.c_str());
      string_vector_t args = Utils::get_top_level_parse(this->expression_str, ',');
      if (args.size() != 1) {
	this->err = "Import_list rule needs 1 argument:\n  " + this->name;
	Utils::print_error(get_err_msg().c_str());
	return false;
      }
      this->expression_str = args[0];
      this->expression = new Expression(this->expression_str);
      if (this->expression->parse() && this->expression->is_list_expression()) {
	Rule::compiled_rules.push_back(this);
	FRED_VERBOSE(0, "COMPILED IMPORT_LIST RULE %s\n", this->name.c_str());
	this->action = "import_list";
	return true;
      }
      else {
	this->warning = this->expression->is_warning();
	delete this->expression;
	this->err = "Expression  " + this->expression_str + " not recognized:\n  " + this->name;
	if (is_warning()) {
	  Utils::print_warning(get_err_msg().c_str());
	}
	else {
	  Utils::print_error(get_err_msg().c_str());
	}
	return false;
      }
      break;
    }

  default:
    FRED_VERBOSE(0,"COMPILE RULE UNKNOWN ACTION ACTION: |%s|\n", this->name.c_str());
    this->err = "Unknown Rule Action:\n  " + this->name;
    Utils::print_error(get_err_msg().c_str());
    return false;
    
    break;
  }
}
