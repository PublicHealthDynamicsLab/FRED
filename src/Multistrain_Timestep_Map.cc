/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

/*
 *  Multistrain_Timestep_Map.cpp
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

#include "Multistrain_Timestep_Map.h"
#include "Params.h"
#include "Utils.h"

using namespace std;

Multistrain_Timestep_Map::Multistrain_Timestep_Map(string _name) : Timestep_Map(_name) {
}

Multistrain_Timestep_Map::~Multistrain_Timestep_Map() {
  for ( iterator i = multistrain_timestep_map.begin(); i != multistrain_timestep_map.end(); ++i ) {
    delete (*i);
    (*i) = NULL;
  }
  multistrain_timestep_map.clear();
}


void Multistrain_Timestep_Map::read_map() {
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
  delete ts_input;
}

void Multistrain_Timestep_Map::read_map_line(ifstream * ts_input) {
  // There is a file, lets read in the data structure.
  values = new map<int,int>;
  string line;
  while(getline(*ts_input,line)){
    Multistrain_Timestep * msts = new Multistrain_Timestep();
    if (msts->parse_line_format(line)) { insert(msts); } 
  }
}

void Multistrain_Timestep_Map::read_map_structured(ifstream * ts_input) {
}

void Multistrain_Timestep_Map::insert(Multistrain_Timestep * mst) {
   multistrain_timestep_map.push_back(mst);
}

void Multistrain_Timestep_Map::print() {
  cout << "\n";
  cout << name << " Multistrain_Timestep_Map size = " << (int) multistrain_timestep_map.size() <<"\n";
  vector < Multistrain_Timestep * >::iterator itr;
  for(itr=begin(); itr!=end(); ++itr){
    (*itr)->print(); 
  }
  cout << "\n";
}
