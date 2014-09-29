/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/


//
// File: AV_Decision.h
//

#ifndef _FRED_AV_DECISIONS_H
#define _FRED_AV_DECISIONS_H

#include "AV_Policies.h"
#include "Decision.h"

class Policy;

/**
 * This policy defines that each agent can only have one AV throughout
 *  the course of the simulation
 */
class AV_Decision_Allow_Only_One: public Decision {
  
public: 
  AV_Decision_Allow_Only_One(Policy * p);
  AV_Decision_Allow_Only_One();

  /**
   * @see Decision::evaluate(Person* person, int disease, int current_day);
   */
  int evaluate(Person* person, int disease, int current_day);
};

/**
 * This policy defines that each agent only has one chance to get AV,
 * i.e. only check once.
 */
class AV_Decision_Give_One_Chance: public Decision {
public:
  AV_Decision_Give_One_Chance(Policy *p);
  AV_Decision_Give_One_Chance();

  /**
   * @see Decision::evaluate(Person* person, int disease, int current_day);
   */
  int evaluate(Person* person, int disease, int current_day);
};

/**
 * This policy defines that an agent will receive AV if symptomatic
 */
class AV_Decision_Give_to_Sympt: public Decision {
public:
  AV_Decision_Give_to_Sympt(Policy *p);
  AV_Decision_Give_to_Sympt();

  /**
   * @see Decision::evaluate(Person* person, int disease, int current_day);
   */
  int evaluate(Person* person, int disease, int current_day);
};

/**
 * This policy defines that agent begins AV on a given day
 */
class AV_Decision_Begin_AV_On_Day: public Decision {
public:
  AV_Decision_Begin_AV_On_Day(Policy *p);
  AV_Decision_Begin_AV_On_Day();

  /**
   * @see Decision::evaluate(Person* person, int disease, int current_day);
   */
  int evaluate(Person* person, int disease, int current_day);
};

#endif
