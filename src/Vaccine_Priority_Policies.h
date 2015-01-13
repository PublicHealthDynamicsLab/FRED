/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Vaccine_Priority_Policies.h
//

#ifndef _FRED_VACCINE_PRIORITY_POLICIES_H
#define _FRED_VACCINE_PRIORITY_POLICIES_H

#include <iostream>
#include <string>

#include "Policy.h"

class Decision;
class Person;
class Vaccines;
class Vaccine_Manager;
class Manager;

using namespace std;

class Vaccine_Priority_Policy_No_Priority: public Policy {
  Vaccine_Manager *vacc_manager;
  
public:
  Vaccine_Priority_Policy_No_Priority() { }
  Vaccine_Priority_Policy_No_Priority(Vaccine_Manager* vcm);
};

class Vaccine_Priority_Policy_Specific_Age:public Policy {
  Vaccine_Manager *vacc_manager;
  
public:
  Vaccine_Priority_Policy_Specific_Age();
  Vaccine_Priority_Policy_Specific_Age(Vaccine_Manager* vcm);
};  

class Vaccine_Priority_Policy_ACIP:public Policy {
  Vaccine_Manager *vacc_manager;
  
 public: 
  Vaccine_Priority_Policy_ACIP();
  Vaccine_Priority_Policy_ACIP(Vaccine_Manager* vcm);
};

#endif
