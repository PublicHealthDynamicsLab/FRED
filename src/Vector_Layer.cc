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
#include <fstream>
#include <string>
#include <sstream>
using namespace std;

#include "Household.h"
#include "Condition.h"
#include "Condition_List.h"
#include "Epidemic.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include "Params.h"
#include "Place.h"
#include "Place_List.h"
#include "Random.h"
#include "Regional_Layer.h"
#include "Regional_Patch.h"
#include "School.h"
#include "Tracker.h"
#include "Utils.h"
#include "Vector_Layer.h"
#include "Vector_Patch.h"
#include "Visualization_Layer.h"
#include "Workplace.h"

typedef vector<Person*> pvec;//Vector of person pointers
typedef vector <Neighborhood_Patch*> patch_vec;//Vector of person pointers
typedef struct county_record {
  int id;
  pvec habitants;
  double immunity_by_age [102];
  int pop;
  int people_by_age[102];
  int people_immunized;
} county_record_t;
typedef struct census_tract_record {
  float threshold;
  int ind;
  int popsize;
  int total_neighborhoods;
  int first_day_infectious;
  patch_vec infectious_neighborhoods;
  patch_vec non_infectious_neighborhoods;
  patch_vec vector_control_neighborhoods;
  patch_vec neighborhoods;
  bool eligible_for_vector_control;
  bool exceeded_threshold;
} census_t;

bool Vector_Layer::Enable_Vector_Control = false;
bool Vector_Layer::School_Vector_Control = false;
bool Vector_Layer::Workplace_Vector_Control = false;
bool Vector_Layer::Household_Vector_Control = false;
bool Vector_Layer::Neighborhood_Vector_Control = false;
bool Vector_Layer::Limit_Vector_Control = false;

