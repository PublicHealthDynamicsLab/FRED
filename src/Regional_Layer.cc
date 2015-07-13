/*
 This file is part of the FRED system.

 Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
 Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
 Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

 Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
 more information.
 */

//
//
// File: Regional_Layer.cc
//
#include <utility>
#include <list>
#include <string>
using namespace std;

#include "Global.h"
#include "Geo_Utils.h"
#include "Regional_Layer.h"
#include "Regional_Patch.h"
#include "Place_List.h"
#include "Place.h"
#include "Params.h"
#include "Random.h"
#include "Utils.h"
#include "Household.h"
#include "Population.h"

Regional_Layer::Regional_Layer(fred::geo minlon, fred::geo minlat, fred::geo maxlon, fred::geo maxlat) {
  this->min_lon = minlon;
  this->min_lat = minlat;
  this->max_lon = maxlon;
  this->max_lat = maxlat;
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "Regional_Layer min_lon = %f\n", this->min_lon);
    fprintf(Global::Statusfp, "Regional_Layer min_lat = %f\n", this->min_lat);
    fprintf(Global::Statusfp, "Regional_Layer max_lon = %f\n", this->max_lon);
    fprintf(Global::Statusfp, "Regional_Layer max_lat = %f\n", this->max_lat);
    fflush(Global::Statusfp);
  }

  // read in the patch size for this layer
  Params::get_param_from_string("regional_patch_size", &this->patch_size);

  // find the global x,y coordinates of SW corner of grid
  this->min_x = Geo_Utils::get_x(this->min_lon);
  this->min_y = Geo_Utils::get_y(this->min_lat);

  // find the global row and col in which SW corner occurs
  this->global_row_min = (int)(this->min_y / this->patch_size);
  this->global_col_min = (int)(this->min_x / this->patch_size);

  // align coords to global grid
  this->min_x = this->global_col_min * this->patch_size;
  this->min_y = this->global_row_min * this->patch_size;

  // compute lat,lon of SW corner of aligned grid
  this->min_lat = Geo_Utils::get_latitude(this->min_y);
  this->min_lon = Geo_Utils::get_longitude(this->min_x);

  // find x,y coords of NE corner of bounding box
  this->max_x = Geo_Utils::get_x(this->max_lon);
  this->max_y = Geo_Utils::get_y(this->max_lat);

  // find the global row and col in which NE corner occurs
  this->global_row_max = (int)(this->max_y / this->patch_size);
  this->global_col_max = (int)(this->max_x / this->patch_size);

  // align coords_y to global grid
  this->max_x = (this->global_col_max + 1) * this->patch_size;
  this->max_y = (this->global_row_max + 1) * this->patch_size;

  // compute lat,lon of NE corner of aligned grid
  this->max_lat = Geo_Utils::get_latitude(this->max_y);
  this->max_lon = Geo_Utils::get_longitude(this->max_x);

  // number of rows and columns needed
  this->rows = this->global_row_max - this->global_row_min + 1;
  this->cols = this->global_col_max - this->global_col_min + 1;

  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "Regional_Layer new min_lon = %f\n", this->min_lon);
    fprintf(Global::Statusfp, "Regional_Layer new min_lat = %f\n", this->min_lat);
    fprintf(Global::Statusfp, "Regional_Layer new max_lon = %f\n", this->max_lon);
    fprintf(Global::Statusfp, "Regional_Layer new max_lat = %f\n", this->max_lat);
    fprintf(Global::Statusfp, "Regional_Layer rows = %d  cols = %d\n", this->rows, this->cols);
    fprintf(Global::Statusfp, "Regional_Layer min_x = %f  min_y = %f\n", this->min_x, this->min_y);
    fprintf(Global::Statusfp, "Regional_Layer max_x = %f  max_y = %f\n", this->max_x, this->max_y);
    fprintf(Global::Statusfp, "Regional_Layer global_col_min = %d  global_row_min = %d\n",
        this->global_col_min, this->global_row_min);
    fflush(Global::Statusfp);
  }

  this->grid = new Regional_Patch *[this->rows];
  for(int i = 0; i < this->rows; ++i) {
    this->grid[i] = new Regional_Patch[this->cols];
  }
  for(int i = 0; i < this->rows; ++i) {
    for(int j = 0; j < this->cols; ++j) {
      this->grid[i][j].setup(this, i, j);
      if(Global::Verbose > 1) {
        printf("print grid[%d][%d]:\n", i, j);
        fflush(stdout);
        this->grid[i][j].print();
      }
      //printf( "row = %d col = %d id = %d\n", i, j, grid[i][j].get_id() );
    }
  }
}

Regional_Patch* Regional_Layer::get_patch(int row, int col) {
  if(row >= 0 && col >= 0 && row < this->rows && col < this->cols) {
    return &this->grid[row][col];
  } else {
    return NULL;
  }
}

Regional_Patch* Regional_Layer::get_patch(fred::geo lat, fred::geo lon) {
  int row = get_row(lat);
  int col = get_col(lon);
  if(row >= 0 && col >= 0 && row < this->rows && col < this->cols) {
    return &this->grid[row][col];
  } else {
    return NULL;
  }
}

Regional_Patch* Regional_Layer::get_patch(Place* place) {
  return get_patch(place->get_latitude(), place->get_longitude());
}

