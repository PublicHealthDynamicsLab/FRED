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
// File: Global.cc
//

#include "Global.h"
#include "Params.h"
#include "Demographics.h"
#include "Population.h"
#include "Condition_List.h"
#include "Place_List.h"

// global simulation variables
char Global::Simulation_directory[FRED_STRING_SIZE];
char Global::Plot_directory[FRED_STRING_SIZE];
char Global::Visualization_directory[FRED_STRING_SIZE];
int Global::Simulation_run_number = 1;
unsigned long Global::Simulation_seed = 1;
high_resolution_clock::time_point Global::Simulation_start_time = high_resolution_clock::now();
int Global::Simulation_Day = 0;

// global runtime parameters
char Global::Output_directory[FRED_STRING_SIZE];
char Global::Tracefilebase[FRED_STRING_SIZE];
int Global::Quality_control = 0;
int Global::RR_delay = 0;
int Global::Track_infection_events = 0;
char Global::Prevfilebase[FRED_STRING_SIZE];
char Global::Incfilebase[FRED_STRING_SIZE];
char Global::ErrorLogbase[FRED_STRING_SIZE];
bool Global::Track_age_distribution = false;
bool Global::Track_household_distribution = false;
bool Global::Track_network_stats = false;
bool Global::Report_Mean_Household_Income_Per_School = false;
bool Global::Report_Mean_Household_Size_Per_School = false;
bool Global::Report_Mean_Household_Distance_From_School = false;
bool Global::Report_Mean_Household_Stats_Per_Income_Category = false;
bool Global::Report_Epidemic_Data_By_County = false;
bool Global::Report_Epidemic_Data_By_Census_Tract = false;
int Global::Popsize_by_age[Demographics::MAX_AGE+1];

int Global::Verbose = 0;
int Global::Debug = 0;
int Global::Test = 0;
int Global::Days = 0;
int Global::Reseed_day = 0;
unsigned long Global::Seed = 0;
char Global::Start_date[FRED_STRING_SIZE];
char Global::Seasonality_Timestep[FRED_STRING_SIZE];
bool Global::Enable_Health_Records = false;
bool Global::Enable_Transmission_Network = false;
bool Global::Enable_Sexual_Partner_Network = false;
bool Global::Enable_Transmission_Bias = false;
bool Global::Enable_New_Transmission_Model = false;
bool Global::Enable_Hospitals = false;
bool Global::Enable_Health_Insurance = false;
bool Global::Enable_Group_Quarters = false;
bool Global::Enable_Visualization_Layer = false;
int Global::Visualization_Run = 1;
bool Global::Enable_Vector_Layer = false;
bool Global::Report_Vector_Population = false;
bool Global::Enable_Vector_Transmission = false;
bool Global::Enable_Population_Dynamics = false;
bool Global::Enable_Travel = false;
bool Global::Enable_Local_Workplace_Assignment = false;
bool Global::Enable_Seasonality = false;
bool Global::Enable_Climate = false;
bool Global::Use_Mean_Latitude = false;
bool Global::Print_Household_Locations = false;
int Global::Report_Age_Of_Infection = 0;
int Global::Age_Of_Infection_Log_Level = Global::LOG_LEVEL_MIN;
bool Global::Report_Place_Of_Infection = false;
bool Global::Report_Distance_Of_Infection = false;
bool Global::Report_Presenteeism = false;
bool Global::Report_Childhood_Presenteeism = false;
bool Global::Report_Serial_Interval = false;
bool Global::Report_Incidence_By_County = false;
bool Global::Report_Incidence_By_Census_Tract = false;
bool Global::Report_Symptomatic_Incidence_By_Census_Tract = false;
bool Global::Report_County_Demographic_Information = false;
bool Global::Assign_Teachers = false;
char Global::PSA_Method[FRED_STRING_SIZE];
char Global::PSA_List_File[FRED_STRING_SIZE];
int Global::PSA_Sample_Size = 0;
int Global::PSA_Sample = 0;

