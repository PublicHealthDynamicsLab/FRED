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
// File: Fred.cc
//

#include "County.h"
#include "Date.h"
#include "Demographics.h"
#include "Condition.h"
#include "Epidemic.h"
#include "Fred.h"
#include "Global.h"
#include "Group_Type.h"
#include "Neighborhood_Layer.h"
#include "Network.h"
#include "Network_Type.h"
#include "Property.h"
#include "Place_Type.h"
#include "Preference.h"
#include "Predicate.h"
#include "Random.h"
#include "Regional_Layer.h"
#include "Rule.h"
#include "Transmission.h"
#include "Travel.h"
#include "Utils.h"
#include "Visualization_Layer.h"

class Place;

#ifndef __CYGWIN__
#include "execinfo.h"
#endif /* __CYGWIN__ */

#include <csignal>
#include <cstdlib>
#include <cxxabi.h>

#include <unistd.h>
#include <stdio.h>

// for reporting
std::vector<int> daily_popsize;
double_vector_t* daily_globals;

//FRED main program

int main(int argc, char* argv[]) {
  fred_setup(argc, argv);
  for(Global::Simulation_Day = 0; Global::Simulation_Day < Global::Simulation_Days; ++Global::Simulation_Day) {
    fred_day(Global::Simulation_Day);
  }
  fred_finish();
  return 0;
}


