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

#include "Expression.h"
#include "Clause.h"
#include "Factor.h"
#include "Geo.h"
#include "Global.h"
#include "Person.h"
#include "Place.h"
#include "Place_Type.h"
#include "Preference.h"
#include "Random.h"
#include "Utils.h"

#define TWOARGS 11

std::map<std::string,int> Expression::op_map = {
  {"add", 1}, {"sub", 2}, {"mult", 3}, {"div", 4}, {"dist", 5}, {"equal", 6},
  {"min", 7}, {"max", 8}, {"uniform", 9}, {"normal", 10}, {"lognormal", 11},
  {"exponential", 12}, {"geometric", 13}, {"pow", 14}, {"log", 15}, {"exp", 16}, {"abs", 17},
  {"sin", 18}, {"cos", 19},
  {"pool", 20}, {"filter", 21}, {"list", 22}, {"value", 23}, {"distance",24},
};

std::map<std::string,int> Expression::value_map = {
  {"male", 1},
  {"female", 0},
  {"householder", 0},
  {"spouse", 1},
  {"child", 2},
  {"sibling", 3},
  {"parent", 4},
  {"grandchild", 5},
  {"in_law", 6},
  {"other_relative", 7},
  {"boarder", 8},
  {"housemate", 9},
  {"partner", 10},
  {"foster_child", 11},
  {"other_non_relative", 12},
  {"institutionalized_group_quarters_pop", 13},
  {"noninstitutionalized_group_quarters_pop", 14},
  {"unknown_race", -1},
  {"white", 1},
  {"african_american", 2},
  {"american_indian", 3},
  {"alaska_native", 4},
  {"tribal", 5},
  {"asian", 6},
  {"hawaiian_native", 7},
  {"other_race", 8},
  {"multiple_race", 9},
  {"infant", 0},
  {"preschool", 1},
  {"student", 2},
  {"teacher", 3},
  {"worker", 4},
  {"weekend_worker", 5},
  {"unemployed", 6},
  {"retired", 7},
  {"prisoner", 8},
  {"college_student", 9},
  {"military", 10},
  {"nursing_home_resident", 11},
  {"Sun", 0},
  {"Mon", 1},
  {"Tue", 2},
  {"Wed", 3},
  {"Thu", 4},
  {"Fri", 5},
  {"Sat", 6},
  {"Jan", 1},
  {"Feb", 2},
  {"Mar", 3},
  {"Apr", 4},
  {"May", 5},
  {"Jun", 6},
  {"Jul", 7},
  {"Aug", 8},
  {"Sep", 9},
  {"Oct", 10},
  {"Nov", 11},
  {"Dec", 12},
};


Expression::Expression(string s) {
  this->name = convert_infix_to_prefix(Utils::delete_spaces(s));
  // printf("CONSTRUCTOR EXPRESSION: s = |%s| name = |%s|\n", s.c_str(), this->name.c_str());
  this->expr1 = NULL;
  this->expr2 = NULL;
  this->expr3 = NULL;
  this->expr4 = NULL;
  this->factor = NULL;
  this->op = "";
  this->op_index = 0;
  this->number_of_expressions = 0;
  this->number = 0.0;
  this->use_other = false;
  this->warning = false;
  this->is_select = false;
  this->list_var = "";
  this->list_var_id = -1;
  this->pref_str = "";
  this->preference = NULL;
  this->is_list_var = false;
  this->is_global = false;
  this->is_pool = false;
  this->pool.clear();
  this->is_filter = false;
  this->is_list_expr = false;
  this->is_list = false;
  this->is_value = false;
  this->is_distance = false;
}


string Expression::get_name() {
  return this->name;
}


string Expression::expand_minus(string s) {
  // printf("expand_minus entered |%s|\n", s.c_str());
  string result = "";
  int size = s.length();
  if (size==0) {
    return result;
  }
  // position of next token
  int next_pos = 0;
  while (next_pos < s.length()) {
    string token = get_next_token(s,next_pos);
    next_pos += token.length();

    if (token != "#") {
      // case: non-#
      result += token;
    }
    else if (size <= next_pos) {
      this->minus_err = "unary minus at end of string: |" + s + "|"; 
      return "";
    }
    else if (s[next_pos]=='(') {
      // case: #(...)
      // find matching right paren
      int j = next_pos+1;
      int level = 1;
      while (j < size && level!=0) {
	if (s[j]=='(')
	  level++;
	if (s[j]==')')
	  level--;
	j++;
      }
      if (j==size && level!=0) {
	this->minus_err = "ill-formed expression missing right paren: |"+s+"|";
	return "";
      }
      string sub = "";
      if (j < size) {
	sub = s.substr(next_pos+1,j-next_pos-1);
      }
      else {
	sub = s.substr(next_pos+1);
      }
      result += "(0-" + expand_minus(sub) + ")";
      next_pos += sub.length();
    }
    else {
      // case: ## or #operand or #function(...)
      string next = get_next_token(s,next_pos);
      next_pos += next.length();
      string sub = "";
      if (next=="#") {
	// case: ##
	while (next=="#") {
	  sub += next;
	  next = get_next_token(s,next_pos);
	  next_pos += next.length();
	}
	sub += next;
	if (is_function(next)) {
	  // find args of func
	  int start = next_pos;
	  int end = start;
	  while (end < size && s[end]!=')') end++;
	  if (end==size) {
	    this->minus_err = "ill-formed expression missing right paren: |"+s+"|";
	    return "";
	  }
	  string arg = s.substr(next_pos+1,end-next_pos-1);
	  sub +=  "(" + expand_minus(arg) + ")";
	  next_pos += end+1;
	}
	string inner = expand_minus(sub);
	result += "(0-" + inner + ")";
      }
      else {
	if (is_function(next)) {
	  // case: #function(...)

	  // find args of func
	  int start = next_pos;
	  int end = start;
	  while (end<size && s[end]!=')') end++;
	  if (end==size) {
	    this->minus_err = "ill-formed expression missing right paren: |"+s+"|";
	    return "";
	  }
	  string arg = s.substr(next_pos+1,end-next_pos-1);
	  result += "(0-" + next + "(" + expand_minus(arg) + "))";
	  next_pos += end+1;
	}
	else {
	  // case: #operand
	  result += "(0-" + next + ")";
	}
      }
    }
  }
  return result;
}

