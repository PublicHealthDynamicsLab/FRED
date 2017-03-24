/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
//  File: Seasonality.h
//

#ifndef _FRED_SEASONALITY_H
#define _FRED_SEASONALITY_H

#include "Abstract_Grid.h"
#include "Seasonality_Timestep_Map.h"
#include "Condition.h"
#include "Population.h"
#include "Global.h"
#include <vector>

class Abstract_Grid;

class Seasonality {

public:

  Seasonality(Abstract_Grid * grid);
  //~Seasonality(Abstract_Grid * grid);
  void update(int day);
  double get_seasonality_multiplier_by_lat_lon(fred::geo lat, fred::geo lon, int condition_id);
  double get_seasonality_multiplier_by_cartesian(double x, double y, int condition_id);
  double get_seasonality_multiplier(int row, int col, int condition_id);
  double get_average_seasonality_multiplier(int condition_id);

  void print();
  void print_summary();

private:
  void update_seasonality_multiplier();
  double ** seasonality_values; // rectangular array of seasonality values
  vector < double ** > seasonality_multiplier; // result of applying each condition's seasonality/transmissibily kernel
  Seasonality_Timestep_Map * seasonality_timestep_map;
  Abstract_Grid * grid;

  struct point {
    int x, y;
    double value;
    point(int _x, int _y, double _value) {
      x = _x; y = _y; value = _value;
    }
  };

  void nearest_neighbor_interpolation(vector <point> points, double *** field);
  void print_field(double *** field);
};

#endif // _FRED_SEASONALITY_H
