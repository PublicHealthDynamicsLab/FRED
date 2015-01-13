/*
   Copyright 2009 by the University of Pittsburgh
   Licensed under the Academic Free License version 3.0
   See the file "LICENSE" for more information
   */

//
//
// File: Large_Grid.cc
//

#include <utility>
#include <list>
#include <string>
using namespace std;

#include "Global.h"
#include "Geo_Utils.h"
#include "Large_Grid.h"
#include "Large_Cell.h"
#include "Place_List.h"
#include "Place.h"
#include "Params.h"
#include "Random.h"
#include "Utils.h"
#include "Household.h"
#include "Population.h"
#include "Date.h"

Large_Grid::Large_Grid(fred::geo minlon, fred::geo minlat, fred::geo maxlon, fred::geo maxlat) {
  min_lon  = minlon;
  min_lat  = minlat;
  max_lon  = maxlon;
  max_lat  = maxlat;
  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "Large_Grid min_lon = %f\n", min_lon);
    fprintf(Global::Statusfp, "Large_Grid min_lat = %f\n", min_lat);
    fprintf(Global::Statusfp, "Large_Grid max_lon = %f\n", max_lon);
    fprintf(Global::Statusfp, "Large_Grid max_lat = %f\n", max_lat);
    fflush(Global::Statusfp);
  }

  get_parameters();
  min_x = 0.0;
  min_y = 0.0;

  // find the global x,y coordinates of SW corner of grid
  min_x = (min_lon + 180.0)*Geo_Utils::km_per_deg_longitude;
  min_y = (min_lat + 90.0)*Geo_Utils::km_per_deg_latitude;

  // find the global row and col in which SW corner occurs
  global_row_min = (int) (min_y / grid_cell_size);
  global_col_min = (int) (min_x / grid_cell_size);

  // align min_x and min_y to global grid
  min_x = global_col_min * grid_cell_size;
  min_y = global_row_min * grid_cell_size;

  // find the global x,y coordinates of NE corner of grid
  max_x = (max_lon + 180.0)*Geo_Utils::km_per_deg_longitude;
  max_y = (max_lat + 90.0)*Geo_Utils::km_per_deg_latitude;

  // find the global row and col in which NE corner occurs
  global_row_max = (int) (max_y / grid_cell_size);
  global_col_max = (int) (max_x / grid_cell_size);

  // align max_x and max_y to global grid
  max_x = (global_col_max +1) * grid_cell_size;
  max_y = (global_row_max +1) * grid_cell_size;

  rows = global_row_max - global_row_min + 1;
  cols = global_col_max - global_col_min + 1;

  // compute lat,lon of aligned grid
  min_lat = min_y / Geo_Utils::km_per_deg_latitude - 90.0;
  min_lon = min_x / Geo_Utils::km_per_deg_longitude - 180.0;
  max_lat = max_y / Geo_Utils::km_per_deg_latitude - 90.0;
  max_lon = max_x / Geo_Utils::km_per_deg_longitude - 180.0;

  // Added by Anuroop
  //fprintf(Global::Statusfp, "Large_Grid global_col_min = %d  global_row_min = %d\n",
  //  global_col_min, global_row_min);
  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "Large_Grid new min_lon = %f\n", min_lon);
    fprintf(Global::Statusfp, "Large_Grid new min_lat = %f\n", min_lat);
    fprintf(Global::Statusfp, "Large_Grid new max_lon = %f\n", max_lon);
    fprintf(Global::Statusfp, "Large_Grid new max_lat = %f\n", max_lat);
    fprintf(Global::Statusfp, "Large_Grid global_col_min = %d  global_row_min = %d\n",
        global_col_min, global_row_min);
    fprintf(Global::Statusfp, "Large_Grid global_col_max = %d  global_row_max = %d\n",
        global_col_max, global_row_max);
    fprintf(Global::Statusfp, "Large_Grid rows = %d  cols = %d\n",rows,cols);
    fprintf(Global::Statusfp, "Large_Grid min_x = %f  min_y = %f\n",min_x,min_y);
    fprintf(Global::Statusfp, "Large_Grid max_x = %f  max_y = %f\n",max_x,max_y);
    fflush(Global::Statusfp);
  }

  grid = new Large_Cell * [rows];
  for (int i = 0; i < rows; i++) {
    grid[i] = new Large_Cell[cols];
    for (int j = 0; j < cols; j++) {
      grid[i][j].setup(this,i,j,j*grid_cell_size,(j+1)*grid_cell_size,
          i*grid_cell_size,(i+1)*grid_cell_size);
      if (Global::Verbose > 2) {
        printf("print grid[%d][%d]:\n",i,j);
        grid[i][j].print();
      }
    }
  }

}

void Large_Grid::get_parameters() {
  Params::get_param_from_string("grid_large_cell_size", &grid_cell_size);
}