void fred_setup(int argc, char* argv[]) {
  char error_file [FRED_STRING_SIZE];
  char warnings_file [FRED_STRING_SIZE];

  // GLOBAL SETUP
  Global::Simulation_Day = 0;
  Global::Statusfp = stdout;
  Utils::fred_print_wall_time("FRED started");
  Utils::fred_start_initialization_timer();
  Utils::fred_start_timer();

  strcpy(Global::Program_file, "");
  Global::Simulation_run_number = 1;
  strcpy(Global::Simulation_directory, "");
  Global::Compile_FRED = 0;

  // command line args using getopt
  int ch;
  while((ch = getopt(argc, argv, "cd:p:r:")) != -1) {
    switch (ch) {
    case 'c':
      Global::Compile_FRED = 1;
      break;
    case 'd':
      strcpy(Global::Simulation_directory, optarg);
      break;
    case 'p':
      strcpy(Global::Program_file, optarg);
      break;
    case 'r':
      sscanf(optarg,"%d",&Global::Simulation_run_number);
      break;
    case '?':
    default:
      printf("usage: FRED -p program -r run_number -d output_directory [ -c ]\n");
    }
  }

  if(strcmp(Global::Program_file, "") == 0) {
    strcpy(Global::Program_file, "model.fred");
    FILE* fp = fopen(Global::Program_file, "r");
    if(fp==NULL) {
      strcpy(Global::Program_file, "params");
    } else {
      fclose(fp);
    }
  }
  fprintf(Global::Statusfp, "FRED program file = %s\n", Global::Program_file);
  fflush(Global::Statusfp);
  FILE* fp = fopen(Global::Program_file, "r");
  if(fp == NULL) {
    fprintf(Global::Statusfp, "FRED program file %s not found\n", Global::Program_file);
    fflush(Global::Statusfp);
    exit(0);
  } else {
    fclose(fp);
  }

  // get runtime properties
  Property::read_properties(Global::Program_file);

  // extract global variables
  Global::get_global_properties();

  // select output directory location
  if(strcmp(Global::Simulation_directory, "") == 0) {
    // use the directory in the FRED program
    strcpy(Global::Simulation_directory, Global::Output_directory);
  } else {
    // use the command line arg
    strcpy(Global::Output_directory, Global::Simulation_directory);
    FRED_STATUS(0, "Overridden from command line: Output_directory = %s\n", Global::Output_directory);
  }

  // create the output directory if necessary
  Utils::fred_make_directory(Global::Simulation_directory);

  // open output files with global file pointers
  Utils::fred_open_output_files();

  // clear warnings_file and error_file
  sprintf(error_file, "%s/errors.txt", Global::Simulation_directory);
  unlink(error_file);
  sprintf(warnings_file, "%s/warnings.txt", Global::Simulation_directory);
  unlink(warnings_file);

  Utils::fred_print_wall_time("\nFRED run %d started", Global::Simulation_run_number);

  // set up dates and verify start_date and end_date
  Date::setup_dates();

  // set random number seed based on run number
  if(Global::Simulation_run_number > 1 && Global::Reseed_day == -1) {
    Global::Simulation_seed = Global::Seed * 100 + (Global::Simulation_run_number - 1);
  } else {
    Global::Simulation_seed = Global::Seed;
  }
  fprintf(Global::Statusfp, "seed = %lu\n", Global::Simulation_seed);
  Random::set_seed(Global::Simulation_seed);
  Utils::fred_print_lap_time("RNG setup");

  // create visualization layer if requested
  if(Global::Enable_Visualization_Layer) {
    Global::Visualization = new Visualization_Layer();
  }

  // PHASE 1: READ CLASS SPECIFIC PARAMETERS THAT DON'T DEPEND ON OTHER CLASSES

  Condition::get_condition_properties();
  Person::get_population_properties();
  Person::get_activity_properties();
  Demographics::initialize_static_variables();
  Place_Type::get_place_type_properties();
  Network_Type::get_network_type_properties();
  Place::get_place_properties();
  Utils::fred_print_lap_time("PHASE 1: get_properties");

  // PHASE 2: SETUP CONDITIONS AND DEFINE PLACES

  Condition::setup_conditions();
  Person::initialize_static_variables();
  Place::read_all_places();
  Utils::fred_print_lap_time("PHASE 2: read_all_places");

  // PHASE 3: DEFINE POPULATION

  // read in the population and have each person begin_membership
  // in each daily activity location identified in the population file
  Person::setup();
  Utils::fred_print_lap_time("Pop.setup");

  // PHASE 4: GROUP QUARTERS

  Place::setup_group_quarters();
  Utils::fred_print_lap_time("Places.setup_group_quarters");

  // PHASE 5: SETUP HOUSEHOLD RELATIONSHIPS

  Place::setup_households();
  Utils::fred_print_lap_time("setup_households");

  if(Global::Report_Contacts) {
    Place_Type::report_contacts();
    exit(0);
  }

  // PHASE 5: ADMINISTRATIVE REGIONS

  // associate each household with a block group
  Place::setup_block_groups();
  Utils::fred_print_lap_time("setup_block_groups");

  // setup county lists of schools and workplaces.
  // NOTE: this may optionally re-assign students to in-state schools
  County::setup_counties();
  Utils::fred_print_lap_time("setup_counties");

  // setup census_tract lists of schools and workplaces;
  //needs to be done after county school re-assignments
  Census_Tract::setup_census_tracts();
  Utils::fred_print_lap_time("setup_census_tracts");

  // PHASE 7: Define Partitions and begin_membership each person as needed

  // partition schools into classrooms, workplace into offices, others as needed
  Place::setup_partitions();
  Utils::fred_print_lap_time("setup_partitions");
  
  County::move_students_in_counties();
  Utils::fred_print_lap_time("move students in counties");

  // PHASE 8: Assign hospitals to households

  Place::assign_hospitals_to_households();
  Utils::fred_print_lap_time("assign hospitals to households");

  // PHASE 9: Prepare each place to receive visitors

  Place::prepare_places();
  Utils::fred_print_lap_time("place preparation");
  
  // PHASE 10: Reassign workers (to schools, hospitals, groups quarters, etc)

  Place::reassign_workers();
  Utils::fred_print_lap_time("reassign workers");

  // PHASE 11: Final preparation of groups.
  // Note: At this point all groups are known

  Place_Type::prepare_place_types();
  Network_Type::prepare_network_types();
  Utils::fred_print_lap_time("prepare_mixing_group_types");

  // printf("FRED_SETUP %d\n", Random::draw_random_int(0,10000));

  // PHASE 12: Final preparation of conditions
  Condition::prepare_to_track_group_state_counts();

  // printf("FRED_SETUP %d\n", Random::draw_random_int(0,10000));

  // PHASE 13: Final preparation of conditions
  Rule::prepare_rules();
  // printf("FRED_SETUP %d\n", Random::draw_random_int(0,10000));

  Condition::prepare_conditions();
  // printf("FRED_SETUP %d\n", Random::draw_random_int(0,10000));

  Utils::fred_print_lap_time("prepare_conditions");

  // PHASE 14: Update elevation if necessary

  Place::update_elevations(); // terminate FRED if elevations are updated
  Utils::fred_print_lap_time("update_elevations");

  //////////////////////////////////////////////////

  // PHASE 15: Update Layers

  // N/A

  // PHASE 16: Setup Travel

  Travel::get_properties();
  if(Global::Enable_Travel) {
    Utils::fred_print_wall_time("\nFRED Travel setup started");
    Global::Simulation_Region->set_population_size();
    Travel::setup(Global::Simulation_directory);
    Utils::fred_print_lap_time("Travel setup");
    Utils::fred_print_wall_time("FRED Travel setup finished");
  }

  // PHASE 17: Quality Control

  if(Global::Quality_control > 0) {
    Person::quality_control();
    Place::quality_control();
    Global::Simulation_Region->quality_control();
    Global::Neighborhoods->quality_control();
    if(Global::Track_network_stats) {
      Person::get_network_stats(Global::Simulation_directory);
    }
    Utils::fred_print_lap_time("quality control");
  }

  /*
    if(Global::Track_age_distribution) {
    Person::print_age_distribution(Global::Simulation_directory,
    (char *) Global::Sim_Start_Date->get_YYYYMMDD().c_str(),
    Global::Simulation_run_number);
    }
  */

  // PHASE 18: Check Parameters (and exit)
  Property::print_errors(error_file);
  Property::print_warnings(warnings_file);
  Rule::print_warnings();

  if(Global::Compile_FRED || Global::Error_found) {
    FRED_VERBOSE(0, "FRED terminating compile %d error %d\n", Global::Compile_FRED, Global::Error_found);
    exit(0);
  }

  if(Property::check_properties > 0) {
    Property::report_parameter_check();
    FRED_VERBOSE(0, "FRED terminating after check_properties\n");
    fred_finish();
    exit(0);
  }

  // prepare for daily reports
  daily_popsize.clear();
  daily_globals = new double_vector_t [Person::get_number_of_global_vars()];
  Utils::fred_print_wall_time("FRED initialization complete");
  Utils::fred_start_timer(&Global::Simulation_start_time);
  Utils::fred_print_initialization_timer();

  // printf("FRED_SETUP %d\n", Random::draw_random_int(0,10000));
}