string Expression::get_next_token(string s, int pos) {
  // printf("get_next_token |%s| pos = %d\n", s.c_str(),pos);
  int pos2 = s.find_first_of(",+-*/%^()#", pos);
  string token = "";
  if (pos2!=string::npos) {
    if (pos==pos2) {
      token = s.substr(pos,1);
    }
    else {
      token = s.substr(pos,pos2-pos);
    }
  }
  else {
    token = s.substr(pos);
  }
  return token;
}

string Expression::expand_operator(string s) {
  if (s=="+") {
    return "add";
  }
  if (s=="-") {
    return "sub";
  }
  if (s=="*") {
    return "mult";
  }
  if (s=="/") {
    return "div";
  }
  /*
    if (s=="%") {
    return "mod";
    }
  */
  if (s=="^") {
    return "pow";
  }
  return s;
}

// check if given string is an operator or not. 
bool Expression::is_operator(string s) { 
  return (s=="+" || s=="-" || s=="*" || s=="/" || s=="%" || s=="^" || s=="#");
} 

// check if given string is a function or not. 
bool Expression::is_function(string s) { 
  return (s=="," || s=="select" || s=="pref" || Expression::is_known_function(s));
} 

int Expression::get_number_of_args(string s) { 
  if (s==",")
    return 2;
  else
    return 1;
}


// Function to find priority of given 
// operator. 
int Expression::get_priority(string s) { 
  if (s=="#" || s == "-" || s == "+") 
    return 2; 
  else if (s == "*" || s == "/") 
    return 3; 
  else if (s == "^") 
    return 4; 
  else if (s == ",") 
    return 1; 
  else if (is_function(s))
    return 5;
  return 0; 
} 

// Function that converts infix 
// expression to prefix expression. 
string Expression::infix_to_prefix(string infix) { 

  // stack for operators. 
  std::stack<string> operators; 

  // stack for operands. 
  std::stack<string> operands; 

  // position of next token
  int next_pos = 0;
  while (next_pos < infix.length()) {
    string token = get_next_token(infix,next_pos);
    // printf("|%s|\n", token.c_str());
    next_pos += token.length();

    // if current token is an opening bracket, then push into the
    // operators stack.
    if (token == "(") { 
      // printf("token is (\n");
      operators.push(token); 
      // printf("push operator %s\n", token.c_str()); fflush(stdout);
    } 

    // if current token is a closing bracket, then pop from both stacks
    // and push result in operands stack until matching opening bracket
    // is not found.
    else if (token == ")") { 
      // printf("token is )\n");
      while (!operators.empty() && 
	     operators.top() != "(") { 

	// operator 
	if (operators.empty()) {
	  this->minus_err = "ill-formed expression missing operator: |"+infix+"|";
	  return "";
	}
	string op = operators.top(); 
	// printf("popped operator %s\n", op.c_str());
	operators.pop(); 

	if (is_function(op)) {
	  int nargs = get_number_of_args(op);
	  // printf("1 func %s nargs %d\n", op.c_str(),nargs); fflush(stdout);
	  if (nargs==1) {
	    if (operands.empty()) {
	      this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	      return "";
	    }
	    string op1 = operands.top(); 
	    operands.pop(); 
	    string tmp = op + "(" + op1 + ")";
	    operands.push(tmp); 
	    // printf("1 push operands %s\n", tmp.c_str()); fflush(stdout);
	  }
	  if (nargs==2){
	    if (operands.empty()) {
	      this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	      return "";
	    }
	    string op1 = operands.top(); 
	    operands.pop(); 
	    if (operands.empty()) {
	      this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	      return "";
	    }
	    string op2 = operands.top(); 
	    operands.pop(); 
	    string tmp;
	    if (op==",") {
	      tmp = op2 + "," + op1; 
	    }
	    else {
	      tmp = op + "(" + op2 + "," + op1 + ")"; 
	    }
	    operands.push(tmp); 
	    // printf("2 push operands %s\n", tmp.c_str()); fflush(stdout);
	  }
	}
	else {
	  string oper = expand_operator(op);
	  // printf("1 operator |%s| => |%s|\n", op.c_str(), oper.c_str());
	  if (operands.empty()) {
	    this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	    return "";
	  }
	  string op1 = operands.top(); 
	  operands.pop(); 
	  string tmp;
	  if (operands.empty()) {
	    this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	    return "";
	  }
	  string op2 = operands.top(); 
	  operands.pop(); 
	  tmp = oper + "(" + op2 + "," + op1 + ")"; 
	  operands.push(tmp); 
	  // printf("3 push operands %s\n", tmp.c_str()); fflush(stdout);
	}
      } 

      // Pop opening bracket from stack.
      operators.pop(); 
    } 

    // If current token is an operand then push it into operands stack.
    else if (!is_operator(token) && (!is_function(token))) { 
      operands.push(token); 
      // printf("4 push operands %s\n", token.c_str()); fflush(stdout);
    } 

    // If current token is an operator, then push it into operators
    // stack after popping high priority operators from operators stack
    // and pushing result in operands stack.
    else { 
      while (!operators.empty() && 
	     get_priority(token) <= 
	     get_priority(operators.top())) { 

	string op = operators.top(); 
	operators.pop(); 

	if (is_function(op)) {
	  int nargs = get_number_of_args(op);
	  // printf("2 func %s nargs %d\n", op.c_str(),nargs);  fflush(stdout);
	  if (nargs==1) {
	    if (operands.empty()) {
	      this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	      return "";
	    }
	    string op1 = operands.top(); 
	    operands.pop(); 
	    string tmp = op + "(" + op1 + ")";
	    operands.push(tmp); 
	    // printf("5 push operands %s\n", tmp.c_str()); fflush(stdout);
	  }
	  if (nargs==2){
	    if (operands.empty()) {
	      this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	      return "";
	    }
	    string op1 = operands.top(); 
	    operands.pop(); 
	    if (operands.empty()) {
	      this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	      return "";
	    }
	    string op2 = operands.top(); 
	    operands.pop(); 
	    string tmp;
	    if (op==",") {
	      tmp = op2 + "," + op1; 
	    }
	    else {
	      tmp = op + "(" + op2 + "," + op1 + ")"; 
	    }
	    operands.push(tmp); 
	    // printf("6 push operands %s\n", tmp.c_str()); fflush(stdout);
	  }
	}
	else {
	  string oper = expand_operator(op);
	  // printf("2 operator |%s| => |%s|\n", op.c_str(), oper.c_str());
	  if (operands.empty()) {
	    this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	    return "";
	  }
	  string op1 = operands.top(); 
	  operands.pop(); 
	  if (operands.empty()) {
	    this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	    return "";
	  }
	  string op2 = operands.top(); 
	  operands.pop(); 
	  string tmp = oper + "(" + op2 + "," + op1 + ")"; 
	  operands.push(tmp); 
	  // printf("7 push operands %s\n", tmp.c_str()); fflush(stdout);
	}
      } 
      operators.push(token); 
      // printf("push operators %s\n", token.c_str()); fflush(stdout);
    } 
  } 

  // pop operators from operators stack until it is empty and add result
  // of each pop operation in operands stack.
  while (!operators.empty()) { 
    string op = operators.top(); 
    operators.pop(); 
    if (is_function(op)) {
      int nargs = get_number_of_args(op);
      // printf("3 func %s nargs %d\n", op.c_str(),nargs); fflush(stdout);
      if (nargs==1) {
	if (operands.empty()) {
	  this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	  return "";
	}
	string op1 = operands.top(); 
	operands.pop(); 
	string tmp = op + "(" + op1 + ")";
	operands.push(tmp); 
	// printf("8 push operands %s\n", tmp.c_str()); fflush(stdout);
      }
      if (nargs==2){
	if (operands.empty()) {
	  this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	  return "";
	}
	string op1 = operands.top(); 
	operands.pop(); 
	if (operands.empty()) {
	  this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	  return "";
	}
	string op2 = operands.top(); 
	operands.pop(); 
	string tmp;
	if (op==",") {
	  tmp = op2 + "," + op1; 
	}
	else {
	  tmp = op + "(" + op2 + "," + op1 + ")"; 
	}
	operands.push(tmp); 
	// printf("9 push operands %s\n", tmp.c_str()); fflush(stdout);
      }
    }
    else {
      string oper = expand_operator(op);
      // printf("3 operator |%s| => |%s|\n", op.c_str(), oper.c_str());
      if (operands.empty()) {
	this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	return "";
      }
      string op1 = operands.top(); 
      operands.pop(); 
      if (operands.empty()) {
	this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
	return "";
      }
      string op2 = operands.top(); 
      operands.pop(); 
      string tmp = oper + "(" + op2 + "," + op1 + ")"; 
      operands.push(tmp); 
      // printf("8 push operands %s\n", tmp.c_str()); fflush(stdout);
    }
  } 

  // final prefix expression is present in operands stack.
  if (operands.empty()) {
    this->minus_err = "ill-formed expression missing operand: |"+infix+"|";
    return "";
  }
  return operands.top(); 
} 

string Expression::replace_unary_minus(string s) {
  string result = "";
  // position of next token
  bool last = true;
  int next_pos = 0;
  while (next_pos < s.length()) {
    string token = get_next_token(s,next_pos);
    // printf("|%s|\n", token.c_str()); fflush(stdout);
    next_pos += token.length();
    if (last && token=="-") {
      token = "#";
    }
    else {
      last = is_operator(token) || token=="(" || token==",";
    }
    result += token;
    // printf("result=|%s|\n",result.c_str());fflush(stdout);
  }
  return result;
}