Regional_Patch* Regional_Layer::get_patch_with_global_coords(int row, int col) {
  return get_patch(row - this->global_row_min, col - this->global_col_min);
}

Regional_Patch* Regional_Layer::get_patch_from_id(int id) {
  int row = id / this->cols;
  int col = id % this->cols;
  FRED_VERBOSE(4,
      "patch lookup for id = %d ... calculated row = %d, col = %d, rows = %d, cols = %d\n", id,
      row, col, rows, cols);
  assert(this->grid[row][col].get_id() == id);
  return &(this->grid[row][col]);
}

Regional_Patch* Regional_Layer::select_random_patch() {
  int row = Random::draw_random_int(0, this->rows - 1);
  int col = Random::draw_random_int(0, this->cols - 1);
  return &this->grid[row][col];
}

void Regional_Layer::quality_control() {
  if(Global::Verbose) {
    fprintf(Global::Statusfp, "grid quality control check\n");
    fflush(Global::Statusfp);
  }

  for(int row = 0; row < this->rows; ++row) {
    for(int col = 0; col < this->cols; ++col) {
      this->grid[row][col].quality_control();
    }
  }

  if(Global::Verbose > 1) {
    char filename[FRED_STRING_SIZE];
    sprintf(filename, "%s/large_grid.dat", Global::Simulation_directory);
    FILE* fp = fopen(filename, "w");
    for(int row = 0; row < this->rows; ++row) {
      if(row % 2) {
        for(int col = this->cols - 1; col >= 0; --col) {
          double x = this->grid[row][col].get_center_x();
          double y = this->grid[row][col].get_center_y();
          fprintf(fp, "%f %f\n", x, y);
        }
      } else {
        for(int col = 0; col < this->cols; ++col) {
          double x = this->grid[row][col].get_center_x();
          double y = this->grid[row][col].get_center_y();
          fprintf(fp, "%f %f\n", x, y);
        }
      }
    }
    fclose(fp);
  }

  if(Global::Verbose) {
    fprintf(Global::Statusfp, "grid quality control finished\n");
    fflush(Global::Statusfp);
  }
}

// Specific to Regional_Patch Regional_Layer:

void Regional_Layer::set_population_size() {
  for(int p = 0; p < Global::Pop.get_index_size(); ++p) {
    Person* person = Global::Pop.get_person_by_index(p);
    if(person != NULL) {
      Place* hh = person->get_household();
      if(hh == NULL) {
        if(Global::Enable_Hospitals && person->is_hospitalized() && person->get_permanent_household() != NULL) {
          hh = person->get_permanent_household();
        }
      }
      assert(hh != NULL);
      int row = get_row(hh->get_latitude());
      int col = get_col(hh->get_longitude());
      Regional_Patch* patch = get_patch(row, col);
      patch->add_person(person);
    }
  }
}

void Regional_Layer::read_max_popsize() {
  int r, c, n;
  char filename[FRED_STRING_SIZE];
  if(Global::Enable_Travel) {
    Params::get_param_from_string("regional_patch_popfile", filename);
    FILE* fp = Utils::fred_open_file(filename);
    if(fp == NULL) {
      Utils::fred_abort("Help! Can't open patch_pop_file %s\n", filename);
    }
    printf("reading %s\n", filename);
    while(fscanf(fp, "%d %d %d ", &c, &r, &n) == 3) {
      Regional_Patch* patch = get_patch_with_global_coords(r, c);
      if(patch != NULL) {
        patch->set_max_popsize(n);
      }
    }
    fclose(fp);
    printf("finished reading %s\n", filename);
  }
}

void Regional_Layer::report_grid_stats(int day) {
#if SQLITE
  for(int dis = 0; dis < Global::Diseases; ++dis) {
#pragma omp parallel for schedule(runtime)
    for(int r = 0; r < get_rows(); ++r) {
      for(int c = 0; c < get_cols(); ++c) {
        Global::db.enqueue_transaction(this->grid[r][c].collect_patch_stats(day, dis));
      }
    }
  }
#endif
}

void Regional_Layer::add_workplace(Place* place) {
  Regional_Patch* patch = this->get_patch(place);
  if(patch != NULL) {
    patch->add_workplace(place);
  }
}


Place *Regional_Layer::get_nearby_workplace(int row, int col, double x, double y, int min_staff,
    int max_staff, double* min_dist) {
  //find nearest workplace that has right number of employees
  Place* nearby_workplace = NULL;
  *min_dist = 1e99;
  for(int i = row - 1; i <= row + 1; ++i) {
    for(int j = col - 1; j <= col + 1; ++j) {
      Regional_Patch * patch = get_patch(i, j);
      if(patch != NULL) {
	// printf("Looking for nearby workplace in row %d col %d\n", i, j); fflush(stdout);
        Place* closest_workplace = patch->get_closest_workplace(x, y, min_staff, max_staff,
            min_dist);
        if(closest_workplace != NULL) {
          nearby_workplace = closest_workplace;
        } else {
	        // printf("No nearby workplace in row %d col %d\n", i, j); fflush(stdout);
	      }
      }
    }
  }
  return nearby_workplace;
}

void Regional_Layer::unenroll(fred::geo lat, fred::geo lon, Person* person) {
  Regional_Patch* regional_patch = this->get_patch(lat, lon);
  if(regional_patch != NULL) {
    regional_patch->unenroll(person);
  }
}

