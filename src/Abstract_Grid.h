
#ifndef _ABSTRACT_GRID_H
#define _ABSTRACT_GRID_H

#include "Global.h"

class Abstract_Grid {

public:

  int get_rows() { return rows; }
  int get_cols() { return cols; }
  fred::geo get_min_lat() { return min_lat; }
  fred::geo get_min_lon() { return min_lon; }
  fred::geo get_max_lat() { return max_lat; }
  fred::geo get_max_lon() { return max_lon; }
  double get_min_x() { return min_x; }
  double get_min_y() { return min_y; }
  double get_grid_cell_size() { return grid_cell_size; }
  


protected:

  int rows;          // number of rows
  int cols;          // number of columns
  double grid_cell_size;      // km per side
  fred::geo min_lat;
  fred::geo min_lon;
  fred::geo max_lat;
  fred::geo max_lon;
  double min_x;
  double max_x;
  double min_y;
  double max_y;

};

#endif