void fred_day(int day) {

  Utils::fred_start_day_timer();
  fred_setup_day(day);

  for(Global::Simulation_Hour = 0; Global::Simulation_Hour < 24; ++Global::Simulation_Hour) {
    Global::Simulation_Step = 24 * day + Global::Simulation_Hour;
    fred_step(day, Global::Simulation_Hour);
  }

  fred_finish_day(day);

}

void fred_step(int day, int hour) {
  
  FRED_VERBOSE(1, "fred_step day %d hour %d\n", day, hour);

  // order of condition updates:
  vector<int> order;
  order.clear();
  for(int d = 0; d < Condition::get_number_of_conditions(); ++d) {
    order.push_back(d);
  }
  
  // if fixed order is not selected, shuffle the order in which FRED evaluates the conditions
  if(Condition::get_number_of_conditions() > 1 && !Global::Enable_Fixed_Order_Condition_Updates) {
    FYShuffle<int>(order);
    std::cout << "Shuffling order of conditions" << std::endl;
  }
  
  // update epidemic for each condition in turn
  for(int d = 0; d < Condition::get_number_of_conditions(); ++d) {
    int condition_id = order[d];
    Condition* condition = Condition::get_condition(condition_id);
    condition->update(day, hour);
  }
}

void fred_setup_day(int day) {

  // optional: reseed the random number generator to create alternative
  // simulation from a given initial point
  if(day == Global::Reseed_day) {
    fprintf(Global::Statusfp, "************** reseed day = %d\n", day);
    fflush(Global::Statusfp);
    Random::set_seed(Global::Simulation_seed + Global::Simulation_run_number - 1);
  }

  // optional: periodically output distributions of the population demographics
  /*
  if(Global::Track_age_distribution) {
    if(Date::get_month() == 1 && Date::get_day_of_month() == 1) {
      char date_string[80];
      strcpy(date_string, (char *) Global::Sim_Current_Date->get_YYYYMMDD().c_str());
      Person::print_age_distribution(Global::Simulation_directory, date_string, Global::Simulation_run_number);
      Place::print_household_size_distribution(Global::Simulation_directory, date_string, Global::Simulation_run_number);
    }
  }
  */

  // update vector population, if any; update insurance status
  Place::update(day);
  Utils::fred_print_lap_time("day %d update places", day);

  // update population demographics
  Person::update_population_demographics(day);
  Utils::fred_print_lap_time("day %d update demographics", day);

  // update population mobility and stage-of-life activities
  Place::update_population_dynamics(day);
  Utils::fred_print_lap_time("day %d update population dynamics", day);

  // remove dead from population
  Person::remove_dead_from_population(day);
  Utils::fred_print_lap_time("day %d remove dead from population", day);

  // remove migrants from population
  Person::remove_migrants_from_population(day);
  Utils::fred_print_lap_time("day %d remove_migrants", day);

  // update travel decisions
  Travel::update_travel(day);
  Utils::fred_print_lap_time("day %d update travel", day);

  // update generic activities (individual activities updated only if
  // needed -- see below)
  Person::update(day);
  // Utils::fred_print_lap_time("day %d update activities", day);

  // external updates
  if (Global::Enable_External_Updates) {
    Person::get_external_updates(day);
    Utils::fred_print_lap_time("day %d external updates", day);
  }
}

