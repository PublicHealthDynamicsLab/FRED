/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Visualization_Layer.cc
//

#include <list>
#include <string>
#include <utility>
using namespace std;

#include "Params.h"
#include "Place_List.h"
#include "Random.h"
#include "Regional_Layer.h"
#include "Vector_Layer.h"
#include "Visualization_Layer.h"
#include "Visualization_Patch.h"
#include "Condition_List.h"

typedef std::map<unsigned long long, unsigned long> census_tract_t;
census_tract_t census_tract;
census_tract_t census_tract_pop;

void Visualization_Layer::add_census_tract(unsigned long long tract) {
  census_tract[tract] = 0;
  census_tract_pop[tract] = 0;
}

Visualization_Layer::Visualization_Layer() {
  this->rows = 0;
  this->cols = 0;
  Params::get_param("household_visualization_mode", &this->household_mode);
  Params::get_param("census_tract_visualization_mode", &this->census_tract_mode);
  this->households.clear();
}

Visualization_Patch* Visualization_Layer::get_patch(int row, int col) {
  if(row >= 0 && col >= 0 && row < this->rows && col < this->cols) {
    return &(this->grid[row][col]);
  } else {
    return NULL;
  }
}

Visualization_Patch* Visualization_Layer::get_patch(fred::geo lat, fred::geo lon) {
  int row = get_row(lat);
  int col = get_col(lon);
  return get_patch(row,col);
}

Visualization_Patch* Visualization_Layer::get_patch(double x, double y) {
  int row = get_row(y);
  int col = get_col(x);
  return get_patch(row,col);
}

Visualization_Patch* Visualization_Layer::select_random_patch() {
  int row = Random::draw_random_int(0, this->rows-1);
  int col = Random::draw_random_int(0, this->cols-1);
  return &(this->grid[row][col]);
}

void Visualization_Layer::quality_control() {
  if(Global::Verbose) {
    fprintf(Global::Statusfp, "visualization grid quality control finished\n");
    fflush(Global::Statusfp);
  }
}

void Visualization_Layer::setup() {
  // create visualization data directories
  sprintf(Global::Visualization_directory, "%s/RUN%d/VIS",
	  Global::Simulation_directory,
	  Global::Simulation_run_number);
  Utils::fred_make_directory(Global::Visualization_directory);
}



void Visualization_Layer::print_visualization_data(int day) {
}

void Visualization_Layer::print_census_tract_data(int condition_id, int output_code, char* output_str, int day) {
}

void Visualization_Layer::print_vector_data(char* dir, int condition_id, int day) {
  char filename[FRED_STRING_SIZE];
  string name = Global::Conditions.get_condition_name(condition_id);
  sprintf(filename,"%s/%s/Vec/day-%d.txt",dir,name.c_str(),day);
  FILE* fp = fopen(filename, "w");

  Global::Vectors->update_visualization_data(condition_id, day);
  for(int i = 0; i < this->rows; ++i) {
    for(int j = 0; j < this->cols; ++j) {
      Visualization_Patch* patch = (Visualization_Patch*) &grid[i][j];
      int count = patch->get_count();
      if(count > 0){
	      fprintf(fp, "%d %d %d\n", i, j, count);
      }
      patch->reset_counts();
    }
  }
  fclose(fp);
}


void Visualization_Layer::initialize_household_data(fred::geo latitude, fred::geo longitude, int count) {
}

void Visualization_Layer::update_data(fred::geo latitude, fred::geo longitude, int count, int popsize) {
}

void Visualization_Layer::update_data(double x, double y, int count, int popsize) {
  Visualization_Patch* patch = get_patch(x, y);
  if(patch != NULL) {
    patch->update_patch_count(count, popsize);
  }
}

void Visualization_Layer::update_data(unsigned long long tract, int count, int popsize) {
  census_tract[tract] += count;
  census_tract_pop[tract] += popsize;
}



