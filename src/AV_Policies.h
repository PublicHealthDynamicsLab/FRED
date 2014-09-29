/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: AV_Decision.h
//

#ifndef _FRED_AV_POLICIES_H
#define _FRED_AV_POLICIES_H

#include <iostream>
#include <string>
#include "Policy.h"

class Decision;
//class Policy;
class Person;
class Antivirals;
class AV_Manager;
class Manager;

using namespace std;

class AV_Policy_Distribute_To_Symptomatics: public Policy {
  AV_Manager* av_manager;
  
public:
  AV_Policy_Distribute_To_Symptomatics();
  AV_Policy_Distribute_To_Symptomatics(AV_Manager* avm);
  
  /**
   * @param person a pointer to a person object
   * @param disease the disease
   * @param current_day the simulation day
   *
   * @return
   * @see Policy::choose(Person* person, int disease, int current_day)
   */
  virtual int choose(Person* person, int disease, int current_day);
};

class AV_Policy_Distribute_To_Everyone: public Policy {
  AV_Manager* av_manager;  
public:
  AV_Policy_Distribute_To_Everyone();
  AV_Policy_Distribute_To_Everyone(AV_Manager* avm);
  
  /**
   * @param person a pointer to a person object
   * @param disease the disease
   * @param current_day the simulation day
   *
   * @return
   * @see Policy::choose(Person* person, int disease, int current_day)
   */
  virtual int choose(Person* person, int disease, int current_day);
};
#endif
