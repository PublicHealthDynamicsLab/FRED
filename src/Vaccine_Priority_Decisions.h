/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */
//
//
// File: Vaccine_Priority_Decision.h
//

#ifndef _FRED_VACCINE_PRIORITY_DECISIONS_H
#define _FRED_VACCINE_PRIORITY_DECISIONS_H


#include "Decision.h"

class Policy;
class Person;

class Vaccine_Priority_Decision_Specific_Age: public Decision {
public:
  Vaccine_Priority_Decision_Specific_Age(Policy* p);
  Vaccine_Priority_Decision_Specific_Age();
  int evaluate(Person* person, int disease, int day);
};

class Vaccine_Priority_Decision_Pregnant: public Decision {
 public:
  Vaccine_Priority_Decision_Pregnant(Policy* p);
  Vaccine_Priority_Decision_Pregnant();
  int evaluate(Person* person, int disease, int day);
};

class Vaccine_Priority_Decision_At_Risk: public Decision {
 public:
  Vaccine_Priority_Decision_At_Risk(Policy* p);
  Vaccine_Priority_Decision_At_Risk();
  int evaluate(Person* person, int disease, int day);
};

class Vaccine_Priority_Decision_No_Priority: public Decision {
public:
  Vaccine_Priority_Decision_No_Priority(Policy *p);
  Vaccine_Priority_Decision_No_Priority();
  int evaluate(Person* person, int disease, int day);
};

#endif
