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
#include "Place_List.h"
#include "Place.h"
#include "School.h"
#include "Params.h"
#include "Random.h"
#include "Utils.h"
#include "Household.h"
#include "Population.h"
#include "Regional_Layer.h"

Neighborhood_Layer::Neighborhood_Layer() {
  Regional_Layer * base_grid = Global::Simulation_Region;
  this->min_lat = base_grid->get_min_lat();
  this->min_lon = base_grid->get_min_lon();
  this->max_lat = base_grid->get_max_lat();
  this->max_lon = base_grid->get_max_lon();
  this->min_x = base_grid->get_min_x();
  this->min_y = base_grid->get_min_y();
  this->max_x = base_grid->get_max_x();
  this->max_y = base_grid->get_max_y();

  // determine patch size for this layer
  Params::get_param_from_string("neighborhood_patch_size", &this->patch_size);

  // determine number of rows and cols
  this->rows = (double)(this->max_y - this->min_y) / this->patch_size;
  if (this->min_y + this->rows * this->patch_size < this->max_y) this->rows++;

  this->cols = (double)(this->max_x - this->min_x) / this->patch_size;
  if (this->min_x + this->cols * this->patch_size < this->max_x) this->cols++;

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
  for(int i = 0; i < this->rows; i++) {
    this->grid[i] = new Neighborhood_Patch[this->cols];
    for(int j = 0; j < this->cols; j++) {
      this->grid[i][j].setup(this, i, j);
    }
  }

  // params to determine neighborhood visitation patterns
  int temp_int = 0;
  Params::get_param_from_string("enable_neighborhood_gravity_model", &temp_int);
  this->Enable_neighborhood_gravity_model = (temp_int == 0 ? false : true);

  Params::get_param_from_string("neighborhood_max_distance", &this->max_distance);
  Params::get_param_from_string("neighborhood_max_destinations", &this->max_destinations);
  Params::get_param_from_string("neighborhood_min_distance", &this->min_distance);
  Params::get_param_from_string("neighborhood_distance_exponent", &this->dist_exponent);
  Params::get_param_from_string("neighborhood_population_exponent", &this->pop_exponent);

  // params for old neighborhood model (deprecated)
  Params::get_param_from_string("community_distance", &this->Community_distance);
  Params::get_param_from_string("community_prob", &this->Community_prob);
  Params::get_param_from_string("home_neighborhood_prob", &this->Home_neighborhood_prob);
}

void Neighborhood_Layer::setup() {
  // create one neighborhood per patch
  for(int i = 0; i < this->rows; i++) {
    for(int j = 0; j < this->cols; j++) {
      if(this->grid[i][j].get_houses() > 0) {
        this->grid[i][j].make_neighborhood();
      }
    }
  }

  if(Global::Verbose > 1) {
    for(int i = 0; i < this->rows; i++) {
      for(int j = 0; j < this->cols; j++) {
        printf("print grid[%d][%d]:\n", i, j);
        this->grid[i][j].print();
      }
    }
  }
}

void Neighborhood_Layer::prepare() {
  record_daily_activity_locations();
  if (Enable_neighborhood_gravity_model) {
    FRED_VERBOSE(0, "setup gravity model ...\n");
    setup_gravity_model();
    FRED_VERBOSE(0, "setup gravity model complete\n");
  }
}

Neighborhood_Patch * Neighborhood_Layer::get_patch(int row, int col) {
  if(row >= 0 && col >= 0 && row < this->rows && col < this->cols)
    return &grid[row][col];
  else
    return NULL;
}

Neighborhood_Patch * Neighborhood_Layer::get_patch(fred::geo lat, fred::geo lon) {
  int row = get_row(lat);
  int col = get_col(lon);
  return get_patch(row, col);
}

