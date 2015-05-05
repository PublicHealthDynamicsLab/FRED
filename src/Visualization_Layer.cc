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

  Params::get_param_from_string("gaia_visualization_mode", &this->gaia_mode);
  Params::get_param_from_string("household_visualization_mode", &this->household_mode);
  Params::get_param_from_string("census_tract_visualization_mode", &this->census_tract_mode);

  this->households.clear();

  if(this->gaia_mode) {
    // create visualization grid
    Regional_Layer* base_grid = Global::Simulation_Region;
    this->min_lat = base_grid->get_min_lat();
    this->min_lon = base_grid->get_min_lon();
    this->max_lat = base_grid->get_max_lat();
    this->max_lon = base_grid->get_max_lon();
    this->min_x = base_grid->get_min_x();
    this->min_y = base_grid->get_min_y();
    this->max_x = base_grid->get_max_x();
    this->max_y = base_grid->get_max_y();

    // determine patch size for this layer
    Params::get_param_from_string("visualization_grid_size", &this->max_grid_size);
    if(this->max_x - this->min_x > this->max_y - this->min_y) {
      this->patch_size = (this->max_x - this->min_x) / (double)this->max_grid_size;
    } else {
      this->patch_size = (this->max_y - this->min_y) / (double)this->max_grid_size;
    }
    this->rows = (double)(this->max_y - this->min_y) / this->patch_size;
    if(this->min_y + this->rows * this->patch_size < this->max_y) {
      this->rows++;
    }
    this->cols = (double)(this->max_x - this->min_x) / this->patch_size;
    if(this->min_x + this->cols * this->patch_size < this->max_x) {
      this->cols++;
    }

    if(Global::Verbose > 0) {
      fprintf(Global::Statusfp, "Visualization_Layer min_lon = %f\n", this->min_lon);
      fprintf(Global::Statusfp, "Visualization_Layer min_lat = %f\n", this->min_lat);
      fprintf(Global::Statusfp, "Visualization_Layer max_lon = %f\n", this->max_lon);
      fprintf(Global::Statusfp, "Visualization_Layer max_lat = %f\n", this->max_lat);
      fprintf(Global::Statusfp, "Visualization_Layer rows = %d  this->cols = %d\n",this->rows, this->cols);
      fprintf(Global::Statusfp, "Visualization_Layer min_x = %f  min_y = %f\n", this->min_x, this->min_y);
      fprintf(Global::Statusfp, "Visualization_Layer max_x = %f  max_y = %f\n", this->max_x, this->max_y);
      fflush(Global::Statusfp);
    }

    this->grid = new Visualization_Patch* [this->rows];
    for(int i = 0; i < this->rows; ++i) {
      this->grid[i] = new Visualization_Patch[this->cols];
    }

    for(int i = 0; i < this->rows; ++i) {
      for(int j = 0; j < this->cols; ++j) {
        this->grid[i][j].setup(i, j, this->patch_size, this->min_x, this->min_y);
      }      
    }
  }
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
  int row = IRAND(0, this->rows-1);
  int col = IRAND(0, this->cols-1);
  return &(this->grid[row][col]);
}

void Visualization_Layer::quality_control() {
  if(Global::Verbose) {
    fprintf(Global::Statusfp, "visualization grid quality control check\n");
    fflush(Global::Statusfp);
  }
  
  if(this->gaia_mode) {
    for(int row = 0; row < this->rows; ++row) {
      for(int col = 0; col < this->cols; ++col) {
        this->grid[row][col].quality_control();
      }
    }
    if(Global::Verbose > 1 and this->rows > 0) {
      char filename[FRED_STRING_SIZE];
      sprintf(filename, "%s/visualization_grid.dat", Global::Simulation_directory);
      FILE* fp = fopen(filename, "w");
      for(int row = 0; row < this->rows; ++row) {
	      if(row % 2) {
	        for(int col = this->cols - 1; col >= 0; --col) {
	          double x = this->grid[row][col].get_center_x();
	          double y = this->grid[row][col].get_center_y();
	          fprintf(fp, "%f %f\n",x,y);
	        }
	      } else {
	        for(int col = 0; col < this->cols; ++col) {
	          double x = this->grid[row][col].get_center_x();
	          double y = this->grid[row][col].get_center_y();
	          fprintf(fp, "%f %f\n",x,y);
	        }
	      }
      }
      fclose(fp);
    }
  }

  if(Global::Verbose) {
    fprintf(Global::Statusfp, "visualization grid quality control finished\n");
    fflush(Global::Statusfp);
  }
}

