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
// File: Global.h
//
#ifndef _FRED_GLOBAL_H
#define _FRED_GLOBAL_H

#include <algorithm>
#include <assert.h>
#include <bitset>
#include <chrono>
#include <cstring>
#include <err.h>
#include <errno.h>
#include <map>
#include <iostream>
#include <set>
#include <sstream>
#include <stack>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace std;
using namespace std::chrono;

// For multithreading NCPU should be set in the Makefile.  NCPU must be greater
// than or equal to the maximum number of threads that will be detected by OpenMP
// If NCPU > omp_get_max_threads, some memory will be wasted, but it's harmless
// otherwise.
//
// Define NCPU=1 if value not set in Makefile
#ifndef NCPU
#define NCPU 1
#endif

// Size of strings (usually file names)
#define FRED_STRING_SIZE 102400

class Clause;
class Expression;
class Group;
class Group_Type;
class Predicate;
class Network;
class Network_Type;
class Person;
class Place;
class Rule;
class Neighborhood_Layer;
class Regional_Layer;
class Visualization_Layer;

typedef std::vector<int> int_vector_t;
typedef std::vector<double> double_vector_t;
typedef std::vector<std::string> string_vector_t;
typedef std::vector<Rule*> rule_vector_t;
typedef std::vector<Clause*> clause_vector_t;
typedef std::vector<Expression*> expression_vector_t;
typedef std::vector<Predicate*> predicate_vector_t;
typedef std::vector<Person*> person_vector_t;
typedef std::vector<Place*> place_vector_t;
typedef std::vector<Network*> network_vector_t;
typedef std::vector<Network_Type*> network_type_vector_t;
typedef std::vector<Group_Type*> group_type_vector_t;
typedef std::vector<Group*> group_vector_t;
typedef std::vector<std::pair<int,int>> pair_vector_t;

typedef struct {

  char name[FRED_STRING_SIZE];

  // these are inclusive upper bounds for the indicated categories.
  // values that exceed the highest given cutoffs are assigned to the
  // highest category: fifth_quintile, fourth_quartile, or max

  // cutoffs for quintiles
  double first_quintile;
  double second_quintile;
  double third_quintile;
  double fourth_quintile;

  // cutoffs for quartiles
  double first_quartile;
  double second_quartile;
  double third_quartile;

} cutoffs_t;

/**
 * This class contains the static variables used by the FRED program.  The variables all have public access,
 * so they can be modified by any class that uses the <code>Global</code> class.  However, by making them
 * static class variables, the compiler forces the programmer to reference them using the full nomenclature
 * <code>Global::variable_name</code>, which in turn makes it clear for code maintenance where the actual
 * variable resides.
 *
 * The static method <code>get_global_properties</code> is used to set the properties from the program file.
 */
class Global {
public:
  
  // global constants
  static const int DAYS_PER_WEEK = 7;
  static const int ADULT_AGE = 18;
  static const int SCHOOL_AGE = 5;
  static const int RETIREMENT_AGE = 67;
  static const int GRADES = 21;

  // Change this constant and recompile to allow more threads.  For efficiency should be
  // equal to OMP_NUM_THREADS value that will be used.  If OMP_NUM_THREADS greater than
  // MAX_NUM_THREADS is used, FRED will abort the run.
  static const int MAX_NUM_THREADS = NCPU;

  // global simulation variables
  static char Program_file[FRED_STRING_SIZE];
  static char Simulation_directory[FRED_STRING_SIZE];
  static char Plot_directory[FRED_STRING_SIZE];
  static char Visualization_directory[FRED_STRING_SIZE];
  static int Simulation_run_number;
  static unsigned long Simulation_seed;
  static high_resolution_clock::time_point Simulation_start_time;
  static int Simulation_Day;
  static int Simulation_Days;
  static int Simulation_Hour;
  static int Simulation_Step;

  // global runtime properties
  static char Output_directory[];
  static int Quality_control;
  static char ErrorLogbase[];
  static bool Track_age_distribution;
  static bool Track_network_stats;
  static int Verbose;
  static int Debug;
  static int Test;
  static int Reseed_day;
  static unsigned long Seed;
  static int Use_FRED;
  static int Use_rules;
  static int Compile_FRED;
  static int Max_Loops;

  //Boolean flags
  static bool Enable_Profiles;
  static bool Enable_Records;
  static bool Enable_Var_Records;
  static bool Enable_Transmission_Network;
  static bool Enable_Transmission_Bias;
  static bool Enable_New_Transmission_Model;
  static bool Enable_Hospitals;
  static bool Enable_Health_Insurance;
  static bool Enable_Group_Quarters;
  static bool Enable_Visualization_Layer;
  static int Visualization_Run;
  static int Health_Records_Run;
  static bool Enable_Population_Dynamics;
  static bool Enable_Travel;
  static bool Enable_Local_Workplace_Assignment;
  static bool Enable_Fixed_Order_Condition_Updates;
  static bool Enable_External_Updates;
  static bool Use_Mean_Latitude;
  static bool Report_Serial_Interval;
  static bool Report_County_Demographic_Information;
  static bool Assign_Teachers;
  static bool Report_Contacts;
  static bool Error_found;

  // grid layers
  static Neighborhood_Layer* Neighborhoods;
  static Regional_Layer* Simulation_Region;
  static Visualization_Layer* Visualization;

  // global file pointers
  static FILE* Statusfp;
  static FILE* Birthfp;
  static FILE* Deathfp;
  static FILE* ErrorLogfp;
  static FILE* Recordsfp;

  /**
   * Fills the static variables with values from the program file.
   */
  static void get_global_properties();

};

/* 
 * Put fred's global typedefs in namespace fred.
 */

namespace fred {
  typedef float geo;

#ifdef _OPENMP
  
#include <omp.h>

  struct Mutex {
    Mutex() {
      omp_init_lock(&lock);
    }
      
    ~Mutex() {
      omp_destroy_lock(&lock);
    }
    
    void Lock() {
      omp_set_lock(&lock);
    }
      
    void Unlock() {
      omp_unset_lock(&lock);
    }
   
    Mutex(const Mutex &) {
      omp_init_lock(&lock);
    }
      
    Mutex & operator=(const Mutex &) {
      return *(this);
    }
      
    omp_lock_t lock;
  };

#else
  
  /// Dummy Mutex when _OPENMP not defined
  struct Mutex {
    void Lock() {}
    void Unlock() {}
  };

  static int omp_get_max_threads() {
    return 1; 
  }

  static int omp_get_num_threads() {
    return 1;
  }

  static int omp_get_thread_num() {
    return 0;
  }
#endif
}


#endif // _FRED_GLOBAL_H
