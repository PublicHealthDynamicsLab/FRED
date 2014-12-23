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
#include "Visualization_Layer.h"
#include "Regional_Layer.h"
#include "Regional_Patch.h"
#include "Params.h"
#include "Random.h"
#include "Place.h"
#include "Place_List.h"
#include "Household.h"
#include "Tracker.h"
typedef vector <Person*> pvec;//Vector of person pointers
typedef struct county_record {
  int id;
  pvec habitants;
  double immunity_by_age [102];
  int pop;
  int people_by_age[102];
  int people_immunized;
} county_record_t;

vector < county_record_t > county_set;

Vector_Layer::Vector_Layer() {
  Regional_Layer * base_grid = Global::Simulation_Region;
  min_lat = base_grid->get_min_lat();
  min_lon = base_grid->get_min_lon();
  max_lat = base_grid->get_max_lat();
  max_lon = base_grid->get_max_lon();
  min_x = base_grid->get_min_x();
  min_y = base_grid->get_min_y();
  max_x = base_grid->get_max_x();
  max_y = base_grid->get_max_y();
  // determine patch size for this layer
  Params::get_param_from_string("vector_patch_size", &patch_size);
  // Get probabilities of transmission 
  Params::get_param_from_string("vector_infection_efficiency", &infection_efficiency);
  Params::get_param_from_string("vector_transmission_efficiency", &transmission_efficiency);
  Params::get_param_from_string("place_seeding_probability", &place_seeding_probability);
  Params::get_param_from_string("mosquito_seeds", &mosquito_seeds);
  // determine number of rows and cols
  rows = (double)(max_y - min_y) / patch_size;
  if (min_y + rows * patch_size < max_y) rows++;

  cols = (double)(max_x - min_x) / patch_size;
  if (min_x + cols * patch_size < max_x) cols++;

  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "Vector_Layer min_lon = %f\n", min_lon);
    fprintf(Global::Statusfp, "Vector_Layer min_lat = %f\n", min_lat);
    fprintf(Global::Statusfp, "Vector_Layer max_lon = %f\n", max_lon);
    fprintf(Global::Statusfp, "Vector_Layer max_lat = %f\n", max_lat);
    fprintf(Global::Statusfp, "Vector_Layer rows = %d  cols = %d\n",rows,cols);
    fprintf(Global::Statusfp, "Vector_Layer min_x = %f  min_y = %f\n",min_x,min_y);
    fprintf(Global::Statusfp, "Vector_Layer max_x = %f  max_y = %f\n",max_x,max_y);
    fflush(Global::Statusfp);
  }

  grid = new Vector_Patch * [rows];
  for (int i = 0; i < rows; i++) {
    grid[i] = new Vector_Patch[cols];
  }

  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      grid[i][j].setup(i, j, patch_size, min_x, min_y,transmission_efficiency,infection_efficiency);
    }      
  }
  // To read the temperature grid
  this-> read_temperature();
 // Read location where a proportion of mosquitoes susceptibles are infected externaly (birth or migration)
  this-> read_vector_seeds();
}
void Vector_Layer::swap_county_people(){
  int cols = Global::Simulation_Region->get_cols();
  int rows = Global::Simulation_Region->get_rows();
  for(int i = 0;i < rows;i++){
    for(int j = 0;j<cols;j++){
      Regional_Patch * region_patch = Global::Simulation_Region->get_patch(i,j);
      int pop_size = region_patch ->get_popsize();
      if (pop_size > 0)
      region_patch -> swap_county_people();
    }
  }
}
void Vector_Layer::read_temperature(){
  double  patch_temperature;
  double lati;
  double longi;
  fred::geo lat;
  fred::geo lon;
  FILE *fp;
  char filename[FRED_STRING_SIZE];
  strcpy(filename, "$FRED_HOME/input_files/countries/colombia/temperature_grid.txt");
  fp = Utils::fred_open_file(filename);
  //Obtain temperature values for each patch...lat,lon,oC
  if (fp !=NULL)
    {
      while(!feof(fp))
	{
	  fscanf(fp,"%lg, %lg, %lg",&lati,&longi,&patch_temperature);
	  lat = lati;
	  lon = longi;
	  Vector_Patch * patch = get_patch(lat,lon);
	  if (patch != NULL){
	    patch->set_temperature(patch_temperature);
	  }
	}
    }else{
    Utils::fred_abort("Cannot  open %s to read the average temperature grid \n", filename);
  }
  fclose(fp);
}
void Vector_Layer::seed_patches_by_distance_in_km(fred::geo lat, fred::geo lon,
						  double radius_in_km, int dis,int day_on, int day_off,double seeds_) {
  //ASSUMMING WE ARE IN THE TROPIC 1/120 degree ~ 1 km
  //  printf("SEED_PATCHES_BY_DISTANCE entered\n");
  int kilometers_per_degree = 120;
  int number_of_patches = (int) radius_in_km / this->patch_size;
  //find the patch of the middle point
  Vector_Patch * patch = get_patch(lat, lon);
  if(patch!=NULL){
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
    
    for(int r = r1; r <= r2; r++) {
      for(int c = c1; c <= c2; c++) {
	Vector_Patch * p = get_patch(r, c);
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
  FILE *fp;
  Params::get_param_from_string("vector_seeds_file", filename);
  fp = Utils::fred_open_file(filename);
  //  printf("seeds filename: %s\n", filename);
  if (fp != NULL) {
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
      if(radius_>0){
	FRED_VERBOSE(0,"Attempting to seed infectious vectors %lg proportion in %lg proportion of houses, day_on %d day_off %d disease %d lat %lg lon %lg radius %lg \n", mosquito_seeds,place_seeding_probability,day_on,day_off,dis,lat_,lon_,radius_);
	//make a list of houses in the radius 
	fred::geo lat = lat_;
	fred::geo lon = lon_;
	if(mosquito_seeds<0) mosquito_seeds = 0;
	if((dis >= 0) && (dis<Global::MAX_NUM_DISEASES) && (day_on >= 0) && (day_off >= 0) ){
	  this->seed_patches_by_distance_in_km(lat,lon,radius_,dis,day_on,day_off,mosquito_seeds);
	}
      }else{
	FRED_VERBOSE(0,"Attempting to seed infectious vectors %lg proportion in %lg proportion of houses, day_on %d day_off %d disease %d in all houses \n", mosquito_seeds,place_seeding_probability,day_on,day_off,dis);
	if(mosquito_seeds<0) mosquito_seeds = 0;
	if((dis >= 0) && (dis<Global::MAX_NUM_DISEASES) && (day_on >= 0) && (day_off >= 0) ){
	  for (int i = 0; i < rows; i++) {
	    for (int j = 0; j < cols; j++) {
	      grid[i][j].set_vector_seeds(dis,day_on,day_off,mosquito_seeds);
	    }      
	  }
	}
      }
    }
  }
  fclose(fp);
}

Vector_Patch * Vector_Layer::get_patch(int row, int col) {
  if ( row >= 0 && col >= 0 && row < rows && col < cols)
    return &grid[row][col];
  else
    return NULL;
}

Vector_Patch * Vector_Layer::get_patch(fred::geo lat, fred::geo lon) {
  int row = get_row(lat);
  int col = get_col(lon);
  return get_patch(row,col);
}

Vector_Patch * Vector_Layer::select_random_patch() {
  int row = IRAND(0, rows-1);
  int col = IRAND(0, cols-1);
  return &grid[row][col];
}

void Vector_Layer::quality_control() {
  if (Global::Verbose) {
    fprintf(Global::Statusfp, "vector grid quality control check\n");
    fflush(Global::Statusfp);
  }
  
  for (int row = 0; row < rows; row++) {
    for (int col = 0; col < cols; col++) {
      grid[row][col].quality_control();
    }
  }
  
  if (Global::Verbose) {
    fprintf(Global::Statusfp, "vector grid quality control finished\n");
    fflush(Global::Statusfp);
  }
}

void Vector_Layer::initialize() {
  return;
}

void Vector_Layer::update(int day) {
  total_infected_vectors = 0;
  total_infected_hosts = 0;
  total_infectious_hosts = 0;
  FRED_VERBOSE(1,"Vector_Layer::update() entered on day %d\n", day);
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      Vector_Patch * patch = (Vector_Patch *) &grid[i][j];
      patch->update(day);
      // total_infected_vectors += patch->get_infected_vectors();
      // total_infected_hosts += patch->get_infected_hosts();
      // total_infectious_hosts += patch->get_infectious_hosts();
    }
  }
  // Global::Daily_Tracker->log_key_value("Vec_I", total_infected_vectors);
  // Global::Daily_Tracker->log_key_value("Vec_H", total_infectious_hosts);

  /*
  FILE *fp;
  char filename[80];
  sprintf(filename, "vec-%d.txt", day);
  fp = fopen(filename,"w");
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      Vector_Patch * patch = (Vector_Patch *) &grid[i][j];
      int popsize = patch->get_popsize();
      int infected = patch->get_infected();
      int count = patch->get_infectious_hosts();
      fprintf(fp,"i %d j %d popsize %d infected %d inf_hosts %d\n",i,j,popsize,infected,count);
    }
  }
  fclose(fp);
  */
}

