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
// File: Preference.cc
//

#include <unordered_set>
#include "Global.h"
#include "Expression.h"
#include "Preference.h"
#include "Person.h"
#include "Random.h"

Preference::Preference() {
  this->expressions.clear();
}

Preference::~Preference() {
  this->expressions.clear();
}

string Preference::get_name(){
  string result;
  result += "pref: ";
  for (int i = 0; i < this->expressions.size(); i++) {
    result += this->expressions[i]->get_name();
    result += "|";
  }
  result += "\n";
  return result;
}

void Preference::add_preference_expressions(string expr_str) {
  if (expr_str != "") {
    string_vector_t expression_strings = Utils::get_top_level_parse(expr_str, ',');
    for (int i = 0; i < expression_strings.size(); i++) {
      string e = expression_strings[i];
      // printf("ADDING PREF expr %s\n", e.c_str());
      Expression* expression = new Expression(e);
      if (expression->parse()==false) {
	char msg[FRED_STRING_SIZE];
	sprintf(msg, "Bad expression: |%s|", e.c_str());
	Utils::print_error(msg);
	return;
      }
      else {
	this->expressions.push_back(expression);
	// printf("ADD PREF expr %s\n", expression->get_name().c_str());
      }
    }
  }
}

Person* Preference::select_person(Person* person, person_vector_t &people) {
  
  FRED_VERBOSE(1, "select_person entered for person %d age %d sex %c people size %d\n",
	       person->get_id(), person->get_age(), person->get_sex(), (int)people.size());

  int psize = people.size();
  if (psize==0) {
    return NULL;
  }

  // create a cdf based on preference values
  double cdf [ psize ];
  double total = 0.0;
  for (int i = 0; i < psize; i++) {
    cdf[i] = get_value(person, people[i]);
    total += cdf[i];
  }

  // create a cumulative density distribution
  for (int i = 0; i < psize; i++) {
    if (total > 0) {
      cdf[i] /= total;
    }
    else {
      cdf[i] = 1.0 / psize;
    }
    if (i > 0) {
      cdf[i] += cdf[i-1];
    }
    // printf("cdf[%d] = %f\n", i, cdf[i]);
  }

  // select
  double r = Random::draw_random();
  int p;
  for (p = 0; p < psize; p++) {
    if (r <= cdf[p]) {
      break;
    }
  }
  if (p == psize) {
    p = psize-1;
  }
  Person* other = people[p];
  /*
  printf("SELECT person %d age %f sex %c other person %d age %f sex %c value %f\n\n", 
	 person->get_id(), person->get_real_age(), person->get_sex(),
	 other->get_id(), other->get_real_age(), other->get_sex(), 
	 p ? total*(cdf[p]-cdf[p-1]) : total*cdf[p]);
  */
  return other;
}

double Preference::get_value(Person* person, Person* other) {
  int size = this->expressions.size();
  double numerator = 1.0;
  double denominator = 1.0;
  for (int p = 0; p < size; p++) {
    double value = this->expressions[p]->get_value(person, other);
    if (value > 0) {
      numerator += value;
    }
    else {
      denominator += fabs(value);
    }
  }
  // note: denominator >= 1 and numerator >= 0
  double result = numerator / denominator;
  /*
  printf("person %d age %f sex %c other person %d age %f sex %c numer %f denom %f result %f\n", 
	 person->get_id(), person->get_real_age(), person->get_sex(),
	 other->get_id(), other->get_real_age(), other->get_sex(),
	  numerator, denominator, result);
  */
  return result;
}


