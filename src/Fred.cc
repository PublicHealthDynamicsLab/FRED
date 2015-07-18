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
// File: Fred.cc
//
#include "Fred.h"
#include "Utils.h"
#include "Global.h"
#include "Population.h"
#include "Disease.h"
#include "Disease_List.h"
#include "Place_List.h"
#include "Neighborhood_Layer.h"
#include "Regional_Layer.h"
#include "Visualization_Layer.h"
#include "Vector_Layer.h"
#include "Params.h"
#include "Random.h"
#include "Vaccine_Manager.h"
#include "Date.h"
#include "Evolution.h"
#include "Travel.h"
#include "Epidemic.h"
#include "Seasonality.h"
#include "Activities.h"
#include "Behavior.h"
#include "Demographics.h"
#include "Health.h"
#include "Tracker.h"
class Place;

#if SQLITE
#include "DB.h"
#endif

#ifndef __CYGWIN__
#include "execinfo.h"
#endif /* __CYGWIN__ */

#include <csignal>
#include <cstdlib>
#include <cxxabi.h>

//FRED main program

int main(int argc, char* argv[]) {
  fred_setup(argc, argv);
  for(Global::Simulation_Day = 0; Global::Simulation_Day < Global::Days; Global::Simulation_Day++) {
    fred_step(Global::Simulation_Day);
  }
  fred_finish();
  return 0;
}


