/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Vaccine_Priority_Policies.cpp
//

#include "Vaccine_Priority_Decisions.h"
#include "Vaccine_Priority_Policies.h"
#include "Manager.h"
#include "Vaccine_Manager.h"
#include "Vaccines.h"
#include "Random.h"
#include "Person.h"
#include "Health.h"
#include "Demographics.h"
#include <iostream>

Vaccine_Priority_Decision_Specific_Age::Vaccine_Priority_Decision_Specific_Age() : Decision(){ }

Vaccine_Priority_Decision_Specific_Age::Vaccine_Priority_Decision_Specific_Age(Policy *p): Decision(p) {
  name = "Vaccine Priority Decision Specific Age";
  type = "Y/N";
  policy = p;
}

int Vaccine_Priority_Decision_Specific_Age::evaluate(Person* person, int disease, int day){
  Vaccine_Manager* vcm = dynamic_cast < Vaccine_Manager* >(policy->get_manager());
  int low_age = vcm->get_vaccine_priority_age_low();
  int high_age = vcm->get_vaccine_priority_age_high();
  
  if(person->get_age() >= low_age && person->get_age() <= high_age){
    return 1;
  }
  return -1;
}

Vaccine_Priority_Decision_Pregnant::Vaccine_Priority_Decision_Pregnant() : Decision(){ }

Vaccine_Priority_Decision_Pregnant::Vaccine_Priority_Decision_Pregnant(Policy *p): Decision(p) {
  name = "Vaccine Priority Decision to Include Pregnant Women";
  type = "Y/N";
  policy = p;
}

int Vaccine_Priority_Decision_Pregnant::evaluate(Person* person, int disease, int day){
  if(person->get_demographics()->is_pregnant()) return 1;
  return -1;
}

Vaccine_Priority_Decision_At_Risk::Vaccine_Priority_Decision_At_Risk() : Decision(){ }

Vaccine_Priority_Decision_At_Risk::Vaccine_Priority_Decision_At_Risk(Policy *p): Decision(p) {
  name = "Vaccine Priority Decision to Include At_Risk";
  type = "Y/N";
  policy = p;
}

int Vaccine_Priority_Decision_At_Risk::evaluate(Person* person, int disease, int day){
  if(person->get_health()->is_at_risk(disease)) return 1;
  return -1;
}


Vaccine_Priority_Decision_No_Priority::Vaccine_Priority_Decision_No_Priority() : Decision() { }

Vaccine_Priority_Decision_No_Priority::Vaccine_Priority_Decision_No_Priority(Policy *p) : Decision(p){
  name = "Vaccine Priority Decision No Priority";
  type = "Y/N";
  policy=p;
}

int Vaccine_Priority_Decision_No_Priority::evaluate(Person* person, int disease, int day){  
  return -1;
}

