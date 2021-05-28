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

//
//
// File: Neighborhood_Layer.cc
//
#include <utility>
#include <list>
#include <string>
#include <vector>
#include <algorithm>
using namespace std;

#include "Global.h"
#include "Geo.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include "Place_Type.h"
#include "Place.h"
#include "Property.h"
#include "Random.h"
#include "Utils.h"
#include "Household.h"
#include "Regional_Layer.h"

Neighborhood_Layer::Neighborhood_Layer() {
  char property[FRED_STRING_SIZE];

  Regional_Layer* base_grid = Global::Simulation_Region;
  this->min_lat = base_grid->get_min_lat();
  this->min_lon = base_grid->get_min_lon();
  this->max_lat = base_grid->get_max_lat();
  this->max_lon = base_grid->get_max_lon();
  this->min_x = base_grid->get_min_x();
  this->min_y = base_grid->get_min_y();
  this->max_x = base_grid->get_max_x();
  this->max_y = base_grid->get_max_y();

  this->offset = NULL;
  this->gravity_cdf = NULL;
  this->max_offset = 0;

  // determine patch size for this layer
  strcpy(property, "Neighborhood.patch_size");
  if(Property::does_property_exist(property)) {
    Property::get_property("Neighborhood.patch_size", &this->patch_size);
  } else {
    Property::get_property("Neighborhood_patch_size", &this->patch_size);
  }

  // determine number of rows and cols
  this->rows = (double)(this->max_y - this->min_y) / this->patch_size;
  if(this->min_y + this->rows * this->patch_size < this->max_y) {
    this->rows++;
  }

  this->cols = (double)(this->max_x - this->min_x) / this->patch_size;
  if(this->min_x + this->cols * this->patch_size < this->max_x) {
    this->cols++;
  }

  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "Neighborhood_Layer min_lon = %f\n", this->min_lon);
    fprintf(Global::Statusfp, "Neighborhood_Layer min_lat = %f\n", this->min_lat);
    fprintf(Global::Statusfp, "Neighborhood_Layer max_lon = %f\n", this->max_lon);
    fprintf(Global::Statusfp, "Neighborhood_Layer max_lat = %f\n", this->max_lat);
    fprintf(Global::Statusfp, "Neighborhood_Layer rows = %d  cols = %d\n", this->rows, this->cols);
    fprintf(Global::Statusfp, "Neighborhood_Layer min_x = %f  min_y = %f\n", this->min_x, this->min_y);
    fprintf(Global::Statusfp, "Neighborhood_Layer max_x = %f  max_y = %f\n", this->max_x, this->max_y);
    fflush(Global::Statusfp);
  }

  // setup patches
  this->grid = new Neighborhood_Patch *[this->rows];
  for(int i = 0; i < this->rows; ++i) {
    this->grid[i] = new Neighborhood_Patch[this->cols];
    for(int j = 0; j < this->cols; ++j) {
      this->grid[i][j].setup(this, i, j);
    }
  }
  // FRED_VERBOSE(0, "setup neighborhood patches finished for %d rows and %d cols\n", this->rows, this->cols);

  // properties to determine neighborhood visitation patterns
  strcpy(property, "Neighborhood.max_distance");
  if(Property::does_property_exist(property)) {
    Property::get_property("Neighborhood.max_distance", &this->max_distance);
  } else {
    Property::get_property("Neighborhood_max_distance", &this->max_distance);
  }
  strcpy(property, "Neighborhood.max_destinations");
  if(Property::does_property_exist(property)) {
    Property::get_property("Neighborhood.max_destinations", &this->max_destinations);
  } else {
    Property::get_property("Neighborhood_max_destinations", &this->max_destinations);
  }
  strcpy(property, "Neighborhood.min_distance");
  if(Property::does_property_exist(property)) {
    Property::get_property("Neighborhood.min_distance", &this->min_distance);
  } else {
    Property::get_property("Neighborhood_min_distance", &this->min_distance);
  }
  strcpy(property, "Neighborhood.distance_exponent");
  if(Property::does_property_exist(property)) {
    Property::get_property("Neighborhood.distance_exponent", &this->dist_exponent);
  } else {
    Property::get_property("Neighborhood_distance_exponent", &this->dist_exponent);
  }
  strcpy(property, "Neighborhood.population_exponent");
  if(Property::does_property_exist(property)) {
    Property::get_property("Neighborhood.population_exponent", &this->pop_exponent);
  } else {
    Property::get_property("Neighborhood_population_exponent", &this->pop_exponent);
  }

}