void fred_setup(int argc, char* argv[]) {
  char paramfile[FRED_STRING_SIZE];

  Global::Simulation_Day = 0;
  Global::Statusfp = stdout;
  Utils::fred_print_wall_time("FRED started");
  Utils::fred_start_initialization_timer();
  Utils::fred_start_timer();

  // read optional param file name from command line
  if(argc > 1) {
    strcpy(paramfile, argv[1]);
  } else {
    strcpy(paramfile, "params");
  }
  fprintf(Global::Statusfp, "param file = %s\n", paramfile);
  fflush(Global::Statusfp);

  // read optional run number from command line (must be 2nd arg)
  if(argc > 2) {
    sscanf(argv[2], "%d", &Global::Simulation_run_number);
  } else {
    Global::Simulation_run_number = 1;
  }

  // read optional working directory from command line (must be 3rd arg)
  if(argc > 3) {
    strcpy(Global::Simulation_directory, argv[3]);
  } else {
    strcpy(Global::Simulation_directory, "");
  }

  // get runtime parameters
  Params::read_parameters(paramfile);
  Global::get_global_parameters();
  Date::setup_dates(Global::Start_date);

  // create diseases
  Global::Diseases.get_parameters();

  Global::Pop.get_parameters();
  Utils::fred_print_lap_time("get_parameters");

#if SQLITE
  Global::db.open_database(Global::DBfile);
#endif 

  // initialize masks in Global::Pop
  Global::Pop.initialize_masks();

  if(strcmp(Global::Simulation_directory, "") == 0) {
    // use the directory in the params file
    strcpy(Global::Simulation_directory, Global::Output_directory);
    //printf("Global::Simulation_directory = %s\n",Global::Simulation_directory);
  } else {
    // change the Output_directory
    strcpy(Global::Output_directory, Global::Simulation_directory);
    FRED_STATUS(0, "Overridden from command line: Output_directory = %s\n",
		Global::Output_directory);
  }

  // create the output directory, if necessary
  Utils::fred_make_directory(Global::Simulation_directory);

  // open output files with global file pointers
  Utils::fred_open_output_files();

  // Date Setup -- Start_date parameter must have format 'YYYY-MM-DD'
  // Global::Sim_Start_Date = new Date(string(Global::Start_date));
  // Global::Sim_Current_Date = new Date(string(Global::Start_date));

  if(Global::Rotate_start_date) {
    // add one day to the start date for each additional run,
    // rotating the days of the week to reduce weekend effect.
    // Global::Sim_Start_Date->advance((Global::Simulation_run_number - 1) % 7);
    // Global::Sim_Current_Date->advance((Global::Simulation_run_number - 1) % 7);
  }

  // set random number seed based on run number
  if(Global::Simulation_run_number > 1 && Global::Reseed_day == -1) {
    Global::Simulation_seed = Global::Seed * 100 + (Global::Simulation_run_number - 1);
  } else {
    Global::Simulation_seed = Global::Seed;
  }
  fprintf(Global::Statusfp, "seed = %lu\n", Global::Simulation_seed);
  Random::set_seed(Global::Simulation_seed);

  Utils::fred_print_lap_time("RNG setup");
  Utils::fred_print_wall_time("\nFRED run %d started", Global::Simulation_run_number);

  // initializations

  // Initializes Synthetic Population parameters, determines the synthetic
  // population id if the city or county was specified as a parameter
  // Must be called BEFORE Pop.split_synthetic_populations_by_deme() because
  // city/county population lookup may overwrite Global::Synthetic_population_id
  Global::Places.get_parameters();

  // split the population id parameter string ( that was initialized in 
  // Places::get_parameters ) on whitespace; each population id is processed as a
  // separate deme, and stored in the Population object.
  Global::Pop.split_synthetic_populations_by_deme();

  // Loop over all Demes and read in the household, schools and workplaces
  // and setup geographical layers
  Utils::fred_print_wall_time("\nFRED read_places started");
  Global::Places.read_all_places(Global::Pop.get_demes());
  Utils::fred_print_lap_time("Places.read_places");
  Utils::fred_print_wall_time("FRED read_places finished");

  // create visualization layer, if requested
  if(Global::Enable_Visualization_Layer) {
    Global::Visualization = new Visualization_Layer();
  }

  // create vector layer, if requested
  if(Global::Enable_Vector_Layer) {
    Global::Vectors = new Vector_Layer();
  }

  // initialize parameters and other static variables
  Place::initialize_static_variables();
  Demographics::initialize_static_variables();
  Activities::initialize_static_variables();
  Behavior::initialize_static_variables();
  Health::initialize_static_variables();
  Utils::fred_print_lap_time("initialize_static_variables");

  // finished setting up Diseases
  Global::Diseases.setup();

  // read in the population and have each person enroll
  // in each favorite place identified in the population file
  Utils::fred_print_wall_time("\nFRED Pop.setup started");
  Global::Pop.setup();
  Global::Places.setup_group_quarters();
  Global::Places.setup_households();
  Utils::fred_print_lap_time("Pop.setup");
  Utils::fred_print_wall_time("FRED Pop.setup finished");

  // define FRED-specific places and have each person enroll as needed

  // classrooms
  Global::Places.setup_classrooms();
  Global::Pop.assign_classrooms();
  Utils::fred_print_lap_time("assign classrooms");

  // reassign workers (to schools, hospitals, groups quarters, etc)
  Global::Places.reassign_workers();
  Utils::fred_print_lap_time("reassign workers");

  // offices
  Global::Places.setup_offices();
  Global::Pop.assign_offices();
  Utils::fred_print_lap_time("assign offices");

  // after all enrollments, prepare to receive visitors
  Global::Places.prepare();
  Utils::fred_print_lap_time("place preparation");

  if(Global::Enable_Hospitals) {
    Global::Places.assign_hospitals_to_households();
    Utils::fred_print_lap_time("assign hospitals to households");
  }

  FRED_STATUS(0, "deleting place_label_map\n", "");
  Global::Places.delete_place_label_map();
  FRED_STATUS(0, "prepare places finished\n", "");

  if(Global::Enable_Travel) {
    Utils::fred_print_wall_time("\nFRED Travel setup started");
    Global::Simulation_Region->set_population_size();
    Travel::setup(Global::Simulation_directory);
    Utils::fred_print_lap_time("Travel setup");
    Utils::fred_print_wall_time("FRED Travel setup finished");
  }

  if(Global::Quality_control) {
    Global::Pop.quality_control();
    Global::Places.quality_control();
    Global::Simulation_Region->quality_control();
    Global::Neighborhoods->quality_control();
    if(Global::Enable_Visualization_Layer) {
      Global::Visualization->quality_control();
    }
    if(Global::Enable_Vector_Layer) {
      Global::Vectors->quality_control();
      Global::Simulation_Region->set_population_size();
      Global::Vectors->swap_county_people();
    }
    if(Global::Track_network_stats) {
      Global::Pop.get_network_stats(Global::Simulation_directory);
    }
    Utils::fred_print_lap_time("quality control");
  }

  if(Global::Track_age_distribution) {
    /*
    Global::Pop.print_age_distribution(Global::Simulation_directory,
				       (char *) Global::Sim_Start_Date->get_YYYYMMDD().c_str(),
				       Global::Simulation_run_number);
    */
  }

  if(Global::Enable_Seasonality) {
    Global::Clim->print_summary();
  }

  if(Global::Report_Mean_Household_Income_Per_School) {
    Global::Pop.report_mean_hh_income_per_school();
  }

  if(Global::Report_Mean_Household_Size_Per_School) {
    Global::Pop.report_mean_hh_size_per_school();
  }

  if(Global::Report_Mean_Household_Distance_From_School) {
    Global::Pop.report_mean_hh_distance_from_school();
  }

  if(Global::Report_Childhood_Presenteeism) {
    Global::Pop.set_school_income_levels();
    Global::Places.setup_school_income_quartile_pop_sizes();
    //Global::Places.setup_household_income_quartile_sick_days();
  }

  if(Global::Enable_hh_income_based_susc_mod) {
    int pct_90_min = Global::Places.get_min_household_income_by_percentile(90);
    Household::set_min_hh_income_90_pct(pct_90_min);
  }

  if(Global::Report_Mean_Household_Stats_Per_Income_Category &&
     Global::Report_Epidemic_Data_By_Census_Tract) {
    Global::Income_Category_Tracker = new Tracker<int>("Income Category Tracker","income_cat");
    Global::Tract_Tracker = new Tracker<long int>("Census Tract Tracker","Tract");
    Global::Pop.report_mean_hh_stats_per_income_category_per_census_tract();
  } else if(Global::Report_Mean_Household_Stats_Per_Income_Category) {
    Global::Income_Category_Tracker = new Tracker<int>("Income Category Tracker","income_cat");
    Global::Pop.report_mean_hh_stats_per_income_category();
  } else if(Global::Report_Epidemic_Data_By_Census_Tract) {
    Global::Tract_Tracker = new Tracker<long int>("Census Tract Tracker","Tract");
    Global::Pop.report_mean_hh_stats_per_census_tract();
  }

  // Global tracker allows us to have as many variables we
  // want from wherever in the output file
  Global::Daily_Tracker = new Tracker<int>("Main Daily Tracker","Day");
  
  // initialize diseases
  for(int d = 0; d < Global::Diseases.get_number_of_diseases(); ++d) {
    Disease* disease = Global::Diseases.get_disease(d);
    disease->initialize_evolution_reporting_grid(Global::Simulation_Region);
    if(!Global::Enable_Vector_Layer) {
      disease->init_prior_immunity();
    }
  }
  Utils::fred_print_lap_time("disease_initialization");

  if(Global::Enable_Vector_Layer) {
    Global::Vectors->init_prior_immunity_by_county();
    for(int d = 0; d < Global::Diseases.get_number_of_diseases(); ++d) {
      Global::Vectors->init_prior_immunity_by_county(d);
    }
    Utils::fred_print_lap_time("vector_layer_initialization");
  }

  // initialize visualization data if desired
  // if(Global::Enable_Visualization_Layer && Global::Simulation_run_number == 1) {
  if(Global::Enable_Visualization_Layer) {
    Global::Visualization->initialize();
  }

  Utils::fred_print_wall_time("FRED initialization complete");

  Utils::fred_start_timer(&Global::Simulation_start_time);
  Utils::fred_print_initialization_timer();
}


