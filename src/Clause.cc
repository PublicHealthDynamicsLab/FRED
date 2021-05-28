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

#include "Clause.h"
#include "Person.h"
#include "Predicate.h"

Clause::Clause() {
  this->name = "";
}

Clause::Clause(string s) {
  this->name = s;
  this->predicates.clear();
}


string Clause::get_name() {
  return this->name;
}


bool Clause::parse() {

  if (this->name=="") {
    return true;
  }

  FRED_VERBOSE(1, "RULE CLAUSE: recognizing clause |%s|\n", this->name.c_str());

  // change top level commas to semicolons
  char x [FRED_STRING_SIZE];
  strcpy(x,this->name.c_str());
  int inside = 0;
  char* s = x;
  while (*s!='\0') {
    if (*s==',' && !inside) {
      *s = ';';
    }
    if (*s=='(') {
      inside++;
    }
    if (*s==')') {
      inside--;
    }
    s++;
  }
  // printf("RULE CLAUSE: clause |%s|  after |%s|\n", this->name.c_str(), x); fflush(stdout);

  // find predicates
  string predicate_string = string(x);
  Predicate* predicate = NULL;
  while (predicate_string!="") {
    int pos = predicate_string.find(";");
    if (pos==string::npos) {
      // printf("RULE CLAUSE creating new Predicate |%s|\n", predicate_string.c_str());
      predicate = new Predicate(predicate_string);
      // printf("RULE CLAUSE created new Predicate |%s|\n", predicate_string.c_str());
      predicate_string = "";
    }
    else {
      // printf("RULE CLAUSE creating new Predicate |%s|\n", predicate_string.substr(0,pos).c_str());
      predicate = new Predicate(predicate_string.substr(0,pos));
      predicate_string = predicate_string.substr(pos+1);;
    }
    // printf("RULE is predicate recognized?\n"); fflush(stdout);
    if (predicate != NULL && predicate->parse()) {
      this->predicates.push_back(predicate);
    }
    else{
      this->warning = predicate->is_warning();
      delete predicate;
      for (int i = 0; i < this->predicates.size(); i++) {
	delete this->predicates[i];
      }
      FRED_VERBOSE(0, "HELP: UNRECOGNIZED PREDICATE = |%s|\n", this->name.c_str());
      return false;
    }
  }

  return true;
}

bool Clause::get_value(Person* person, Person* other) {
  // printf("RULE GET_VALUE for person %d\n", person->get_id());  fflush(stdout);
  for (int i = 0; i < this->predicates.size(); i++) {
    if (predicates[i]->get_value(person, other)==false) {
      // printf("RULE GET_VALUE FALSE due to |%s|\n", predicates[i]->get_name().c_str());  fflush(stdout);
      return false;
    }
    // printf("RULE GET_VALUE true for |%s|\n", predicates[i]->get_name().c_str()); fflush(stdout);
  }
  return true;
}