Neighborhood_Patch * Neighborhood_Layer::select_random_patch(double x0, double y0, double dist) {
  // select a random patch within given distance.
  // if no luck after 20 attempts, return NULL
  for(int i = 0; i < 20; i++) {
    double r = Random::draw_random()*dist;      // random distance
    double ang = Geo::DEG_TO_RAD * Random::draw_random(0,360);// random angle
    double x = x0 + r*cos(ang);// corresponding x coord
    double y = y0 + r*sin(ang);// corresponding y coord
    int row = get_row(y);
    int col = get_col(x);
    Neighborhood_Patch * patch = get_patch(row,col);
    if (patch != NULL) return patch;
  }
  return NULL;
}

Neighborhood_Patch * Neighborhood_Layer::select_random_patch() {
  int row = Random::draw_random_int(0, this->rows - 1);
  int col = Random::draw_random_int(0, this->cols - 1);
  return &grid[row][col];
}

Neighborhood_Patch * Neighborhood_Layer::select_random_neighbor(int row, int col) {
  int n = Random::draw_random_int(0,7);
  if(n > 3)
    n++;        // excludes local patch
  int r = row - 1 + (n / 3);
  int c = col - 1 + (n % 3);
  return get_patch(r, c);
}

void Neighborhood_Layer::quality_control() {
  if(Global::Verbose) {
    fprintf(Global::Statusfp, "grid quality control check\n");
    fflush(Global::Statusfp);
  }

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

  if(1 || Global::Verbose) {
    int total_area = this->rows * this->cols;
    int convex_area = tot_occ_patches;
    fprintf(Global::Statusfp, "Density: popsize = %d total region = %d total_density = %f\n",
	    popsize, total_area, (total_area > 0) ? (double)popsize / (double)total_area : 0.0);
    fprintf(Global::Statusfp, "Density: popsize = %d convex region = %d convex_density = %f\n",
	    popsize, convex_area, (convex_area > 0) ? (double)popsize / (double)convex_area : 0.0);
    fprintf(Global::Statusfp, "grid quality control finished\n");
    fflush(Global::Statusfp);
  }
}

