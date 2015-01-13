/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Multistrain_Timestep_Map.h
// 
//
// Created by Jay DePasse on 06/21/2011
//
// Extends functionality of Timestep_Map to control seeding of multiple strains.
// Adds ability to seed at a specific geographic locations.
// Based on Shawn Brown's updated version of David Galloway's original
// seeding schedule code.
// The format has been made more explicit (and complicated).
// The first line of the file must declare the type of format that will be used.
// "#line_format" means each record occupies a single space delimited line
// "#structured_format" means that each record will be given in "BoulderIO" format
// see http://stein.cshl.org/boulder/docs/Boulder.html


#ifndef _FRED_MULTISTRAIN_TIMESTEP_MAP_H
#define _FRED_MULTISTRAIN_TIMESTEP_MAP_H

#include "Global.h"
#include <stdio.h>
#include <map>
#include <vector>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <iterator>

#include "Timestep_Map.h"
#include "Utils.h"

using namespace std;

class Multistrain_Timestep_Map : public Timestep_Map {
  
public:

  Multistrain_Timestep_Map(string _name);
  ~Multistrain_Timestep_Map();
 
  void read_map();
  void read_map_line(ifstream * ts_input);
  void read_map_structured(ifstream * ts_input);

  void print() ;

  class Multistrain_Timestep {

  public:

    Multistrain_Timestep() {
      strain = 0;
      is_complete = false;
      seedingAttemptProb = 1;
      minNumSuccessful = 0;
      lat = 51.47795;
      lon = 00.00000;
      radius = 40075; // in kilometers
    }

    bool parse_line_format(string tsStr) {

      if ( tsStr.size() <= 0 || tsStr.at(0) == '#' ) { // empty line or comment
        return false;
      } else {
        vector<string> tsVec; 
        size_t p1 = 0;
        size_t p2 = 0;
        while ( p2 < tsStr.npos ) {
          p2 = tsStr.find( " ", p1 );
          tsVec.push_back( tsStr.substr( p1, p2 - p1 ) );
          p1 = p2 + 1;
        }
        int n = tsVec.size(); 
        if ( n < 3 ) {
          Utils::fred_abort("Need to specify at least SimulationDayStart, SimulationDayEnd and NumSeedingAttempts for Multistrain_Timestep_Map. ");
        } else {
          stringstream( tsVec[0] ) >> simDayStart;
          stringstream( tsVec[1] ) >> simDayEnd;
          stringstream( tsVec[2] ) >> numSeedingAttempts;
          if ( n > 3 ) { stringstream( tsVec[3] ) >> strain; }
          if ( n > 4 ) { stringstream( tsVec[4] ) >> seedingAttemptProb; }
          if ( n > 5 ) { stringstream( tsVec[5] ) >> minNumSuccessful; }
          if ( n >= 9 ) {
            stringstream( tsVec[6] ) >> lat;
            stringstream( tsVec[7] ) >> lon;
            stringstream( tsVec[8] ) >> radius;
          }
        }
        is_complete = true;   
      }
      return is_complete;
    }

    bool is_applicable(int ts, int offset) {
      int t = ts - offset;
      return t >= simDayStart && t <= simDayEnd;
    }

    int get_strain() {
      return strain;
    }

    int get_num_seeding_attempts() {
      return numSeedingAttempts;
    }

    int get_min_successes() {
      return minNumSuccessful;
    }

    double get_seeding_prob() {
      return seedingAttemptProb;
    }

    bool has_location() {
      return ( radius < 40075 && radius >= 0 );
    }

    fred::geo get_lon() { return lon; }
    fred::geo get_lat() { return lat; }
    double get_radius() { return radius; }

    void print() {
      printf("start day = %d, end day = %d, number of attempts = %d, strain = %d\n",simDayStart,simDayEnd,numSeedingAttempts,strain);
    }

  private:

    int simDayStart, simDayEnd, strain, numSeedingAttempts, minNumSuccessful;
    fred::geo lat, lon;
    double seedingAttemptProb, radius;
    
    bool is_complete;

  };

private:

  vector < Multistrain_Timestep * > multistrain_timestep_map;
  
  void insert(Multistrain_Timestep * mst);

public:

  typedef vector < Multistrain_Timestep * >::iterator iterator;

  vector < Multistrain_Timestep * >::iterator begin() {
    return multistrain_timestep_map.begin();
  }

  vector < Multistrain_Timestep * >::iterator end() {
    return multistrain_timestep_map.end();
  }

  
};

#endif // _FRED_MULTISTRAIN_TIMESTEP_MAP_H