void Visualization_Layer::initialize() {

  char vis_top_dir[FRED_STRING_SIZE];

  sprintf(vis_top_dir, "%s/VIS", Global::Simulation_directory);
  create_data_directories(vis_top_dir);

  // create visualization data directory
  if(this->gaia_mode) {
    sprintf(vis_top_dir, "%s/GAIA", Global::Simulation_directory);
    create_data_directories(vis_top_dir);
    // create GAIA setup file
    char setup_file[FRED_STRING_SIZE];
    sprintf(setup_file, "%s/grid.txt", vis_top_dir);
    FILE* fp = fopen(setup_file, "w");
    fprintf(fp, "rows = %d\n", this->rows);
    fprintf(fp, "cols = %d\n", this->cols);
    fprintf(fp, "min_lat = %f\n", this->min_lat);
    fprintf(fp, "min_lon = %f\n", this->min_lon);
    fprintf(fp, "patch_x_size = %f\n", Geo_Utils::x_to_degree_longitude(patch_size));
    fprintf(fp, "patch_y_size = %f\n", Geo_Utils::y_to_degree_latitude(patch_size));
    fclose(fp);
  }
}


void Visualization_Layer::create_data_directories(char* vis_top_dir) {
  char vis_run_dir[FRED_STRING_SIZE];
  char vis_dis_dir[FRED_STRING_SIZE];
  char vis_var_dir[FRED_STRING_SIZE];

  // make top level data directory
  Utils::fred_make_directory(vis_top_dir);
    
  // make directory for this run
  sprintf(vis_run_dir, "%s/run%d", vis_top_dir, Global::Simulation_run_number);
  Utils::fred_make_directory(vis_run_dir);
  
  // create sub directories for diseases and output vars
  for(int d = 0; d < Global::Diseases; ++d) {
    sprintf(vis_dis_dir, "%s/dis%d", vis_run_dir, d);
    Utils::fred_make_directory(vis_dis_dir);
    
    if(d == 0) {
      // print out household locations
      char filename[256];
      sprintf(filename, "%s/households.txt", vis_dis_dir);
      FILE* fp = fopen(filename, "w");
      int num_households = this->households.size();
      for(int i = 0; i < num_households; ++i) {
        Place* h = this->households[i];
	      fprintf(fp, "%f %f %3d %s\n", h->get_latitude(), h->get_longitude(), h->get_size(), h->get_label());
      }
      fclose(fp);
    } else {
      // create symbolic links
      char cmd[FRED_STRING_SIZE];
      sprintf(cmd, "ln -s %s/dis0/households.txt %s/households.txt", vis_run_dir, vis_dis_dir);
      if(system(cmd) != 0) {
        Utils::fred_abort("Error using system command \"%s\"\n", cmd);
      }
    }

    // create directories for specific output variables
    sprintf(vis_var_dir, "%s/I", vis_dis_dir);
    Utils::fred_make_directory(vis_var_dir);
    sprintf(vis_var_dir, "%s/Is", vis_dis_dir);
    Utils::fred_make_directory(vis_var_dir);
    sprintf(vis_var_dir, "%s/C", vis_dis_dir);
    Utils::fred_make_directory(vis_var_dir);
    sprintf(vis_var_dir, "%s/Cs", vis_dis_dir);
    Utils::fred_make_directory(vis_var_dir);
    sprintf(vis_var_dir, "%s/P", vis_dis_dir);
    Utils::fred_make_directory(vis_var_dir);
    sprintf(vis_var_dir, "%s/N", vis_dis_dir);
    Utils::fred_make_directory(vis_var_dir);
    sprintf(vis_var_dir, "%s/R", vis_dis_dir);
    Utils::fred_make_directory(vis_var_dir);
    sprintf(vis_var_dir, "%s/Vec", vis_dis_dir);
    Utils::fred_make_directory(vis_var_dir);

    if(this->household_mode && Global::Enable_HAZEL) {
      sprintf(vis_var_dir, "%s/HH_primary_hc_open", vis_dis_dir);
      Utils::fred_make_directory(vis_var_dir);
      sprintf(vis_var_dir, "%s/HH_accept_insr_hc_open", vis_dis_dir);
      Utils::fred_make_directory(vis_var_dir);
      sprintf(vis_var_dir, "%s/HH_receive_hc", vis_dis_dir);
      Utils::fred_make_directory(vis_var_dir);
    }
  }
}


