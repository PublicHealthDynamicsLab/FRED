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
// File Property.h
//
#ifndef _FRED_params_H
#define _FRED_params_H

#include "Global.h"

class Rule;

class Property {

public:

  static int UseFRED;

  static bool does_property_exist(string name);

  static void read_psa_program(char* program_file, int line_number);

  static void read_program_file(char* program_file);

  static void process_single_line(string linestr, int linenum, char* program_file);

  static int read_properties(char* program_file);

  static std::string find_property(string name);

  static int get_next_property(string name, string* value, int start);

  static int get_property(string name, bool* value);

  static int get_property(string name, int* value);

  static int get_property(string name, unsigned long* value);

  static int get_property(string name, long long int* value);

  static int get_property(string name, double* value);

  static int get_property(string name, float* value);

  static int get_property(string name, char* value);

  static int get_property(string name, string* p);

  static int get_property(string name, string &value);

  // the following form a property name from the first n-1 args

  template <typename T>
  static int get_property(string s, int index, T* value){
    char name[256];
    sprintf(name, "%s[%d]", s.c_str(), index);
    int found = get_property(name, value);
    return found;
  }

  template <typename T>
  static int get_property(string s, int index_i, int index_j, T* value){
    char name[256];
    sprintf(name, "%s[%d][%d]", s.c_str(), index_i, index_j);
    int found = get_property(name, value);
    return found;
  }

  template <typename T>
  static int get_property(string s1, string s2, T* value){
    char name[256];
    sprintf(name, "%s.%s", s1.c_str(), s2.c_str());
    int found = get_property(name, value);
    return found;
  }

  template <typename T>
  static int get_property(string s1, string s2, string s3, T* value){
    char name[256];
    sprintf(name, "%s.%s.%s", s1.c_str(), s2.c_str(), s3.c_str());
    int found = get_property(name, value);
    return found;
  }

  template <typename T>
  static int get_property(string s1, string s2, string s3, string s4, T* value){
    char name[256];
    sprintf(name, "%s.%s.%s.%s", s1.c_str(), s2.c_str(), s3.c_str(), s4.c_str());
    int found = get_property(name, value);
    return found;
  }

  // property vectors

  /**
   * @property s the program name
   * @property p a pointer to the vector of ints that will be set
   * @return 1 if found
   */
  static int get_property_vector(char* s, vector<int> &p);

  /**
   * @property s the program name
   * @property p a pointer to the vector of doubles that will be set
   * @return 1 if found
   */
  static int get_property_vector(char* s, vector<double> &p);

  /**
   * @property s char* with value to be vectorized
   * @property p a pointer to the vector of doubles that will be set
   * @return 1 if found
   */
  static int get_property_vector_from_string(char *s, vector < double > &p);

  static int get_property_vector(char* s, double* p);

  static int get_property_vector(char* s, int* p);

  static int get_property_vector(char* s, string* p);

  /**
   * @property s the program name
   * @property p a pointer to the 2D array of doubles that will be set
   * @return 1 if found
   */
  static int get_property_matrix(char* s, double*** p);

  template <typename T>
  static int get_indexed_property_vector(string s, int index, T* p){
    char name[256];
    sprintf(name, "%s[%d]",s.c_str(),index);
    int found = get_property_vector(name,p);
    return found;
  }

  template <typename T>
  static int get_indexed_property_vector(string s1, string s2, T* p){
    char name[256];
    sprintf(name, "%s.%s",s1.c_str(),s2.c_str());
    int found = get_property_vector(name,p);
    return found;
  }

  template <typename T>
  static int get_double_indexed_property_vector(string s, int index_i, int index_j, T* p) {
    char name[256];
    sprintf(name, "%s[%d][%d]", s.c_str(), index_i, index_j);
    int found = get_property_vector(name, p);
    return found;
  }

  // determine whether property is required or optional

  static void set_abort_on_failure() {
    abort_on_failure = 1;
  }

  static void disable_abort_on_failure() {
    abort_on_failure = 0;
  }

  static void print_errors(char* filename);
  static void print_warnings(char* filename);
  static void report_parameter_check();
  static int check_properties;
  static pair_vector_t get_edges(string network_name);

private:
  static std::vector<std::string> property_names;
  static std::vector<std::string> property_values;
  static std::vector<std::string> property_not_found;
  static std::vector<std::string> property_file;
  static std::vector<int> property_lineno;
  static std::vector<int> property_is_duplicate;
  static std::vector<int> property_is_used;
  static std::vector<int> property_is_default;
  static std::unordered_map<std::string, int> property_map;
  static int number_of_properties;
  static int abort_on_failure;
  static char PSA_Method[];
  static char PSA_List_File[];
  static int PSA_Sample_Size;
  static int PSA_Sample;
  static int default_properties;
  static std::string error_string;
};

#endif // _FRED_params_H
