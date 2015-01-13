/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Policy.h
//



#ifndef _FRED_POLICY_H
#define _FRED_POLICY_H

#include <iostream>
#include <string>
#include <vector>

class Manager;
class Decision;
class Person;

using namespace std;

/**
 * A Policy is a class that is accessed by the manager to decide something.
 * Will be used for mitigation strategies.
 */
class Policy{
  
public:

  /**
   * Default constructor
   */
  Policy();
  
  /**
   * Constructor that sets this Policy's manager.
   *
   * @param mgr the manager of this Policy
   */
  Policy(Manager* mgr);
  
  ~Policy();

  /**
   * @param person a pointer to a person object
   * @param disease the disease
   * @param current_day the simulation day
   *
   * @return
   */
  virtual int choose(Person* person, int disease, int current_day);

  /**
   * @param person a pointer to a person object
   * @param disease the disease
   * @param current_day the simulation day
   *
   * @return
   */
  virtual bool choose_first_positive(Person* person, int disease, int current_day);

  /**
   * @param person a pointer to a person object
   * @param disease the disease
   * @param current_day the simulation day
   *
   * @return
   */
  virtual bool choose_first_negative(Person* person, int disease, int current_day);
  // decision will return -1 if the decision is no
  // or the integer result of the policies in the decision   
  
  /**
   * @return a pointer to this Policy's Manager object
   */
  Manager* get_manager() const { return manager; }

  /**
   * Put this object back to its original state
   */
  void reset();

  /**
   * Print out information about this object
   */
  void print() const;
  
protected:
  vector < Decision * > decision_list;
  string Name;
  Manager* manager;
  
};

#endif
