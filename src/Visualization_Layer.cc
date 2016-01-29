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
  Params::get_param_from_string("household_visualization_mode", &this->household_mode);
  Params::get_param_from_string("census_tract_visualization_mode", &this->census_tract_mode);
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
  sprintf(Global::Visualization_directory, "%s/VIS", Global::Simulation_directory);
  Utils::fred_make_directory(Global::Visualization_directory);
  sprintf(Global::Visualization_directory, "%s/run%d", Global::Visualization_directory, Global::Simulation_run_number);
  Utils::fred_make_directory(Global::Visualization_directory);

  if (Global::Enable_HAZEL) {
    char vis_var_dir[FRED_STRING_SIZE];
    for(int d = 0; d < Global::Conditions.get_number_of_conditions(); ++d) {
      sprintf(vis_var_dir, "%s/cond%d/HH_primary_hc_unav", Global::Visualization_directory, d);
      Utils::fred_make_directory(vis_var_dir);
      sprintf(vis_var_dir, "%s/cond%d/HH_accept_insr_hc_unav", Global::Visualization_directory, d);
      Utils::fred_make_directory(vis_var_dir);
      sprintf(vis_var_dir, "%s/cond%d/HH_hc_unav", Global::Visualization_directory, d);
      Utils::fred_make_directory(vis_var_dir);
      sprintf(vis_var_dir, "%s/cond%d/HC_DEFICIT", Global::Visualization_directory, d);
      Utils::fred_make_directory(vis_var_dir);
    }
  }
}



void Visualization_Layer::print_visualization_data(int day) {

  if(Global::Enable_HAZEL) {
    if(this->census_tract_mode) {
      print_census_tract_data(0, Global::OUTPUT_HC_DEFICIT, (char*)"HC_DEFICIT", day);
    }

    if(this->household_mode) {
      for(int condition_id = 0; condition_id < Global::Conditions.get_number_of_conditions(); ++condition_id) {
	print_hazel_household_data(condition_id, day);
      }
    }
  }
}

void Visualization_Layer::print_hazel_household_data(int condition_id, int day) {

  // household with Healthcare availability deficiency
  char filename[FRED_STRING_SIZE];
  FILE* fp;
  int size = this->households.size();

  //!is_primary_healthcare_available
  sprintf(filename, "%s/cond%d/HH_primary_hc_unav/households-%d.txt",
	  Global::Visualization_directory, condition_id, day);
  fp = fopen(filename, "w");
  assert(fp != NULL);
  fprintf(fp, "lat long\n");
  for(int i = 0; i < size; ++i) {
    Household* hh = static_cast<Household*>(this->households[i]);
    if(hh->is_seeking_healthcare() && !hh->is_primary_healthcare_available()) {
      fprintf(fp, "%f %f\n", hh->get_latitude(), hh->get_longitude());
    }
  }
  fclose(fp);

  //!is_other_healthcare_location_that_accepts_insurance_available
  sprintf(filename, "%s/cond%d/HH_accept_insr_hc_unav/households-%d.txt",
	  Global::Visualization_directory, condition_id, day);
  fp = fopen(filename, "w");
  fprintf(fp, "lat long\n");
  for(int i = 0; i < size; ++i) {
    Household* hh = static_cast<Household*>(this->households[i]);
    if(hh->is_seeking_healthcare() && !hh->is_other_healthcare_location_that_accepts_insurance_available()) {
      fprintf(fp, "%f %f\n", hh->get_latitude(), hh->get_longitude());
    }
  }
  fclose(fp);

  //!is_healthcare_available
  sprintf(filename, "%s/cond%d/HH_hc_unav/households-%d.txt",
	  Global::Visualization_directory, condition_id, day);
  fp = fopen(filename, "w");
  fprintf(fp, "lat long\n");
  for(int i = 0; i < size; ++i) {
    Household* hh = static_cast<Household*>(this->households[i]);
    if(hh->is_seeking_healthcare() && !hh->is_healthcare_available()) {
      fprintf(fp, "%f %f\n", hh->get_latitude(), hh->get_longitude());
    }
  }
  fclose(fp);
}

void Visualization_Layer::print_census_tract_data(int condition_id, int output_code, char* output_str, int day) {
  char filename[FRED_STRING_SIZE];
  sprintf(filename, "%s/cond%d/%s/census_tracts-%d.txt",
	  Global::Visualization_directory, condition_id, output_str, day);

  // get the counts for this output_code
  Global::Places.get_census_tract_data_from_households(day, condition_id, output_code);

  FILE* fp = fopen(filename, "w");
  fprintf(fp, "Census_tract\tCount\tPopsize\n");
  for(census_tract_t::iterator itr = census_tract.begin(); itr != census_tract.end(); ++itr) {
    unsigned long long tract = itr->first;
    fprintf(fp, "%011lld\t%lu\t%lu\n", tract, itr->second, census_tract_pop[tract]);
  }
  fclose(fp);

  // clear census_tract_counts
  for(census_tract_t::iterator itr = census_tract.begin(); itr != census_tract.end(); ++itr) {
    itr->second = 0;
  }
  for(census_tract_t::iterator itr = census_tract_pop.begin(); itr != census_tract_pop.end(); ++itr) {
    itr->second = 0;
  }
}

void Visualization_Layer::print_vector_data(char* dir, int condition_id, int day) {
  char filename[FRED_STRING_SIZE];
  // printf("Printing population size for GAIA\n");
  sprintf(filename,"%s/cond%d/Vec/day-%d.txt",dir,condition_id,day);
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
  /*
    if(count > 0) {
    point p = std::make_pair(latitude, longitude);
    this->all_households.push_back(p);
    }
  */
}

void Visualization_Layer::update_data(fred::geo latitude, fred::geo longitude, int count, int popsize) {
  if(Global::Enable_HAZEL) {
    int size = this->households.size();
    for(int i = 0; i < size; ++i) {
      Household* hh = static_cast<Household*>(this->households[i]);
      hh->reset_healthcare_info();
    }
  }
  /*
    if (count > 0) {
    point p = std::make_pair(latitude, longitude);
    this->infected_households.push_back(p);
    }
  */
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



