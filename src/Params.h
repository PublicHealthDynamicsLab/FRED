/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File Params.h
//
#ifndef _FRED_PARAMS_H
#define _FRED_PARAMS_H

#define MAX_PARAMS 1000
#define MAX_PARAM_SIZE 1024

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
     * @param s the parameter name
     * @param p a pointer to the int that will be set
     * @return 1 if found
     */
    static int get_param(char *s, int *p);

    /**
     * @param s the parameter name
     * @param p a pointer to the unsigned long that will be set
     * @return 1 if found
     */
    static int get_param(char *s, unsigned long *p);

    /**
     * @param s the parameter name
     * @param p a pointer to the double that will be set
     * @return 1 if found
     */
    static int get_param(char *s, double *p);

    /**
     * @param s the parameter name
     * @param p a pointer to the float that will be set
     * @return 1 if found
     */
    static int get_param(char *s, float *p);

    /**
     * @param s the parameter name
     * @param p a pointer to the c-style string that will be set
     * @return 1 if found
     */
    static int get_param(char *s, char *p);

    /**
     * @param s the parameter name
     * @param p a pointer to the string that will be set
     * @return 1 if found
     */
    static int get_param(char *s, string &p);

    /**
     * Read all of the parameters from a file and store them internally.  This method sets the private static
     * values param_name, param_value, and param_count
     *
     * @param  paramfile the file to read
     * @return 1 if found
     */
    static int read_parameters(char *paramfile);

    /**
     * @param s the parameter name
     * @param p a pointer to the vector of ints that will be set
     * @return 1 if found
     */
    static int get_param_vector(char *s, vector < int > &p);

    /**
     * @param s the parameter name
     * @param p a pointer to the vector of doubles that will be set
     * @return 1 if found
     */
    static int get_param_vector(char *s, vector < double > &p);


    static int get_param_vector(char *s, double *p);

    static int get_param_vector(char *s, int *p);

    /**
     * @param s the parameter name
     * @param p a pointer to the 2D array of doubles that will be set
     * @return 1 if found
     */
    static int get_param_matrix(char *s, double ***p);

    static int get_param_map(char *s, map<string, double> *p);
    static int get_double_indexed_param_map(string s, int index_i, int index_j, map<string, double> *p);
    static int get_indexed_param_map(string s, int index, map<string, double> *p);

    /**
     * @param s the parameter to check
     * @return <code>true</code> if the parameter exists, <code>false</code> if not
     */
    static bool does_param_exist(char *s);

    /**
     * @param s the parameter to check
     * @return <code>true</code> if the parameter exists, <code>false</code> if not
     */
    static bool does_param_exist(string s);

    template <typename T>
    static int get_param_from_string(string s, T *p){
      char st[80];
      sprintf(st,"%s",s.c_str());
      int err = get_param(st,p);
      return err;
    }

    template <typename T>
    static int get_indexed_param(string s, int index, T *p){
      char st[80];
      sprintf(st, "%s[%d]",s.c_str(),index);
      int err = get_param(st,p);
      return err;
    }

    template <typename T>
    static int get_double_indexed_param(string s, int index_i, int index_j, T* p){
      char st[80];
      sprintf(st, "%s[%d][%d]",s.c_str(),index_i,index_j);
      int err = get_param(st,p);
      return err;
    }

    template <typename T>
    static int get_indexed_param_vector(string s, int index, T* p){
      char st[80];
      sprintf(st, "%s[%d]",s.c_str(),index);
      int err = get_param_vector(st,p);
      return err;
    }

    template <typename T>
    static int get_double_indexed_param_vector(string s, int index_i, int index_j, T* p) {
      char st[80];
      sprintf(st, "%s[%d][%d]",s.c_str(),index_i,index_j);
      int err = get_param_vector(st,p);
      return err;
    }

  private:
    static char param_name[][MAX_PARAM_SIZE];
    static char param_value[][MAX_PARAM_SIZE];
    static int param_count;

};

#endif // _FRED_PARAMS_H