void Vector_Layer::update_visualization_data(int disease_id, int day) {
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      Vector_Patch * patch = (Vector_Patch *) &grid[i][j];
      int count = patch->get_infected_vectors();
      if (count > 0) {
	double x = patch->get_center_x();
	double y = patch->get_center_y();
	Global::Visualization->update_data(x,y,count,0);
      }
    }
  }
}

void Vector_Layer::add_hosts(Place * p) {
  fred::geo lat = p->get_latitude();
  fred::geo lon = p->get_longitude();
  int hosts = p->get_size();
  Vector_Patch * patch = get_patch(lat,lon);
  if (patch != NULL) {
    patch->add_hosts(hosts);
  }
}

double Vector_Layer::get_temperature(Place * p) {
  fred::geo lat = p->get_latitude();
  fred::geo lon = p->get_longitude();
  Vector_Patch * patch = get_patch(lat,lon);
  if (patch != NULL) {
    return patch->get_temperature();
  }
  else {
    return -999.9;
  }
}

double Vector_Layer::get_seeds(Place * p, int dis, int day) {
  fred::geo lat = p->get_latitude();
  fred::geo lon = p->get_longitude();
  Vector_Patch * patch = get_patch(lat,lon);
  if (patch != NULL) {
    return patch->get_seeds(dis, day);
  }
  else {
    return 0.0;
  }
}