string Expression::convert_infix_to_prefix(string infix) {
  if (infix=="") {
    return infix;
  }
  this->minus_err = "";
  string changed = replace_unary_minus(infix);
  if(this->minus_err!="") {
    printf("Error: %s\n", this->minus_err.c_str());
    return "???";
  }
  else {
    // printf("|%s| => |%s| => ", infix.c_str(), changed.c_str());
    string expanded = expand_minus(changed);
    if(this->minus_err!="") {
      printf("Error: %s\n", this->minus_err.c_str());
      return "???";
    }
    else {
      // printf("|%s| => ", expanded.c_str());
      string prefix = infix_to_prefix(expanded);
      if(this->minus_err!="") {
	printf("Error: %s\n", this->minus_err.c_str());
	return "???";
      }
      else {
	// printf("|%s|\n", prefix.c_str());
	return prefix;
      }
    }
  }
}

int Expression::find_comma(string s) {
  char x [FRED_STRING_SIZE];
  strcpy(x, s.c_str());
  int inside = 0;
  int n = 0;
  while (x[n]!='\0') {
    if (x[n]==',' && !inside) {
      return n;
    }
    if (x[n]=='(') {
      inside++;
    }
    if (x[n]==')') {
      inside--;
    }
    n++;
  }
  return -1;
}

double Expression::get_value(Person* person, Person* other) {

  FRED_VERBOSE(1, "Expr::get_value entered person %d other %d number_expr %d name %s factor %s\n",
	       person? person->get_id(): -1,
	       other? other->get_id(): -1,
	       this->number_of_expressions,
	       this->name.c_str(),
	       this->factor? this->factor->get_name().c_str() : "NULL");

  if (this->is_value) {
    int agent_id = this->expr1->get_value(person, other);
    Person* agent = Person::get_person_with_id(agent_id);
    // FRED_VERBOSE(0, "get value person %d other %d expr |%s|\n", person ? person->get_id() : -1, agent ? agent->get_id() : -1, this->expr2->get_name().c_str());
    double value = 0.0;
    if (agent!=NULL) {
      // printf("getting value for agent %d\n", agent->get_id());
      value = this->expr2->get_value(agent, NULL);
    }
    // FRED_VERBOSE(0, "get value person %d other %d expr |%s| value %f\n", person ? person->get_id() : -1, agent ? agent->get_id() : -1, this->expr2->get_name().c_str(), value);
    return value;
  }

  if (this->is_distance) {
    double lat1 = this->expr1->get_value(person,other);
    double lon1 = this->expr2->get_value(person,other);
    double lat2 = this->expr3->get_value(person,other);
    double lon2 = this->expr4->get_value(person,other);
    double value =  Geo::xy_distance(lat1,lon1,lat2,lon2);
    // printf("lat1 %f lon1 %f lat1 %f lon1 %f  value %f\n", lat1, lon1, lat2, lon2, value); exit(0);
    return value;
  }

  if (this->is_select) {
    // this is a select expression
    // FRED_VERBOSE(0, "get_value selection entered for person %d |%s|\n", person->get_id(), this->name.c_str());

    double_vector_t id_vec = this->expr1->get_list_value(person, other);
    int size = id_vec.size();
    // FRED_VERBOSE(0, "get_value selection for person %d size %d\n", person->get_id(), size);
    if (0) {
      for (int i=0; i < size; i++) {
	printf("%d %f\n", i, id_vec[i]);
      }
    }

    if (this->preference == NULL ) {
      //this is a select-by-index expression
      // FRED_VERBOSE(0, "get_value selection select-by-index for person %d size %d\n", person->get_id(), size);
      int index = this->expr2->get_value(person,other);
      if (index < size) {
	// FRED_VERBOSE(0, "get_value selection select-by-index for person %d index %d other_id %d\n", person->get_id(), index, (int)id_vec[index]);
	return id_vec[index];
      }
      else {
	// FRED_VERBOSE(0, "get_value selection select-by-index for person %d other_id %d\n", person->get_id(), -999999);
	return -99999999;
      }
    }
    else {
      // this is a select by preference expression
      // FRED_VERBOSE(0, "get_value selection select-by-pref for person %d size %d\n", person->get_id(), size);
      person_vector_t people;
      people.clear();
      for (int i = 0; i < size; i++) {
	people.push_back(Person::get_person_with_id(id_vec[i]));
      }
      Person* selected = this->preference->select_person(person,people);
      if (selected!=NULL) {
	return selected->get_id();
      }
      else {
	return -99999999;
      }
    }
  }

  if (this->number_of_expressions == 0) {
    if (this->factor != NULL) {
      return this->factor->get_value(this->use_other? other : person);
    }
    else {
      // FRED_VERBOSE(0, "get_value person %d result %f\n", person->get_id(), this->number);
      return this->number;
    }
  }

  Place* place1 = NULL;
  Place* place2 = NULL;
  double value1 = 0.0;
  double value2 = 0.0;
  double result = 0.0;
  double sigma, mu;

  value1 = this->expr1->get_value(person,other);
  // FRED_VERBOSE(0, "get_value value1 %f\n", value1);
  if (this->number_of_expressions == 2) {
    value2 = this->expr2->get_value(person,other);
  }
  // FRED_VERBOSE(0, "get_value value2 %f\n", value2);
  // FRED_VERBOSE(0, "get_value op_index %d\n", this->op_index);

  switch(this->op_index) {
  case 0:
    result = value1;
    break;
  case 1:
    result = value1 + value2;
    break;
  case 2:
    result = value1 - value2;
    break;
  case 3:
    result = value1 * value2;
    break;
  case 4:
    if (value2 == 0.0) {
      result = 0.0;
    }
    else {
      result = value1 / value2;
    }
    break;
  case 5:
    // FRED_VERBOSE(0, "DIST person %d value1 %lld value2 %lld\n", person->get_id(), (long long int)value1, (long long int)value2);
    // value1 and value2 should evaluate to place ids
    place1 = Place::get_place_from_sp_id((long long int)value1);
    place2 = Place::get_place_from_sp_id((long long int)value2);
    // FRED_VERBOSE(0, "DIST person %d place1 %lld %s place2 %lld %s\n", person->get_id(), (long long int)value1, place1?place1->get_label():"NULL", (long long int)value2, place2?place2->get_label():"NULL");
    if (place1 && place2) {
      result = Place::distance_between_places(place1, place2);
    }
    else {
      result = 9999999.0;
    }
    //FRED_VERBOSE(0, "DIST person %d place1 %lld %s place2 %lld %s result %f\n", person->get_id(), (long long int)value1, place1?place1->get_label():"NULL", (long long int)value2, place2?place2->get_label():"NULL", result);
    break;
  case 6:
    result = (value1 == value2);
    break;
  case 7:
    result = std::min(value1,value2);
    break;
  case 8:
    result = std::max(value1,value2);
    break;
  case 9:
    result = Random::draw_random(value1,value2);
    break;
  case 10:
    result = Random::draw_normal(value1,value2);
    break;
  case 11:
    sigma = log(value2);
    if (sigma == 0.0) {
      result = value1;
    }
    else {
      mu = log(value1);
      result = Random::draw_lognormal(mu,sigma);
    }
    break;
  case 12:
    result = Random::draw_exponential(value1);
    break;
  case 13:
    if (value1 <= 0.0) {
      result = 0;
    }
    else {
      result = Random::draw_geometric(1.0/value1);
    }
    break;
  case 14:
    result = pow(value1,value2);
    break;
  case 15:
    if (value1 <= 0) {
      result = -1.0e100;
    }
    else {
      result = log(value1);
    }
    break;
  case 16:
    result = exp(value1);
    break;
  case 17:
    result = fabs(value1);
    break;
  case 18:
    result = sin(value1);
    break;
  case 19:
    result = cos(value1);
    break;
  default:
    FRED_VERBOSE(0,"unknown function code\n");
  }
  return result;
}
  