// global singleton objects
Population Global::Pop;
Condition_List Global::Conditions;
Place_List Global::Places;
Neighborhood_Layer* Global::Neighborhoods = NULL;
Regional_Layer* Global::Simulation_Region;
Visualization_Layer* Global::Visualization;
Vector_Layer* Global::Vectors;
Seasonality* Global::Clim = NULL;
Tracker<int>* Global::Daily_Tracker = NULL;
Tracker<long int>* Global::Tract_Tracker = NULL;
Tracker<int>* Global::Income_Category_Tracker = NULL;
Network* Global::Transmission_Network = NULL;
Sexual_Transmission_Network* Global::Sexual_Partner_Network = NULL;

// global file pointers
FILE* Global::Statusfp = NULL;
FILE* Global::Outfp = NULL;
FILE* Global::HealthRecordfp = NULL;
FILE* Global::Tracefp = NULL;
FILE* Global::Infectionfp = NULL;
FILE* Global::Birthfp = NULL;
FILE* Global::Deathfp = NULL;
FILE* Global::Prevfp = NULL;
FILE* Global::Incfp = NULL;
FILE* Global::ErrorLogfp = NULL;
FILE* Global::Householdfp = NULL;
FILE* Global::Tractfp = NULL;
FILE* Global::IncomeCatfp = NULL;