void Neighborhood_Layer::setup() {

  int type = Place_Type::get_type_id("Neighborhood");

  // create one neighborhood per patch
  for(int i = 0; i < this->rows; ++i) {
    for(int j = 0; j < this->cols; ++j) {
      if(this->grid[i][j].get_houses() > 0) {
        this->grid[i][j].make_neighborhood(type);
      }
    }
  }

  if(Global::Verbose > 1) {
    for(int i = 0; i < this->rows; ++i) {
      for(int j = 0; j < this->cols; ++j) {
        printf("print grid[%d][%d]:\n", i, j);
        this->grid[i][j].print();
      }
    }
  }
}

void Neighborhood_Layer::prepare() {
  FRED_VERBOSE(0, "Neighborhood_Layer prepare entered\n");
  record_activity_groups();
  FRED_VERBOSE(0, "setup gravity model ...\n");
  setup_gravity_model();
  FRED_VERBOSE(0, "setup gravity model complete\n");
  FRED_VERBOSE(0, "Neighborhood_Layer prepare finished\n");
}

Neighborhood_Patch* Neighborhood_Layer::get_patch(Place* place) {
  return get_patch(place->get_latitude(), place->get_longitude());
}

Neighborhood_Patch* Neighborhood_Layer::get_patch(int row, int col) {
  if(row >= 0 && col >= 0 && row < this->rows && col < this->cols) {
    return &this->grid[row][col];
  } else {
    return NULL;
  }
}

Neighborhood_Patch* Neighborhood_Layer::get_patch(fred::geo lat, fred::geo lon) {
  int row = get_row(lat);
  int col = get_col(lon);
  return get_patch(row, col);
}

void Neighborhood_Layer::quality_control() {
  FRED_STATUS(0, "grid quality control check\n");

  int popsize = 0;
  int tot_occ_patches = 0;
  for(int row = 0; row < this->rows; row++) {
    int min_occ_col = this->cols + 1;
    int max_occ_col = -1;
    for(int col = 0; col < this->cols; col++) {
      this->grid[row][col].quality_control();
      int patch_pop = this->grid[row][col].get_popsize();
      if(patch_pop > 0) {
        if(col > max_occ_col) {
          max_occ_col = col;
        }
        if(col < min_occ_col) {
          min_occ_col = col;
        }
        popsize += patch_pop;
      }
    }
    if(min_occ_col < this->cols) {
      int patches_occ = max_occ_col - min_occ_col + 1;
      tot_occ_patches += patches_occ;
    }
  }

  if(Global::Verbose > 1) {
    char filename[FRED_STRING_SIZE];
    sprintf(filename, "%s/grid.dat", Global::Simulation_directory);
    FILE *fp = fopen(filename, "w");
    for(int row = 0; row < rows; row++) {
      if(row % 2) {
        for(int col = this->cols - 1; col >= 0; col--) {
          double x = this->grid[row][col].get_center_x();
          double y = this->grid[row][col].get_center_y();
          fprintf(fp, "%f %f\n", x, y);
        }
      } else {
        for(int col = 0; col < this->cols; col++) {
          double x = this->grid[row][col].get_center_x();
          double y = this->grid[row][col].get_center_y();
          fprintf(fp, "%f %f\n", x, y);
        }
      }
    }
    fclose(fp);
  }

  if(Global::Verbose > 0) {
    int total_area = this->rows * this->cols;
    int convex_area = tot_occ_patches;
    fprintf(Global::Statusfp, "Density: popsize = %d total region = %d total_density = %f\n", popsize,
        total_area, (total_area > 0) ? (double)popsize / (double)total_area : 0.0);
    fprintf(Global::Statusfp, "Density: popsize = %d convex region = %d convex_density = %f\n", popsize,
        convex_area, (convex_area > 0) ? (double)popsize / (double)convex_area : 0.0);
    fprintf(Global::Statusfp, "grid quality control finished\n");
    fflush(Global::Statusfp);
  }
}

