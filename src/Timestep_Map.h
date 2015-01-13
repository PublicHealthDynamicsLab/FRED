/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Timestep_Map.h
//

#ifndef _FRED_TIMESTEP_MAP_H
#define _FRED_TIMESTEP_MAP_H

#include "Global.h"
#include <stdio.h>
#include <map>

using namespace std;

class Timestep_Map {
  // The Day Map class is an input structure to hold a list of values
  // that are indexed by timesteps, this is used for values that 
  // need to be changed on certain timesteps throughout the simulation
  // Based on David Galloway's initial seeding.
  
  // A Timestep_Map is to be specified in a seperate file
  // If a Timestep_Map is specified with [] on the end, it is assumed to
  // be disease specific 
  
  // This structure is designed so that if there is a specification in the 
  // input that specifies "none" as the keyword, the structure is empty.
  
  // The input keyword will be the name of the structure_file.  So if the
  // name of the Map is passed as "primary_cases", the param keyword will
  // be "primary_cases_file".

  // The format of the file is as such
  // 0 100
  // 1 0 
  //   Should be interpreted as 100 on timestep 0, 0 on timestep 1 and beyond
  //
  // 0 100
  // 100 0
  //   Should be interpreted as 100 for timestep 0 - 99, and 0 on timestep 100 and above
  // Updated: Shawn Brown
  
public:
  Timestep_Map();  
  Timestep_Map(string _name);
  virtual ~Timestep_Map();
  
  // Utility Members
  int get_value_for_timestep(int ts, int offset); // returns the value for the given timestep - delay
  bool is_empty() const { return values->empty(); }
  virtual void print();

  virtual void read_map();

protected:
  map <int, int>* values;  // Map structure that holds <ts, value>
  string name;             // Name of the map
  char map_file_name[255];
  int current_value;       // Holds the current value of th map.
};

#endif // _FRED_TIMESTEP_MAP_H
