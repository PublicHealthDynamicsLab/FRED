/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_NATURAL_HISTORY_H
#define _FRED_NATURAL_HISTORY_H

#include <vector>
#include <map>

typedef std::map< int, double > Loads; 

class Infection;
class Trajectory;
class Disease;

class Natural_History {
public:
  
  virtual ~Natural_History() {};

  /**
   * This static factory method is used to get an instance of a specific Natural_History Model.
   * Depending on the type parameter, it will create a specific Natural_History Model and return
   * a pointer to it.
   *
   * @param the specific Natural_History model type
   * @return a pointer to a specific Natural_History model
   */
  static Natural_History * get_natural_history(char* natural_history_model);

  /**
   * Set the attributes for the Natural_History
   *
   * @param dis the disease to which this Natural_History model is associated
   */
  virtual void setup(Disease *dis);

  /**
   * An interface for a specific Natural_History model to implement to return its infection Trajectory
   *
   * @param infection
   * @param loads
   * @return a pointer to a Trajectory object
   */
  virtual Trajectory * get_trajectory() { return NULL; };
  virtual Trajectory * get_trajectory(int age) { return NULL; };
  virtual Trajectory * get_trajectory(double real_age) { return NULL; };

  /**
   * @return the days symptomatic
   */
  virtual int get_days_symp();

protected:
  Disease *disease;
};

#endif