void Neighborhood_Layer::quality_control(double min_x, double min_y) {
  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "grid quality control check\n");
    fflush(Global::Statusfp);
  }

  for(int row = 0; row < this->rows; row++) {
    for(int col = 0; col < this->cols; col++) {
      this->grid[row][col].quality_control();
    }
  }

  if(Global::Verbose > 1) {
    char filename[FRED_STRING_SIZE];
    sprintf(filename, "%s/grid.dat", Global::Simulation_directory);
    FILE *fp = fopen(filename, "w");
    for(int row = 0; row < rows; row++) {
      if(row % 2) {
        for(int col = this->cols - 1; col >= 0; col--) {
          double x = this->grid[row][col].get_center_x();
          double y = this->grid[row][col].get_center_y();
          fprintf(fp, "%f %f\n", x, y);
        }
      } else {
        for(int col = 0; col < this->cols; col++) {
          double x = this->grid[row][col].get_center_x();
          double y = this->grid[row][col].get_center_y();
          fprintf(fp, "%f %f\n", x, y);
        }
      }
    }
    fclose(fp);
  }

  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "grid quality control finished\n");
    fflush(Global::Statusfp);
  }
}

int Neighborhood_Layer::get_number_of_neighborhoods() {
  int n = 0;
  for(int row = 0; row < this->rows; row++) {
    for(int col = 0; col < this->cols; col++) {
      if(this->grid[row][col].get_houses() > 0)
        n++;
    }
  }
  return n;
}

void Neighborhood_Layer::record_activity_groups() {
  FRED_VERBOSE(0, "record_daily_activities entered\n");
  for(int row = 0; row < this->rows; row++) {
    for(int col = 0; col < this->cols; col++) {
      Neighborhood_Patch * patch = (Neighborhood_Patch *)&this->grid[row][col];
      if(patch != NULL) {
        if (patch->get_houses() > 0) {
          patch->record_activity_groups();
          patch->prepare();
        }
      }
    }
  }
  FRED_VERBOSE(0, "record_daily_activities finished\n");
}

// Comparison used to sort by probability
static bool compare_pair( const pair<double,int>& p1, const pair<double,int>& p2) {
  return ((p1.first == p2.first) ? (p1.second < p2.second) : (p1.first > p2.first));
}

void Neighborhood_Layer::setup_gravity_model() {
  int tmp_offset[256*256];
  double tmp_prob[256*256];
  int mark[256*256];
  int count = 0;

  // print_distances();  // DEBUGGING

  this->offset = new offset_t*[this->rows];
  this->gravity_cdf = new gravity_cdf_t*[this->rows];
  for(int i = 0; i < rows; ++i) {
    this->offset[i] = new offset_t[this->cols];
    this->gravity_cdf[i] = new gravity_cdf_t[this->cols];
  }

  if(this->max_distance < 0) {
    setup_null_gravity_model();
    return;
  }

  this->max_offset = this->max_distance / this->patch_size;
  assert(this->max_offset < 128);

  for(int i = 0; i < this->rows; ++i) {
    for(int j = 0; j < this->cols; ++j) {
      // set up gravity model for grid[i][j];
      Neighborhood_Patch * patch = (Neighborhood_Patch *)&grid[i][j];
      assert(patch != NULL);
      double x_src = patch->get_center_x();
      double y_src = patch->get_center_y();
      int pop_src = patch->get_popsize();
      if (pop_src == 0) continue;
      count = 0;
      for(int ii = i - this->max_offset; ii < this->rows && ii <= i + this->max_offset; ++ii) {
        if(ii < 0) {
          continue;
        }

        for(int jj = j - this->max_offset; jj < this->cols && jj <= j + this->max_offset; ++jj) {
          if(jj < 0) {
            continue;
          }

          Neighborhood_Patch* dest_patch = (Neighborhood_Patch*)&this->grid[ii][jj];
          assert (dest_patch != NULL);
          int pop_dest = dest_patch->get_popsize();
          if(pop_dest == 0) {
            continue;
          }

          double x_dest = dest_patch->get_center_x();
          double y_dest = dest_patch->get_center_y();
          double dist = sqrt((x_src - x_dest) * (x_src - x_dest) + (y_src - y_dest) * (y_src - y_dest));
          if(this->max_distance < dist) {
            continue;
          }

          double gravity = pow(pop_dest, this->pop_exponent) / (1.0 + pow(dist / this->min_distance, this->dist_exponent));
          int off = 256*(i - ii + this->max_offset) + (j - jj + this->max_offset);

          /*
           * consider income similarity in gravity model
           * double mean_household_income_dest = dest_patch->get_mean_household_income();
           * double income_similarity = mean_household_income_dest / mean_household_income_src;
           * if(income_similarity > 1.0) {
           *   income_similarity = 1.0 / income_similarity;
           * }
           * gravity = ...;
           */
          tmp_offset[count] = off;
          tmp_prob[count++] = gravity;
        }
      }

      // sort by gravity value
      this->sort_pair.clear();
      for(int k = 0; k < count; ++k) {
        this->sort_pair.push_back(pair <double,int> (tmp_prob[k], tmp_offset[k]));
      }
      std::sort(this->sort_pair.begin(), this->sort_pair.end(), compare_pair);

      // keep at most largest max_destinations
      if(count > this->max_destinations) {
        count = this->max_destinations;
      }
      for(int k = 0; k < count; ++k) {
        tmp_prob[k] = this->sort_pair[k].first;
        tmp_offset[k] = this->sort_pair[k].second;
      }
      this->sort_pair.clear();

      // transform gravity values into a prob distribution
      double total = 0.0;
      for(int k = 0; k < count; ++k) {
        total += tmp_prob[k];
      }
      for(int k = 0; k < count; ++k) {
        tmp_prob[k] /= total;
      }

      // convert to cdf
      for(int k = 1; k < count; ++k) {
        tmp_prob[k] += tmp_prob[k - 1];
      }

      // store gravity prob and offsets for this patch
      this->gravity_cdf[i][j].reserve(count);
      this->gravity_cdf[i][j].clear();
      this->offset[i][j].reserve(count);
      this->offset[i][j].clear();
      for(int k = 0; k < count; ++k) {
        this->gravity_cdf[i][j].push_back(tmp_prob[k]);
        this->offset[i][j].push_back(tmp_offset[k]);
      }
    }
  }
}

