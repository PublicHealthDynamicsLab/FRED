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
// File: Population.h
//

#ifndef _FRED_POPULATION_H
#define _FRED_POPULATION_H

#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <new>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>
#include "Bloque.h"
#include "Demographics.h"
#include "Global.h"
#include "Utils.h"

using namespace std;

class Person;
class Disease;
class Antivirals;
class AV_Manager;
class Vaccine_Manager;
class Place;



class Person_Init_Data;

class Population {

public:

  /**
   * Default constructor
   */
  Population();
  ~Population();

  /**
   * Sets the static variables for the class from the parameter file.
   */
  void get_parameters();

  /**
   * Prepare this Population (calls read_population)
   * @see Population::read_population()
   */
  void setup();

  /**
   * Used during debugging to verify that code is functioning properly.
   */
  void quality_control();

  /**
   * Perform end of run operations (clean up)
   */
  void end_of_run();

  /**
   * Report the disease statistics for a given day
   * @param day the simulation day
   */
  void report(int day);

  /**
   * @return the pop_size
   */
  int get_pop_size() {
    return this->pop_size;
  }

  //Mitigation Managers
  /**
   * @return a pointer to this Population's AV_Manager
   */
  AV_Manager* get_av_manager(){
    return this->av_manager;
  }

  /**
   * @return a pointer to this Population's Vaccine_Manager
   */
  Vaccine_Manager* get_vaccine_manager() {
    return this->vacc_manager;
  }

  /**
   * @param args passes to Person ctor; all persons added to the
   * Population must be created through this method
   *
   * @return pointer to the person created and added
   */
  Person* add_person(int age, char sex, int race, int rel, Place* house,
		     Place* school, Place* work, int day, bool today_is_birthday);

  /**
   * @param per a pointer to the Person to remove from the Population
   */
  void delete_person(Person* per);

  /**
   * Perform the necessary steps for an agent's death
   * @param day the simulation day
   * @param person the agent who will die
   */
  void prepare_to_die(int day, Person* person);

  void remove_dead_from_population(int day);

  void remove_dead_person_from_population(int day, Person *person);

  /**
   * @param index the index of the Person
   * Return a pointer to the Person object at this index
   */
  Person* get_person_by_index(int index);

  size_t get_index_size() {
    return this->blq.get_index_size();
  }

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

  /**
   * Write degree information to a file degree.txt
   * @param directory the directory where the file will be written
   */
  void get_network_stats(char* directory);

  void split_synthetic_populations_by_deme();

  void read_all_populations();

  void read_population(const char* pop_dir, const char* pop_id, const char* pop_type );

  /**
   *
   */
  void print_age_distribution(char* dir, char* date_string, int run);

  /**
   * @return a pointer to a random Person in this population
   */
  Person* select_random_person();

  /**
   * @return a pointer to a random Person in this population
   * whose age is in the given range
   */
  Person* select_random_person_by_age(int min_age, int max_age);

  void set_school_income_levels();
  void report_mean_hh_income_per_school();
  void report_mean_hh_size_per_school();
  void report_mean_hh_distance_from_school();
  void report_mean_hh_stats_per_income_category();
  void report_mean_hh_stats_per_census_tract();
  void report_mean_hh_stats_per_income_category_per_census_tract();

  //  //TODO REMOVE
  //  void print_HAZEL_data();
  
  int size() {
    assert(this->blq.size() == this->pop_size);
    return this->blq.size();
  }

  void get_age_distribution(int* count_males_by_age, int* count_females_by_age);

  const std::vector<Utils::Tokens> &get_demes() {
    return this->demes;
  }

  void initialize_demographic_dynamics();

  bool is_load_completed() {
    return this->load_completed;
  }

  //Mitigation Managers
  AV_Manager* av_manager;
  Vaccine_Manager* vacc_manager;

  void update_health_interventions(int day);

private:

  bool load_completed;

  void mother_gives_birth(int day, Person* mother);

  struct PopFileColIndex {
    // all populations
    static const int p_id = 0;
    static const int home_id = 1; // <-- this is either hh_id or gq_id
    int sporder;
    int age_str;
    int sex_str;
    int workplace_id; // <-- same as home_id for gq pop
    int number_of_columns;
    // only synth_people population
    int serial_no;
    int stcotrbg;
    int race_str;
    int relate;
    int school_id;
    // only synth_gq_people population
    int gq_type;
  };

  struct HH_PopFileColIndex : PopFileColIndex {
    HH_PopFileColIndex() {
      serial_no = 2;
      stcotrbg = 3;
      age_str = 4;
      sex_str = 5;
      race_str = 6;
      sporder = 7;
      relate = 8;
      school_id = 9;
      workplace_id = 10;
      number_of_columns = 11;
    }
  } hh_pop_file_col_index;

