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
// File: Global.cc
//

#include "Global.h"
#include "Demographics.h"
#include "Params.h"
#include "Population.h"
#include "Place_List.h"
#include "Neighborhood_Layer.h"
#include "Regional_Layer.h"
#include "Visualization_Layer.h"
#include "Vector_Layer.h"
#include "Seasonality.h"
#include "Utils.h"
#if SQLITE
#include "DB.h"
#endif
// global simulation variables
char Global::Simulation_directory[FRED_STRING_SIZE];
int Global::Simulation_run_number = 1;
unsigned long Global::Simulation_seed = 1;
time_t Global::Simulation_start_time = 0;

// global runtime parameters
char Global::Synthetic_population_directory[FRED_STRING_SIZE];
char Global::Synthetic_population_id[FRED_STRING_SIZE];
char Global::Synthetic_population_version[FRED_STRING_SIZE];
char Global::Output_directory[FRED_STRING_SIZE];
char Global::Tracefilebase[FRED_STRING_SIZE];
char Global::VaccineTracefilebase[FRED_STRING_SIZE];
int Global::Trace_Headers = 0;
int Global::Rotate_start_date = 0;
int Global::Quality_control = 0;
int Global::RR_delay = 0;
int Global::Diseases = 0;
int Global::StrainEvolution = 0;
int Global::Track_infection_events = 0;
char Global::Prevfilebase[FRED_STRING_SIZE];
char Global::Incfilebase[FRED_STRING_SIZE];
char Global::Immunityfilebase[FRED_STRING_SIZE];
char Global::City[FRED_STRING_SIZE];
char Global::County[FRED_STRING_SIZE];
char Global::US_state[FRED_STRING_SIZE];
char Global::FIPS_code[FRED_STRING_SIZE];
//added for cbsa
char Global::MSA_code[FRED_STRING_SIZE];
#if SQLITE
char Global::DBfile[FRED_STRING_SIZE];
#endif

char Global::ErrorLogbase[FRED_STRING_SIZE];
bool Global::Enable_Behaviors = false;
bool Global::Track_age_distribution = false;
bool Global::Track_household_distribution = false;
bool Global::Track_network_stats = false;
bool Global::Track_Residual_Immunity = false;
bool Global::Report_Mean_Household_Income_Per_School = false;
bool Global::Report_Mean_Household_Size_Per_School = false;
bool Global::Report_Mean_Household_Distance_From_School = false;
bool Global::Report_Mean_Household_Stats_Per_Income_Category = false;
bool Global::Report_Epidemic_Data_By_Census_Tract = false;
int Global::Popsize_by_age[Demographics::MAX_AGE+1];

int Global::Verbose = 0;
int Global::Debug = 0;
int Global::Test = 0;
int Global::Days = 0;
int Global::Reseed_day = 0;
unsigned long Global::Seed = 0;
int Global::Epidemic_offset = 0;
int Global::Vaccine_offset = 0;
char Global::Start_date[FRED_STRING_SIZE];
char Global::Seasonality_Timestep[FRED_STRING_SIZE];
double Global::Work_absenteeism = 0.0;
double Global::School_absenteeism = 0.0;
bool Global::Enable_New_Transmission_Model = false;
bool Global::Visit_Home_Neighborhood_Unless_Infectious = false;
bool Global::Enable_Hospitals = false;
bool Global::Enable_Health_Insurance = false;
bool Global::Enable_Group_Quarters = false;
bool Global::Enable_Visualization_Layer = false;
bool Global::Enable_Vector_Layer = false;
bool Global::Enable_Vector_Transmission = false;
bool Global::Enable_Population_Dynamics = false;
bool Global::Enable_Travel = false;
bool Global::Enable_Local_Workplace_Assignment = false;
bool Global::Enable_Seasonality = false;
bool Global::Enable_Climate = false;
bool Global::Enable_Chronic_Condition = false;
bool Global::Seed_by_age = false;
int Global::Seed_age_lower_bound = 0;
int Global::Seed_age_upper_bound = 0;
bool Global::Enable_Antivirals = true;
bool Global::Enable_Vaccination = true;
bool Global::Use_Mean_Latitude = false;
bool Global::Print_Household_Locations = false;
int Global::Report_Age_Of_Infection = 0;
int Global::Age_Of_Infection_Log_Level = Global::LOG_LEVEL_MIN;
bool Global::Report_Place_Of_Infection = false;
bool Global::Report_Distance_Of_Infection = false;
bool Global::Report_Presenteeism = false;
bool Global::Report_Serial_Interval = false;
bool Global::Report_Incidence_By_County = false;
bool Global::Report_Incidence_By_Census_Tract = false;
bool Global::Assign_Teachers = false;
bool Global::Enable_Household_Shelter = 0;
bool Global::Enable_Isolation = 0;
int Global::Isolation_Delay = 1;
double Global::Isolation_Rate = 0.0;
char Global::PSA_Method[FRED_STRING_SIZE];
char Global::PSA_List_File[FRED_STRING_SIZE];
int Global::PSA_Sample_Size = 0;
int Global::PSA_Sample = 0;

