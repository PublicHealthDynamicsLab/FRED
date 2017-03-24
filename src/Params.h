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
// File Params.h
//
#ifndef _FRED_PARAMS_H
#define _FRED_PARAMS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>

using namespace std;

/**
 * This class contains the static methods used by the FRED program to access parameters. By making them
 * static class methods, the compiler forces the programmer to reference them using the full nomenclature
 * <code>Params::method_name()</code>, which in turn makes it clear for code maintenance where the actual
 * method resides.
 *
 */
class Params {

public:

  /**
   * @param s the parameter to check
   * @return <code>true</code> if the parameter exists, <code>false</code> if not
   */
  static bool does_param_exist(string name);

  static void read_psa_parameter(char* paramfile, int line_number);

  static void read_parameter_file(char* paramfile);

  /**
   * Read all of the parameters from a file and store them in vectors
   * param_names and param_values.
   *
   * @param  paramfile the file to read
   * @return 1 if found
   */
  static int read_parameters(char* paramfile);

  /**
   * @param s the parameter name
   * @param p a pointer to the int that will be set
   * @return 1 if found
   */
  static int get_param(string name, int* value);

  /**
   * @param s the parameter name
   * @param p a pointer to the unsigned long that will be set
   * @return 1 if found
   */
  static int get_param(string name, unsigned long* value);

  /**
   * @param s the parameter name
   * @param p a pointer to the double that will be set
   * @return 1 if found
   */
  static int get_param(string name, double* value);

  /**
   * @param s the parameter name
   * @param p a pointer to the float that will be set
   * @return 1 if found
   */
  static int get_param(string name, float* value);

  /**
   * @param s the parameter name
   * @param p a pointer to the c-style string that will be set
   * @return 1 if found
   */
  static int get_param(string name, char* value);

  /**
   * @param s the parameter name
   * @param p a pointer to the string that will be set
   * @return 1 if found
   */
  static int get_param(string name, string &value);

  // the following form a param name from the first n-1 args

  template <typename T>
  static int get_param(string s, int index, T* value){
    char name[256];
    sprintf(name, "%s[%d]", s.c_str(), index);
    int found = get_param(name, value);
    return found;
  }

  template <typename T>
  static int get_param(string s, int index_i, int index_j, T* value){
    char name[256];
    sprintf(name, "%s[%d][%d]", s.c_str(), index_i, index_j);
    int found = get_param(name, value);
    return found;
  }

  template <typename T>
  static int get_param(string s1, string s2, T* value){
    char name[256];
    sprintf(name, "%s.%s", s1.c_str(), s2.c_str());
    int found = get_param(name, value);
    return found;
  }

  template <typename T>
  static int get_param(string s1, string s2, string s3, T* value){
    char name[256];
    sprintf(name, "%s.%s.%s", s1.c_str(), s2.c_str(), s3.c_str());
    int found = get_param(name, value);
    return found;
  }

  template <typename T>
  static int get_param(string s1, string s2, string s3, string s4, T* value){
    char name[256];
    sprintf(name, "%s.%s.%s.%s", s1.c_str(), s2.c_str(), s3.c_str(), s4.c_str());
    int found = get_param(name, value);
    return found;
  }

  // param vectors

  /**
   * @param s the parameter name
   * @param p a pointer to the vector of ints that will be set
   * @return 1 if found
   */
  static int get_param_vector(char* s, vector<int> &p);

  /**
   * @param s the parameter name
   * @param p a pointer to the vector of doubles that will be set
   * @return 1 if found
   */
  static int get_param_vector(char* s, vector<double> &p);

  /**
   * @param s char* with value to be vectorized
   * @param p a pointer to the vector of doubles that will be set
   * @return 1 if found
   */
  static int get_param_vector_from_string(char *s, vector < double > &p);

  static int get_param_vector(char* s, double* p);

  static int get_param_vector(char* s, int* p);

  static int get_param_vector(char* s, string* p);

  /**
   * @param s the parameter name
   * @param p a pointer to the 2D array of doubles that will be set
   * @return 1 if found
   */
  static int get_param_matrix(char* s, double*** p);

  template <typename T>
  static int get_indexed_param_vector(string s, int index, T* p){
    char name[256];
    sprintf(name, "%s[%d]",s.c_str(),index);
    int found = get_param_vector(name,p);
    return found;
  }

  template <typename T>
  static int get_indexed_param_vector(string s1, string s2, T* p){
    char name[256];
    sprintf(name, "%s.%s",s1.c_str(),s2.c_str());
    int found = get_param_vector(name,p);
    return found;
  }

  template <typename T>
  static int get_double_indexed_param_vector(string s, int index_i, int index_j, T* p) {
    char name[256];
    sprintf(name, "%s[%d][%d]", s.c_str(), index_i, index_j);
    int found = get_param_vector(name, p);
    return found;
  }

  // determine whether param is required or optional

  static void set_abort_on_failure() {
    abort_on_failure = 1;
  }

  static void disable_abort_on_failure() {
    abort_on_failure = 0;
  }

private:
  static std::vector<std::string> param_names;
  static std::vector<std::string> param_values;
  static int abort_on_failure;
};

#endif // _FRED_PARAMS_H
