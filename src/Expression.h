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

#ifndef _FRED_EXPRESSION_H
#define _FRED_EXPRESSION_H


#include "Global.h"

class Factor;
class Person;
class Preference;


class Expression {
public:

  Expression(string s);
  string get_name();
  double get_value(Person* person, Person* other = NULL);
  double_vector_t get_list_value(Person* person, Person* other = NULL);
  bool parse();
  
  static bool is_known_function(std::string str) {
    return Expression::op_map.find(str)!=Expression::op_map.end();
  }

  string get_next_token(string s, int pos);
  string expand_minus(string s);
  string expand_operator(string s);
  bool is_operator(string s);
  bool is_function(string s);
  int get_number_of_args(string s);
  int get_priority(string s);
  string infix_to_prefix(string infix);
  string replace_unary_minus(string s);
  string convert_infix_to_prefix(string infix);
  int find_comma(string s);
  double_vector_t get_pool(Person* person);
  double_vector_t get_filtered_list(Person* person, double_vector_t &list);
  bool is_warning() {
    return this->warning;
  }
  bool is_list_expression() {
    return this->is_list_expr;
  }

private:
  std::string name;
  std::string op;
  int op_index;
  Expression* expr1;
  Expression* expr2;
  Expression* expr3;
  Expression* expr4;
  Factor* factor;
  double number;
  int number_of_expressions;
  string minus_err;
  string list_var;
  int list_var_id;
  string pref_str;
  Preference* preference;
  bool is_select;
  bool use_other;
  bool warning;
  bool is_list_expr;
  bool is_list_var;
  bool is_global;
  bool is_pool;
  bool is_filter;
  bool is_list;
  bool is_value;
  bool is_distance;
  int_vector_t pool;
  Clause* clause;

  static std::map<std::string,int> op_map;
  static std::map<std::string,int> value_map;
};

#endif


