/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_LOGIT_H
#define _FRED_LOGIT_H

#include <string>
#include <vector>
using namespace std;

class Person;
class Logistic_Regression;
class Ordinal_Logistic_Regression;

class Logit {

public:
  Logit(char* _name, int number_of_states);
  ~Logit();
  void get_parameters();
  void setup();
  void prepare();
  void print();
  char* get_name() {
    return this->name;
  }
  int get_outcome(Person* person);

  // unit test
  static void test();
  void test_pop();

protected:
  char name[256];
  char logit_type[256];
  int number_of_states;
  double background_prob;
  double intercept;
  double *cutoff;
  Logistic_Regression* LR;
  Ordinal_Logistic_Regression* OLR;
  std::vector<double> beta;

  void fill_x(Person* person, double* x);

  // note: these could be static, since they will be the same for all Logit objects
  std::vector<std::string> factor_name;
  int number_of_factors;
  int number_of_conditions;

};

#endif