void fred_step(int day) {

  Utils::fred_start_day_timer();

  // optional: reseed the random number generator to create alternative
  // simulation form a given initial point
  if(day == Global::Reseed_day) {
    fprintf(Global::Statusfp, "************** reseed day = %d\n", day);
    fflush(Global::Statusfp);
    Random::set_seed(Global::Simulation_seed + Global::Simulation_run_number - 1);
  }

  // optional: periodically output distributions of the population demographics
  if(Global::Track_age_distribution) {
    if(Date::get_month() == 1 && Date::get_day_of_month() == 1) {
      /*
      char date_string[80];
      strcpy(date_string, (char *) Global::Sim_Current_Date->get_YYYYMMDD().c_str());
      Global::Pop.print_age_distribution(Global::Simulation_directory, date_string, Global::Simulation_run_number);
      Global::Places.print_household_size_distribution(Global::Simulation_directory, date_string, Global::Simulation_run_number);
      */
    }
  }

  // reset lists of infectious, susceptibles; update vector population, if any
  Global::Places.update(day);
  Utils::fred_print_lap_time("day %d update places", day);

  // optional: update population dynamics 
  if(Global::Enable_Population_Dynamics) {
    Demographics::update(day);
    Global::Places.update_population_dynamics(day);
    Utils::fred_print_lap_time("day %d update population dynamics", day);
  }

  // update health status of all agents.
  // update activity profiles if dynamic.
  // update travel decisions.
  // update decisions about health behaviors.
  // distribute vaccines and antivirals.
  Global::Pop.update(day);
  Utils::fred_print_lap_time("day %d update population", day);

  // update generic activities (individual activities updated only if
  // needed -- see below)
  Activities::update(day);

  // each infectious person updates its activity schedule, and
  // registers each place visited as a infectious place
  Global::Pop.update_infectious_people(day);
  Utils::fred_print_lap_time("day %d update_infectious_people", day);

  // find all visitors to infectious places
  Global::Places.find_visitors_to_infectious_places(day);
  Utils::fred_print_lap_time("day %d find_visitors_to_infectious_places", day);

  // shuffle the order of diseases to reduce systematic bias
  vector<int> order;
  order.clear();
  for(int d = 0; d < Global::Diseases.get_number_of_diseases(); ++d) {
    order.push_back(d);
  }
  if(Global::Diseases.get_number_of_diseases() > 1) {
    FYShuffle<int>(order);
  }

  // transmit each disease in turn
  for(int d = 0; d < Global::Diseases.get_number_of_diseases(); ++d) {
    int disease_id = order[d];
    Disease* disease = Global::Diseases.get_disease(disease_id);
    disease->update(day);
    Utils::fred_print_lap_time("day %d update epidemic for disease %d", day, disease_id);
  }

  // print daily report
  Global::Pop.report(day);
  Utils::fred_print_lap_time("day %d report population", day);

  if(Global::Enable_HAZEL) {
    Activities::print_stats(day);
    Global::Places.print_stats(day);
  }

  // print visualization data if desired
  // if(Global::Enable_Visualization_Layer && Global::Simulation_run_number == 1) {
  if(Global::Enable_Visualization_Layer) {
    Global::Visualization->print_visualization_data(day);
    Utils::fred_print_lap_time("day %d print_visualization_data", day);
  }

  // optional: report change in demographics and end of each year
  if(Global::Enable_Population_Dynamics && Global::Verbose
     && Date::get_month() == 12 && Date::get_day_of_month() == 31) {
    Global::Pop.quality_control();
  }

#pragma omp parallel sections
  {
#pragma omp section
    {
      // flush infections file buffer
      fflush(Global::Infectionfp);
    }
#pragma omp section
    {
#if SQLITE
      Global::db.process_transactions();
#endif
    }
  }

  // print daily reports
  Utils::fred_print_resource_usage(day);
  Utils::fred_print_wall_time("day %d finished", day);
  Utils::fred_print_day_timer(day);
  Global::Daily_Tracker->output_inline_report_format_for_index(day, Global::Outfp);

  // advance date counter
  Date::update();
}