vector <census_t> census_tract_set;
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
  Params::get_param_from_string("pupae_per_host", &this->pupae_per_host); 
  this->life_span = 18.0; // From Chao and longini
  this->sucess_rate = 0.83; // Focks 2000
  this->female_ratio = 0.5; // Focks 2000

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
    Params::get_param_from_string("vector_control_threshold", &vector_control_threshold);
    Params::get_param_from_string("vector_control_day_on", &vector_control_day_on);
    Params::get_param_from_string("vector_control_day_off", &vector_control_day_off);
    Params::get_param_from_string("vector_control_coverage", &vector_control_coverage);
    Params::get_param_from_string("vector_control_efficacy", &vector_control_efficacy);
    Params::get_param_from_string("vector_control_neighborhoods_rate", &vector_control_neighborhoods_rate);
    Params::get_param_from_string("vector_control_max_places", &vector_control_max_places);
    Params::get_param_from_string("vector_control_random",&vector_control_random);
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
  census_tract_set.clear();
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
std::vector<int> Vector_Layer::read_vector_control_tracts(char * filename){
  std::vector<int>temp_tracts;
  temp_tracts.clear();
  FILE *fp;
  Utils::get_fred_file_name(filename);
  fp = Utils::fred_open_file(filename);
  if (fp !=NULL){
    while(!feof(fp))
      {
	int temp_census_tract_ = 0;
	fscanf(fp,"%d\n",&temp_census_tract_);
	if (temp_census_tract_ > 100){
	  temp_tracts.push_back(temp_census_tract_);
	}
      }
  }else{
    Utils::fred_abort("Cannot  open %s to read the vector control tracts\n", filename);
  }
  fclose(fp);
  return temp_tracts;
}
void Vector_Layer::add_infectious_patch(Place * p, int day){
  int col_ = Global::Neighborhoods->get_col(p->get_longitude());
  int row_ = Global::Neighborhoods->get_row(p->get_latitude());
  Neighborhood_Patch * patch_n = Global::Neighborhoods->get_patch(row_,col_);
  Place * n_ = patch_n->get_neighborhood();
  if(n_ == NULL){
    FRED_VERBOSE(1,"add_infectious_patch neighborhood is NULL\n");
    return;
  }
  int tract = n_->get_census_tract_fips();
  int tract_index_ = -1;
  for (int i = 0; i < census_tract_set.size(); i++) {
    if (tract == census_tract_set[i].ind) {
      tract_index_ = i;
    }
  }
  if(tract_index_ < 0){
    FRED_VERBOSE(1,"add_infectious_patch tract_index is < 0\n");
    return;
  }
  FRED_VERBOSE(1,"add_infectious_patch entered day %d tract %d -> %d\n",day,tract_index_,census_tract_set[tract_index_].ind);
  if(census_tract_set[tract_index_].eligible_for_vector_control){
    if( census_tract_set[tract_index_].first_day_infectious == -1){
      census_tract_set[tract_index_].first_day_infectious = day;
      FRED_VERBOSE(1,"add_infectious_patch::Census_tract: %d First day of symptomatics %d \n",census_tract_set[tract_index_].ind,day);
    }
    if(vector_control_random == 0 && patch_n->get_vector_control_status() == 0){
      census_tract_set[tract_index_].infectious_neighborhoods.push_back(patch_n);
      //vector control status: 0 -> non_infectious, 1 -> infectious, 2-> in vector control
      patch_n->set_vector_control_status(1);
      FRED_VERBOSE(1,"add_infectious_patch finished day %d tract %d -> %d\n",day,tract_index_,census_tract_set[tract_index_].ind);
    }
  }
}
void Vector_Layer::update_vector_control_by_census_tract(int day){
  FRED_VERBOSE(1,"update_vector_control_by_census_tract entered day %d\n",day);
  if(vector_control_places_enrolled >= vector_control_max_places && Limit_Vector_Control == true){
    return;
  }
  int total_census_tracts = Global::Places.get_number_of_census_tracts();
  for(int i = 0;i < total_census_tracts;i++){
    if(census_tract_set[i].eligible_for_vector_control && (census_tract_set[i].first_day_infectious >=0)){
      int symp_incidence_by_tract = 0;
      for(int d = 0; d < Global::Conditions.get_number_of_conditions(); d++) {
        Condition * condition = Global::Conditions.get_condition(d);
        Epidemic * epidemic = condition->get_epidemic();
        symp_incidence_by_tract += epidemic->get_symptomatic_incidence_by_tract_index(i);
      }
      //calculate the weekly incidence rate by 100.000 inhabitants                                                            
      double symp_incidence_ = (double) symp_incidence_by_tract / (double) census_tract_set[i].popsize * 100000.0 * 7.0; 
      FRED_VERBOSE(1,"Census_tract: %d, symp_incidence %lg population %d symp_incidence_tract %d\n",census_tract_set[i].ind,symp_incidence_,census_tract_set[i].popsize,symp_incidence_by_tract);
      if(symp_incidence_  >= census_tract_set[i].threshold && census_tract_set[i].exceeded_threshold == false){
        census_tract_set[i].exceeded_threshold = true;
        // if it is the capital of the county, then all the census tracts of the county are choosen for vector control                                                                                               
        int census_end = census_tract_set[i].ind % 1000;
        FRED_VERBOSE(1,"Census_tract: %d threshold exceeded\n",census_tract_set[i].ind);
        if(census_end == 1){
          FRED_VERBOSE(1,"Capital : Census_tract: %d\n",census_tract_set[i].ind);
          int county_capital = floor(census_tract_set[i].ind / 1000);
          for(int k = 0;k<total_census_tracts;k++){
            int county_temp =  floor(census_tract_set[k].ind / 1000);
            if(county_temp == county_capital){
              FRED_VERBOSE(1,"Census_tract: %d and %d same county, threshold exceeded\n",census_tract_set[i].ind, census_tract_set[k].ind);
	      census_tract_set[k].exceeded_threshold = true;                                                                                                                                                         
            }
          }
        }
      }
      if(census_tract_set[i].exceeded_threshold == true){
        int total_neighborhoods_enrolled = 0;
        int max_n = 0;
        int neighborhoods_enrolled_today = floor(vector_control_neighborhoods_rate * census_tract_set[i].total_neighborhoods);
        FRED_VERBOSE(1,"update_vector_control_by_census_tract::Census_tract: %d, %d infectious neighborhoods, neighborhoods to enroll %d\n",census_tract_set[i].ind,census_tract_set[i].infectious_neighborhoods.size(),neighborhoods_enrolled_today);
        if(census_tract_set[i].infectious_neighborhoods.size() > 0){
          max_n = (neighborhoods_enrolled_today <= census_tract_set[i].infectious_neighborhoods.size() ? neighborhoods_enrolled_today : census_tract_set[i].infectious_neighborhoods.size());
          for(int j = 0;j < max_n;j++){
            Neighborhood_Patch * p_n = census_tract_set[i].infectious_neighborhoods.front();
            vector_control_places_enrolled += this->select_places_for_vector_control(p_n,day);
            census_tract_set[i].infectious_neighborhoods.erase(census_tract_set[i].infectious_neighborhoods.begin());
	    census_tract_set[i].vector_control_neighborhoods.push_back(p_n);
            total_neighborhoods_enrolled++;
            if(vector_control_places_enrolled >= vector_control_max_places && Limit_Vector_Control == true){
              break;
            }
          }
        }
        if(census_tract_set[i].non_infectious_neighborhoods.size() > 0){
          patch_vec non_inf_neighborhoods;
          non_inf_neighborhoods.clear();
          FYShuffle<Neighborhood_Patch*>(census_tract_set[i].non_infectious_neighborhoods);
	  for(int k = 0;k<census_tract_set[i].non_infectious_neighborhoods.size();k++){
            if(census_tract_set[i].non_infectious_neighborhoods[k]->get_vector_control_status() == 0){
	      non_inf_neighborhoods.push_back(census_tract_set[i].non_infectious_neighborhoods[k]);
	    }
	  }
	  census_tract_set[i].non_infectious_neighborhoods.clear();
	  census_tract_set[i].non_infectious_neighborhoods = non_inf_neighborhoods;
          non_inf_neighborhoods.clear();
          int neighborhoods_to_enroll = neighborhoods_enrolled_today - max_n;
          if(neighborhoods_to_enroll > census_tract_set[i].non_infectious_neighborhoods.size()){
            neighborhoods_to_enroll = census_tract_set[i].non_infectious_neighborhoods.size();
          }
	  if(neighborhoods_to_enroll > 0){
	    for(int j = 0;j<neighborhoods_to_enroll;j++){
	      Neighborhood_Patch * p_n = census_tract_set[i].non_infectious_neighborhoods.front();
	      vector_control_places_enrolled += this->select_places_for_vector_control(p_n,day);
	      census_tract_set[i].vector_control_neighborhoods.push_back(p_n);
	      census_tract_set[i].non_infectious_neighborhoods.erase(census_tract_set[i].non_infectious_neighborhoods.begin());
	      total_neighborhoods_enrolled++;
	      if(vector_control_places_enrolled >= vector_control_max_places && Limit_Vector_Control == true){
		break;
	      }
	    }
	  }
	}
        FRED_VERBOSE(1,"update_vector_control_by_census_tract::Census_tract: %d,total neighborhoods enrolled %d, neighborhoods to enroll %d, neighborhoods enrolled %d places enrolled %d\n",census_tract_set[i].ind,census_tract_set[i].vector_control_neighborhoods.size(),neighborhoods_enrolled_today, total_neighborhoods_enrolled, vector_control_places_enrolled);
      }
    }
  }

  FRED_VERBOSE(1,"update_vector_control_by_census_tract_finished\n");



  // this->make_eligible_for_vector_control(neighborhood_temp);
}
int Vector_Layer::select_places_for_vector_control(Neighborhood_Patch * patch_n, int day){
  int places_enrolled = 0;
  if(patch_n != NULL){
    FRED_VERBOSE(1,"select_places_for_vector_control entered\n");
    patch_n->set_vector_control_status(2);
    int places = 0;
    if(Household_Vector_Control){
      places = patch_n->get_number_of_households();
      for (int i = 0; i < places; i++) {
	double r = Random::draw_random(0,1);
	if(r < vector_control_coverage){
	  Place *place = patch_n->get_household(i);
	  place->set_vector_control();
	  places_enrolled++;
	}
      }
    }

    if(School_Vector_Control){
      places = patch_n->get_number_of_schools();
      for (int i = 0; i < places; i++) {
	double r = Random::draw_random(0,1);
	if(r < vector_control_coverage){
	  Place *place = patch_n->get_school(i);
	  place->set_vector_control();
	  places_enrolled++;
	}
      }
    }

    if(Workplace_Vector_Control){
      places = patch_n->get_number_of_workplaces();
      for (int i = 0; i < places; i++) {
	double r = Random::draw_random(0,1);
	if(r < vector_control_coverage){
	  Place *place = patch_n->get_workplace(i);
	  place->set_vector_control();
	  places_enrolled++;
	}
      }
    }
    // skip neighborhoods?
    /*
      places = Global::Places.get_number_of_neighborhoods();
      for (int i = 0; i < places; i++) {
      Place *place = Global::Places.get_neighborhood(i);
      neighborhood_vectors += place->get_vector_population_size();
      neighborhood_infected_vectors += place->get_infected_vectors(condition_id);
      if(place->get_vector_control_status()){
      neighborhoods_in_vector_control++;
      }
      }
    */

  }else{
    FRED_VERBOSE(1,"select_places_for_vector_control patch NULL\n");
  }
  return places_enrolled;
}
void Vector_Layer::setup_vector_control_by_census_tract(){
  FRED_VERBOSE(0,"setup_vector_control_by_census_tract entered\n");
  int temp_int = 0;
  char filename[FRED_STRING_SIZE];
  int cols_n = Global::Neighborhoods->get_cols();
  int rows_n = Global::Neighborhoods->get_rows();
  Params::get_param_from_string("vector_control_census_tracts_file", filename);
  census_tracts_with_vector_control.clear();
  vector_control_places_enrolled = 0;

  //read census tracts to implement vector control
  census_tracts_with_vector_control = this->read_vector_control_tracts(filename);
  int total_census_tracts = Global::Places.get_number_of_census_tracts();

  //Load census tracts in simulation and assign eligibility for vector control

  for(int i = 0;i < total_census_tracts;i++){
    census_t census_temp;
    census_temp.infectious_neighborhoods.clear();
    census_temp.non_infectious_neighborhoods.clear();
    census_temp.vector_control_neighborhoods.clear();
    census_temp.neighborhoods.clear();
    census_temp.ind = Global::Places.get_census_tract_with_index(i);
    census_temp.total_neighborhoods = 0;
    census_temp.first_day_infectious = -1;
    census_temp.popsize = 0;
    census_temp.threshold = 1000.00;
    census_temp.eligible_for_vector_control = false;
    census_temp.exceeded_threshold = false;
    for(int j = 0;j<census_tracts_with_vector_control.size();j++){
      if(census_temp.ind == census_tracts_with_vector_control[j]){
	census_temp.eligible_for_vector_control = true;
	census_temp.threshold = this->vector_control_threshold;
      }
    }
    census_tract_set.push_back(census_temp);
  }

  // For each neighborhood that implements vector control, allocate the neighborhoods in the census tract set

  for (int i = 0; i < rows_n; i++) {
    for (int j = 0; j < cols_n; j++) {
      Neighborhood_Patch * neighborhood_temp = Global::Neighborhoods->get_patch(i,j);
      int pop_size = neighborhood_temp->get_popsize();
      if(pop_size > 0){
	Household * h = (Household *) neighborhood_temp->select_random_household();
	int tract = h->get_census_tract_fips();
	neighborhood_temp->get_neighborhood()->set_census_tract_fips(tract);
	int t = -1;
	for (int i = 0; i < census_tract_set.size(); i++) {
	  if (tract == census_tract_set[i].ind) {
	    t = i;
	  }
	}
	census_tract_set[t].popsize +=pop_size;
	if(census_tract_set[t].eligible_for_vector_control == true){
	  census_tract_set[t].total_neighborhoods++;
	  census_tract_set[t].neighborhoods.push_back(neighborhood_temp);
	  census_tract_set[t].non_infectious_neighborhoods.push_back(neighborhood_temp);
	}
      }
    }
    }
  for(int i = 0;i < total_census_tracts;i++){
    if(census_tract_set[i].eligible_for_vector_control){
      FRED_VERBOSE(0,"setup_vector_control Census_tract: %d is eligible for vector control with %d neighborhoods threshold %f cols %d rows %d pop %d\n",census_tract_set[i].ind,census_tract_set[i].total_neighborhoods,census_tract_set[i].threshold, cols_n,rows_n,census_tract_set[i].popsize);
    }
  }
  FRED_VERBOSE(0,"setup_vector_control_by_census_tract finished\n");
}
void Vector_Layer::read_temperature(){
  double  patch_temperature;
  double lati;
  double longi;
  fred::geo lat;
  fred::geo lon;
  FILE* fp;
  char filename[FRED_STRING_SIZE];
  Params::get_param_from_string("temperature_grid_file", filename);
  fp = Utils::fred_open_file(filename);
  //Obtain temperature values for each patch...lat,lon,oC
  double house_;
  double reservoir_;
  double breteau_;
  if(fp != NULL) {
    while(!feof(fp))
      {
	int char_count = fscanf(fp,"%lg,%lg,%lg,%lg,%lg,%lg",&lati,&longi,&patch_temperature,&house_,&reservoir_,&breteau_);
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
	FRED_VERBOSE(0,"Attempting to seed infectious vectors %lg proportion in %lg proportion of houses, day_on %d day_off %d condition %d lat %lg lon %lg radius %lg \n", mosquito_seeds,place_seeding_probability,day_on,day_off,dis,lat_,lon_,radius_);
	//make a list of houses in the radius
        fred::geo lat = lat_;
        fred::geo lon = lon_;
        if(this->mosquito_seeds < 0) {
          this->mosquito_seeds = 0;
        }
        if((dis >= 0) && (dis < Global::MAX_NUM_CONDITIONS) && (day_on >= 0) && (day_off >= 0)) {
          this->seed_patches_by_distance_in_km(lat, lon, radius_, dis, day_on, day_off, this->mosquito_seeds);
        }
      } else {
        FRED_VERBOSE(0,"Attempting to seed infectious vectors %lg proportion in %lg proportion of houses, day_on %d day_off %d condition %d in all houses \n", mosquito_seeds,place_seeding_probability,day_on,day_off,dis);
        if(this->mosquito_seeds < 0) {
          this->mosquito_seeds = 0;
        }
        if((dis >= 0) && (dis<Global::MAX_NUM_CONDITIONS) && (day_on >= 0) && (day_off >= 0)) {
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
    Household* hh = Global::Places.get_household(i);
    add_hosts(hh);
  }
  if(Vector_Layer::Enable_Vector_Control){
    this->setup_vector_control_by_census_tract();
  }
}

void Vector_Layer::update(int day) {
  this->total_infected_vectors = 0;
  this->total_infected_hosts = 0;
  this->total_infectious_hosts = 0;
  if(Vector_Layer::Enable_Vector_Control){
    this->update_vector_control_by_census_tract(day);
  }
  FRED_VERBOSE(1,"Vector_Layer::update() entered on day %d\n", day);
  // Global::Daily_Tracker->log_key_value("Vec_I", total_infected_vectors);
  // Global::Daily_Tracker->log_key_value("Vec_H", total_infectious_hosts);
}

void Vector_Layer::update_visualization_data(int condition_id, int day) {
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
    Household* hh = Global::Places.get_household(i);
    int hh_county = hh->get_county_fips();
    for (int j = 0;j<county_set.size();j++){
      if(hh_county == county_set[j].id){
	household_county = j;
      }
    }
    //find the county for each household
    if(household_county == -1){
      Utils::fred_abort("No county code found for house in county %d\n", hh_county);
    }
    int house_mates = hh->get_size();
    // load every person in the house in a  county
    for(int j = 0; j<house_mates; ++j) {
      Person* p = hh->get_enrollee(j);
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
        int val = fscanf(fp,"%lg ",&temp_immune);
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
    Utils::fred_abort("Can't open prior_immune_file %s for condition %d \n", prior_immune_file, d);
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
	      per->become_unsusceptible(Global::Conditions.get_condition(d));
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
	    per->become_unsusceptible(Global::Conditions.get_condition(d));
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


double Vector_Layer::get_vectors_per_host(Place* place) {

  double development_time = 1.0;
  double vectors_per_host = 0.0;

  //temperatures vs development times..FOCKS2000: DENGUE TRANSMISSION THRESHOLDS
  double temps[8]= {15.0,20.0,22.0,24.0,26.0,28.0,30.0,32.0};  //temperatures
  double dev_times[8] =  {8.49,3.11,4.06,3.3,2.66,2.04,1.46,0.92};//development times

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
    for(int i = 0; i < 8; i++) {
      if(temperature <= temps[i]) {
	//obtain the development time using linear interpolation
	development_time = dev_times[i - 1] + (dev_times[i] - dev_times[i - 1]) / (temps[i] - temps[i-1]) *(temperature - temps[i - 1]);
	break;
      }
    }
    vectors_per_host = pupae_per_host * female_ratio * sucess_rate * life_span / development_time;
  }
  FRED_VERBOSE(1, "SET TEMP: place %s lat %lg lon %lg temp %f devtime %f vectors_per_host %f N_orig %d\n",
	       place->get_label(), place->get_latitude(), place->get_longitude(),temperature, development_time, vectors_per_host,place->get_orig_size());
  return vectors_per_host;
}

vector_condition_data_t Vector_Layer::update_vector_population(int day, Place * place) {

  place->mark_vectors_as_not_infected_today();

  vector_condition_data_t v = place->get_vector_condition_data();

  if(day > vector_control_day_off){
    place->stop_vector_control();
  }
  
  if(place->is_neighborhood()){
    return v;
  }
  
  if(v.N_vectors <= 0) {
    return v;
  }

  if(place->get_vector_control_status()){
    int N_host_orig = place->get_orig_size();
    v.N_vectors = N_host_orig * this->get_vectors_per_host(place) * (1 - vector_control_efficacy);
    if(v.N_vectors < 0){
      v.N_vectors = 0;
    }
    FRED_VERBOSE(1,"update vector pop::Vector_control day %d place %s  N_vectors: %d efficacy: %f\n",day,place->get_label(),v.N_vectors,vector_control_efficacy);
  }
  /*else{
    int N_host_orig = place->get_orig_size();
    v.N_vectors = N_host_orig * this->get_vectors_per_host(place);
    }*/
  FRED_VERBOSE(1,"update vector pop: day %d place %s initially: S %d, N: %d\n",
	       day,place->get_label(),v.S_vectors,v.N_vectors);
  
  int born_infectious[DISEASE_TYPES];
  int total_born_infectious = 0;
  int lifespan_ = 1/this->death_rate;

  // new vectors are born susceptible
  if(v.N_vectors < lifespan_){
    for(int k = 0; k < v.N_vectors; k++){
      double r = Random::draw_random(0,1);
      if(r < this->birth_rate){
	v.S_vectors++;
      }
    }
    for(int k = 0; k < v.S_vectors; k++){
      double r = Random::draw_random(0,1);
      if(r < this->death_rate){
	v.S_vectors--;
      }
    }
  }else{
    v.S_vectors += floor(this->birth_rate * v.N_vectors - this->death_rate * v.S_vectors);
  }
  FRED_VERBOSE(1,"vector_update_population:: S_vector: %d, N_vectors: %d\n", v.S_vectors, v.N_vectors);

  // but some are infected
  for(int d=0;d<DISEASE_TYPES;d++){
    double seeds = place->get_seeds(d, day);
    born_infectious[d] = ceil(v.S_vectors * seeds);
    total_born_infectious += born_infectious[d];
    if(born_infectious[d] > 0) {
      FRED_VERBOSE(1,"vector_update_population:: Vector born infectious condition[%d] = %d \n",d, born_infectious[d]);
      FRED_VERBOSE(1,"Total Vector born infectious: %d \n", total_born_infectious);
    }
  }

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
    if(v.E_vectors[i] < lifespan_ && v.E_vectors[i] > 0){
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
    if(v.E_vectors[i] < lifespan_) {
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
    if(v.I_vectors[i] < lifespan_ && v.I_vectors[i] > 0){
      for(int k = 0; k < v.I_vectors[i]; k++){
	double r = Random::draw_random(0,1);
	if(r < this->death_rate){
	  v.I_vectors[i]--;
	}
      }
    }else{
      v.I_vectors[i] -= floor(this->death_rate * v.I_vectors[i]);
    }
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


void Vector_Layer::get_vector_population(int condition_id){
  vector_pop = 0;
  total_infected_vectors = 0;
  total_susceptible_vectors = 0;
  school_vectors = 0;
  workplace_vectors = 0;
  household_vectors = 0;
  neighborhood_vectors = 0;
  school_infected_vectors = 0;
  workplace_infected_vectors = 0;
  household_infected_vectors = 0;
  neighborhood_infected_vectors = 0;
  schools_in_vector_control = 0;
  households_in_vector_control = 0;
  workplaces_in_vector_control = 0;
  neighborhoods_in_vector_control = 0;
  total_places_in_vector_control = 0;

  int places = Global::Places.get_number_of_households();
  for (int i = 0; i < places; i++) {
    Place *place = Global::Places.get_household(i);
    household_vectors += place->get_vector_population_size();
    household_infected_vectors += place->get_infected_vectors(condition_id);
    total_susceptible_vectors += place->get_susceptible_vectors();
    if(place->get_vector_control_status()){
      households_in_vector_control++;
    }
  }

  // skip neighborhoods?
  /*
  places = Global::Places.get_number_of_neighborhoods();
  for (int i = 0; i < places; i++) {
    Place *place = Global::Places.get_neighborhood(i);
    neighborhood_vectors += place->get_vector_population_size();
    neighborhood_infected_vectors += place->get_infected_vectors(condition_id);
    total_susceptible_vectors += place->get_susceptible_vectors();
    if(place->get_vector_control_status()){
      neighborhoods_in_vector_control++;
    }
  }
  */

  places = Global::Places.get_number_of_schools();
  for (int i = 0; i < places; i++) {
    Place *place = Global::Places.get_school(i);
    school_vectors += place->get_vector_population_size();
    school_infected_vectors += place->get_infected_vectors(condition_id);
    total_susceptible_vectors += place->get_susceptible_vectors();
    if(place->get_vector_control_status()){
      schools_in_vector_control++;
    }
  }

  places = Global::Places.get_number_of_workplaces();
  for (int i = 0; i < places; i++) {
    Place *place = Global::Places.get_workplace(i);
    workplace_vectors += place->get_vector_population_size();
    workplace_infected_vectors += place->get_infected_vectors(condition_id);
    total_susceptible_vectors += place->get_susceptible_vectors();
    if(place->get_vector_control_status()){
      workplaces_in_vector_control++;
    }
  }

  vector_pop = school_vectors + workplace_vectors + household_vectors + neighborhood_vectors;
  total_infected_vectors = school_infected_vectors + workplace_infected_vectors + household_infected_vectors + neighborhood_infected_vectors;

  total_places_in_vector_control = schools_in_vector_control + households_in_vector_control + workplaces_in_vector_control + neighborhoods_in_vector_control;

  assert(vector_pop == total_infected_vectors + total_susceptible_vectors);
}

void Vector_Layer::report(int day, Epidemic * epidemic) {
  get_vector_population(epidemic->get_id());
  epidemic->track_value(day,(char *)"Nv", vector_pop);
  epidemic->track_value(day,(char *)"Nvs", school_vectors);
  epidemic->track_value(day,(char *)"Nvw", workplace_vectors);
  epidemic->track_value(day,(char *)"Nvh", household_vectors);
  epidemic->track_value(day,(char *)"Nvn", neighborhood_vectors);
  epidemic->track_value(day,(char *)"Iv", total_infected_vectors);
  epidemic->track_value(day,(char *)"Ivs", school_infected_vectors);
  epidemic->track_value(day,(char *)"Ivw", workplace_infected_vectors);
  epidemic->track_value(day,(char *)"Ivh", household_infected_vectors);
  epidemic->track_value(day,(char *)"Ivn", neighborhood_infected_vectors);
  epidemic->track_value(day,(char *)"Sv", total_susceptible_vectors);
  if(Vector_Layer::Enable_Vector_Control){
    epidemic->track_value(day,(char *)"Pvc", total_places_in_vector_control);
    epidemic->track_value(day,(char *)"Svc", schools_in_vector_control);
    epidemic->track_value(day,(char *)"Hvc", households_in_vector_control);
    epidemic->track_value(day,(char *)"Wvc", workplaces_in_vector_control);
    epidemic->track_value(day,(char *)"Nvc", neighborhoods_in_vector_control);
  }
}

