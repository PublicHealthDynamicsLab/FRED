/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Small_Grid.cc
//

#include <utility>
#include <list>
#include <string>
using namespace std;

#include "Global.h"
#include "Geo_Utils.h"
#include "Small_Grid.h"
#include "Small_Cell.h"
#include "Place_List.h"
#include "Place.h"
#include "Params.h"
#include "Random.h"
#include "Utils.h"
#include "Household.h"
#include "Population.h"
#include "Date.h"

Small_Grid::Small_Grid(double minlon, double minlat, double maxlon, double maxlat) {
  min_lon  = minlon;
  min_lat  = minlat;
  max_lon  = maxlon;
  max_lat  = maxlat;
  printf("min_lon = %f\n", min_lon);
  printf("min_lat = %f\n", min_lat);
  printf("max_lon = %f\n", max_lon);
  printf("max_lat = %f\n", max_lat);
  fflush(stdout);

  get_parameters();
  min_x = 0.0;
  max_x = (max_lon-min_lon)*Geo_Utils::km_per_deg_longitude;
  min_y = 0.0;
  max_y = (max_lat-min_lat)*Geo_Utils::km_per_deg_latitude;
  rows = 1 + (int) (max_y/grid_cell_size);
  cols = 1 + (int) (max_x/grid_cell_size);
  if (Global::Verbose) {
    printf("rows = %d  cols = %d\n",rows,cols);
    printf("max_x = %f  max_y = %f\n",max_x,max_y);
    fflush(stdout);
  }

  grid = new Small_Cell * [rows];
  for (int i = 0; i < rows; i++) {
    grid[i] = new Small_Cell[cols];
  }

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      grid[i][j].setup(this,i,j,j*grid_cell_size,(j+1)*grid_cell_size,
           (rows-i-1)*grid_cell_size,(rows-i)*grid_cell_size);
    }
  }

  if (Global::Verbose > 1) {
    for (int i = 0; i < rows; i++) {
      for (int j = 0; j < cols; j++) {
  printf("print grid[%d][%d]:\n",i,j);
  grid[i][j].print();
      }
    }
  }
}

void Small_Grid::get_parameters() {
  Params::get_param_from_string("grid_small_cell_size", &grid_cell_size);
}

Small_Cell ** Small_Grid::get_neighbors(int row, int col) {
  Small_Cell ** neighbors = new Small_Cell*[9];
  int n = 0;
  for (int i = row-1; i <= row+1; i++) {
    for (int j = col-1; j <= col+1; j++) {
      neighbors[n++] = get_grid_cell(i,j);
    }
  }
  return neighbors;
}


Small_Cell * Small_Grid::get_grid_cell(int row, int col) {
  if ( row >= 0 && col >= 0 && row < rows && col < cols)
    return &grid[row][col];
  else
    return NULL;
}


Small_Cell * Small_Grid::select_random_grid_cell() {
  int row = IRAND(0, rows-1);
  int col = IRAND(0, cols-1);
  return &grid[row][col];
}


Small_Cell * Small_Grid::get_grid_cell_from_cartesian(double x, double y) {
  int row, col;
  row = rows-1 - (int) (y/grid_cell_size);
  col = (int) (x/grid_cell_size);
  // printf("x = %f y = %f, row = %d col = %d\n",x,y,row,col);
  return get_grid_cell(row, col);
}


Small_Cell * Small_Grid::get_grid_cell_from_lat_lon(double lat, double lon) {
  double x, y;
  Geo_Utils::translate_to_cartesian(lat,lon,&x,&y,min_lat,min_lon);
  return get_grid_cell_from_cartesian(x,y);
}


void Small_Grid::quality_control() {
  if (Global::Verbose) {
    fprintf(Global::Statusfp, "grid quality control check\n");
    fflush(Global::Statusfp);
  }
  
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      grid[row][col].quality_control();
    }
  }
  
  FILE *fp;
  fp = fopen("small_grid.dat", "w");
  for (int row = 0; row < rows; row++) {
    if (row%2) {
      for (int col = cols-1; col >= 0; col--) {
  double x = grid[row][col].get_center_x();
  double y = grid[row][col].get_center_y();
  fprintf(fp, "%f %f\n",x,y);
      }
    }
    else {
      for (int col = 0; col < cols; col++) {
  double x = grid[row][col].get_center_x();
  double y = grid[row][col].get_center_y();
  fprintf(fp, "%f %f\n",x,y);
      }
    }
  }
  fclose(fp);
  
  if (Global::Verbose) {
    fprintf(Global::Statusfp, "grid quality control finished\n");
    fflush(Global::Statusfp);
  }
}


// Specific to Small_Cell Small_Grid:


