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
// File: Vector_Layer.cc
//
#include <utility>
#include <list>
#include <vector>
#include <map>
#include <string>
#include<fstream>
#include<string>
#include<sstream>
using namespace std;

#include "Utils.h"
#include "Vector_Layer.h"
#include "Vector_Patch.h"
#include "Disease.h"
#include "Disease_List.h"
#include "Epidemic.h"
#include "Visualization_Layer.h"
#include "Regional_Layer.h"
#include "Regional_Patch.h"
#include "Params.h"
#include "Random.h"
#include "Place.h"
#include "Place_List.h"
#include "Household.h"
#include "Tracker.h"
typedef vector<Person*> pvec;//Vector of person pointers
typedef struct county_record {
  int id;
  pvec habitants;
  double immunity_by_age [102];
  int pop;
  int people_by_age[102];
  int people_immunized;
} county_record_t;

bool Vector_Layer::Enable_Vector_Control = false;
bool Vector_Layer::School_Vector_Control = false;
bool Vector_Layer::Workplace_Vector_Control = false;
bool Vector_Layer::Household_Vector_Control = false;
bool Vector_Layer::Neighborhood_Vector_Control = false;
bool Vector_Layer::Limit_Vector_Control = false;

vector<county_record_t> county_set;

Vector_Layer::Vector_Layer() {
  Regional_Layer* base_grid = Global::Simulation_Region;
  this->min_lat = base_grid->get_min_lat();
  this->min_lon = base_grid->get_min_lon();
  this->max_lat = base_grid->get_max_lat();
  this->max_lon = base_grid->get_max_lon();
  this->min_x = base_grid->get_min_x();
  this->min_y = base_grid->get_min_y();
  this->max_x = base_grid->get_max_x();
  this->max_y = base_grid->get_max_y();
  this->total_infected_vectors = 0;
  this->total_infectious_hosts = 0;
  this->total_infected_hosts = 0;

  this->death_rate = 1.0/18.0;
  this->birth_rate = 1.0/18.0;
  this->bite_rate = 0.76;
  this->incubation_rate = 1.0/11.0;
  this->suitability = 1.0;
  this->pupae_per_host = 1.02; //Armenia average
  this->life_span = 18.0; // From Chao and longini
  this->sucess_rate = 0.83; // Focks
  this->female_ratio = 0.5;
  // this->development_time = 1.0;

  // get vector_control parameters
  int temp_int;
  Params::get_param_from_string("enable_vector_control", &temp_int);
  Vector_Layer::Enable_Vector_Control = (temp_int == 0 ? false : true);

  if (Vector_Layer::Enable_Vector_Control) {
    Params::get_param_from_string("school_vector_control", &temp_int);
    Vector_Layer::School_Vector_Control = (temp_int == 0 ? false : true);
    Params::get_param_from_string("workplace_vector_control", &temp_int);
    Vector_Layer::Workplace_Vector_Control = (temp_int == 0 ? false : true);
    Params::get_param_from_string("household_vector_control", &temp_int);
    Vector_Layer::Household_Vector_Control = (temp_int == 0 ? false : true);
    Params::get_param_from_string("neighborhood_vector_control", &temp_int);
    Vector_Layer::Neighborhood_Vector_Control = (temp_int == 0 ? false : true);
    Params::get_param_from_string("limit_vector_control", &temp_int);
    Vector_Layer::Limit_Vector_Control = (temp_int == 0 ? false : true);
  }

  // determine patch size for this layer
  Params::get_param_from_string("vector_patch_size", &this->patch_size);
  // Get probabilities of transmission 
  Params::get_param_from_string("vector_infection_efficiency", &this->infection_efficiency);
  Params::get_param_from_string("vector_transmission_efficiency", &this->transmission_efficiency);
  Params::get_param_from_string("place_seeding_probability", &this->place_seeding_probability);
  Params::get_param_from_string("mosquito_seeds", &this->mosquito_seeds);
  // determine number of rows and cols
  this->rows = static_cast<double>((this->max_y - this->min_y)) / this->patch_size;
  if(this->min_y + this->rows * this->patch_size < this->max_y) {
    this->rows++;
  }

  this->cols = static_cast<double>((this->max_x - this->min_x)) / this->patch_size;
  if(this->min_x + this->cols * this->patch_size <this-> max_x) {
    this->cols++;
  }

  if(Global::Verbose > 0) {
    fprintf(Global::Statusfp, "Vector_Layer min_lon = %f\n", this->min_lon);
    fprintf(Global::Statusfp, "Vector_Layer min_lat = %f\n", this->min_lat);
    fprintf(Global::Statusfp, "Vector_Layer max_lon = %f\n", this->max_lon);
    fprintf(Global::Statusfp, "Vector_Layer max_lat = %f\n", this->max_lat);
    fprintf(Global::Statusfp, "Vector_Layer rows = %d  cols = %d\n", this->rows, this->cols);
    fprintf(Global::Statusfp, "Vector_Layer min_x = %f  min_y = %f\n", this->min_x, this->min_y);
    fprintf(Global::Statusfp, "Vector_Layer max_x = %f  max_y = %f\n", this->max_x, this->max_y);
    fflush(Global::Statusfp);
  }

  this->grid = new Vector_Patch*[this->rows];
  for(int i = 0; i < this->rows; ++i) {
    this->grid[i] = new Vector_Patch[this->cols];
  }

  for(int i = 0; i < this->rows; ++i) {
    for(int j = 0; j < this->cols; ++j) {
      this->grid[i][j].setup(i, j, this->patch_size, this->min_x, this->min_y);
    }      
  }
  // To read the temperature grid
  this->read_temperature();
  // Read location where a proportion of mosquitoes susceptibles are infected externaly (birth or migration)
  this->read_vector_seeds();
}
void Vector_Layer::swap_county_people(){
  int cols = Global::Simulation_Region->get_cols();
  int rows = Global::Simulation_Region->get_rows();
  for(int i = 0; i < rows; ++i) {
    for(int j = 0; j < cols; ++j) {
      Regional_Patch* region_patch = Global::Simulation_Region->get_patch(i,j);
      int pop_size = region_patch ->get_popsize();
      if(pop_size > 0) {
        region_patch -> swap_county_people();
      }
    }
  }
}
void Vector_Layer::read_temperature(){
  double  patch_temperature;
  double lati;
  double longi;
  fred::geo lat;
  fred::geo lon;
  FILE* fp;
  char filename[FRED_STRING_SIZE];
  strcpy(filename, "$FRED_HOME/input_files/countries/colombia/temperature_grid.txt");
  fp = Utils::fred_open_file(filename);
  //Obtain temperature values for each patch...lat,lon,oC
  if(fp != NULL) {
    while(!feof(fp))
      {
	int char_count = fscanf(fp, "%lg, %lg, %lg", &lati, &longi, &patch_temperature);
	lat = lati;
	lon = longi;
	Vector_Patch* patch = get_patch(lat,lon);
	if(patch != NULL) {
	  patch->set_temperature(patch_temperature);
	}
      }
  } else {
    Utils::fred_abort("Cannot  open %s to read the average temperature grid \n", filename);
  }
  fclose(fp);
}
void Vector_Layer::seed_patches_by_distance_in_km(fred::geo lat, fred::geo lon,
						  double radius_in_km, int dis,int day_on, int day_off,double seeds_) {
  //ASSUMMING WE ARE IN THE TROPIC 1/120 degree ~ 1 km
  //  printf("SEED_PATCHES_BY_DISTANCE entered\n");
  int kilometers_per_degree = 120;
  int number_of_patches = static_cast<int>(radius_in_km) / this->patch_size;
  //find the patch of the middle point
  Vector_Patch* patch = get_patch(lat, lon);
  if(patch != NULL){
    //printf("SEED_PATCHES_BY_DISTANCE Patch found in the grid lat %lg lon %lg\n",lat,lon);
    int r1 = patch->get_row() - number_of_patches;
    r1 = (r1 >= 0) ? r1 : 0;
    int r2 = patch->get_row() + number_of_patches;
    r2 = (r2 <= this->rows - 1) ? r2 : this->rows - 1;
    
    int c1 = patch->get_col() - number_of_patches;
    c1 = (c1 >= 0) ? c1 : 0;
    int c2 = patch->get_col() + number_of_patches;
    c2 = (c2 <= this->cols - 1) ? c2 : this->cols - 1;
    
    //    printf("SEED_PATCHES_BY_DISTANCE number of patches %d r1 %d r2 %d c1 %d c2 %d\n",number_of_patches,r1,r2,c1,c2);
    
    for(int r = r1; r <= r2; ++r) {
      for(int c = c1; c <= c2; ++c) {
	Vector_Patch* p = get_patch(r, c);
	double hx = (r - patch->get_row()) /this->patch_size;
	double hy = (c - patch->get_col()) / this->patch_size;
	if(sqrt((hx) * ( hx) + ( hy) * ( hy)) <= radius_in_km) {
	  p->set_vector_seeds(dis, day_on,  day_off, seeds_);
	}
      }
    }
  }
}

void Vector_Layer::read_vector_seeds(){
  // first I will try with all seeded 
  char filename[FRED_STRING_SIZE];
  FILE* fp;
  Params::get_param_from_string("vector_seeds_file", filename);
  fp = Utils::fred_open_file(filename);
  //  printf("seeds filename: %s\n", filename);
  if(fp != NULL) {
    char linestring[100];
    while(fgets(linestring,100,fp) != NULL){
      int day_on=-1;
      int day_off=-1;
      int dis=-1;
      double lat_ = -999.9;
      double lon_ = -999.9;
      double radius_ = -1.0; //in kilometers
      int total_houses_seeds = 0;
      sscanf(linestring,"%d %d %d %lg %lg %lg\n",&day_on, &day_off, &dis, &lat_,&lon_, &radius_);
      if(radius_ > 0) {
	FRED_VERBOSE(0,"Attempting to seed infectious vectors %lg proportion in %lg proportion of houses, day_on %d day_off %d disease %d lat %lg lon %lg radius %lg \n", mosquito_seeds,place_seeding_probability,day_on,day_off,dis,lat_,lon_,radius_);
	//make a list of houses in the radius
        fred::geo lat = lat_;
        fred::geo lon = lon_;
        if(this->mosquito_seeds < 0) {
          this->mosquito_seeds = 0;
        }
        if((dis >= 0) && (dis < Global::MAX_NUM_DISEASES) && (day_on >= 0) && (day_off >= 0)) {
          this->seed_patches_by_distance_in_km(lat, lon, radius_, dis, day_on, day_off, this->mosquito_seeds);
        }
      } else {
        FRED_VERBOSE(0,"Attempting to seed infectious vectors %lg proportion in %lg proportion of houses, day_on %d day_off %d disease %d in all houses \n", mosquito_seeds,place_seeding_probability,day_on,day_off,dis);
        if(this->mosquito_seeds < 0) {
          this->mosquito_seeds = 0;
        }
        if((dis >= 0) && (dis<Global::MAX_NUM_DISEASES) && (day_on >= 0) && (day_off >= 0)) {
          for(int i = 0; i < this->rows; ++i) {
            for (int j = 0; j < this->cols; ++j) {
              this->grid[i][j].set_vector_seeds(dis, day_on, day_off, this->mosquito_seeds);
            }
          }
        }
      }
    }
  }
  fclose(fp);
}

Vector_Patch* Vector_Layer::get_patch(int row, int col) {
  if(row >= 0 && col >= 0 && row < this->rows && col < this->cols) {
    return &this->grid[row][col];
  } else {
    return NULL;
  }
}

Vector_Patch* Vector_Layer::get_patch(fred::geo lat, fred::geo lon) {
  int row = get_row(lat);
  int col = get_col(lon);
  return get_patch(row, col);
}

Vector_Patch* Vector_Layer::select_random_patch() {
  int row = Random::draw_random_int(0, this->rows - 1);
  int col = Random::draw_random_int(0, this->cols - 1);
  return &this->grid[row][col];
}

void Vector_Layer::quality_control() {
  if(Global::Verbose) {
    fprintf(Global::Statusfp, "vector grid quality control check\n");
    fflush(Global::Statusfp);
  }
  
  for(int row = 0; row < this->rows; ++row) {
    for(int col = 0; col < this->cols; ++col) {
      this->grid[row][col].quality_control();
    }
  }
  
  if(Global::Verbose) {
    fprintf(Global::Statusfp, "vector grid quality control finished\n");
    fflush(Global::Statusfp);
  }
}

void Vector_Layer::setup() {
  int num_households = Global::Places.get_number_of_households();
  for(int i = 0; i < num_households; ++i) {
    Household* house = Global::Places.get_household_ptr(i);
    add_hosts(house);
  }
}

void Vector_Layer::update(int day) {
  this->total_infected_vectors = 0;
  this->total_infected_hosts = 0;
  this->total_infectious_hosts = 0;

  FRED_VERBOSE(1,"Vector_Layer::update() entered on day %d\n", day);
  // Global::Daily_Tracker->log_key_value("Vec_I", total_infected_vectors);
  // Global::Daily_Tracker->log_key_value("Vec_H", total_infectious_hosts);
}

void Vector_Layer::update_visualization_data(int disease_id, int day) {
  for (int i = 0; i < this->rows; ++i) {
    for (int j = 0; j < this->cols; ++j) {
      Vector_Patch* patch = static_cast<Vector_Patch*>(&this->grid[i][j]);
      int count = 0; // patch->get_infected_vectors();
      if(count > 0) {
	double x = patch->get_center_x();
	double y = patch->get_center_y();
	Global::Visualization->update_data(x,y,count,0);
      }
    }
  }
}

void Vector_Layer::add_hosts(Place* p) {
  /*
  fred::geo lat = p->get_latitude();
  fred::geo lon = p->get_longitude();
  int hosts = p->get_size();
  Vector_Patch* patch = get_patch(lat,lon);
  if(patch != NULL) {
    patch->add_hosts(hosts);
  }
  */
}

double Vector_Layer::get_seeds(Place* p, int dis, int day) {
  fred::geo lat = p->get_latitude();
  fred::geo lon = p->get_longitude();
  Vector_Patch* patch = get_patch(lat,lon);
  if(patch != NULL) {
    return patch->get_seeds(dis, day);
  } else {
    return 0.0;
  }
}

double Vector_Layer::get_seeds(Place* p, int dis) {
  fred::geo lat = p->get_latitude();
  fred::geo lon = p->get_longitude();
  Vector_Patch* patch = get_patch(lat,lon);
  if(patch != NULL) {
    double seeds_ = patch->get_seeds(dis);
    if(seeds_ > 0) {
      if(Random::draw_random(0,1) < this->place_seeding_probability) {
	return seeds_;
      } else {
	return 0.0;
      } 
    } else {
      return 0.0;
    }
  } else {
    return 0.0;
  }
}

double Vector_Layer::get_day_start_seed(Place* p, int dis) {
  fred::geo lat = p->get_latitude();
  fred::geo lon = p->get_longitude();
  Vector_Patch* patch = get_patch(lat,lon);
  if(patch != NULL) {
    return patch->get_day_start_seed(dis);
  } else {
    return 0;
  }
}

double Vector_Layer::get_day_end_seed(Place* p, int dis) {
  fred::geo lat = p->get_latitude();
  fred::geo lon = p->get_longitude();
  Vector_Patch* patch = get_patch(lat,lon);
  if(patch != NULL) {
    return patch->get_day_end_seed(dis);
  } else {
    return 0;
  }
}

void Vector_Layer::add_host(Person* person, Place* place) {
  /*
  if(place->is_neighborhood()) {
    return;
  }
  fred::geo lat = place->get_latitude();
  fred::geo lon = place->get_longitude();
  Vector_Patch* patch = get_patch(lat,lon);
  if(patch != NULL) {
    patch->add_host(person);
  }
  */
}
void Vector_Layer::get_county_ids(){
  // get the county ids from external file
  char county_codes_file[FRED_STRING_SIZE];
  int num_households = Global::Places.get_number_of_households();
  county_record_t county_temp;
  Params::get_param_from_string("county_codes_file", county_codes_file);
  FILE* fp = Utils::fred_open_file(county_codes_file);
  if(fp == NULL) {
    Utils::fred_abort("Can't open county_codes_file %s\n", county_codes_file);
  }
  county_set.clear();
  county_temp.habitants.clear();
  while(fscanf(fp, "%d ", &county_temp.id) == 1) {
    county_set.push_back(county_temp);
  }
  fclose(fp);
  for(int i = 0; i < county_set.size(); ++i) {
    FRED_VERBOSE(2, "County record set for county id:  %d \n", county_set[i].id);
  }
  for(int i = 0; i < num_households; ++i) {
    int household_county = -1;
    Household* h = Global::Places.get_household_ptr(i);
    int c = h->get_county_index();
    int h_county = Global::Places.get_fips_of_county_with_index(c);
    for (int j = 0;j<county_set.size();j++){
      if(h_county == county_set[j].id){
	household_county = j;
      }
    }
    //find the county for each household
    if(household_county == -1){
      Utils::fred_abort("No county code found for house in county %d\n", h_county);
    }
    int house_mates = h->get_size();
    // load every person in the house in a  county
    for(int j = 0; j<house_mates; ++j) {
      Person* p = h->get_enrollee(j);
      county_set[household_county].habitants.push_back(p);
    }
  }
  FRED_VERBOSE(0,"get_county_ids finished\n");
}
void Vector_Layer::get_immunity_from_file() {
  FILE* fp;
  // get the prior immune proportion by age  from external file for each county for total immunity
  char prior_immune_file[FRED_STRING_SIZE];
  Params::get_param_from_string("prior_immune_file", prior_immune_file);
  fp = Utils::fred_open_file(prior_immune_file);
  if(fp == NULL) {
    Utils::fred_abort("Can't open prior_immune_file %s\n", prior_immune_file);
  }
  while(!feof(fp)) {
    int temp_county;
    int index_county = -1;
    double temp_immune ;
    if(fscanf(fp,"%d ", &temp_county) == 1) {
      for(int i = 0; i < county_set.size(); ++i) {
	if(county_set[i].id == temp_county) {
	  index_county = i;
	}
      }
      if(index_county == -1) {
	Utils::fred_abort("No county found %d\n",temp_county);
      }
      FRED_VERBOSE(2,"County code  %d\n",temp_county);
      for(int i = 0; i < 102; ++i) {
	fscanf(fp,"%lg ",&temp_immune);
	county_set[index_county].immunity_by_age[i] = temp_immune;
	FRED_VERBOSE(2,"Age: %d Immunity  %lg\n", i, temp_immune);
      }
    }
  }
  fclose(fp);
}

void Vector_Layer::get_immunity_from_file(int d) {
  FILE* fp;
  // get the prior immune proportion by age  from external file for each county for total immunity
  char prior_immune_file[FRED_STRING_SIZE];
  char immune_param_string[FRED_STRING_SIZE];
  sprintf(immune_param_string,"prior_immune_file[%d]",d);
  Params::get_param_from_string(immune_param_string, prior_immune_file);
  fp = Utils::fred_open_file(prior_immune_file);
  if(fp == NULL) {
    Utils::fred_abort("Can't open prior_immune_file %s for disease %d \n", prior_immune_file, d);
  }
  while(!feof(fp)) {
    int temp_county;
    int index_county = -1;
    double temp_immune ;
    if(fscanf(fp, "%d ", &temp_county) == 1) {
      for(int i = 0; i < county_set.size(); ++i) {
	if(county_set[i].id == temp_county){
	  index_county = i;
	}
      }
      if(index_county == -1) {
	Utils::fred_abort("No county found %d\n",temp_county);
      }
      FRED_VERBOSE(2,"County code  %d\n",temp_county);
      for(int i = 0; i < 102; ++i) {
	if(fscanf(fp, "%lg ", &temp_immune) > 0) {
	  county_set[index_county].immunity_by_age[i] = temp_immune;
	  FRED_VERBOSE(2,"Age: %d Immunity  %lg\n",i,temp_immune);
	}
      }
    }
  }
  fclose(fp);
}

void Vector_Layer::get_people_size_by_age() {
  //calculate number of people by age
  for(int i = 0; i < county_set.size(); ++i) {
    if(county_set[i].habitants.size() > 0) {
      for(int k = 0; k < 102; ++k) {
	county_set[i].people_by_age[k] = 0;
      }
      for(int j = 0; j < county_set[i].habitants.size(); ++j) {
	Person* per = county_set[i].habitants[j];
	int temp_age = per->get_age();
	if(temp_age > 101) {
	  temp_age = 101;
	}
	county_set[i].people_by_age[temp_age]++;
      }
    }
  }
}

void Vector_Layer::immunize_total_by_age() {
  for(int i = 0; i < county_set.size(); ++i) {
    county_set[i].people_immunized = 0;
    if(county_set[i].habitants.size()>0){
      for(int j = 0; j < county_set[i].habitants.size(); ++j) {
	Person* per = county_set[i].habitants[j];
	double prob_immune_ = Random::draw_random(0,1);
	double prob_immune = prob_immune_ * 100;
	int temp_age = per->get_age();
	if(temp_age > 101) {
	  temp_age = 101;
	}
	double prob_by_age = county_set[i].immunity_by_age[temp_age];
	if(prob_by_age > prob_immune) {
	  for(int d = 0;d < DISEASE_TYPES; ++d) {
	    if(per->is_susceptible(d)){
	      per->become_unsusceptible(Global::Diseases.get_disease(d));
	    }
	  }
	  county_set[i].people_immunized++;
	}
      }
    }
  }
}
void Vector_Layer::immunize_by_age(int d) {
  for(int i = 0; i < county_set.size(); ++i) {
    county_set[i].people_immunized = 0;
    if(county_set[i].habitants.size() > 0) {
      for (int j = 0; j < county_set[i].habitants.size(); ++j) {
	Person* per = county_set[i].habitants[j];
	double prob_immune_ = Random::draw_random(0,1);
	double prob_immune = prob_immune_ * 100;
	int temp_age = per->get_age();
	if(temp_age > 101) {
	  temp_age = 101;
	}
	double prob_by_age = county_set[i].immunity_by_age[temp_age];
	if(prob_by_age > prob_immune) {
	  if(per->is_susceptible(d)) {
	    per->become_unsusceptible(Global::Diseases.get_disease(d));
	    county_set[i].people_immunized++;
	  }
	}
      }
    }
  }
}

void Vector_Layer::init_prior_immunity_by_county() {
  FILE* fp;
  this->get_county_ids();
  this->get_immunity_from_file();
  this->get_people_size_by_age();
  this->immunize_total_by_age();
  FRED_VERBOSE(0, "Number of counties %d\n", county_set.size());
  for(int i = 0; i < county_set.size(); ++i) {
    if(county_set[i].habitants.size() > 0) {
      FRED_VERBOSE(0, "County id:  %d Population %d People Immunized: %d\n", county_set[i].id, county_set[i].habitants.size(), county_set[i].people_immunized);
      //      for (int j = 0; j < 102; ++j) {
      //	      FRED_VERBOSE(0,"AGE::  %d \t immune prob %lg\t people_by_age %d\n", j,county_set[i].immunity_by_age[j], county_set[i].people_by_age[j]);
      //	    }
    }
  }
  //  Utils::fred_abort("Running test finishes here\n");
}

void Vector_Layer::init_prior_immunity_by_county(int d) {
  FILE* fp;
  this->get_county_ids();
  this->get_immunity_from_file(d);
  FRED_VERBOSE(2, "Number of counties %d\n", county_set.size());
  this->get_people_size_by_age();
  this->immunize_by_age(d);
  for(int i = 0; i < county_set.size(); ++i) {
    if(county_set[i].habitants.size() > 0) {
      FRED_VERBOSE(0,"County id:  %d Population %d People Immunized: %d Strain %d\n",county_set[i].id, county_set[i].habitants.size(), county_set[i].people_immunized,d);
      //      for (int j = 0; j < 102; ++j){
      //	      FRED_VERBOSE(0,"AGE::  %d \t immune prob %lg\t people_by_age %d\n", j,county_set[i].immunity_by_age[j], county_set[i].people_by_age[j]);
      //	    }
    }
  }
}


void Vector_Layer::report(int day, Epidemic * epidemic) {
  /*
  int vector_pop_temp = get_vector_population();
  int inf_vectors = get_infected_vectors();
  int sus_vectors = get_susceptible_vectors();
  int vector_pop_school = get_school_vectors();
  int vector_pop_workplace = get_workplace_vectors();
  int vector_pop_household = get_household_vectors();
  int vector_pop_neighborhood = get_neighborhood_vectors();
  int school_inf_vectors = get_school_infected_vectors();
  int household_inf_vectors = get_household_infected_vectors();
  int workplace_inf_vectors = get_workplace_infected_vectors();
  int neighborhood_inf_vectors = get_neighborhood_infected_vectors();
  epidemic->track_value(day,(char *)"Nv", vector_pop_temp);
  epidemic->track_value(day,(char *)"Nvs", vector_pop_school);
  epidemic->track_value(day,(char *)"Nvw", vector_pop_workplace);
  epidemic->track_value(day,(char *)"Nvh", vector_pop_household);
  epidemic->track_value(day,(char *)"Nvn", vector_pop_neighborhood);
  epidemic->track_value(day,(char *)"Iv", inf_vectors);
  epidemic->track_value(day,(char *)"Ivs", school_inf_vectors);
  epidemic->track_value(day,(char *)"Ivw", workplace_inf_vectors);
  epidemic->track_value(day,(char *)"Ivh", household_inf_vectors);
  epidemic->track_value(day,(char *)"Ivn", neighborhood_inf_vectors);
  epidemic->track_value(day,(char *)"Sv", sus_vectors);
  if(Vector_Layer::Enable_Vector_Control){
    int total_places_vc = get_places_in_vector_control();
    int total_schools_vc = get_schools_in_vector_control();
    int total_households_vc = get_households_in_vector_control();
    int total_workplaces_vc = get_workplaces_in_vector_control();
    int total_neighborhoods_vc = get_schools_in_vector_control();
    epidemic->track_value(day,(char *)"Pvc", total_places_vc);
    epidemic->track_value(day,(char *)"Svc", total_schools_vc);
    epidemic->track_value(day,(char *)"Hvc", total_households_vc);
    epidemic->track_value(day,(char *)"Wvc", total_workplaces_vc);
    epidemic->track_value(day,(char *)"Nvc", total_neighborhoods_vc);
  }
  */
}

double Vector_Layer::get_vectors_per_host(Place* place) {

  double development_time = 1.0;
  double vectors_per_host = 0.0;

  //temperatures vs development times..FOCKS2000: DENGUE TRANSMISSION THRESHOLDS
  double temps[8] = {8.49,3.11,4.06,3.3,2.66,2.04,1.46,0.92}; //temperatures
  double dev_times[8] = {15.0,20.0,22.0,24.0,26.0,28.0,30.0,32.0}; //development times

  double temperature = -999.9;
  fred::geo lat = place->get_latitude();
  fred::geo lon = place->get_longitude();
  Vector_Patch* patch = get_patch(lat,lon);
  if(patch != NULL) {
    temperature = patch->get_temperature();
  }
  if(temperature > 32) {
    temperature = 32;
  }
  if(temperature <= 18) {
    vectors_per_host = 0;
  } else {
    for(int i = 0; i < 8; ++i) {
      if(temperature <= temps[i]) {
        //obtain the development time using linear interpolation
        development_time = dev_times[i - 1] + (dev_times[i] - dev_times[i - 1]) / (temps[i] - temps[i-1]) * (temperature - temps[i - 1]);
      }
    }
    vectors_per_host = pupae_per_host * female_ratio * sucess_rate * life_span / development_time;
  }
  FRED_VERBOSE(1, "SET TEMP: place %s temp %f vectors_per_host %f N_vectors %d N_orig %d\n",
	       place->get_label(), temperature, vectors_per_host);
  return vectors_per_host;
}

vector_disease_data_t Vector_Layer::update_vector_population(int day, Place * place) {

  place->mark_vectors_as_not_infected_today();

  vector_disease_data_t v = place->get_vector_disease_data();

  if(place->is_neighborhood()){
    return v;
  }
  
  if(v.N_vectors <= 0) {
    return v;
  }
  FRED_VERBOSE(1,"update vector pop: day %d place %s initially: S %d, N: %d\n",
	       day,place->get_label(),v.S_vectors,v.N_vectors);
  
  int born_infectious[DISEASE_TYPES];
  int total_born_infectious = 0;

  // new vectors are born susceptible
  v.S_vectors += floor(this->birth_rate * v.N_vectors - this->death_rate * v.S_vectors);
  FRED_VERBOSE(1,"vector_update_population:: S_vector: %d, N_vectors: %d\n", v.S_vectors, v.N_vectors);
  // but some are infected
  for(int d=0;d<DISEASE_TYPES;d++){
    double seeds = place->get_seeds(d, day);
    born_infectious[d] = ceil(v.S_vectors * seeds);
    total_born_infectious += born_infectious[d];
    if(born_infectious[d] > 0) {
      FRED_VERBOSE(1,"vector_update_population:: Vector born infectious disease[%d] = %d \n",d, born_infectious[d]);
      FRED_VERBOSE(1,"Total Vector born infectious: %d \n", total_born_infectious);
    }
  }
  // printf("PLACE %s SEEDS %d S_vectors %d DAY %d\n",place->get_label(),total_born_infectious,v.S_vectors,day);
  v.S_vectors -= total_born_infectious;
  FRED_VERBOSE(1,"vector_update_population - seeds:: S_vector: %d, N_vectors: %d\n", v.S_vectors, v.N_vectors);
  // print this 
  if(total_born_infectious > 0){
    FRED_VERBOSE(1,"Total Vector born infectious: %d \n", total_born_infectious);// 
  }
  if(v.S_vectors < 0) {
    v.S_vectors = 0;
  }
    
  // accumulate total number of vectors
  v.N_vectors = v.S_vectors;
  // we assume vectors can have at most one infection, if not susceptible
  for(int i = 0; i < DISEASE_TYPES; ++i) {
    // some die
    FRED_VERBOSE(1,"vector_update_population:: E_vectors[%d] = %d \n",i, v.E_vectors[i]);
    if(v.E_vectors[i] < 10){
      for(int k = 0; k < v.E_vectors[i]; ++k) {
	double r = Random::draw_random(0,1);
	if(r < this->death_rate){
	  v.E_vectors[i]--;
	}
      }
    } else {
      v.E_vectors[i] -= floor(this->death_rate * v.E_vectors[i]);
    } 
    // some become infectious
    int become_infectious = 0;
    if(v.E_vectors[i] < 10) {
      for(int k = 0; k < v.E_vectors[i]; ++k) {
	double r = Random::draw_random(0,1);
	if(r < this->incubation_rate) {
	  become_infectious++;
	}
      }
    } else {
      become_infectious = floor(this->incubation_rate * v.E_vectors[i]);
    }
      
    // int become_infectious = floor(incubation_rate * E_vectors[i]);
    FRED_VERBOSE(1,"vector_update_population:: become infectious [%d] = %d, incubation rate: %f,E_vectors[%d] %d \n", i,
		 become_infectious, this->incubation_rate, i, v.E_vectors[i]);
    v.E_vectors[i] -= become_infectious;
    if(v.E_vectors[i] < 0) v.E_vectors[i] = 0;
    // some die
    FRED_VERBOSE(1,"vector_update_population:: I_Vectors[%d] = %d \n", i, v.I_vectors[i]);
    v.I_vectors[i] -= floor(this->death_rate * v.I_vectors[i]);
    FRED_VERBOSE(1,"vector_update_population:: I_Vectors[%d] = %d \n", i, v.I_vectors[i]);
    // some become infectious
    v.I_vectors[i] += become_infectious;
    FRED_VERBOSE(1,"vector_update_population:: I_Vectors[%d] = %d \n", i, v.I_vectors[i]);
    // some were born infectious
    v.I_vectors[i] += born_infectious[i];
    FRED_VERBOSE(1,"vector_update_population::+= born infectious I_Vectors[%d] = %d,born infectious[%d] = %d \n", i,
		 v.I_vectors[i], i, born_infectious[i]);
    if(v.I_vectors[i] < 0) {
      v.I_vectors[i] = 0;
    }

    // add to the total
    v.N_vectors += (v.E_vectors[i] + v.I_vectors[i]);
    FRED_VERBOSE(1, "update_vector_population entered S_vectors %d E_Vectors[%d] %d  I_Vectors[%d] %d N_Vectors %d\n",
		 v.S_vectors, i, v.E_vectors[i], i, v.I_vectors[i], v.N_vectors);

  }
  return v;
}