void Neighborhood_Layer::print_gravity_model() {
  printf("\n=== GRAVITY MODEL ========================================================\n");
  for(int i_src = 0; i_src < rows; i_src++) {
    for(int j_src = 0; j_src < cols; j_src++) {
      Neighborhood_Patch * src_patch = (Neighborhood_Patch *)&grid[i_src][j_src];
      double x_src = src_patch->get_center_x();
      double y_src = src_patch->get_center_y();
      int pop_src = src_patch->get_popsize();
      if(pop_src == 0) {
        continue;
      }
      int count = (int)this->offset[i_src][j_src].size();
      for(int k = 0; k < count; ++k) {
        int off = this->offset[i_src][j_src][k];
        printf("GRAVITY_MODEL row %3d col %3d pop %5d count %4d k %4d offset %d ", i_src, j_src, pop_src, count, k, off);
        int i_dest = i_src + this->max_offset - (off / 256);
        int j_dest = j_src + this->max_offset - (off % 256);
        printf("row %3d col %3d ", i_dest, j_dest);
        Neighborhood_Patch* dest_patch = this->get_patch(i_dest, j_dest);
        assert (dest_patch != NULL);
        double x_dest = dest_patch->get_center_x();
        double y_dest = dest_patch->get_center_y();
        double dist = sqrt((x_src-x_dest)*(x_src-x_dest) + (y_src - y_dest) * (y_src - y_dest));
        int pop_dest = dest_patch->get_popsize();
        double gravity_prob = this->gravity_cdf[i_src][j_src][k];
        if(k > 0) {
          gravity_prob -= this->gravity_cdf[i_src][j_src][k-1];
        }
        printf("pop %5d dist %0.4f prob %f", pop_dest, dist, gravity_prob);
        printf("\n");
      }
      printf("\n");
    }
  }
}

void Neighborhood_Layer::print_distances() {
  FILE *fp;
  fp = fopen("all_distances.dat", "w");
  for(int i_src = 0; i_src < this->rows; ++i_src) {
    for(int j_src = 0; j_src < this->cols; ++j_src) {
      Neighborhood_Patch* src_patch = (Neighborhood_Patch*)&this->grid[i_src][j_src];
      double x_src = src_patch->get_center_x();
      double y_src = src_patch->get_center_y();
      int pop_src = src_patch->get_popsize();
      if(pop_src == 0) {
        continue;
      }

      for(int i_dest = 0; i_dest < this->rows; ++i_dest) {
        for(int j_dest = 0; j_dest < this->cols; ++j_dest) {
          if(i_dest < i_src) {
            continue;
          }
          if(i_dest == i_src and j_dest < j_src) {
            continue;
          }
          fprintf(fp,"row %3d col %3d pop %5d ", i_src, j_src, pop_src);
          fprintf(fp,"row %3d col %3d ", i_dest, j_dest);
          Neighborhood_Patch* dest_patch = this->get_patch(i_dest, j_dest);
          assert (dest_patch != NULL);
          double x_dest = dest_patch->get_center_x();
          double y_dest = dest_patch->get_center_y();
          double dist = sqrt((x_src-x_dest)*(x_src-x_dest) + (y_src - y_dest) * (y_src - y_dest));
          int pop_dest = dest_patch->get_popsize();
          fprintf(fp, "pop %5d dist %0.4f\n", pop_dest, dist);
        }
      }
    }
  }
  fclose(fp);
  exit(0);
}