  struct GQ_PopFileColIndex : PopFileColIndex {
    GQ_PopFileColIndex() {
      gq_type = 2;
      sporder = 3;
      age_str = 4;
      sex_str = 5;
      workplace_id = 1; // <-- same as home_id
      number_of_columns = 6;
    }
  } gq_pop_file_col_index;

  struct GQ_PopFileColIndex_2010_ver1 : PopFileColIndex {
    GQ_PopFileColIndex_2010_ver1() {
      sporder = 2;
      age_str = 3;
      sex_str = 4;
      workplace_id = 1; // <-- same as home_id
      number_of_columns = 5;
    }
  } gq_pop_file_col_index_2010_ver1;

  const PopFileColIndex &get_pop_file_col_index(bool is_group_quarters, bool is_2010_ver1) const {
    if(is_group_quarters) {
      if(is_2010_ver1) {
	return this->gq_pop_file_col_index_2010_ver1;
      } else {
	return this->gq_pop_file_col_index;
      }
    } else {
      return this->hh_pop_file_col_index;
    }
  }

  std::vector<Utils::Tokens> demes;

  void parse_lines_from_stream(std::istream &stream, bool is_group_quarters_pop);

  Person_Init_Data get_person_init_data(char* line,
					bool is_group_quarters_population,
					bool is_2010_ver1_format);


  bloque<Person, fred::Pop_Masks> blq;   // all Persons in the population
  vector<Person*> death_list;		  // list of agents to die today
  int pop_size;

  int enable_copy_files;
    
  static char profilefile[FRED_STRING_SIZE];
  static char pop_outfile[FRED_STRING_SIZE];
  static char output_population_date_match[FRED_STRING_SIZE];
  static int output_population;
  static bool is_initialized;
  static int next_id;

  /**
   * Write out the population in a format similar to the population input files (with additional runtime information)
   * @param day the simulation day
   */
  void write_population_output_file(int day);

  // functor for health interventions (vaccination & antivirals) updates
  struct Update_Health_Interventions {
    int day;
    Update_Health_Interventions(int d) : day(d) { }
    void operator() (Person &p );
  };
    
  // functor for prepare activities
  struct Prepare_Population_Activities {
    int day;
    Prepare_Population_Activities(int d) : day(d) { }
    void operator() (Person &p);
  };

  // functor for behavior setup
  struct Setup_Population_Behavior {
    void operator() (Person &p);
  };

  // functor for Health Insurance setup
  struct Setup_Population_Health_Insurance {
    void operator() (Person &p);
  };

  // functor for behavior updates
  struct Update_Population_Behaviors {
    int day;
    Update_Population_Behaviors(int d) : day(d) { }
    void operator() (Person &p);
  };

  struct infectious_sampler {
    double prob;
    vector<Person*>* samples;
    void operator() (Person &p);
  };

  fred::Mutex mutex;
  fred::Mutex add_person_mutex;
  fred::Mutex batch_add_person_mutex;

};

struct Person_Init_Data {

  char house_label[32], school_label[32], work_label[32];
  char label[32];
  int age, race, relationship;
  char sex;
  bool today_is_birthday;
  int day;
  Place* house;
  Place* work;
  Place* school;
  bool in_grp_qrtrs;
  char gq_type;

  Person_Init_Data() {
    default_initialization();
  }

  Person_Init_Data(int _age, int _race, int _relationship,
		   char _sex, bool _today_is_birthday, int _day) {

    default_initialization();
    age = _age;
    race = _race;
    relationship = _relationship;
    sex = _sex;
    today_is_birthday = _today_is_birthday;
    day = _day;
  }

  void default_initialization() {
    this->house = NULL;
    this->work = NULL;
    this->school = NULL;
    strcpy(this->label, "-1");
    strcpy(this->house_label, "-1");
    strcpy(this->school_label, "-1");
    strcpy(this->work_label, "-1");
    this->age = -1;
    this->race = -1;
    this->relationship = -1;
    this->sex = -1;
    this->day = 0;
    this->today_is_birthday = false;
    this->in_grp_qrtrs = false;
    this->gq_type = ' ';
  }

  const std::string to_string() const {
    std::stringstream ss;
    //ss << setw( 8 ) << setfill( ' ' ); 
    ss << "Person Init Data:"
       << " label " << this->label
       << " age " << this->age
       << " race " << this->race
       << " relationship " << this->relationship
       << " today_is_birthday? " << this->today_is_birthday
       << " day " << this->day
       << " house_label " << this->house_label
       << " work_label " << this->work_label
       << " school_label " << this->school_label
       << " in_group_quarters? " << this->in_grp_qrtrs;

    return ss.str();
  }

};

#endif // _FRED_POPULATION_H