bool Expression::parse() {

  // printf("EXPRESSION: parsing expression |%s|\n", this->name.c_str()); fflush(stdout);

  if (this->minus_err!="") {
    FRED_VERBOSE(0, "HELP: EXPRESSION |%s| PROBLEM WITH UNARY MINUS: %s\n", this->name.c_str(), this->minus_err.c_str());
    return false;
  }

  // process real numbers
  if (Utils::is_number(this->name)) {
    this->number = strtod(this->name.c_str(), NULL);
    this->number_of_expressions = 0;
    // FRED_VERBOSE(0, "EXPRESSION RECOGNIZED NUMBER = |%s| = %0.f\n", this->name.c_str(), this->number);
    return true;
  }

  // process symbolic values
  if (Expression::value_map.find(this->name)!=Expression::value_map.end()) {
    this->number = Expression::value_map[this->name];
    this->number_of_expressions = 0;
    // FRED_VERBOSE(0, "EXPRESSION RECOGNIZED SYM = |%s| = %0.f\n", this->name.c_str(), this->number);
    return true;
  }

  // printf("EXPRESSION: before if -- expression |%s|\n", this->name.c_str()); fflush(stdout);
  // process select expression
  if (this->name.find("select(")==0) {
    // printf("EXPRESSION: inside if -- expression |%s|\n", this->name.c_str()); fflush(stdout);
    FRED_VERBOSE(0,"PARSE select expression |%s|\n", this->name.c_str());
    this->expr1 = NULL;
    this->expr2 = NULL;
    this->pref_str = "1";
    int pos1 = this->name.find(",");
    string list_expr = "";
    if (pos1==string::npos) {
      FRED_VERBOSE(0, "HELP: BAD 1st ARG for SELECT |%s|\n", this->name.c_str());
      Utils::print_error("Select function needs 2 arguments:\n  " + this->name);
      return false;
    }
    list_expr = this->name.substr(7,pos1-7);
    // printf("EXPRESSION: inside if -- list expression |%s|\n", list_expr.c_str()); fflush(stdout);
    this->expr1 = new Expression(list_expr);
    if (this->expr1->parse()==false || this->expr1->is_list_expression()==false) {
      FRED_VERBOSE(0, "HELP: BAD 1st ARG for SELECT |%s|\n", this->name.c_str());
      Utils::print_error("List expression " + list_expr + " not recognized:\n  " + this->name);
      return false;
    }
    if (this->name.substr(pos1+1,5)=="pref(") {
      this->pref_str = this->name.substr(pos1+6,this->name.length()-pos1-8);
      // printf("EXPRESSION: inside if -- pref expression |%s|\n", this->pref_str.c_str()); fflush(stdout);
      this->preference = new Preference();
      this->preference->add_preference_expressions(this->pref_str);
    }
    else {
      string index_expr = this->name.substr(pos1+1,this->name.length()-pos1-2);
      // printf("EXPRESSION: inside if -- index expression |%s|\n", index_expr.c_str()); fflush(stdout);
      FRED_VERBOSE(0, "index_expr |%s|\n", index_expr.c_str());
      this->expr2 = new Expression(index_expr);
      if (this->expr2->parse()==false || this->expr2->is_list_expression()==true) {
	FRED_VERBOSE(0, "HELP: BAD 2nd ARG for SELECT |%s|\n", this->name.c_str());
	Utils::print_error("List index expression " +  index_expr + " not recognized:\n  " + this->name);
	return false;
      }
    }
    // printf("EXPRESSION: inside if -- return true -- expression |%s|\n", this->name.c_str()); fflush(stdout);
    this->is_select = true;
    return true;
  }
  // printf("EXPRESSION: after if -- expression |%s|\n", this->name.c_str()); fflush(stdout);

  if (this->name.find("value(")==0) {
    FRED_VERBOSE(0,"PARSE value expression |%s|\n", this->name.c_str());
    this->expr1 = NULL;
    this->expr2 = NULL;
    string inner = this->name.substr(6,this->name.length()-6);
    // printf("inner |%s|\n", inner.c_str()); exit(0);
    string_vector_t exp_strings = Utils::get_top_level_parse(inner,',');
    if (exp_strings.size() != 2) {
      FRED_VERBOSE(0, "HELP: Need two arguments for VALUE |%s|\n", this->name.c_str());
      Utils::print_error("Value function needs 2 arguments:\n  " + this->name);
      return false;
    }
    // first expression is agent id
    string index_expr = exp_strings[0];
    if (Group_Type::get_group_type(index_expr)!=NULL) {
      index_expr = "admin_of_" + index_expr;
    }
    FRED_VERBOSE(0, "index_expr = |%s| |%s|\n", index_expr.c_str(), this->name.c_str());
    this->expr1 = new Expression(index_expr);
    if (this->expr1->parse()==false || this->expr1->is_list_expression()) {
      FRED_VERBOSE(0, "HELP: BAD 1st ARG for VALUE |%s|\n", this->name.c_str());
      Utils::print_error("Index expression " + index_expr + " not recognized:\n  " + this->name);
      return false;
    }
    string value_expr = exp_strings[1].substr(0,exp_strings[1].length()-1);
    FRED_VERBOSE(0, "value_expr |%s|\n", value_expr.c_str());
    this->expr2 = new Expression(value_expr);
    if (this->expr2->parse()==false || this->expr2->is_list_expression()==true) {
      FRED_VERBOSE(0, "HELP: BAD 2nd ARG for VALUE |%s|\n", this->name.c_str());
      Utils::print_error("Value expression " +  value_expr + " not recognized:\n  " + this->name);
      return false;
    }
    this->is_value = true;
    return true;
  }

  if (this->name.find("distance(")==0) {
    FRED_VERBOSE(0,"PARSE distance expression |%s|\n", this->name.c_str());
    this->expr1 = NULL;
    this->expr2 = NULL;
    this->expr3 = NULL;
    this->expr4 = NULL;
    string inner = this->name.substr(9,this->name.length()-10);
    // printf("inner |%s|\n", inner.c_str()); exit(0);
    string_vector_t exp_strings = Utils::get_top_level_parse(inner,',');
    if (exp_strings.size()==4) {
      this->expr1 = new Expression(exp_strings[0]);
      if (this->expr1->parse()==false || this->expr1->is_list_expression()) {
	FRED_VERBOSE(0, "HELP: BAD 1st ARG for DISTANCE |%s|\n", this->name.c_str());
	Utils::print_error("Distance expression " + exp_strings[0] + " not recognized:\n  " + this->name);
	return false;
      }
      this->expr2 = new Expression(exp_strings[1]);
      if (this->expr2->parse()==false || this->expr2->is_list_expression()) {
	FRED_VERBOSE(0, "HELP: BAD 2nd ARG for DISTANCE |%s|\n", this->name.c_str());
	Utils::print_error("Distance expression " + exp_strings[1] + " not recognized:\n  " + this->name);
	return false;
      }
      this->expr3 = new Expression(exp_strings[2]);
      if (this->expr3->parse()==false || this->expr3->is_list_expression()) {
	FRED_VERBOSE(0, "HELP: BAD 3rd ARG for DISTANCE |%s|\n", this->name.c_str());
	Utils::print_error("Distance expression " + exp_strings[2] + " not recognized:\n  " + this->name);
	return false;
      }
      this->expr4 = new Expression(exp_strings[3]);
      if (this->expr4->parse()==false || this->expr4->is_list_expression()) {
	FRED_VERBOSE(0, "HELP: BAD 4th ARG for DISTANCE |%s|\n", this->name.c_str());
	Utils::print_error("Distance expression " + exp_strings[3] + " not recognized:\n  " + this->name);
	return false;
      }
      this->is_distance = true;
      return true;
    }
    else {
      FRED_VERBOSE(0, "HELP: PROBLEM DISTANCE FUNCTION NEED 4 ARGS |%s|\n", this->name.c_str());
      return false;
    }
  }

  int pos1 = this->name.find("(");
  if (pos1==string::npos) {
    if (this->name.find("other:")==0) {
      this->use_other = true;
      this->name = this->name.substr(strlen("other:"));
    }

    // is this a list_variable?
    this->list_var_id = Person::get_list_var_id(this->name);
    if (0 <= this->list_var_id) {
      this->number_of_expressions = 0;
      this->is_list_var = true;
      this->is_list_expr = true;
      return true;
    }
    else {
      this->list_var_id = Person::get_global_list_var_id(this->name);
      if (0 <= this->list_var_id) {
	this->number_of_expressions = 0;
	this->is_list_var = true;
	this->is_list_expr = true;
	this->is_global = true;
	return true;
      }
    }

    // create new Factor and parse the name
    this->factor = new Factor(this->name);
    if (factor->parse()) {
      FRED_VERBOSE(0, "EXPRESSION RECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      this->number_of_expressions = 0;
      return true;
    }
    else{
      warning = factor->is_warning();
      delete this->factor;
      FRED_VERBOSE(0, "HELP: EXPRESSION UNRECOGNIZED FACTOR = |%s|\n", this->name.c_str());
      return false;
    }
  }
  else {
    this->op = this->name.substr(0,pos1);
    if (Expression::op_map.find(this->op)!=Expression::op_map.end()) {
      // record the index of the operator
      this->op_index = Expression::op_map[this->op];
      // process args
      int pos2 = this->name.find_last_of(")");
      if (pos2==string::npos || pos2 < pos1) {
	FRED_VERBOSE(0, "HELP: UNRECOGNIZED EXPRESSION = |%s|\n", this->name.c_str());
	return false;
      }
      // discard outer parentheses

      string inner = this->name.substr(pos1+1,pos2-pos1-1);
      // FRED_VERBOSE(0, "RULE inner: |%s|\n", inner.c_str());
      if (this->op=="pool") {
	string_vector_t groups = Utils::get_string_vector(inner,',');
	for (int j = 0; j < groups.size(); j++) {
	  int group_type_id = Group_Type::get_type_id(groups[j]);
	  if (group_type_id == -1) {
	    FRED_VERBOSE(0, "HELP: BAD group type |%s| in %s\n", groups[j].c_str(), this->name.c_str());
	    return false;
	  }
	  this->pool.push_back(group_type_id);
	}
	this->is_pool = true;
	this->is_list_expr = true;
	return true;
      }
      int pos_comma = find_comma(inner);

      // LIST
      if (pos_comma < 0 && this->op=="list") {
	FRED_VERBOSE(0, "parsing list expression |%s|\n", this->name.c_str());
	this->expr1 = new Expression(inner);
	if (this->expr1->parse()==false) {
	  FRED_VERBOSE(0, "HELP: BAD 1st ARG for OP %s = |%s|\n", this->op.c_str(), this->name.c_str());
	  return false;
	}
	this->is_list = true;
	this->is_list_expr = true;
	return true;
      }
      
      if (-1 < pos_comma) {
	// get args
	string first = inner.substr(0,pos_comma);
	this->expr1 = new Expression(first);
	if (this->expr1->parse()==false) {
	  FRED_VERBOSE(0, "HELP: BAD 1st ARG for OP %s = |%s|\n", this->op.c_str(), this->name.c_str());
	  return false;
	}

	// LIST
	if (this->op=="list") {
	  FRED_VERBOSE(0, "parsing list expression |%s|\n", this->name.c_str());
	  if (inner.substr(pos_comma+1)!="") {
	    string remainder = "list(" + inner.substr(pos_comma+1) + ")";
	    this->expr2 = new Expression(remainder);
	    if (this->expr2->parse()==false) {
	      FRED_VERBOSE(0, "HELP: BAD remainder ARG for OP %s = |%s|\n", this->op.c_str(), this->name.c_str());
	      return false;
	    }
	  }
	  this->is_list = true;
	  this->is_list_expr = true;
	  return true;
	}

	// FILTER
	if (this->op=="filter") {
	  if (this->expr1->is_list_expression()==false) {
	    FRED_VERBOSE(0, "First arg is not a list expression: %s\n", this->name.c_str());
	    return false;
	  }
	  this->clause = new Clause(inner.substr(pos_comma+1));
	  if (this->clause->parse()==false) {
	    FRED_VERBOSE(0, "BAD CLAUSE in Expression %s\n", this->name.c_str());
	    return false;
	  }
	  this->is_filter = true;
	  this->is_list_expr = true;
	  return true;
	}

	string second = inner.substr(pos_comma+1);
	this->expr2 = new Expression(second);
	if (this->expr2->parse()==false) {
	  FRED_VERBOSE(0, "HELP: BAD 2nd ARG for OP %s = |%s|\n", this->op.c_str(), this->name.c_str());
	  return false;
	}
	this->number_of_expressions = 2;
	return true;
      }
      else if (this->op_index > TWOARGS) {
	// get single args
	this->expr1 = new Expression(inner);
	if (this->expr1->parse()==false) {
	  FRED_VERBOSE(0, "HELP: BAD ARG for OP %s = |%s|\n", this->op.c_str(), this->name.c_str());
	  return false;
	}
	this->number_of_expressions = 1;
	return true;
      }
      else {
	FRED_VERBOSE(0, "HELP: MISSING ARG for OP %s = |%s|\n", this->op.c_str(), this->name.c_str());
	return false;
      }
    }
    else {
      FRED_VERBOSE(0, "HELP: EXPRESSION UNRECOGNIZED OPERATOR = |%s| in |%s|\n", this->op.c_str(), this->name.c_str());
      return false;
    }
  }
  FRED_VERBOSE(0, "HELP: PROBLEM WITH EXPRESSION IS_RECOGNIZED = |%s|\n", this->name.c_str());
  return false;
}

double_vector_t Expression::get_list_value(Person* person, Person* other) {
  double_vector_t results;
  results.clear();

  // FRED_VERBOSE(0, "get_list_value : |%s|: person %d\n", this->name.c_str(), person->get_id());

  FRED_VERBOSE(1, "get_list_value person %d other %d list_var %d is_pool %d is_filter %d use_other %d\n",
	       person->get_id(), other? other->get_id(): -999,
	       this->is_list_var, this->is_pool, this->is_filter, this->use_other);

  if (this->is_list) {
    // FRED_VERBOSE(0, "get_list_value : |%s|: person %d\n", this->name.c_str(), person->get_id());
    double_vector_t list1;    
    double_vector_t list2;    
    list1.clear();
    if (this->expr1->is_list_expression()) {
      // FRED_VERBOSE(0, "get_list_value : |%s|: person %d expr1 is list expr\n", this->name.c_str(), person->get_id());
      list1 = this->expr1->get_list_value(person,other);
    }
    else {
      // FRED_VERBOSE(0, "get_list_value : |%s|: person %d expr1 is value expr\n", this->name.c_str(), person->get_id());
      list1.push_back(this->expr1->get_value(person,other));
    }
    int size1 = list1.size();
    // FRED_VERBOSE(0, "get_list_value : |%s|: person %d expr1 has size %d\n", this->name.c_str(), person->get_id(), size1);

    list2.clear();
    if (this->expr2 != NULL) {
      if (this->expr2->is_list_expression()) {
	// FRED_VERBOSE(0, "get_list_value : |%s|: person %d expr2 is list expr\n", this->name.c_str(), person->get_id());
	list2 = this->expr2->get_list_value(person,other);
      }
      else {
	// FRED_VERBOSE(0, "get_list_value : |%s|: person %d expr2 is value expr\n", this->name.c_str(), person->get_id());
	list2.push_back(this->expr2->get_value(person,other));
      }
    }
    int size2 = list2.size();
    // FRED_VERBOSE(0, "get_list_value : |%s|: person %d expr2 has size %d\n", this->name.c_str(), person->get_id(), size2);

    for (int i = 0; i < size2; i++) {
      list1.push_back(list2[i]);
    }
    int size = list1.size();
    // FRED_VERBOSE(0, "get_list_value : |%s|: person %d result list has size %d\n", this->name.c_str(), person->get_id(), size1);
    return list1;
  }

  if (this->is_list_var) {
    // FRED_VERBOSE(0, "get_list_value : |%s|: person %d  is_list_var is_global %d\n", this->name.c_str(), person->get_id(), this->is_global);
    if (this->is_global) {
      if (this->use_other) {
	return other->get_global_list_var(this->list_var_id);
      }
      else {
	// FRED_VERBOSE(0, "get_list_value : |%s|: person %d  calling get_list_var \n", this->name.c_str(), person->get_id());
	return person->get_global_list_var(this->list_var_id);
      }
    }
    else {
      if (this->use_other) {
	return other->get_list_var(this->list_var_id);
      }
      else {
	// FRED_VERBOSE(0, "get_list_value : |%s|: person %d  calling get_list_var \n", this->name.c_str(), person->get_id());
	return person->get_list_var(this->list_var_id);
      }
    }
  }

  if (this->is_pool) {
    if (this->use_other) {
      return get_pool(other);
    }
    else {
      return get_pool(person);
    }
  }

  if (this->is_filter) {
    double_vector_t initial_list = this->expr1->get_list_value(person,other);
    return get_filtered_list(person, initial_list);
  }

  return results;
}


double_vector_t Expression::get_pool(Person* person) {

  // FRED_VERBOSE(0, "get_pool person %d\n", person->get_id());

  // return the list of ids of people in the person's pool groups

  std::unordered_set<int> found = {};
  double_vector_t people;
  people.clear();
  for (int i = 0; i < this->pool.size(); i++) {
    int group_type_id = this->pool[i];
    // printf("group_type_id = %d\n", group_type_id); fflush(stdout);
    Group* group = person->get_activity_group(group_type_id);
    // printf("group = %s\n", group ? group->get_label() : "NULL"); fflush(stdout);
    if (group!=NULL) {
      int size = group->get_size();
      // printf("size = %d\n", size);
      for (int j = 0; j < size; j++) {
	// printf("size = %d j = %d\n", size, j); fflush(stdout);
	int other_id = group->get_member(j)->get_id();
	if (found.insert(other_id).second) {
	  people.push_back(other_id);
	  // printf("added id %d\n", other_id);
	}
	else {
	  // printf("duplicate id %d\n", other_id);
	}
      }
    }
  }
  // FRED_VERBOSE(0, "get_pool person %d pool size %d\n", person->get_id(), (int)people.size());

  return people;
}

double_vector_t Expression::get_filtered_list(Person* person, double_vector_t &list) {

  // create a filtered list of qualified people
  std::unordered_set<int> found = {};
  double_vector_t filtered;
  filtered.clear();

  // filter out anyone who fails any requirement
  for (int j = 0; j < list.size(); j++) {
    int other_id = list[j];
    Person* other = Person::get_person_with_id(other_id);
    // printf("testing filter: other %d age %d sex %c\n", other_id, other->get_age(), other->get_sex());
    if (this->clause->get_value(person, other)) {
      if (found.insert(other_id).second) {
	filtered.push_back(other_id);
	// printf("passed filter other %d age %d sex %c\n", other_id, other->get_age(), other->get_sex());
      }
    }
    else {
      // printf("failed filter other %d age %d sex %c\n", other_id, other->get_age(), other->get_sex());
    }
  }
  return filtered;
}