void fred_finish() {
  //Global::Daily_Tracker->create_full_log(10,cout);
#if SQLITE
  Global::db.close_database();
#endif
  fflush(Global::Infectionfp);
  
  // final reports
  if(Global::Report_Mean_Household_Stats_Per_Income_Category && Global::Report_Epidemic_Data_By_Census_Tract) {
    Global::Tract_Tracker->output_csv_report_format(Global::Tractfp);
    Global::Income_Category_Tracker->output_csv_report_format(Global::IncomeCatfp);
  } else if(Global::Report_Mean_Household_Stats_Per_Income_Category)  {
    Global::Income_Category_Tracker->output_csv_report_format(Global::IncomeCatfp);
  } else if(Global::Report_Epidemic_Data_By_Census_Tract) {
    Global::Tract_Tracker->output_csv_report_format(Global::Tractfp);
  }

  // report timing info
  Utils::fred_print_lap_time(&Global::Simulation_start_time,
			     "\nFRED simulation complete. Excluding initialization, %d days",
			     Global::Days);
  Utils::fred_print_wall_time("FRED finished");
  Utils::fred_print_finish_timer();

  Global::Pop.end_of_run();
  Global::Places.end_of_run();
  
  // close all open output files with global file pointers
  Utils::fred_end();

}
