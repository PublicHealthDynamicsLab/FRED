/*
 * This file is part of the FRED system.
 *
 * Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette, Shawn Brown, 
 * Roni Rosenfield, Alona Fyshe, David Galloway, Nathan Stone, Jay DePasse, 
 * Anuroop Sriram, and Donald Burke
 * All rights reserved.
 *
 * Copyright (c) 2013-2019, University of Pittsburgh, John Grefenstette, Robert Frankeny,
 * David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
 * All rights reserved.
 *
 * FRED is distributed on the condition that users fully understand and agree to all terms of the 
 * End User License Agreement.
 *
 * FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.
 *
 * See the file "LICENSE" for more information.
 */


#ifndef _ABSTRACT_GRID_H
#define _ABSTRACT_GRID_H

#include "Global.h"
#include "Geo.h"

class Abstract_Grid {
  
 public:
  
  int get_rows() {
    return this->rows;
  }

  int get_cols() {
    return this->cols;
  }

  int get_number_of_patches() {
    return this->rows * this->cols;
  }

  fred::geo get_min_lat() {
    return this->min_lat;
  }

  fred::geo get_min_lon() {
    return this->min_lon;
  }

  fred::geo get_max_lat() {
    return this->max_lat;
  }

  fred::geo get_max_lon() {
    return this->max_lon;
  }

  double get_min_x() {
    return this->min_x;
  }

  double get_min_y() {
    return this->min_y;
  }

  double get_max_x() {
    return this->max_x;
  }

  double get_max_y() {
    return this->max_y;
  }

  double get_patch_size() {
    return this->patch_size;
  }

  int get_row(double y) {
    return static_cast<int>(((y - this->min_y) / this->patch_size));
  }

  int get_col(double x) {
    return static_cast<int>(((x - this->min_x) / this->patch_size));
  }

  int get_row(fred::geo lat) {
    double y = Geo::get_y(lat);
    return static_cast<int>(((y - this->min_y) / this->patch_size));
  }

  int get_col(fred::geo lon) {
    double x = Geo::get_x(lon);
    return static_cast<int>(((x - this->min_x) / this->patch_size));
  }

  int get_global_row_min() {
    return this->global_row_min;        // global row coord of S row
  }

  int get_global_col_min() {
    return this->global_col_min;        // global col coord of W col
  }

  int get_global_row_max() {
    return this->global_row_max;        // global row coord of N row
  }

  int get_global_col_max() {
    return this->global_col_max;        // global col coord of E col
  }
  
 protected:

  int rows;          // number of rows
  int cols;          // number of columns
  double patch_size;        // km per side
  fred::geo min_lat;        // lat of SW corner
  fred::geo min_lon;        // lon of SW corner
  fred::geo max_lat;        // lat of NE corner
  fred::geo max_lon;        // lon of NE corner
  double min_x;          // global x of SW corner
  double min_y;          // global y of SW corner
  double max_x;          // global x of NE corner
  double max_y;          // global y of NE corner
  int global_row_min;        // global row coord of S row
  int global_col_min;        // global col coord of W col
  int global_row_max;        // global row coord of N row
  int global_col_max;        // global col coord of E col
};

#endif