void Neighborhood_Layer::quality_control(double min_x, double min_y) {
  if(Global::Verbose) {
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

  if(Global::Verbose) {
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

void Neighborhood_Layer::record_daily_activity_locations() {
#pragma omp parallel
  {
    int partial_popsize = 0;
    int partial_households = 0;
#pragma omp for
    for(int row = 0; row < this->rows; row++) {
      for(int col = 0; col < this->cols; col++) {
        Neighborhood_Patch * patch = (Neighborhood_Patch *)&this->grid[row][col];
        if(patch->get_houses() > 0) {
          patch->record_daily_activity_locations();
          partial_popsize += patch->get_neighborhood()->get_size();
          partial_households += patch->get_houses();
        }
      }
    }
#pragma omp atomic
    this->popsize += partial_popsize;
#pragma omp atomic
    this->households += partial_households;
  }
}

vector<Place *> Neighborhood_Layer::get_households_by_distance(fred::geo lat, fred::geo lon,
							       double radius_in_km) {
  double px = Geo::get_x(lon);
  double py = Geo::get_y(lat);
  //  get patches around the point, make sure their rows & cols are in bounds
  int r1 = this->rows - 1 - (int)((py + radius_in_km) / this->patch_size);
  r1 = (r1 >= 0) ? r1 : 0;
  int r2 = this->rows - 1 - (int)((py - radius_in_km) / this->patch_size);
  r2 = (r2 <= this->rows - 1) ? r2 : this->rows - 1;

  int c1 = (int)((px - radius_in_km) / this->patch_size);
  c1 = (c1 >= 0) ? c1 : 0;
  int c2 = (int)((px + radius_in_km) / this->patch_size);
  c2 = (c2 <= this->cols - 1) ? c2 : this->cols - 1;

  vector<Place *> households; // store all households in patches ovelapping the radius

  for(int r = r1; r <= r2; r++) {
    for(int c = c1; c <= c2; c++) {
      Neighborhood_Patch * p = get_patch(r, c);
      int number_of_households = p->get_number_of_households();
      for (int i = 0; i < number_of_households; i++) {
	Place * house = p->get_household(i);
        double hlat =  house->get_latitude();
        double hlon = house->get_longitude();
        double hx = Geo::get_x(hlon);
        double hy = Geo::get_y(hlat);
        if(sqrt((px - hx) * (px - hx) + (py - hy) * (py - hy)) <= radius_in_km) {
          households.push_back(house);
        }
      }
    }
  }
  return households;
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

  offset = new offset_t * [ rows ];
  gravity_cdf = new gravity_cdf_t * [ rows ];
  for(int i = 0; i < rows; i++) {
    offset[i] = new offset_t [ cols ];
    gravity_cdf[i] = new gravity_cdf_t [ cols ];
  }

  if (max_distance < 0) {
    setup_null_gravity_model();
    return;
  }

  max_offset = max_distance / this->patch_size;
  assert(max_offset < 128);

  for(int i = 0; i < rows; i++) {
    for(int j = 0; j < cols; j++) {
      // set up gravity model for grid[i][j];
      Neighborhood_Patch * patch = (Neighborhood_Patch *)&grid[i][j];
      assert(patch != NULL);
      double x_src = patch->get_center_x();
      double y_src = patch->get_center_y();
      int pop_src = patch->get_popsize();
      double mean_household_income_src = patch->get_mean_household_income();
      if (pop_src == 0) continue;
      count = 0;
      for(int ii = i - max_offset; ii < rows && ii <= i + max_offset; ii++) {
	if (ii < 0) continue;
	for(int jj = j - max_offset; jj < cols && jj <= j + max_offset; jj++) {
	  if (jj < 0) continue;
	  Neighborhood_Patch * dest_patch = (Neighborhood_Patch *)&grid[ii][jj];
	  assert (dest_patch != NULL);
	  int pop_dest = dest_patch->get_popsize();
	  if (pop_dest == 0) continue;
	  double x_dest = dest_patch->get_center_x();
	  double y_dest = dest_patch->get_center_y();
	  double dist = sqrt((x_src-x_dest)*(x_src-x_dest) + (y_src-y_dest)*(y_src-y_dest));
	  if (max_distance < dist) continue;
	  double gravity = pow(pop_dest,pop_exponent) / (1.0 + pow(dist/min_distance, dist_exponent));
	  int off = 256*(i - ii + max_offset) + (j - jj + max_offset);

	  // consider income similarity in gravity model
	  // double mean_household_income_dest = dest_patch->get_mean_household_income();
	  // double income_similarity = mean_household_income_dest / mean_household_income_src;
	  // if (income_similarity > 1.0) { income_similarity = 1.0 / income_similarity; }
	  // gravity = ...;

	  tmp_offset[count] = off;
	  tmp_prob[count++] = gravity;
	  // printf("SETUP row %3d col %3d pop %5d count %4d ", i,j,pop_src,count);
	  // printf("offset %5d off_i %3d off_j %3d ii %3d jj %3d ",off,i+max_offset-(off/256),j+max_offset-(off%256),ii,jj);
	  // printf("dist %.2f pop_dest %5d gravity %7.3f\n",dist,pop_dest,gravity);
	}
      }
      // printf("\n");

      // sort by gravity value
      sort_pair.clear();
      for (int k = 0; k < count; k++) {
	sort_pair.push_back(pair <double,int> (tmp_prob[k], tmp_offset[k]));
      }
      std::sort(sort_pair.begin(), sort_pair.end(), compare_pair);

      // keep at most largest max_destinations
      if (count > max_destinations) count = max_destinations;
      for (int k = 0; k < count; k++) {
	tmp_prob[k] = sort_pair[k].first;
	tmp_offset[k] = sort_pair[k].second;
      }
      sort_pair.clear();

      // transform gravity values into a prob distribution
      double total = 0.0;
      for (int k = 0; k < count; k++) {
	total += tmp_prob[k];
      }
      for (int k = 0; k < count; k++) {
	tmp_prob[k] /= total;
      }

      // convert to cdf
      for (int k = 1; k < count; k++) {
	tmp_prob[k] += tmp_prob[k-1];
      }

      // store gravity prob and offsets for this patch
      gravity_cdf[i][j].reserve(count);
      gravity_cdf[i][j].clear();
      offset[i][j].reserve(count);
      offset[i][j].clear();
      for (int k = 0; k < count; k++) {
	gravity_cdf[i][j].push_back(tmp_prob[k]);
	offset[i][j].push_back(tmp_offset[k]);
      }
    }
  }  
  // this->print_gravity_model();
}

void Neighborhood_Layer::print_gravity_model() {
  printf("\n=== GRAVITY MODEL ========================================================\n");
  for(int i_src = 0; i_src < rows; i_src++) {
    for(int j_src = 0; j_src < cols; j_src++) {
      Neighborhood_Patch * src_patch = (Neighborhood_Patch *)&grid[i_src][j_src];
      double x_src = src_patch->get_center_x();
      double y_src = src_patch->get_center_y();
      int pop_src = src_patch->get_popsize();
      if (pop_src == 0) continue;
      int count = (int) offset[i_src][j_src].size();
      for (int k = 0; k < count; k++) {
	int off = offset[i_src][j_src][k];
	printf("GRAVITY_MODEL row %3d col %3d pop %5d count %4d k %4d offset %d ", i_src,j_src,pop_src,count,k,off);
	int i_dest = i_src + max_offset - (off / 256);
	int j_dest = j_src + max_offset - (off % 256);
	printf("row %3d col %3d ", i_dest,j_dest);
	Neighborhood_Patch * dest_patch = this->get_patch(i_dest, j_dest);
	assert (dest_patch != NULL);
	double x_dest = dest_patch->get_center_x();
	double y_dest = dest_patch->get_center_y();
	double dist = sqrt((x_src-x_dest)*(x_src-x_dest) + (y_src-y_dest)*(y_src-y_dest));
	int pop_dest = dest_patch->get_popsize();
	double gravity_prob = gravity_cdf[i_src][j_src][k];
	if (k>0) gravity_prob -= gravity_cdf[i_src][j_src][k-1];
	printf("pop %5d dist %0.4f prob %f", pop_dest,dist,gravity_prob);
	printf("\n");
      }
      printf("\n");
    }
  }
  // exit(0);
}


void Neighborhood_Layer::print_distances() {
  FILE *fp;
  fp = fopen("all_distances.dat", "w");
  for(int i_src = 0; i_src < rows; i_src++) {
    for(int j_src = 0; j_src < cols; j_src++) {
      Neighborhood_Patch * src_patch = (Neighborhood_Patch *)&grid[i_src][j_src];
      double x_src = src_patch->get_center_x();
      double y_src = src_patch->get_center_y();
      int pop_src = src_patch->get_popsize();
      if (pop_src == 0) continue;

      for(int i_dest = 0; i_dest < rows; i_dest++) {
	for(int j_dest = 0; j_dest < cols; j_dest++) {
	  if (i_dest < i_src) continue;
	  if (i_dest == i_src and j_dest < j_src) continue;

	  fprintf(fp,"row %3d col %3d pop %5d ", i_src,j_src,pop_src);
	  fprintf(fp,"row %3d col %3d ", i_dest,j_dest);
	  Neighborhood_Patch * dest_patch = this->get_patch(i_dest, j_dest);
	  assert (dest_patch != NULL);
	  double x_dest = dest_patch->get_center_x();
	  double y_dest = dest_patch->get_center_y();
	  double dist = sqrt((x_src-x_dest)*(x_src-x_dest) + (y_src-y_dest)*(y_src-y_dest));
	  int pop_dest = dest_patch->get_popsize();

	  fprintf(fp,"pop %5d dist %0.4f\n", pop_dest,dist);
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

  offset = new offset_t * [ rows ];
  gravity_cdf = new gravity_cdf_t * [ rows ];
  offset[0] = new offset_t [ cols ];
  gravity_cdf[0] = new gravity_cdf_t [ cols ];

  max_offset = rows * this->patch_size;
  assert(max_offset < 128);

  for(int i_dest = 0; i_dest < rows; i_dest++) {
    for(int j_dest = 0; j_dest < cols; j_dest++) {
      Neighborhood_Patch * dest_patch = this->get_patch(i_dest, j_dest);
      int pop_dest = dest_patch->get_popsize();
      if (pop_dest == 0) continue;
      // double gravity = pow(pop_dest,pop_exponent);
      double gravity = pop_dest;
      int off = 256*(0 - i_dest + max_offset) + (0 - j_dest + max_offset);
      tmp_offset[count] = off;
      tmp_prob[count++] = gravity;
    }
  }

  // transform gravity values into a prob distribution
  double total = 0.0;
  for (int k = 0; k < count; k++) {
    total += tmp_prob[k];
  }
  for (int k = 0; k < count; k++) {
    tmp_prob[k] /= total;
  }

  // convert to cdf
  for (int k = 1; k < count; k++) {
    tmp_prob[k] += tmp_prob[k-1];
  }

  // store gravity prob and offsets for this patch
  gravity_cdf[0][0].reserve(count);
  gravity_cdf[0][0].clear();
  offset[0][0].reserve(count);
  offset[0][0].clear();
  for (int k = 0; k < count; k++) {
    gravity_cdf[0][0].push_back(tmp_prob[k]);
    offset[0][0].push_back(tmp_offset[k]);
  }
}


Place * Neighborhood_Layer::select_destination_neighborhood(Place * src_neighborhood) {
  if (Enable_neighborhood_gravity_model) {
    return select_destination_neighborhood_by_gravity_model(src_neighborhood);
  }
  else {
    // original FRED neighborhood model (deprecated)
    return select_destination_neighborhood_by_old_model(src_neighborhood);
  }
}


Place * Neighborhood_Layer::select_destination_neighborhood_by_gravity_model(Place * src_neighborhood) {
  Neighborhood_Patch * src_patch = this->get_patch(src_neighborhood->get_latitude(),src_neighborhood->get_longitude());
  int i_src = src_patch->get_row();;
  int j_src = src_patch->get_col();
  if (max_distance < 0) { 
    // use null gravity model
    i_src = j_src = 0;
  }
  int offset_index = Random::draw_from_cdf_vector( gravity_cdf[i_src][j_src] ) ;
  int off = offset[i_src][j_src][offset_index];
  int i_dest = i_src + max_offset - (off / 256);
  int j_dest = j_src + max_offset - (off % 256);

  Neighborhood_Patch * dest_patch = this->get_patch(i_dest, j_dest);
  assert (dest_patch != NULL);

  // int pop_src = src_patch->get_popsize(); int pop_dest = dest_patch->get_popsize();
  // printf("SELECT_DEST src (%3d, %3d) pop %d dest (%3d, %3d) pop %5d\n", i_src,j_src,pop_src,i_dest,j_dest,pop_dest);

  return dest_patch->get_neighborhood();
}

Place * Neighborhood_Layer::select_destination_neighborhood_by_old_model(Place * src_neighborhood) {
  Neighborhood_Patch * src_patch = get_patch(src_neighborhood->get_latitude(),src_neighborhood->get_longitude());
  Neighborhood_Patch * dest_patch = NULL;
  int i_src = src_patch->get_row();
  int j_src = src_patch->get_col();
  double x_src = src_patch->get_center_x();
  double y_src = src_patch->get_center_y();
  double r = Random::draw_random();

  if( r < this->Home_neighborhood_prob) {
    dest_patch = src_patch;
  }
  else {
    if (r < this->Community_prob + this->Home_neighborhood_prob) {
      // select a random patch with community_prob
      dest_patch = select_random_patch(x_src, y_src, this->Community_distance);
    }
    else {
      // select randomly from among immediate neighbors
      dest_patch = select_random_neighbor(src_patch->get_row(),src_patch->get_col());
    }
    if (dest_patch == NULL || dest_patch->get_houses() == 0) dest_patch = src_patch; // fall back to src patch
  }
  return dest_patch->get_neighborhood();
}


Place * Neighborhood_Layer::select_workplace_in_area(int row, int col) {

  FRED_VERBOSE(1, "SELECT_WORKPLACE_IN_AREA for row %d col %d\n", row, col);

  // look for workplaces in increasingly expanding adjacent neighborhood
  vector<Neighborhood_Patch *> patches;
  Neighborhood_Patch * patch;
  for (int level = 0; level < 100; level++) {
    FRED_VERBOSE(2,"level %d\n",level);
    patches.clear();
    if (level == 0) {
      patch = get_patch(row, col);
      if (patch != NULL) {
	FRED_VERBOSE(2,"adding (%d,%d)\n", row, col); 
	patches.push_back(patch);
      }
    }
    else {
      for(int c = col-level; c <= col+level; c++) {
	patch = get_patch(row - level, c);
	if (patch != NULL) {
	  FRED_VERBOSE(2,"adding (%d,%d)\n", row - level, c); 
	  patches.push_back(patch);
	}
	patch = get_patch(row + level, c);
	if (patch != NULL) {
	  FRED_VERBOSE(2,"adding (%d,%d)\n", row + level, c); 
	  patches.push_back(patch);
	}
      }
      for(int r = row-level+1; r <= row+level-1; r++) {
	patch = get_patch(r, col - level);
	if (patch != NULL) {
	  FRED_VERBOSE(2,"adding (%d,%d)\n", r, col - level); 
	  patches.push_back(patch);
	}
	patch = get_patch(r, col + level);
	if (patch != NULL) {
	  FRED_VERBOSE(2,"adding (%d,%d)\n", r, col + level); 
	  patches.push_back(patch);
	}
      }
    }

    // shuffle the patches
    FYShuffle<Neighborhood_Patch *>(patches);

    FRED_VERBOSE(1,"Level %d include %d patches\n", level, patches.size());

    // look for a suitable workplace in each patch
    for(size_t i = 0; i < patches.size(); i++) {
      Neighborhood_Patch *patch = patches.at(i);
      if(patch == NULL)
        continue;
      Place * p = patch->select_workplace_in_neighborhood();
      if(p != NULL) {
	FRED_VERBOSE(1, "SELECT_WORKPLACE_IN_AREA found workplace %s at level %d\n",
		     p->get_label(), level);
	return(p); // success
      }
    }
  }
  return NULL;
}

bool more_room(Place *p1, Place *p2){ 
  School * s1 = static_cast<School *>(p1);
  School * s2 = static_cast<School *>(p2);
  int vac1 = s1->get_orig_number_of_students() - s1->get_number_of_students();
  int vac2 = s2->get_orig_number_of_students() - s2->get_number_of_students();
  // return vac1 > vac2;
  double relvac1 = (vac1 + 0.000001) / (s1->get_orig_number_of_students() + 1.0);
  double relvac2 = (vac2 + 0.000001) / (s2->get_orig_number_of_students() + 1.0);
  // resolve ties by place id
  return (relvac1 == relvac2) ? (s1->get_id() <= s2->get_id()) : (relvac1 > relvac2);
}

Place * Neighborhood_Layer::select_school_in_area(int age, int row, int col) {
  FRED_VERBOSE(1, "SELECT_SCHOOL_IN_AREA for age %d row %d col %d\n", age, row, col);
  // make a list of all schools within 50 kms that have grades for this age
  place_vector_t * schools = new place_vector_t;
  schools->clear();
  Neighborhood_Patch * patch;
  int max_dist = 60;
  for(int c = col-max_dist; c <= col+max_dist; c++) {
    for(int r = row-max_dist; r <= row+max_dist; r++) {
      patch = get_patch(r,c);
      if (patch != NULL) {
	// find all age-appropriate schools in this patch
	patch->find_schools_for_age(age, schools);
      }
    }
  }
  assert(schools->size() > 0);
  FRED_VERBOSE(1, "SELECT_SCHOOL_IN_AREA found %d possible schools\n", schools->size());
  // sort schools by vacancies
  std::sort(schools->begin(),schools->end(),more_room);
  // pick the school with largest vacancy or smallest crowding 
  Place * place = schools->front();
  School * s = static_cast<School *>(place);
  FRED_VERBOSE(1, "SELECT_SCHOOL_IN_AREA found school %s orig %d current %d vacancies %d\n",
	       s->get_label(), s->get_orig_number_of_students(), s->get_number_of_students(),
	       s->get_orig_number_of_students()-s->get_number_of_students());
  // s->print_size_distribution();
  return place;
}

/*
  Place * Neighborhood_Layer::select_school_in_area(int age, int row, int col) {

  FRED_VERBOSE(1, "SELECT_SCHOOL_IN_AREA for age %d row %d col %d\n", age, row, col);

  for (int attempt = 1; attempt <= 20; attempt++) {
  double threshold = attempt * -0.1;
  int max_level = 20;

  // look for schools in increasingly expanding adjacent neighborhood
  vector<Neighborhood_Patch *> patches;
  Neighborhood_Patch * patch;
  for (int level = 0; level < max_level; level++) {
  FRED_VERBOSE(2,"level %d\n",level);
  patches.clear();
  if (level == 0) {
  patch = get_patch(row, col);
  if (patch != NULL) {
  FRED_VERBOSE(2,"adding (%d,%d)\n", row, col); 
  patches.push_back(patch);
  }
  }
  else {
  for(int c = col-level; c <= col+level; c++) {
  patch = get_patch(row - level, c);
  if (patch != NULL) {
  FRED_VERBOSE(2,"adding (%d,%d)\n", row - level, c); 
  patches.push_back(patch);
  }
  patch = get_patch(row + level, c);
  if (patch != NULL) {
  FRED_VERBOSE(2,"adding (%d,%d)\n", row + level, c); 
  patches.push_back(patch);
  }
  }
  for(int r = row-level+1; r <= row+level-1; r++) {
  patch = get_patch(r, col - level);
  if (patch != NULL) {
  FRED_VERBOSE(2,"adding (%d,%d)\n", r, col - level); 
  patches.push_back(patch);
  }
  patch = get_patch(r, col + level);
  if (patch != NULL) {
  FRED_VERBOSE(2,"adding (%d,%d)\n", r, col + level); 
  patches.push_back(patch);
  }
  }
  }

  if (patches.size() == 0) {
  FRED_VERBOSE(1, "SELECT_SCHOOL_IN_AREA: no patches at level %d\n", level);
  continue;
  }

  // shuffle the patches
  FYShuffle<Neighborhood_Patch *>(patches);

  FRED_VERBOSE(1,"Level %d include %d patches\n", level, patches.size());

  // look for a suitable school in each patch
  for(size_t i = 0; i < patches.size(); i++) {
  Neighborhood_Patch *patch = patches.at(i);
  if(patch == NULL)
  continue;
  Place * p = patch->select_school_in_neighborhood(age, threshold);
  if(p != NULL) {
  FRED_VERBOSE(0, "SELECT_SCHOOL_IN_AREA found school %s at level %d threshold %0.1f\n",
  p->get_label(), level, threshold);
  return(p); // success
  }
  }
  }
  }
  return NULL;
  }

*/

void Neighborhood_Layer::register_place(Place *place) {
  Neighborhood_Patch * patch = get_patch(place->get_latitude(), place->get_longitude());
  if (patch != NULL) {
    patch->register_place(place);
  }
  else {
    FRED_VERBOSE(1, "register place: can't find patch for place %s county = %d\n",
		 place->get_label(), place->get_county_fips());
  }
}