Large_Cell ** Large_Grid::get_neighbors(int row, int col) {
  Large_Cell ** neighbors = new Large_Cell*[9];
  int n = 0;
  for (int i = row-1; i <= row+1; i++) {
    for (int j = col-1; j <= col+1; j++) {
      neighbors[n++] = get_grid_cell(i,j);
    }
  }
  return neighbors;
}


Large_Cell * Large_Grid::get_grid_cell(int row, int col) {
  if ( row >= 0 && col >= 0 && row < rows && col < cols)
    return &grid[row][col];
  else
    return NULL;
}


Large_Cell * Large_Grid::get_grid_cell_with_global_coords(int row, int col) {
  return get_grid_cell(row-global_row_min, col-global_col_min);
}


Large_Cell * Large_Grid::select_random_grid_cell() {
  int row = IRAND(0, rows-1);
  int col = IRAND(0, cols-1);
  return &grid[row][col];
}


Large_Cell * Large_Grid::get_grid_cell_from_cartesian(double x, double y) {
  int row, col;
  row = (int) (y/grid_cell_size);
  col = (int) (x/grid_cell_size);
  return get_grid_cell(row, col);
}


Large_Cell * Large_Grid::get_grid_cell_from_lat_lon(fred::geo lat, fred::geo lon) {
  double x, y;
  Geo_Utils::translate_to_cartesian(lat,lon,&x,&y,min_lat,min_lon);
  return get_grid_cell_from_cartesian(x,y);
}


void Large_Grid::quality_control(char * directory) {
  if (Global::Verbose) {
    fprintf(Global::Statusfp, "grid quality control check\n");
    fflush(Global::Statusfp);
  }

  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      grid[row][col].quality_control();
    }
  }

  if (Global::Verbose>1) {
    char filename [256];
    sprintf(filename, "%s/large_grid.dat", directory);
    FILE *fp = fopen(filename, "w");
    for (int row = 0; row < rows; row++) {
      if (row%2) {
        for (int col = cols-1; col >= 0; col--) {
          double x = min_x + grid[row][col].get_center_x();
          double y = min_y + grid[row][col].get_center_y();
          fprintf(fp, "%f %f\n",x,y);
        }
      }
      else {
        for (int col = 0; col < cols; col++) {
          double x = min_x + grid[row][col].get_center_x();
          double y = min_y + grid[row][col].get_center_y();
          fprintf(fp, "%f %f\n",x,y);
        }
      }
    }
    fclose(fp);
  }

  if (Global::Verbose) {
    fprintf(Global::Statusfp, "grid quality control finished\n");
    fflush(Global::Statusfp);
  }
}


// Specific to Large_Cell Large_Grid:

void Large_Grid::set_population_size() {
  int pop_size = Global::Pop.get_pop_size();
  for (int p = 0; p < pop_size; p++) {
    Person *per = Global::Pop.get_person_by_index(p);
    Place * h = per->get_household();
    assert (h != NULL);
    fred::geo lat = h->get_latitude();
    fred::geo lon = h->get_longitude();
    Large_Cell * cell = get_grid_cell_from_lat_lon(lat, lon);
    cell->add_person(per);
  }
  return;
  FILE *fp;
  char filename[256];
  sprintf(filename, "OUT/largegrid.txt");
  fp = fopen(filename,"w");
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      Large_Cell * cell = get_grid_cell(i,j);
      fred::geo lon, lat;
      double x = cell->get_center_x();
      double y = cell->get_center_y();
      Geo_Utils::translate_to_lat_lon(x,y,&lat,&lon,min_lat,min_lon);
      int popsize = cell->get_popsize();
      fprintf(fp, "%d %d %f %f %f %f %d\n", i,j,x,y,lon,lat,popsize);
    }
    fprintf(fp, "\n");
  }
  fclose(fp);
}

void Large_Grid::translate_to_lat_lon(double x, double y, fred::geo *lat, fred::geo *lon) {
  Geo_Utils::translate_to_lat_lon(x,y,lat,lon,min_lat,min_lon);
}

void Large_Grid::translate_to_cartesian(fred::geo lat, fred::geo lon, double *x, double *y) {
  Geo_Utils::translate_to_cartesian(lat,lon,x,y,min_lat,min_lon);
}

void Large_Grid::read_max_popsize() {
  int r,c, n;
  char filename[256];
  if (Global::Enable_Travel) {
    Params::get_param_from_string("cell_popfile", filename);
    FILE *fp = Utils::fred_open_file(filename);
    if (fp == NULL) {
      Utils::fred_abort("Help! Can't open cell_pop_file %s\n", filename);
    }
    printf("reading %s\n", filename);
    while (fscanf(fp, "%d %d %d ", &c,&r,&n) == 3) {
      Large_Cell * cell = get_grid_cell_with_global_coords(r,c);
      if (cell != NULL) {
        cell->set_max_popsize(n);
      }
    }
    fclose(fp);
    printf("finished reading %s\n", filename);
  }
}
