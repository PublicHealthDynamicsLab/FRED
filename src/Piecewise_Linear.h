/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Piecewise_Linear.h
//

#ifndef _FRED_PIECEWISE_LINEAR_H
#define _FRED_PIECEWISE_LINEAR_H

#include "Global.h"
#include <stdio.h>
#include <vector>
#include <string>

#include "Params.h"
#include "Disease.h"

class Disease;

using namespace std;

class Piecewise_Linear{
public:
  Piecewise_Linear();
  void setup(string _name, Disease *_disease);
  double get_prob(double distance);

private:
  bool quality_control();

  string name;
  Disease *disease;
  vector <double> ag_distances; // Antigenic distances
  vector <double> probabilities;    // Corresponding values of the function
};

#endif