void fred_finish_day(int day) {

  FRED_VERBOSE(1, "day %d fred_finish_day entered\n", day);

  // print daily reports and visualization data
  for(int d = 0; d < Condition::get_number_of_conditions(); ++d) {
    Condition::get_condition(d)->report(day);
  }
  Utils::fred_print_lap_time("day %d report conditions", day);
  
  for(int place_type_id = 0; place_type_id < Place_Type::get_number_of_place_types(); ++place_type_id) {
    Place_Type::get_place_type(place_type_id)->report(day);
  }
  Utils::fred_print_lap_time("day %d report place_types", day);
  
  Network_Type::print_network_types(day);
  Utils::fred_print_lap_time("day %d print network_types", day);
  
  // print population stats
  Person::report(day);
  Utils::fred_print_lap_time("day %d report population", day);

  // optional: report change in demographics at end of each year
  if(Global::Enable_Population_Dynamics && Global::Verbose > 0
     && Date::get_month() == 12 && Date::get_day_of_month() == 31) {
    Person::quality_control();
  }

  // optional: report County demographics at end of each year
  if(Global::Report_County_Demographic_Information && Date::get_month() == 12 && Date::get_day_of_month() == 31) {
    // Place::report_county_populations();
  }

  // record daily stats
  daily_popsize.push_back(Person::get_population_size());
  for(int i = 0; i < Person::get_number_of_global_vars(); ++i) {
    daily_globals[i].push_back(Person::get_global_var(i));
  }

  // print daily reports
  Utils::fred_print_resource_usage(day);
  Utils::fred_print_wall_time("day %d finished", day);
  FRED_STATUS(0, "%s %s ", Date::get_day_of_week_string().c_str(), Date::get_date_string().c_str());
  Utils::fred_print_day_timer(day);

  // advance date counter
  Date::update();
}