void Visualization_Layer::print_visualization_data(int day) {
  if(this->census_tract_mode) {
    for(int disease_id = 0; disease_id < Global::Diseases; ++disease_id) {
      char dir[FRED_STRING_SIZE];
      sprintf(dir, "%s/VIS/run%d", Global::Simulation_directory, Global::Simulation_run_number);
      print_census_tract_data(dir, disease_id, Global::OUTPUT_I, (char*)"I", day);
      print_census_tract_data(dir, disease_id, Global::OUTPUT_Is, (char*)"Is", day);
      print_census_tract_data(dir, disease_id, Global::OUTPUT_C, (char*)"C", day);
      print_census_tract_data(dir, disease_id, Global::OUTPUT_Cs, (char*)"Cs", day);
      print_census_tract_data(dir, disease_id, Global::OUTPUT_P, (char*)"P", day);
    }
  }

  if(this->household_mode) {
    for(int disease_id = 0; disease_id < Global::Diseases; ++disease_id) {
      char dir[FRED_STRING_SIZE];
      sprintf(dir, "%s/VIS/run%d", Global::Simulation_directory, Global::Simulation_run_number);
      print_household_data(dir, disease_id, day);
      // print_household_data(dir, disease_id, Global::OUTPUT_P, (char *)"P", day);
      // print_household_data(dir, disease_id, Global::OUTPUT_R, (char *)"R", day);
    }
  }

  if(this->gaia_mode) {
    for(int disease_id = 0; disease_id < Global::Diseases; ++disease_id) {
      char dir[FRED_STRING_SIZE];
      sprintf(dir, "%s/GAIA/run%d", Global::Simulation_directory, Global::Simulation_run_number);
      print_output_data(dir, disease_id, Global::OUTPUT_I, (char*) "I", day);
      print_output_data(dir, disease_id, Global::OUTPUT_Is, (char*)"Is", day);
      print_output_data(dir, disease_id, Global::OUTPUT_C, (char*)"C", day);
      print_output_data(dir, disease_id, Global::OUTPUT_Cs, (char*)"Cs", day);
      print_output_data(dir, disease_id, Global::OUTPUT_P, (char*)"P", day);
      print_population_data(dir, disease_id, day);
      if(Global::Enable_Vector_Layer) {
	      print_vector_data(dir, disease_id, day);
      }
    }
  }
}

void Visualization_Layer::print_household_data(char* dir, int disease_id, int day) {

  // household with new cases
  char filename[FRED_STRING_SIZE];
  sprintf(filename, "%s/dis%d/C/households-%d.txt", dir, disease_id, day);
  FILE* fp = fopen(filename, "w");
  fprintf(fp, "lat long\n");
  int size = this->households.size();
  for(int i = 0; i < size; ++i) {
    Place* house = this->households[i];
    if(house->is_exposed(disease_id)) {
      fprintf(fp, "%f %f\n", house->get_latitude(), house->get_longitude());
    }
  }
  fclose(fp);

  // household with infectious cases
  sprintf(filename, "%s/dis%d/I/households-%d.txt", dir, disease_id, day);
  fp = fopen(filename, "w");
  fprintf(fp, "lat long\n");
  for(int i = 0; i < size; ++i) {
    Place* house = this->households[i];
    //  just consider human infectious, not mosquito neither infectious places visited
    if(house->is_human_infectious(disease_id)) {
      fprintf(fp, "%f %f\n", house->get_latitude(), house->get_longitude());
    }
  }
  fclose(fp);

  // household with recovered cases
  sprintf(filename, "%s/dis%d/R/households-%d.txt", dir, disease_id, day);
  fp = fopen(filename, "w");
  fprintf(fp, "lat long\n");
  for(int i = 0; i < size; ++i) {
    Place* house = this->households[i];
    if(house->is_recovered(disease_id)) {
      fprintf(fp, "%f %f\n", house->get_latitude(), house->get_longitude());
    }
  }
  fclose(fp);

  // household with Healthcare availability deficiency
  if(Global::Enable_HAZEL) {

    //primary_healthcare_location_open
    sprintf(filename, "%s/dis%d/HH_primary_hc_open/households-%d.txt", dir, disease_id, day);
    fp = fopen(filename, "w");
    assert(fp != NULL);
    fprintf(fp, "lat long\n");
    for(int i = 0; i < size; ++i) {
      Household* house = static_cast<Household*>(this->households[i]);
      if(house->is_primary_healthcare_location_open()) {
        fprintf(fp, "%f %f\n", house->get_latitude(), house->get_longitude());
      }
    }
    fclose(fp);

    //other_healthcare_location_that_accepts_insurance_open
    sprintf(filename, "%s/dis%d/HH_accept_insr_hc_open/households-%d.txt", dir, disease_id, day);
    fp = fopen(filename, "w");
    fprintf(fp, "lat long\n");
    for(int i = 0; i < size; ++i) {
      Household* house = static_cast<Household*>(this->households[i]);
      if(house->is_other_healthcare_location_that_accepts_insurance_open()) {
        fprintf(fp, "%f %f\n", house->get_latitude(), house->get_longitude());
      }
    }
    fclose(fp);

    //is_able_to_receive_healthcare
    sprintf(filename, "%s/dis%d/HH_receive_hc/households-%d.txt", dir, disease_id, day);
    fp = fopen(filename, "w");
    fprintf(fp, "lat long\n");
    for(int i = 0; i < size; ++i) {
      Household* house = static_cast<Household*>(this->households[i]);
      if(house->is_able_to_receive_healthcare()) {
        fprintf(fp, "%f %f\n", house->get_latitude(), house->get_longitude());
      }
    }
    fclose(fp);

  }

}