void Neighborhood_Layer::setup_null_gravity_model() {
  int tmp_offset[256*256];
  double tmp_prob[256*256];
  int count = 0;

  this->offset = new offset_t*[this->rows];
  this->gravity_cdf = new gravity_cdf_t*[this->rows];
  this->offset[0] = new offset_t[this->cols];
  this->gravity_cdf[0] = new gravity_cdf_t[this->cols];

  this->max_offset = this->rows * this->patch_size;
  assert(this->max_offset < 128);

  for(int i_dest = 0; i_dest < this->rows; ++i_dest) {
    for(int j_dest = 0; j_dest < this->cols; ++j_dest) {
      Neighborhood_Patch* dest_patch = this->get_patch(i_dest, j_dest);
      int pop_dest = dest_patch->get_popsize();
      if(pop_dest == 0) {
        continue;
      }
      // double gravity = pow(pop_dest,pop_exponent);
      double gravity = pop_dest;
      int off = 256 * (0 - i_dest + this->max_offset) + (0 - j_dest + this->max_offset);
      tmp_offset[count] = off;
      tmp_prob[count++] = gravity;
    }
  }

  // transform gravity values into a prob distribution
  double total = 0.0;
  for(int k = 0; k < count; ++k) {
    total += tmp_prob[k];
  }
  for(int k = 0; k < count; ++k) {
    tmp_prob[k] /= total;
  }

  // convert to cdf
  for (int k = 1; k < count; ++k) {
    tmp_prob[k] += tmp_prob[k - 1];
  }

  // store gravity prob and offsets for this patch
  this->gravity_cdf[0][0].reserve(count);
  this->gravity_cdf[0][0].clear();
  this->offset[0][0].reserve(count);
  this->offset[0][0].clear();
  for(int k = 0; k < count; ++k) {
    this->gravity_cdf[0][0].push_back(tmp_prob[k]);
    this->offset[0][0].push_back(tmp_offset[k]);
  }
}

Place* Neighborhood_Layer::select_destination_neighborhood(Place* src_neighborhood) {

  Neighborhood_Patch* src_patch = this->get_patch(src_neighborhood->get_latitude(), src_neighborhood->get_longitude());
  int i_src = src_patch->get_row();
  int j_src = src_patch->get_col();
  if(this->max_distance < 0) {
    // use null gravity model
    i_src = j_src = 0;
  }
  int offset_index = Random::draw_from_cdf_vector(this->gravity_cdf[i_src][j_src]);
  int off = this->offset[i_src][j_src][offset_index];
  int i_dest = i_src + this->max_offset - (off / 256);
  int j_dest = j_src + this->max_offset - (off % 256);

  Neighborhood_Patch* dest_patch = this->get_patch(i_dest, j_dest);
  assert (dest_patch != NULL);

  return dest_patch->get_neighborhood();
}

/**
 * Adds a pointer to a Place to this Neighborhood Layer. A patch in this Layer's Grid will be found using the Neighborhood.patch_size and the
 * Min and Max latitude and longitude of this Layer.
 *
 * If a Place has a latitude and/or longitude that is outside of the range of this layer
 * (this->min_lat, this->max_lat) (this->min_lon, this->max_lon) then a warning will be issued and the place will not be added to a patch and
 * the place's patch will be set to NULL.
 *
 * @param place a pointer to the place that we want to add to this layer
 */
void Neighborhood_Layer::add_place(Place* place) {
  int row = get_row(place->get_latitude());
  int col = get_col(place->get_longitude());
  Neighborhood_Patch* patch = Global::Neighborhoods->get_patch(row, col);
  if(patch == NULL) {
    // Raised the verbosity to a 1 since we really don't want this filling up the LOG file
    FRED_VERBOSE(1, "WARNING: place %d %s has bad patch,  lat = %f (not in [%f, %f])  lon = %f (not in [%f, %f])\n", place->get_id(), place->get_label(),
        place->get_latitude(), this->min_lat, this->max_lat,
        place->get_longitude(), this->min_lon, this->max_lon);
  } else {
    patch->add_place(place);
  }
  place->set_patch(patch);
}

