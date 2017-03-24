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
// File: Population.h
//

#ifndef _FRED_POPULATION_H
#define _FRED_POPULATION_H

#include <fstream>
#include <iostream>
#include <vector>

using namespace std;

class Person;
class Place;

#include "Global.h"
#include "Utils.h"


class Population {

public:

  /**
   * Default constructor
   */
  Population();
  ~Population();

  // reading population files

  void get_parameters();

  void setup();

  void read_all_populations();

  void read_population(const char* pop_dir, const char* pop_type );

  void get_person_data(char* line, bool gq);

  /**
   * @param args passes to Person ctor; all persons added to the
   * Population must be created through this method
   *
   * @return pointer to the person created and added
   */
  Person* add_person_to_population(int age, char sex, int race, int rel, Place* house,
		     Place* school, Place* work, int day, bool today_is_birthday);


  // accessing members of the populstion

  /**
   * @return the pop_size
   */
  int get_population_size() {
    return this->pop_size;
  }

  /**
   * @param index the index of the Person
   * Return a pointer to the Person object at this index
   */
  Person* get_person(int p) {
    if (p < this->pop_size) {
      return this->people[p];
    }
    else {
      return NULL;
    }
  }

  /**
   * @return a pointer to a random Person in this population
   */
  Person* select_random_person();

  /**
   * @return a pointer to a random Person in this population
   * whose age is in the given range
   */
  Person* select_random_person_by_age(int min_age, int max_age);

  // leaving the population

  /**
   * Perform the necessary steps for an agent's death
   * @param day the simulation day
   * @param person the agent who will die
   */
  void prepare_to_die(int day, Person* person);

  void remove_dead_from_population(int day);

  void prepare_to_migrate(int day, Person* person);

  void remove_migrants_from_population(int day);

  void delete_person_from_population(int day, Person *person);

  // location assignments

  /**
   * Assign agents in Schools to specific Classrooms within the school
   */
  void assign_classrooms();

  /**
   * Assign agents in Workplaces to specific Offices within the workplace
   */
  void assign_offices();

  /**
   * Assign all agents a primary healthcare facility
   */
  void assign_primary_healthcare_facilities();

  // output methods

  /**
   * Report the condition statistics for a given day
   * @param day the simulation day
   */
  void report(int day);

  /**
   * Write degree information to a file degree.txt
   * @param directory the directory where the file will be written
   */
  void get_network_stats(char* directory);

  /**
   *
   */
  void print_age_distribution(char* dir, char* date_string, int run);

  /**
   * Used during debugging to verify that code is functioning properly.
   */
  void quality_control();

  /**
   * Perform end of run operations (clean up)
   */
  void end_of_run();

  void set_school_income_levels();
  void report_mean_hh_income_per_school();
  void report_mean_hh_size_per_school();
  void report_mean_hh_distance_from_school();
  void report_mean_hh_stats_per_income_category();
  void report_mean_hh_stats_per_census_tract();
  void report_mean_hh_stats_per_income_category_per_census_tract();

  void get_age_distribution(int* count_males_by_age, int* count_females_by_age);

  // initialization methods

  void initialize_activities();

  void initialize_demographic_dynamics();

  bool is_load_completed() {
    return this->load_completed;
  }

  void initialize_health_insurance();

  void set_health_insurance(Person* p);

  // periodic updates

  void update_health_interventions(int day);

  void update_demographics(int day);

private:
  std::vector<Person*> people;
  std::vector<Person*> death_list;		  // list of agents to die today
  std::vector<Person*> migrant_list;		  // list of agents to out migrate today
  int pop_size;
  int next_id;

  // used during input
  bool is_initialized;
  bool load_completed;
  int enable_copy_files;
  fred::Mutex mutex;
  void parse_lines_from_stream(std::istream &stream, bool is_group_quarters_pop);
    
  // output data
  int report_initial_population;
  int output_population;
  char pop_outfile[FRED_STRING_SIZE];
  char output_population_date_match[FRED_STRING_SIZE];
  void write_population_output_file(int day);

};

#endif // _FRED_POPULATION_H
