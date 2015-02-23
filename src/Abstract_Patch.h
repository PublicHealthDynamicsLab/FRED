/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_ABSTRACT_PATCH_H
#define _FRED_ABSTRACT_PATCH_H

#include <stdio.h>

class Abstract_Patch {

public:
  int get_row() {
    return this->row;
  }

  int get_col() {
    return this->col;
  }

  double get_min_x() {
    return this->min_x;
  }

  double get_max_x() {
    return this->max_x;
  }

  double get_min_y() {
    return this->min_y;
  }

  double get_max_y() {
    return this->max_y;
  }

  double get_center_y() {
    return this->center_y;
  }

  double get_center_x() {
    return this->center_x;
  }

  void print() {
    printf("patch %d %d: %f, %f, %f, %f\n", this->row, this->col, this->min_x, this->min_y, this->max_x, this->max_y);
  }

protected:
  int row;
  int col;
  double min_x;
  double max_x;
  double min_y;
  double max_y;
  double center_x;
  double center_y;
};

#endif
