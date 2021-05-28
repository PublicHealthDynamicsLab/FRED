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
// File: Global.cc
//

#include "Global.h"
#include "Property.h"
#include "Demographics.h"
#include "Utils.h"

// global simulation variables
char Global::Program_file[FRED_STRING_SIZE];
char Global::Simulation_directory[FRED_STRING_SIZE];
char Global::Plot_directory[FRED_STRING_SIZE];
char Global::Visualization_directory[FRED_STRING_SIZE];
int Global::Simulation_run_number = 1;
unsigned long Global::Simulation_seed = 1;
high_resolution_clock::time_point Global::Simulation_start_time = high_resolution_clock::now();
int Global::Simulation_Day = 0;
int Global::Simulation_Days = 0;
int Global::Simulation_Hour = 0;
int Global::Simulation_Step = 0;

// global runtime properties
char Global::Output_directory[FRED_STRING_SIZE];
int Global::Quality_control = 0;
char Global::ErrorLogbase[FRED_STRING_SIZE];
bool Global::Track_age_distribution = false;
bool Global::Track_network_stats = false;

int Global::Verbose = 0;
int Global::Debug = 0;
int Global::Test = 0;
int Global::Reseed_day = 0;
unsigned long Global::Seed = 0;
int Global::Use_FRED = 0;
int Global::Use_rules = 0;
int Global::Compile_FRED = 0;
int Global::Max_Loops = 0;
bool Global::Enable_Profiles = false;
bool Global::Enable_Records = false;
bool Global::Enable_Var_Records = false;
bool Global::Enable_Transmission_Bias = false;
bool Global::Enable_New_Transmission_Model = false;
bool Global::Enable_Hospitals = false;
bool Global::Enable_Health_Insurance = false;
bool Global::Enable_Group_Quarters = false;
bool Global::Enable_Visualization_Layer = false;
int Global::Visualization_Run = 1;
int Global::Health_Records_Run = 1;
bool Global::Enable_Population_Dynamics = false;
bool Global::Enable_Travel = false;
bool Global::Enable_Local_Workplace_Assignment = false;
bool Global::Enable_Fixed_Order_Condition_Updates = false;
bool Global::Enable_External_Updates = false;
bool Global::Use_Mean_Latitude = false;
bool Global::Report_Serial_Interval = false;
bool Global::Report_County_Demographic_Information = false;
bool Global::Assign_Teachers = false;
bool Global::Report_Contacts = false;
bool Global::Error_found = false;

// grid layers
Neighborhood_Layer* Global::Neighborhoods = NULL;
Regional_Layer* Global::Simulation_Region = NULL;
Visualization_Layer* Global::Visualization = NULL;

// global file pointers
FILE* Global::Statusfp = NULL;
FILE* Global::Recordsfp = stdout;
FILE* Global::Birthfp = NULL;
FILE* Global::Deathfp = NULL;
FILE* Global::ErrorLogfp = NULL;

void Global::get_global_properties() {
  Property::get_property("verbose", &Global::Verbose);
  Property::get_property("debug", &Global::Debug);
  Property::get_property("test", &Global::Test);
  Property::get_property("quality_control", &Global::Quality_control);
  Property::get_property("seed", &Global::Seed);
  Property::get_property("reseed_day", &Global::Reseed_day);
  Property::get_property("outdir", Global::Output_directory);
  Property::get_property("max_loops", &Global::Max_Loops);

  //Set global boolean flags
  Property::get_property("track_age_distribution", &Global::Track_age_distribution);
  Property::get_property("track_network_stats", &Global::Track_network_stats);
  Property::get_property("enable_profiles", &Global::Enable_Profiles);
  Property::get_property("enable_health_records", &Global::Enable_Records);
  Property::get_property("enable_var_records", &Global::Enable_Var_Records);
  Property::get_property("enable_transmission_bias", &Global::Enable_Transmission_Bias);
  Property::get_property("enable_new_transmission_model", &Global::Enable_New_Transmission_Model);
  Property::get_property("enable_Hospitals", &Global::Enable_Hospitals);
  Property::get_property("enable_health_insurance", &Global::Enable_Health_Insurance);
  Property::get_property("enable_group_quarters", &Global::Enable_Group_Quarters);
  Property::get_property("enable_visualization_layer", &Global::Enable_Visualization_Layer);
  Property::get_property("enable_population_dynamics", &Global::Enable_Population_Dynamics);
  Property::get_property("enable_travel",&Global::Enable_Travel);
  Property::get_property("enable_local_Workplace_assignment", &Global::Enable_Local_Workplace_Assignment);
  Property::get_property("enable_fixed_order_condition_updates", &Global::Enable_Fixed_Order_Condition_Updates);
  Property::get_property("use_mean_latitude", &Global::Use_Mean_Latitude);
  Property::get_property("assign_teachers", &Global::Assign_Teachers);
  Property::get_property("report_serial_interval", &Global::Report_Serial_Interval);
  Property::get_property("report_contacts", &Global::Report_Contacts);

  // set any properties that are dependent on other properties
  Property::get_property("visualization_run", &Global::Visualization_Run);
  if(Global::Visualization_Run != -1 &&
      Global::Simulation_run_number != Global::Visualization_Run) {
    Global::Enable_Visualization_Layer = false;
  }

  Property::get_property("health_records_run", &Global::Health_Records_Run);
  if(Global::Health_Records_Run != -1 &&
      Global::Simulation_run_number != Global::Health_Records_Run) {
    Global::Enable_Records = false;
  }

  if(Global::Compile_FRED) {
    Global::Debug = 0;
    Global::Verbose = 0;
    Global::Quality_control = 0;
  }

}
