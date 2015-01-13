/*
 *  Seasonality_Timestep_Map.cpp
 *  FRED
 *
 *  Created by Jay DePasse on 06/21/2011.
 *  Copyright 2010 University of Pittsburgh. All rights reserved.
 *
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <map>

#include "Seasonality_Timestep_Map.h"
#include "Params.h"
#include "Utils.h"
#include "Date.h"

using namespace std;

Seasonality_Timestep_Map::Seasonality_Timestep_Map(string _name) : Timestep_Map(_name) {
}

Seasonality_Timestep_Map::~Seasonality_Timestep_Map() {
  for ( iterator i = seasonality_timestep_map.begin(); i != seasonality_timestep_map.end(); ++i ) {
    delete (*i);
    (*i) = NULL;
  }
  seasonality_timestep_map.clear();
}


void Seasonality_Timestep_Map::read_map() {
  ifstream * ts_input = new ifstream(map_file_name);

  if(!ts_input->is_open()) {
    Utils::fred_abort("Help!  Can't read %s Timestep Map\n", map_file_name);
    abort();
  }

  string line;
  if ( getline(*ts_input,line) ) {
    if ( line == "#line_format" ) {
      read_map_line(ts_input);
    }
    else if ( line == "#structured_format" ) {
      read_map_structured(ts_input);
    }
    else {
      Utils::fred_abort("First line has to specify either #line_format or #structured_format; see primary_case_schedule-0.txt for an example. ");
    }
  } else { 
    Utils::fred_abort("Nothing in the file!");
  }
}

void Seasonality_Timestep_Map::read_map_line(ifstream * ts_input) {
  // There is a file, lets read in the data structure.
  values = new map<int,int>;
  string line;
  while(getline(*ts_input,line)){
    Seasonality_Timestep * cts = new Seasonality_Timestep();
    if (cts->parse_line_format(line)) { insert(cts); } 
  }
}

void Seasonality_Timestep_Map::read_map_structured(ifstream * ts_input) {
}

void Seasonality_Timestep_Map::insert(Seasonality_Timestep * ct) {
  seasonality_timestep_map.push_back(ct);
}

void Seasonality_Timestep_Map::print() {
  return;
  cout << "\n";
  cout << name << " Seasonality_Timestep_Map size = " << (int) seasonality_timestep_map.size() <<"\n";
  vector < Seasonality_Timestep * >::iterator itr;
  for(itr=begin(); itr!=end(); ++itr){
    (*itr)->print(); 
  }
  cout << "\n";
}


bool Seasonality_Timestep_Map::Seasonality_Timestep::parseMMDD(string startMMDD, string endMMDD) {
  int years = 0.5 + (Global::Days / 365);    
  int startYear = Global::Sim_Start_Date->get_year();
  string dateFormat = string("YYYY-MM-DD");
  for (int y = startYear; y <= startYear + years; y++) {

    stringstream ss_s;
    stringstream ss_e;
    stringstream ss_s2;
    stringstream ss_e2;

    ss_s << y << "-" << startMMDD;
    string startDateStr = ss_s.str();
    ss_e << y << "-" << endMMDD;
    string end_dateStr = ss_e.str();

    if (!(Date::is_leap_year(y))) {
      if (Date::parse_month_from_date_string(startDateStr,dateFormat) == 2 && Date::parse_day_of_month_from_date_string(startDateStr,dateFormat) == 29) {
        ss_s2 << y << "-" << "03-01";
        startDateStr = ss_s2.str();
      }
      if (Date::parse_month_from_date_string(end_dateStr,dateFormat) == 2 && Date::parse_day_of_month_from_date_string(end_dateStr,dateFormat) == 29) {
        ss_e2 << y << "-" << "03-01";
        end_dateStr = ss_e2.str();
      }
    }

    int simStartDay = 0;
    int simEndDay = 0;

    //Date startDateObj = Date::Date(startDateStr,dateFormat);
    Date startDateObj(startDateStr,dateFormat);

    if (startDateObj >= *Global::Sim_Start_Date) {
      simStartDay = Date::days_between(0, &startDateObj);
    }

    //Date end_dateObj = Date::Date(end_dateStr, dateFormat);
    Date end_dateObj(end_dateStr, dateFormat);

    if (end_dateObj >= *Global::Sim_Start_Date) {
      simEndDay = Date::days_between(0, &end_dateObj);
      sim_day_ranges.push_back( pair <int,int> (simStartDay,simEndDay) );
    }

  }
  return true;
}