// per-strain immunity reporting off by default
// will be enabled in Utils::fred_open_output_files (called from Fred.cc)
// if valid files are given in params
bool Global::Report_Immunity = false;

// global singleton objects
Population Global::Pop;
Place_List Global::Places;
Neighborhood_Layer* Global::Neighborhoods = NULL;
Regional_Layer* Global::Simulation_Region;
Visualization_Layer* Global::Visualization;
Vector_Layer* Global::Vectors;
Date* Global::Sim_Start_Date = NULL;
Date* Global::Sim_Current_Date = NULL;
Evolution* Global::Evol = NULL;
Seasonality* Global::Clim = NULL;
Tracker<int>* Global::Daily_Tracker = NULL;
Tracker<long int>* Global::Tract_Tracker = NULL;
Tracker<int>* Global::Income_Category_Tracker = NULL;

#if SQLITE
DB Global::db;
#endif

// global file pointers
FILE* Global::Statusfp = NULL;
FILE* Global::Outfp = NULL;
FILE* Global::Tracefp = NULL;
FILE* Global::Infectionfp = NULL;
FILE* Global::VaccineTracefp = NULL;
FILE* Global::Birthfp = NULL;
FILE* Global::Deathfp = NULL;
FILE* Global::Prevfp = NULL;
FILE* Global::Incfp = NULL;
FILE* Global::ErrorLogfp = NULL;
FILE* Global::Immunityfp = NULL;
FILE* Global::Householdfp = NULL;
FILE* Global::Tractfp = NULL;
FILE* Global::IncomeCatfp = NULL;

