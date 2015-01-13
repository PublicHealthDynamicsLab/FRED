/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Large_Grid.h
//

#ifndef _FRED_LARGE_GRID_H
#define _FRED_LARGE_GRID_H

#include <string.h>

#include "Place.h"
#include "Abstract_Grid.h"
#include "Global.h"

class Large_Cell;

class Large_Grid : public Abstract_Grid {
public:
  Large_Grid(fred::geo minlon, fred::geo minlat, fred::geo maxlon, fred::geo maxlat);
  ~Large_Grid() {}
  void get_parameters();
  Large_Cell ** get_neighbors(int row, int col);
  Large_Cell * get_grid_cell(int row, int col);
  Large_Cell * get_grid_cell_with_global_coords(int row, int col);
  Large_Cell * select_random_grid_cell();
  Large_Cell * get_grid_cell_from_cartesian(double x, double y);
  Large_Cell * get_grid_cell_from_lat_lon(fred::geo lat, fred::geo lon);
  void set_population_size();
  void quality_control(char * directory);

  // Added by Anuroop
  int getGlobalRow(int row) { return row + global_row_min; }
  int getGlobalCol(int col) { return col + global_col_min; }

  /**
   * Translate a given (x,y) coordinate to a latitude and longitude.
   *
   * @param x the x coordinate of the point
   * @param y the y coordinate of the point
   * @param lat pointer to the latitude of the point
   * @param lon pointer to the longitude of the point
   * @see Geo_Utils::translate_to_lat_lon(double x, double y, double *lat, double *lon, double min_lat, double min_lon)
   */
  void translate_to_lat_lon(double x, double y, fred::geo *lat, fred::geo *lon);

  /**
   * Translate a given latitude and longitude to an (x,y) coordinate.
   *
   * @param lat the latitude of the point
   * @param lon the longitude of the point
   * @param x pointer to the x coordinate of the point
   * @param y pointer to the y coordinate of the point
   * @see Geo_Utils::translate_to_cartesian(double lat, double lon, double *x, double *y, double min_lat, double min_lon)
   */
  void translate_to_cartesian(fred::geo lat, fred::geo lon, double *x, double *y);

  void read_max_popsize();

protected:
  Large_Cell ** grid;            // Rectangular array of grid_cells
  int global_row_min;
  int global_col_min;
  int global_row_max;
  int global_col_max;
};

#endif // _FRED_LARGE_GRID_H
