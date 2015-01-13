/*
 *  Timestep_Map.cpp
 *  FRED
 *
 *  Created by Shawn Brown on 4/30/10.
 *  Copyright 2010 University of Pittsburgh. All rights reserved.
 *
 *  Note: changed behavior of constructor; now necessary to call read_map() after instantiation -- JVD
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <map>

#include "Timestep_Map.h"
#include "Params.h"
#include "Utils.h"

using namespace std;

Timestep_Map::Timestep_Map(){
  values = NULL;
  name = "";
  current_value = -1;
}

Timestep_Map::Timestep_Map(string _name){
  name = _name;
  current_value = 0;
  
  char map_file_param[255];
  // Need Special parsing if this is an array from input
  // Allows for Disease specific values
  if(name.find("[") != string::npos) {
    string name_tmp;
    string number;
    size_t found = name.find_last_of("[");
    size_t found2 = name.find_last_of("]");
    name_tmp.assign(name.begin(),name.begin()+found);
    number.assign(name.begin()+found+1,name.begin()+found2);
    sprintf(map_file_param,"%s_file[%s]",name_tmp.c_str(),number.c_str());
  }
  else {
    sprintf(map_file_param,"%s_file",name.c_str());
  }

  // Read the filename from params
  Params::get_param(map_file_param,map_file_name);
  
  Utils::get_fred_file_name(map_file_name);
  
  // If this parameter is "none", then there is no map
  if(strncmp(map_file_name,"none",4)==0){
    fflush(stdout);
    values = NULL;
    return;
  }
}

void Timestep_Map::read_map() {
  ifstream ts_input;
  
  ts_input.open(map_file_name);
  if(!ts_input.is_open()) {
    Utils::fred_abort("Help!  Can't read %s Timestep Map\n", map_file_name);
  }
  
  // There is a file, lets read in the data structure.
  values = new map<int,int>;
  string line;
  while(getline(ts_input,line)){
    int ts, value;
    istringstream istr(line);
    istr >> ts >> value;
    values->insert(pair<int,int>(ts,value));
  }
}

Timestep_Map::~Timestep_Map() {
  if (values) delete values;
}

int Timestep_Map::get_value_for_timestep(int ts, int offset) {
  map<int,int>::iterator itr;
  
  if ((ts - offset) < 0) {
    current_value = 0;
  } else {
    itr = values->find(ts - offset);
    if (itr != values->end()) {
      current_value = itr->second;
    }
  }
  
  return current_value;
}

void Timestep_Map::print() {
  cout << "\n";
  cout << name << " Timestep Map  " << values->size() <<"\n";
  map<int,int>::iterator itr;
  for(itr=values->begin(); itr!=values->end(); ++itr){
    cout << setw(5) << itr->first << ": " << setw(10) << itr->second << "\n";
  }
  cout << "\n";
}