double Vector_Layer::get_seeds(Place * p, int dis) {
  fred::geo lat = p->get_latitude();
  fred::geo lon = p->get_longitude();
  Vector_Patch * patch = get_patch(lat,lon);
  if (patch != NULL) {
    double seeds_ = patch->get_seeds(dis);
    if (seeds_ > 0){
      if(URAND(0,1) < place_seeding_probability){
	return seeds_;    
      }else{
	return 0.0;
      } 
    }else{
      return 0.0;
    }
  }else {
    return 0.0;
  }
}

double Vector_Layer::get_day_start_seed(Place * p, int dis) {
  fred::geo lat = p->get_latitude();
  fred::geo lon = p->get_longitude();
  Vector_Patch * patch = get_patch(lat,lon);
  if (patch != NULL) {
    return patch->get_day_start_seed(dis);
  }
  else {
    return 0;
  }
}

double Vector_Layer::get_day_end_seed(Place * p, int dis) {
  fred::geo lat = p->get_latitude();
  fred::geo lon = p->get_longitude();
  Vector_Patch * patch = get_patch(lat,lon);
  if (patch != NULL) {
    return patch->get_day_end_seed(dis);
  }
  else {
    return 0;
  }
}

void Vector_Layer::add_host(Person * person, Place * place) {
  if (place->is_neighborhood()) return;
  fred::geo lat = place->get_latitude();
  fred::geo lon = place->get_longitude();
  Vector_Patch * patch = get_patch(lat,lon);
  if (patch != NULL) {
    patch->add_host(person);
  }
}
void Vector_Layer::get_county_ids(){
  // get the county ids from external file
  char county_codes_file[FRED_STRING_SIZE];
  int num_households = Global::Places.get_number_of_households();
  county_record_t county_temp;
  Params::get_param_from_string("county_codes_file", county_codes_file);
  FILE * fp = Utils::fred_open_file(county_codes_file);
  if (fp == NULL) { Utils::fred_abort("Can't open county_codes_file %s\n", county_codes_file); }
  county_set.clear();
  county_temp.habitants.clear();
  while (fscanf(fp, "%d ", &county_temp.id) == 1){
    county_set.push_back(county_temp);
  }
  fclose(fp);
  for (int i = 0;i<county_set.size();i++){
    FRED_VERBOSE(2,"County record set for county id:  %d \n",county_set[i].id);
  }
  for(int i=0;i<num_households;i++){
    int household_county = -1;
    Household * h = Global::Places.get_household_ptr(i);
    int c = h->get_county();
    int h_county = Global::Places.get_county_with_index(c);
    for (int j = 0;j<county_set.size();j++){
      if(h_county == county_set[j].id){
	household_county = j;
      }
    }
    //find the county for each household
    if (household_county == -1){
      Utils::fred_abort("No county code found for house in county %d\n", h_county);
    }
    int house_mates = h->get_size();
    // load every person in the house in a  county
    for (int j = 0; j<house_mates; j++){
      Person * p = h->get_housemate(j);
      county_set[household_county].habitants.push_back(p);
    }
  }
  FRED_VERBOSE(0,"get_county_ids finished\n");
}
void Vector_Layer::get_immunity_from_file(){
  FILE * fp;
  // get the prior immune proportion by age  from external file for each county for total immunity
  char prior_immune_file[FRED_STRING_SIZE];
  Params::get_param_from_string("prior_immune_file", prior_immune_file);
  fp = Utils::fred_open_file(prior_immune_file);
  if (fp == NULL) { Utils::fred_abort("Can't open prior_immune_file %s\n", prior_immune_file); }
  while(!feof(fp)){
    int temp_county;
    int index_county = -1;
    double temp_immune ;
    if (fscanf(fp,"%d ", &temp_county)==1){
      for (int i = 0; i < county_set.size(); i++){
	if (county_set[i].id == temp_county){
	  index_county = i;
	}
      }
      if (index_county == -1){
	Utils::fred_abort("No county found %d\n",temp_county);
      }
      FRED_VERBOSE(2,"County code  %d\n",temp_county);
      for (int i=0;i<102;i++){
	fscanf(fp,"%lg ",&temp_immune);
	county_set[index_county].immunity_by_age[i] = temp_immune;
	FRED_VERBOSE(2,"Age: %d Immunity  %lg\n",i,temp_immune);
      }
    }
  }
  fclose(fp);
}
void Vector_Layer::get_immunity_from_file(int d){
  FILE * fp;
  // get the prior immune proportion by age  from external file for each county for total immunity
  char prior_immune_file[FRED_STRING_SIZE];
  char immune_param_string[FRED_STRING_SIZE];
  sprintf(immune_param_string,"prior_immune_file[%d]",d);
  Params::get_param_from_string(immune_param_string, prior_immune_file);
  fp = Utils::fred_open_file(prior_immune_file);
  if (fp == NULL) { Utils::fred_abort("Can't open prior_immune_file %s for disease %d \n", prior_immune_file, d); }
  while(!feof(fp)){
    int temp_county;
    int index_county = -1;
    double temp_immune ;
    if (fscanf(fp,"%d ", &temp_county)==1){
      for (int i = 0; i < county_set.size(); i++){
	if (county_set[i].id == temp_county){
	  index_county = i;
	}
      }
      if (index_county == -1){
	Utils::fred_abort("No county found %d\n",temp_county);
      }
      FRED_VERBOSE(2,"County code  %d\n",temp_county);
      for (int i=0;i<102;i++){
	if(fscanf(fp,"%lg ",&temp_immune)>0){
	  county_set[index_county].immunity_by_age[i] = temp_immune;
	  FRED_VERBOSE(2,"Age: %d Immunity  %lg\n",i,temp_immune);
	}
      }
    }
  }
  fclose(fp);
}
void Vector_Layer::get_people_size_by_age(){
 //calculate number of people by age
  for(int i=0;i<county_set.size();i++){
    if (county_set[i].habitants.size()>0){
      for (int k = 0;k < 102;k++){
	county_set[i].people_by_age[k] = 0;
      }
      for (int j = 0; j < county_set[i].habitants.size();j++){
	Person * per = county_set[i].habitants[j];
	int temp_age = per->get_age();
	if (temp_age > 101) temp_age = 101;
	county_set[i].people_by_age[temp_age]++;
      }
    }
  }
}
void Vector_Layer::immunize_total_by_age(){
  for(int i=0;i<county_set.size();i++){
      county_set[i].people_immunized = 0;
    if (county_set[i].habitants.size()>0){
      for (int j = 0; j < county_set[i].habitants.size();j++){
	Person * per = county_set[i].habitants[j];
	double prob_immune_ = URAND(0,1);
	double prob_immune = prob_immune_ * 100;
	int temp_age = per->get_age();
	if (temp_age > 101) temp_age = 101;
	double prob_by_age = county_set[i].immunity_by_age[temp_age];
	if (prob_by_age > prob_immune){
	  for (int d = 0;d < DISEASE_TYPES; d++){
	    if(per->is_susceptible(d)){
	      per->become_unsusceptible(Global::Pop.get_disease(d));
	    }
	  }
	  county_set[i].people_immunized++;
	}
      }
    }
  }
}
void Vector_Layer::immunize_by_age(int d){
  for(int i=0;i<county_set.size();i++){
      county_set[i].people_immunized = 0;
    if (county_set[i].habitants.size()>0){
      for (int j = 0; j < county_set[i].habitants.size();j++){
	Person * per = county_set[i].habitants[j];
	double prob_immune_ = URAND(0,1);
	double prob_immune = prob_immune_ * 100;
	int temp_age = per->get_age();
	if (temp_age > 101) temp_age = 101;
	double prob_by_age = county_set[i].immunity_by_age[temp_age];
	if (prob_by_age > prob_immune){
	  if(per->is_susceptible(d)){
	    per->become_unsusceptible(Global::Pop.get_disease(d));
	    county_set[i].people_immunized++;
	  }
	}
      }
    }
  }
}
void Vector_Layer::init_prior_immunity_by_county() {
  FILE *fp;
  this->get_county_ids();
  this->get_immunity_from_file();
  this->get_people_size_by_age();
  this->immunize_total_by_age();
  FRED_VERBOSE(0,"Number of counties %d\n",county_set.size()); 
  for (int i = 0;i<county_set.size();i++){
    if (county_set[i].habitants.size()>0){
      FRED_VERBOSE(0,"County id:  %d Population %d People Immunized: %d\n",county_set[i].id, county_set[i].habitants.size(), county_set[i].people_immunized);
      /*      for (int j = 0;j<102;j++){
	FRED_VERBOSE(0,"AGE::  %d \t immune prob %lg\t people_by_age %d\n", j,county_set[i].immunity_by_age[j], county_set[i].people_by_age[j]);
	}*/
    }
  }
  //  Utils::fred_abort("Running test finishes here\n");
}
void Vector_Layer::init_prior_immunity_by_county(int d) {
  FILE *fp;
  this->get_county_ids();
  this->get_immunity_from_file(d);
  FRED_VERBOSE(2,"Number of counties %d\n",county_set.size()); 
  this->get_people_size_by_age();
  this->immunize_by_age(d);
  for (int i = 0;i<county_set.size();i++){
    if (county_set[i].habitants.size()>0){
      FRED_VERBOSE(0,"County id:  %d Population %d People Immunized: %d Strain %d\n",county_set[i].id, county_set[i].habitants.size(), county_set[i].people_immunized,d);
      /*      for (int j = 0;j<102;j++){
	FRED_VERBOSE(0,"AGE::  %d \t immune prob %lg\t people_by_age %d\n", j,county_set[i].immunity_by_age[j], county_set[i].people_by_age[j]);
	}*/
    }
  }
}