/*
void Visualization_Layer::print_household_data(char* dir, int disease_id, int output_code, char* output_str, int day) {
  char filename[FRED_STRING_SIZE];
  sprintf(filename, "%s/dis%d/%s/households-%d.txt", dir, disease_id, output_str, day);
  FILE* fp = fopen(filename, "w");
  fprintf(fp, "lat long\n");
  this->infected_households.clear();
  // get the counts for this output_code
  Global::Places.get_visualization_data_from_households(disease_id, output_code);
  // print out the lat long of all infected households
  int houses = (int)(this->infected_households.size());
  for(int i = 0; i < houses; ++i) {
    fred::geo lat = infected_households[i].first;
    fred::geo lon = infected_households[i].second;
    fprintf(fp, "%lf %lf\n", lat, lon);
  }
  fclose(fp);
  this->infected_households.clear();
}
*/

void Visualization_Layer::print_output_data(char* dir, int disease_id, int output_code, char* output_str, int day) {
  char filename[FRED_STRING_SIZE];
  sprintf(filename, "%s/dis%d/%s/day-%d.txt", dir, disease_id, output_str, day);
  FILE* fp = fopen(filename, "w");
  // printf("print_output_data to file %s\n", filename);

  // get the counts for this output_code
  Global::Places.get_visualization_data_from_households(disease_id, output_code);

  // print out the non-zero patches
  for(int i = 0; i < this->rows; ++i) {
    for (int j = 0; j < this->cols; ++j) {
      Visualization_Patch* patch = (Visualization_Patch*) &(this->grid[i][j]);
      int count = patch->get_count();
      if(count > 0) {
	      int popsize = patch->get_popsize();
	      fprintf(fp, "%d %d %d %d\n", i, j, count, popsize);
      }
      // zero out this patch
      patch->reset_counts();
    }
  }
  fclose(fp);
}

void Visualization_Layer::print_census_tract_data(char* dir, int disease_id, int output_code, char* output_str, int day) {
  char filename[FRED_STRING_SIZE];
  sprintf(filename, "%s/dis%d/%s/census_tracts-%d.txt", dir, disease_id, output_str, day);

  // get the counts for this output_code
  Global::Places.get_census_tract_data_from_households(disease_id, output_code);

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

void Visualization_Layer::print_population_data(char* dir, int disease_id, int day) {
  char filename[FRED_STRING_SIZE];
  // printf("Printing population size for GAIA\n");
  sprintf(filename,"%s/dis%d/N/day-%d.txt",dir,disease_id,day);
  FILE* fp = fopen(filename, "w");

  // get the counts for an arbitrary output code;
  // we only care about the popsize here.
  Global::Places.get_visualization_data_from_households(disease_id, Global::OUTPUT_C);
  for(int i = 0; i < this->rows; ++i) {
    for(int j = 0; j < this->cols; ++j) {
      Visualization_Patch* patch = (Visualization_Patch*) &grid[i][j];
      int popsize = patch->get_popsize();
      if(popsize > 0) {
	      fprintf(fp, "%d %d %d\n", i, j, popsize);
      }
      // zero out this patch
      patch->reset_counts();
    }
  }
  fclose(fp);
}

void Visualization_Layer::print_vector_data(char* dir, int disease_id, int day) {
  char filename[FRED_STRING_SIZE];
  // printf("Printing population size for GAIA\n");
  sprintf(filename,"%s/dis%d/Vec/day-%d.txt",dir,disease_id,day);
  FILE* fp = fopen(filename, "w");

  Global::Vectors->update_visualization_data(disease_id, day);
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
  if(this->gaia_mode) {
    Visualization_Patch* patch = get_patch(latitude, longitude);
    if(patch != NULL) {
      patch->update_patch_count(count, popsize);
    }
  }

  if(Global::Enable_HAZEL) {
    int size = this->households.size();
    for(int i = 0; i < size; ++i) {
      Household* house = static_cast<Household*>(this->households[i]);
      house->set_is_primary_healthcare_location_open(true);
      house->set_other_healthcare_location_that_accepts_insurance_open(true);
      house->set_is_able_to_receive_healthcare(true);
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