void Global::get_global_parameters() {
  Params::get_param_from_string("verbose", &Global::Verbose);
  Params::get_param_from_string("debug", &Global::Debug);
  Params::get_param_from_string("test", &Global::Test);
  Params::get_param_from_string("quality_control", &Global::Quality_control);
  Params::get_param_from_string("rr_delay", &Global::RR_delay);
  Params::get_param_from_string("days", &Global::Days);
  Params::get_param_from_string("seed", &Global::Seed);
  Params::get_param_from_string("epidemic_offset", &Global::Epidemic_offset);
  Params::get_param_from_string("vaccine_offset", &Global::Vaccine_offset);
  Params::get_param_from_string("start_date", Global::Start_date);
  Params::get_param_from_string("rotate_start_date", &Global::Rotate_start_date);
  Params::get_param_from_string("reseed_day", &Global::Reseed_day);
  Params::get_param_from_string("outdir", Global::Output_directory);
  Params::get_param_from_string("tracefile", Global::Tracefilebase);
  Params::get_param_from_string("track_infection_events", &Global::Track_infection_events);

  Params::get_param_from_string("vaccine_tracefile", Global::VaccineTracefilebase);
  Params::get_param_from_string("trace_headers", &Global::Trace_Headers);
  Params::get_param_from_string("diseases", &Global::Diseases);
  Params::get_param_from_string("immunity_file", Global::Immunityfilebase);
  Params::get_param_from_string("seasonality_timestep_file", Global::Seasonality_Timestep);
  Params::get_param_from_string("work_absenteeism", &Global::Work_absenteeism);
  Params::get_param_from_string("school_absenteeism", &Global::School_absenteeism);
  Params::get_param_from_string("seed_age_lower_bound", &Global::Seed_age_lower_bound);
  Params::get_param_from_string("seed_age_upper_bound", &Global::Seed_age_upper_bound);

#if SQLITE
  // this is the default, global sqlite3 database file.  Overwritten by default.
  Params::get_param_from_string("dbfile", Global::DBfile);
#endif

  //Set all of the boolean flags
  int temp_int = 0;

  Params::get_param_from_string("enable_behaviors", &temp_int);
  Global::Enable_Behaviors = (temp_int == 0 ? false : true);
  Params::get_param_from_string("track_age_distribution", &temp_int);
  Global::Track_age_distribution = (temp_int == 0 ? false : true);
  Params::get_param_from_string("track_household_distribution", &temp_int);
  Global::Track_household_distribution = (temp_int == 0 ? false : true);
  Params::get_param_from_string("track_network_stats", &temp_int);
  Global::Track_network_stats = (temp_int == 0 ? false : true);
  Params::get_param_from_string("track_residual_immunity", &temp_int);
  Global::Track_Residual_Immunity = (temp_int == 0 ? false : true);
  Params::get_param_from_string("report_mean_household_income_per_school", &temp_int);
  Global::Report_Mean_Household_Income_Per_School = (temp_int == 0 ? false : true);
  Params::get_param_from_string("report_mean_household_size_per_school", &temp_int);
  Global::Report_Mean_Household_Size_Per_School = (temp_int == 0 ? false : true);
  Params::get_param_from_string("report_mean_household_distance_from_school", &temp_int);
  Global::Report_Mean_Household_Distance_From_School = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_new_transmission_model", &temp_int);
  Global::Enable_New_Transmission_Model = (temp_int == 0 ? false : true);
  Params::get_param_from_string("report_mean_household_stats_per_income_category", &temp_int);
  Global::Report_Mean_Household_Stats_Per_Income_Category = (temp_int == 0 ? false : true);
  Params::get_param_from_string("visit_home_neighborhood_unless_infectious", &temp_int);
  Global::Visit_Home_Neighborhood_Unless_Infectious = (temp_int == 0 ? false : true);
  Params::get_param_from_string("report_epidemic_data_by_census_tract", &temp_int);
  Global::Report_Epidemic_Data_By_Census_Tract = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_hospitals", &temp_int);
  Global::Enable_Hospitals = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_health_insurance", &temp_int);
  Global::Enable_Health_Insurance = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_group_quarters", &temp_int);
  Global::Enable_Group_Quarters = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_visualization_layer", &temp_int);
  Global::Enable_Visualization_Layer = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_vector_layer", &temp_int);
  Global::Enable_Vector_Layer = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_vector_transmission", &temp_int);
  Global::Enable_Vector_Transmission = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_population_dynamics", &temp_int);
  Global::Enable_Population_Dynamics = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_travel",&temp_int);
  Global::Enable_Travel = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_local_workplace_assignment",&temp_int);
  Global::Enable_Local_Workplace_Assignment = temp_int;
  Params::get_param_from_string("enable_seasonality", &temp_int);
  Global::Enable_Seasonality = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_climate", &temp_int);
  Global::Enable_Climate = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_chronic_condition", &temp_int);
  Global::Enable_Chronic_Condition = (temp_int == 0 ? false : true);
  Params::get_param_from_string("seed_by_age", &temp_int);
  Global::Seed_by_age = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_vaccination",&temp_int);
  Global::Enable_Vaccination = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_antivirals",&temp_int);
  Global::Enable_Antivirals = (temp_int == 0 ? false : true);
  Params::get_param_from_string("use_mean_latitude",&temp_int);
  Global::Use_Mean_Latitude = (temp_int == 0 ? false : true);
  Params::get_param_from_string("print_household_locations",&temp_int);
  Global::Print_Household_Locations = (temp_int == 0 ? false : true);
  Params::get_param_from_string("assign_teachers",&temp_int);
  Global::Assign_Teachers = (temp_int == 0 ? false : true);

  Params::get_param_from_string("report_age_of_infection", &Global::Report_Age_Of_Infection);
  Params::get_param_from_string("age_of_infection_log_level", &Global::Age_Of_Infection_Log_Level);
  Params::get_param_from_string("report_place_of_infection", &temp_int);
  Global::Report_Place_Of_Infection = (temp_int == 0 ? false : true);
  Params::get_param_from_string("report_distance_of_infection", &temp_int);
  Global::Report_Distance_Of_Infection = (temp_int == 0 ? false : true);
  Params::get_param_from_string("report_presenteeism", &temp_int);
  Global::Report_Presenteeism = (temp_int == 0 ? false : true);
  Params::get_param_from_string("report_serial_interval", &temp_int);
  Global::Report_Serial_Interval = (temp_int == 0 ? false : true);
  Params::get_param_from_string("report_incidence_by_county",&temp_int);
  Global::Report_Incidence_By_County = (temp_int == 0 ? false : true);
  Params::get_param_from_string("report_incidence_by_census_tract",&temp_int);
  Global::Report_Incidence_By_Census_Tract = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_shelter_in_place", &temp_int);
  Global::Enable_Household_Shelter = (temp_int == 0 ? false : true);
  Params::get_param_from_string("enable_isolation", &temp_int);
  Global::Enable_Isolation = (temp_int == 0 ? false : true);
  Params::get_param_from_string("isolation_delay", &Global::Isolation_Delay);
  Params::get_param_from_string("isolation_rate", &Global::Isolation_Rate);

  // Sanity Checks
  if(Global::Diseases > Global::MAX_NUM_DISEASES) {
    Utils::fred_abort("Global::Diseases > Global::MAX_NUM_DISEASES!");
  }
}

