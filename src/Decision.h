/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Decision.h
//

#ifndef _FRED_DECISION_H
#define _FRED_DECISION_H

#include <iostream>
#include <string>
#include <list>

class Policy;
class Person;

using namespace std;

class Decision{
  
protected:
  string name;
  string type; 
  Policy *policy;  // This is the policy that the decision belongs to
  
public:
  Decision();
  Decision(Policy *p);
  virtual ~Decision();
  
  /**
   * @return the name of this Decision
   */
  string get_name() const { return name; }

  /**
   * @return the type of this Decision
   */
  string get_type() const { return type; }
  
  /**
   * Evaluate the Decision for an agent and condition on a given day
   *
   * @param person a pointer to a Person object
   * @param condition the condition to evaluate for
   * @param current_day the simulation day
   *
   * @return the evaluation value
   */
  virtual int evaluate(Person* person, int condition, int current_day) = 0;  
};
#endif
