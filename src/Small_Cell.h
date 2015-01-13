/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Small_Cell.h
//

#ifndef _FRED_SMALL_CELL_H
#define _FRED_SMALL_CELL_H

#include "Abstract_Cell.h"

class Small_Grid;

class Small_Cell : public Abstract_Cell {
public:
  Small_Cell() {}
  ~Small_Cell() {}
  void setup(Small_Grid * grd, int i, int j, double xmin, double xmax, double ymin, double ymax);
  void print();
  void print_coord();
  void quality_control();
  double distance_to_grid_cell(Small_Cell *grid_cell2);

protected:
  Small_Grid * grid;
  Small_Cell ** neighbors;
};

#endif // _FRED_SMALL_CELL_H
