/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_IntraHost_H
#define _FRED_IntraHost_H

#include <vector>
#include <map>

typedef std::map< int, double > Loads; 

class Infection;
class Trajectory;
class Disease;

class IntraHost {
public:
  
  virtual ~IntraHost() {};

  /**
   * This static factory method is used to get an instance of a specific IntraHost Model.
   * Depending on the type parameter, it will create a specific IntraHost Model and return
   * a pointer to it.
   *
   * @param the specific IntraHost model type
   * @return a pointer to a specific IntraHost model
   */
  static IntraHost * newIntraHost(int type);

  /**
   * Set the attributes for the IntraHost
   *
   * @param dis the disease to which this IntraHost model is associated
   */
  virtual void setup(Disease *dis);

  /**
   * An interface for a specific IntraHost model to implement to return its infection Trajectory
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
