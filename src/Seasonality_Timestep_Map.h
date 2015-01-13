/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Seasonality_Timestep_Map.h
// 
//
// Created by Jay DePasse on 06/21/2011
//
// Extends functionality of Timestep_Map to control specification of seasonality conditions.
// Adds ability to seed at a specific geographic locations.
// Based on Shawn Brown's updated version of David Galloway's original
// seeding schedule code.
// The format has been made more explicit (and complicated).
// The first line of the file must declare the type of format that will be used.
// "#line_format" means each record occupies a single space delimited line
// "#structured_format" means that each record will be given in "BoulderIO" format
// see http://stein.cshl.org/boulder/docs/Boulder.html


#ifndef _FRED_SEASONALITY_TIMESTEP_MAP_H
#define _FRED_SEASONALITY_TIMESTEP_MAP_H

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
#include <utility>

#include "Timestep_Map.h"
#include "Utils.h"
#include "Date.h"

using namespace std;

class Seasonality_Timestep_Map : public Timestep_Map {
  
public:

  Seasonality_Timestep_Map(string _name);
  ~Seasonality_Timestep_Map();
 
  void read_map();
  void read_map_line(ifstream * ts_input);
  void read_map_structured(ifstream * ts_input);

  void print() ;

  class Seasonality_Timestep {

  public:

    Seasonality_Timestep() {
      is_complete = false;
      lat = 51.47795;
      lon = 00.00000;
      seasonalityValue = 0;
      loc = false;
      sim_day_ranges.clear();
    }

    bool parse_line_format(string tsStr) {

      int simDayStart, simDayEnd;

      if ( tsStr.size() <= 0 || tsStr.at(0) == '#' ) { // empty line or comment
        return false;
      }
      else {
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
          Utils::fred_abort("Need to specify at least SimulationDayStart, \
              SimulationDayEnd and SeasonalityValue for Seasonality_Timestep_Map. ");
        } else {
          if (tsVec[0].find('-') == 2 && tsVec[1].find('-') == 2) {
            // start and end specify only MM-DD (repeat this calendar date's value every year)
            parseMMDD(tsVec[0], tsVec[1]);
          }
          else {
            // start and end specified as (integer) sim days
            stringstream( tsVec[0] ) >> simDayStart;
            stringstream( tsVec[1] ) >> simDayEnd;
            sim_day_ranges.push_back( pair <int,int> (simDayStart, simDayEnd) );
          }
          stringstream( tsVec[2] ) >> seasonalityValue;
          if ( n >= 3 ) {
            stringstream( tsVec[3] ) >> lat;
            stringstream( tsVec[4] ) >> lon;
            loc = true;
          }
        }
        is_complete = true;   
      }
      return is_complete;
    }


    bool is_applicable(int ts, int offset) {
      int t = ts - offset;
      for (unsigned i = 0; i < sim_day_ranges.size(); i++) {
        if ( t >= sim_day_ranges[i].first && t <= sim_day_ranges[i].second ) {
          return true;
        }
      }
      return false;
      //return t >= simDayStart && t <= simDayEnd;
    }

    double get_seasonality_value() {
      return seasonalityValue;
    }

    bool has_location() {
      return loc;
    }

    fred::geo get_lon() { 
      if (loc) { return lon; }
      else { Utils::fred_abort("Tried to access location that was not specified.\
            Calls to get_lon() and get_lat() should be preceeded  by has_location()");}
      return NULL;
    }

    fred::geo get_lat() {
      if (loc) { return lat; }
      else { Utils::fred_abort("Tried to access location that was not specified.\
          Calls to get_lon() and get_lat() should be preceeded  by has_location()");}
      return NULL;
    }

    void print() {
      for (unsigned i = 0; i < sim_day_ranges.size(); i++) {
      Date * tmp_start_date = Global::Sim_Start_Date->clone();
      Date * tmp_end_date = Global::Sim_Start_Date->clone();
      tmp_start_date->advance(sim_day_ranges[i].first);
      tmp_end_date->advance(sim_day_ranges[i].second);
        printf("start day = %d (%s), end day = %d (%s), seasonality value = %f\n",
            sim_day_ranges[i].first, tmp_start_date->to_string().c_str(),
            sim_day_ranges[i].second, tmp_end_date->to_string().c_str(),
            seasonalityValue);
        delete tmp_start_date;
        delete tmp_end_date;
      }
    }

  private:

    bool parseMMDD(string startMMDD, string endMMDD); 

    //int simDayStart, simDayEnd;
    vector < pair <int,int> > sim_day_ranges;
    fred::geo lat, lon;
    double seasonalityValue;
    
    bool is_complete, loc;

  };

private:

  vector < Seasonality_Timestep * > seasonality_timestep_map;
  
  void insert(Seasonality_Timestep * ct);

public:

  typedef vector < Seasonality_Timestep * >::iterator iterator;

  vector < Seasonality_Timestep * >::iterator begin() {
    return seasonality_timestep_map.begin();
  }

  vector < Seasonality_Timestep * >::iterator end() {
    return seasonality_timestep_map.end();
  }

  
};

#endif // _FRED_SEASONALITY_TIMESTEP_MAP_H