void make_output_variable_files() {
  // parse the output files into csv files, one for each column
  char dir[FRED_STRING_SIZE];
  sprintf(dir, "%s/RUN%d/DAILY", Global::Simulation_directory, Global::Simulation_run_number);
  Utils::fred_make_directory(dir);

  char outfile[FRED_STRING_SIZE];
  sprintf(outfile, "%s/Popsize.txt", dir);
  FILE *fp = fopen(outfile, "w");
  if(fp == NULL) {
    Utils::fred_abort("Fred: can't open file %s\n", outfile);
  }
  for(int day = 0; day < Global::Simulation_Days; ++day) {
    fprintf(fp, "%d %d\n", day, daily_popsize[day]);
  }
  fclose(fp);

  sprintf(outfile, "%s/Date.txt", dir);
  fp = fopen(outfile, "w");
  if(fp == NULL) {
    Utils::fred_abort("Fred: can open file %s\n", outfile);
  }
  for(int day = 0; day < Global::Simulation_Days; ++day) {
    string datestring = Date::get_date_string(day);
    fprintf(fp, "%d %s\n", day, datestring.c_str());
  }
  fclose(fp);

  sprintf(outfile, "%s/EpiWeek.txt", dir);
  fp = fopen(outfile, "w");
  if(fp == NULL) {
    Utils::fred_abort("Fred: can open file %s\n", outfile);
  }
  for(int day = 0; day < Global::Simulation_Days; ++day) {
    char epiweek[FRED_STRING_SIZE];
    sprintf(epiweek, "%d.%02d", Date::get_epi_year(day), Date::get_epi_week(day));
    fprintf(fp, "%d %s\n", day, epiweek);
  }
  fclose(fp);

  // create a csv file for this run

  // this joins two files with same value in column 1, from
  // https://stackoverflow.com/questions/14984340/using-awk-to-process-input-from-multiple-files
  char awkcommand[FRED_STRING_SIZE];
  sprintf(awkcommand, "awk 'FNR==NR{a[$1]=$2 FS $3;next}{print $0, a[$1]}' ");

  char command[FRED_STRING_SIZE];
  char csvfile[FRED_STRING_SIZE];

  sprintf(csvfile, "%s/RUN%d/out.csv", Global::Simulation_directory, Global::Simulation_run_number);
  sprintf(command, "cp %s/Date.txt %s", dir, csvfile);
  system(command);

  sprintf(command, "%s %s/EpiWeek.txt %s > %s.tmp; mv %s.tmp %s", awkcommand, dir, csvfile, csvfile, csvfile, csvfile);
  system(command);
    
  sprintf(command, "%s %s/Popsize.txt %s > %s.tmp; mv %s.tmp %s", awkcommand, dir, csvfile, csvfile, csvfile, csvfile);
  system(command);

  // add a header line
  // create a header line for the csv file
  char headerfile[FRED_STRING_SIZE];
  sprintf(headerfile, "%s/RUN%d/out.header", Global::Simulation_directory, Global::Simulation_run_number);
  fp = fopen(headerfile, "w");
  fprintf(fp, "Day Date EpiWeek Popsize \n");
  fclose(fp);

  // concatenate header line
  sprintf(command, "cat %s %s > %s.tmp; mv %s.tmp %s; unlink %s", headerfile, csvfile, csvfile, csvfile, csvfile, headerfile);
  system(command);

  // join all the condition csv files
  char condfile[FRED_STRING_SIZE];
  for(int cond_id = 0; cond_id < Condition::get_number_of_conditions(); ++cond_id) {
    string condname = Condition::get_name(cond_id);
    sprintf(condfile, "%s/RUN%d/%s.csv", Global::Simulation_directory, Global::Simulation_run_number, condname.c_str());
    sprintf(command, "%s %s %s > %s.tmp; mv %s.tmp %s", awkcommand, condfile, csvfile, csvfile, csvfile, csvfile);
    // FRED_VERBOSE(0, "command = |%s|\n", command);
    system(command);
  }

  // replace spaces with commas
  sprintf(command, "sed -E 's/ +/,/g' %s | sed -E 's/,$//' > %s.tmp; mv %s.tmp %s", csvfile, csvfile, csvfile, csvfile);
  system(command);

  return;
}


