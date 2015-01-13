/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Manager.h
//

#ifndef _FRED_MANAGER_H
#define _FRED_MANAGER_H

#include <iostream>
#include <vector>

class Policy;
class Population;
class Person;
class Person;
class Decision;

using namespace std;

/**
 * Manager is an abstract class that is the embodiment of a mitigation manager.
 * The Manager:
 *  1. Handles a stock of mitigation supplies
 *  2. Holds the policy for doling out a mitigation strategy
 */
class Manager{

protected:
  vector <Policy * > policies;   // vector to hold the policies this manager can apply
  vector < int > results;        // DEPRICATE holds the results of the policies
  Population *pop;               // Population in which this manager is tied to
  int current_policy;            // The current policy this manager is using
  
public:

  /**
   * Default constructor
   */
  Manager();

  /**
   * Constructor that sets the Population to which this Manager is tied
   */
  Manager(Population *_pop);
  ~Manager();

  /**
   * Member to allow someone to see if they fit the current policy
   *
   * @param p a pointer to a Person object
   * @param disease the disease to poll for
   * @param day the simulation day
   *
   * @return the manager's decision
   */
  virtual int poll_manager(Person* p, int disease, int day); //member to allow someone to see if they fit the current policy
  
  // Parameters
  /**
   * @return a pointer to the Population object to which this manager is tied
   */
  Population* get_population() const { return pop;}

  /**
   * @return the current policy this manager is using
   */
  int get_current_policy() const { return current_policy; } 
  
  //Utility Members
  /**
   * Perform the daily update for this object
   *
   * @param day the simulation day
   */
  virtual void update(int day) { };

  /**
   * Put this object back to its original state
   */
  virtual void reset() { };

  /**
   * Print out information about this object
   */
  virtual void print() { };
  
};

#endif
