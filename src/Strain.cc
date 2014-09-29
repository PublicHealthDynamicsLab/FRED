/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Strain.cc
//


#include <iostream>
#include <stdio.h>
#include <new>
#include <string>
#include <sstream>
#include <cstdlib>
#include <iomanip>

using namespace std;

#include "Strain.h"
#include "Global.h"
#include "Params.h"
#include "Population.h"
#include "Random.h"
#include "Age_Map.h"
#include "Epidemic.h"
#include "Timestep_Map.h"
#include "Disease.h"

Strain::Strain( int num_elements ) : strain_data( num_elements ) {
  transmissibility = -1.0;
  disease = NULL;
}

Strain::Strain( const Strain & other ) : strain_data( other.strain_data ) { 
  transmissibility = other.transmissibility;
  disease = other.disease;
}

Strain::~Strain() {
}

void Strain::setup( int _strain_id, Disease * _disease, double _transmissibility, Strain * _parent ) {
  id = _strain_id;
  disease = _disease;
  transmissibility = _transmissibility;
  parent = _parent;
}

void Strain::print_alternate(stringstream &out) {
  int pid = -1;
  if ( parent ) {
    pid = parent->get_id();
  }
  out << id << " : " << pid << " : ";
  for ( int i = 0; i < strain_data.size(); ++i ) {
    out << strain_data[ i ] << " ";
  }
}

void Strain::print() {
  cout << "New Strain: " << id << " Trans: " << transmissibility << " Data: ";
  for ( int i = 0; i < strain_data.size(); ++i ) {
    cout << strain_data[ i ]<< " ";
  } 
  cout << endl;
}

std::string Strain::to_string() {
  std::stringstream ss;
  for ( int i = 0; i < strain_data.size(); ++i ) { 
    ss << setw( 3 ) << setfill( ' ' ) << strain_data[ i ] ;  
  }
  return ss.str();
}