void fred_finish() {
  
  // final reports
  Place_Type::finish_place_types();
  Network_Type::finish_network_types();

  // report timing info
  if(Property::check_properties == 0) {
    Utils::fred_print_lap_time(&Global::Simulation_start_time,
        "\nFRED simulation complete. Excluding initialization, %d days",
        Global::Simulation_Days);
  }
  Utils::fred_print_wall_time("FRED finished");
  Utils::fred_print_finish_timer();

  Person::finish();
  Place::finish();
  Condition::finish_conditions();
  fred_finish_global_vars();

  // close all open output files with global file pointers
  Utils::fred_end();

  if(Property::check_properties == 0) {
    make_output_variable_files();
  }
}


void fred_finish_global_vars() {

  int num_vars = Person::get_number_of_global_vars();
  if(num_vars == 0) {
    return;
  }

  char dir[FRED_STRING_SIZE];
  char outfile[FRED_STRING_SIZE];
  FILE* fp;

  sprintf(dir, "%s/RUN%d/DAILY", Global::Simulation_directory, Global::Simulation_run_number);
  Utils::fred_make_directory(dir);

  for(int var_id = 0; var_id < num_vars; ++var_id) {
    string var_name = Person::get_global_var_name(var_id);

    sprintf(outfile, "%s/FRED.%s.txt", dir, var_name.c_str());
    fp = fopen(outfile, "w");
    if(fp == NULL) {
      Utils::fred_abort("Fred: can't open file %s\n", outfile);
    }
    for(int day = 0; day < Global::Simulation_Days; ++day) {
      double value = daily_globals[var_id][day];
      fprintf(fp, "%d %f\n", day, value);
    }
    fclose(fp);
  }

  // create a csv file for global vars
  
  // this joins two files with same value in column 1, from
  // https://stackoverflow.com/questions/14984340/using-awk-to-process-input-from-multiple-files
  char awkcommand[FRED_STRING_SIZE];
  sprintf(awkcommand, "awk 'FNR==NR{a[$1]=$2 FS $3;next}{print $0, a[$1]}' ");
  
  char command[FRED_STRING_SIZE];
  char dailyfile[FRED_STRING_SIZE];
  
  sprintf(outfile, "%s/RUN%d/FRED.csv", Global::Simulation_directory, Global::Simulation_run_number);
  
  for(int var_id = 0; var_id < num_vars; ++var_id) {
    string var_name = Person::get_global_var_name(var_id);
    sprintf(dailyfile, "%s/FRED.%s.txt", dir, var_name.c_str());
    if(var_id == 0) {
      sprintf(command, "cp %s %s", dailyfile, outfile);
    } else {
      sprintf(command, "%s %s %s > %s.tmp; mv %s.tmp %s", awkcommand, dailyfile, outfile, outfile, outfile, outfile);
    }
    system(command);
  }  
  
  // create a header line for the csv file
  char headerfile[FRED_STRING_SIZE];
  sprintf(headerfile, "%s/RUN%d/FRED.header", Global::Simulation_directory, Global::Simulation_run_number);
  fp = fopen(headerfile, "w");
  fprintf(fp, "Day ");
  for (int var_id = 0; var_id < num_vars; var_id++) {
    string var_name = Person::get_global_var_name(var_id);
    fprintf(fp, "FRED.%s ", var_name.c_str());
  }
  fprintf(fp, "\n");
  fclose(fp);
  
  // concatenate header line
  sprintf(command, "cat %s %s > %s.tmp; mv %s.tmp %s; unlink %s", headerfile, outfile, outfile, outfile, outfile, headerfile);
  system(command);
  
  // replace spaces with commas
  sprintf(command, "sed -E 's/ +/,/g' %s | sed -E 's/,$//' | sed -E 's/,/ /' > %s.tmp; mv %s.tmp %s", outfile, outfile, outfile, outfile);
  system(command);

}