void Global::get_global_parameters() {
  Params::get_param("verbose", &Global::Verbose);
  Params::get_param("debug", &Global::Debug);
  Params::get_param("test", &Global::Test);
  Params::get_param("quality_control", &Global::Quality_control);
  Params::get_param("rr_delay", &Global::RR_delay);
  Params::get_param("days", &Global::Days);
  Params::get_param("seed", &Global::Seed);
  Params::get_param("start_date", Global::Start_date);
  Params::get_param("reseed_day", &Global::Reseed_day);
  Params::get_param("outdir", Global::Output_directory);
  // Params::get_param("tracefile", Global::Tracefilebase);
  // Params::get_param("track_infection_events", &Global::Track_infection_events);

  Params::get_param("seasonality_timestep_file", Global::Seasonality_Timestep);

  //Set all of the boolean flags
  int temp_int = 0;

  Params::get_param("track_age_distribution", &temp_int);
  Global::Track_age_distribution = (temp_int == 0 ? false : true);
  Params::get_param("track_household_distribution", &temp_int);
  Global::Track_household_distribution = (temp_int == 0 ? false : true);
  Params::get_param("track_network_stats", &temp_int);
  Global::Track_network_stats = (temp_int == 0 ? false : true);
  Params::get_param("report_mean_household_income_per_school", &temp_int);
  Global::Report_Mean_Household_Income_Per_School = (temp_int == 0 ? false : true);
  Params::get_param("report_mean_household_size_per_school", &temp_int);
  Global::Report_Mean_Household_Size_Per_School = (temp_int == 0 ? false : true);
  Params::get_param("report_mean_household_distance_from_school", &temp_int);
  Global::Report_Mean_Household_Distance_From_School = (temp_int == 0 ? false : true);
  Params::get_param("enable_health_records", &temp_int);
  Global::Enable_Health_Records = (temp_int == 0 ? false : true);
  Params::get_param("enable_transmission_network", &temp_int);
  Global::Enable_Transmission_Network = (temp_int == 0 ? false : true);
  Params::get_param("enable_sexual_partner_network", &temp_int);
  Global::Enable_Sexual_Partner_Network = (temp_int == 0 ? false : true);  
  Params::get_param("enable_transmission_bias", &temp_int);
  Global::Enable_Transmission_Bias = (temp_int == 0 ? false : true);
  Params::get_param("enable_new_transmission_model", &temp_int);
  Global::Enable_New_Transmission_Model = (temp_int == 0 ? false : true);
  Params::get_param("report_mean_household_stats_per_income_category", &temp_int);
  Global::Report_Mean_Household_Stats_Per_Income_Category = (temp_int == 0 ? false : true);
  Params::get_param("report_epidemic_data_by_census_tract", &temp_int);
  Global::Report_Epidemic_Data_By_Census_Tract = (temp_int == 0 ? false : true);
  Params::get_param("report_epidemic_data_by_county", &temp_int);
  Global::Report_Epidemic_Data_By_County = (temp_int == 0 ? false : true);
  Params::get_param("enable_hospitals", &temp_int);
  Global::Enable_Hospitals = (temp_int == 0 ? false : true);
  Params::get_param("enable_health_insurance", &temp_int);
  Global::Enable_Health_Insurance = (temp_int == 0 ? false : true);
  Params::get_param("enable_group_quarters", &temp_int);
  Global::Enable_Group_Quarters = (temp_int == 0 ? false : true);
  Params::get_param("enable_visualization_layer", &temp_int);
  Global::Enable_Visualization_Layer = (temp_int == 0 ? false : true);
  Params::get_param("enable_vector_layer", &temp_int);
  Global::Enable_Vector_Layer = (temp_int == 0 ? false : true);
  Params::get_param("enable_vector_transmission", &temp_int);
  Global::Enable_Vector_Transmission = (temp_int == 0 ? false : true);
  Params::get_param("report_vector_population", &temp_int);
  Global::Report_Vector_Population = (temp_int == 0 ? false : true);
  Params::get_param("enable_population_dynamics", &temp_int);
  Global::Enable_Population_Dynamics = (temp_int == 0 ? false : true);
  Params::get_param("enable_travel",&temp_int);
  Global::Enable_Travel = (temp_int == 0 ? false : true);
  Params::get_param("enable_local_workplace_assignment", &temp_int);
  Global::Enable_Local_Workplace_Assignment = temp_int;
  Params::get_param("enable_seasonality", &temp_int);
  Global::Enable_Seasonality = (temp_int == 0 ? false : true);
  Params::get_param("enable_climate", &temp_int);
  Global::Enable_Climate = (temp_int == 0 ? false : true);
  Params::get_param("use_mean_latitude", &temp_int);
  Global::Use_Mean_Latitude = (temp_int == 0 ? false : true);
  Params::get_param("print_household_locations", &temp_int);
  Global::Print_Household_Locations = (temp_int == 0 ? false : true);
  Params::get_param("assign_teachers", &temp_int);
  Global::Assign_Teachers = (temp_int == 0 ? false : true);

  Params::get_param("visualization_run", &Global::Visualization_Run);
  Params::get_param("report_age_of_infection", &Global::Report_Age_Of_Infection);
  Params::get_param("age_of_infection_log_level", &Global::Age_Of_Infection_Log_Level);
  Params::get_param("report_place_of_infection", &temp_int);
  Global::Report_Place_Of_Infection = (temp_int == 0 ? false : true);
  Params::get_param("report_distance_of_infection", &temp_int);
  Global::Report_Distance_Of_Infection = (temp_int == 0 ? false : true);
  Params::get_param("report_presenteeism", &temp_int);
  Global::Report_Presenteeism = (temp_int == 0 ? false : true);
  Params::get_param("report_childhood_presenteeism", &temp_int);
  Global::Report_Childhood_Presenteeism = (temp_int == 0 ? false : true);
  Params::get_param("report_serial_interval", &temp_int);
  Global::Report_Serial_Interval = (temp_int == 0 ? false : true);
  Params::get_param("report_incidence_by_county", &temp_int);
  Global::Report_Incidence_By_County = (temp_int == 0 ? false : true);
  Params::get_param("report_incidence_by_census_tract", &temp_int);
  Global::Report_Incidence_By_Census_Tract = (temp_int == 0 ? false : true);
  Params::get_param("report_symptomatic_incidence_by_census_tract", &temp_int);
  Global::Report_Symptomatic_Incidence_By_Census_Tract = (temp_int == 0 ? false : true);
  Params::get_param("report_county_demographic_information", &temp_int);
  Global::Report_County_Demographic_Information = (temp_int == 0 ? false : true);

  // set any parameters that are dependent on other parameters
  if (Global::Visualization_Run != -1 &&
      Global::Simulation_run_number != Global::Visualization_Run) {
    Global::Enable_Visualization_Layer = false;
  }

}

