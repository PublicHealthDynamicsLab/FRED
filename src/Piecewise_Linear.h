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
// File: Piecewise_Linear.h
//

#ifndef _FRED_PIECEWISE_LINEAR_H
#define _FRED_PIECEWISE_LINEAR_H

#include <stdio.h>
#include <vector>
#include <string>

#include "Global.h"
#include "Params.h"
#include "Disease.h"

class Disease;

using namespace std;

class Piecewise_Linear {

public:

  Piecewise_Linear();
  void setup(string _name, Disease *_disease);
  double get_prob( double distance );

private:

  bool quality_control();

  string name;
  Disease *disease;
  vector < double > ag_distances;     // Antigenic distances
  vector < double > probabilities;    // Corresponding values of the function
};

#endif
