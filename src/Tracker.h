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
// File: Tracker.h
//

#ifndef _FRED_TRACKER_H
#define _FRED_TRACKER_H

#include <stdlib.h>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>


#ifdef UNIT_TEST
#define ERROR_PRINT printf
#else 
#include "Utils.h"
#define ERROR_PRINT Utils::fred_abort
#endif

using namespace std;
const string allowed_typenames[] = { "double", "int", "string" };
//#############################

/**
 * The Tracker Class is a class that contains maps that allow one to 
 * log on a daily basis different counts of things throughout FRED
 *
 * It basically contains three maps for integers, doubles, and strings
 * and stores a key, and then allows you to log daily values.
 */

// TO DO - reimplement with the counts being members of a template class
  
template <typename T>
class Tracker {
public:
  /**
   * Default constructor
   */
  Tracker() {
    this->title = "Tracker";
    this->index_name = "Generic Index";
  }
  
  /**
   * Constructor with a Title
   */
  Tracker(string _title, string _index_name) {
    this->title = _title;
    this->index_name = _index_name;
  }

  /**
   * Default Destructor
   */
  ~Tracker(){ };
  
  // Accessors
  string get_title() {
    return this->title;
  }

  string get_index_name(void) {
    return this->index_name;
  }

  T get_index_value(int i) {
    return this->indices[i];
  }
  
  bool is_allowed_type(string type_name) {
    vector<string> atypes = this->_get_allowed_typenames();
    return find(atypes.begin(), atypes.end(), type_name) != atypes.end();
  }

#ifdef UNIT_TEST
  // This member is only used for unit testing annd should not be called publicly
  int get_index_position(T index){
    return this->_index_pos(index);
  }
#endif
  
    
  string has_key(string key) {
    vector<string> aTypes = this->_get_allowed_typenames();
    for(int i=0; i < aTypes.size(); ++i) {
      vector<string> keys = this->_get_keys(aTypes[i]);
      vector<string>::iterator iter_index = find(keys.begin(), keys.end(), key);
      if(iter_index != keys.end()) {
        return aTypes[i];
      }
    }
    return "None";
  }

  // Modifiers
  
  // A new index adds an element to each array for each existing category
  int add_index(T index, bool unique = true, bool hardfail = false) {
    if(unique) {
      if(this->_index_pos(index) != -1) {
	if(hardfail) {
	  ERROR_PRINT("Tracker.h::add_index Trying to add an Index that already exists (hardfail set to true)");
	} else {
	  return this->_index_pos(index);
	}
      }
    }
#pragma omp critical
    { 
      this->indices.push_back(index);
      vector<string> aTypes = _get_allowed_typenames();
      for(int i = 0; i < aTypes.size(); ++i) {
	this->_add_new_index(aTypes[i]);
      }
    }
    return this->indices.size() - 1;
  }
  
  void add_key(string key_name,string TypeName) {
#pragma omp critical
    {
      // Check if key exists
      if(has_key(key_name) != "None") {
        ERROR_PRINT("Tracker.h::add_key::Key %s already exists as Type %s\n",
		    key_name.c_str(), has_key(key_name).c_str());
      }
      this->_add_new_key(key_name, TypeName);
    }
  }
 
  /// STB make sure OMP is taken care of in these.
  void set_index_key_pair(T index, string key_name, int value, bool allow_add = true) {
    int index_position = this->_index_pos(index);
    if(index_position == -1) {
      if(allow_add) {
	      index_position = this->add_index(index);
      } else {
	      stringstream ss;
	      ss << index;
	      ERROR_PRINT("Tracker.h::set)index_key_pair(int) there is no index %s\n", ss.str().c_str());
      }
    }
    
    string key_type = this->has_key(key_name);
    if(key_type != "int") {
      if(allow_add && key_type == "None") {
	      this->add_key(key_name, "int");
      } else {
	      ERROR_PRINT("Tracker.h::set_index_key_pair with int, using a key that is not for integers");
      }
    }
    this->values_map_int[key_name][index_position] = value;
  }
  
  void set_index_key_pair(T index, string key_name, double value, bool allow_add = true){
    int index_position = this->_index_pos(index);
    if(index_position == -1) {
      if(allow_add) {
	      index_position = this->add_index(index);
      } else {
	      stringstream ss;
	      ss << index;
	      ERROR_PRINT("Tracker.h::set_index_key_pair (double) there is no index %s\n", ss.str().c_str());
      }
    }    
    string key_type = this->has_key(key_name);
    if(key_type != "double") {
      if(allow_add && key_type == "None") {
	      this->add_key(key_name, "double");
      } else {
	      ERROR_PRINT("Tracker.h::set_index_key_pair with double, using a key that is not for integers");
      }
    }
    this->values_map_double[key_name][index_position] = value;
  }
  
  void set_index_key_pair(T index, string key_name, string value, bool allow_add = true) {
    int index_position = this->_index_pos(index);
    if(index_position == -1) {
      if(allow_add) {
	      index_position = this->add_index(index);
      } else {
	      stringstream ss;
	      ss << index;
	      ERROR_PRINT("Tracker.h::set_index_key_pair (string) there is no index %s\n", ss.str().c_str());
      }
    }
    
    string key_type = this->has_key(key_name);
    if(key_type != "string") {
      if(allow_add && key_type == "None") {
	      this->add_key(key_name,"string");
      } else {
	      ERROR_PRINT("Tracker.h::set_index_key_pair with string, using a key that is not for integers");
      }
    }
    this->values_map_string[key_name][index_position] = value;
  } 
  
  void increment_index_key_pair(T index, string key_name, int value) {
    int index_position = this->_index_pos(index);
    if(index_position == -1) {
      stringstream ss;
      ss << index;
      ERROR_PRINT("Tracker.h::increment_index_key_pair there is no index %s\n", ss.str().c_str());
    }
    
    string key_type = this->has_key(key_name);
    if(key_type == "None") {
      this->add_key(key_name,"int");
      key_type = "int";
    }
    
    if(key_type != "int") {
      ERROR_PRINT("Tracker.h::increment_index_key_pair, (int) trying to increment a key %s with non integer type\n", key_name.c_str());
    }
#pragma omp atomic
    this->values_map_int[key_name][index_position] += value;
  }

  void increment_index_key_pair(T index, string key_name, double value) {
    int index_position = this->_index_pos(index);
    if(index_position == -1) {
      stringstream ss;
      ss << index;
      ERROR_PRINT("Tracker.h::increment_index_key_pair there is no index %s\n", ss.str().c_str());
    }
    
    string key_type = this->has_key(key_name);
    if(key_type == "None") {
      this->add_key(key_name,"double");
      key_type = "double";
    }
    
    
    if(key_type != "double"){
      ERROR_PRINT("Tracker.h::increment_index_key_pair, (double) trying to increment a key %s with non double type\n", key_name.c_str());
    }
#pragma omp atomic
    this->values_map_double[key_name][index_position] += value;
  }
  
  void increment_index_key_pair(T index, string key_name, string value) {
    ERROR_PRINT("Tracker.h::increment_index_key_pair, trying to increment a key %s with string type\n", key_name.c_str());
  }

  void increment_index_key_pair(T index, string key_name){
    int index_position = this->_index_pos(index);
    if(index_position == -1) {
      stringstream ss;
      ss << index;
      ERROR_PRINT("Tracker.h::increment_index_key_pair there is no index %s\n", ss.str().c_str());
    }
    
    string key_type = this->has_key(key_name);
    if(key_type == "string") {
      this->increment_index_key_pair(index, key_name, "FooBar");
    } else if(key_type == "int") {
      this->increment_index_key_pair(index, key_name, static_cast<int>(1));
    } else if(key_type == "double") {
      this->increment_index_key_pair(index,key_name, static_cast<double>(1.0));
    } else {
      ERROR_PRINT("Tracker.h::increment_index_key_pair, trying to increment a typename that doesn't exist");
    }
  }
  
  //Collective Operations
  void reset_index_all_key_pairs_to_zero(T index) {
    int index_position = this->_index_pos(index);
    
    if(index_position == -1) {
      stringstream ss;
      ss << index;
      ERROR_PRINT("Tracker.h::reset_index_all_key_pairs_to_zero there is no index %s\n",ss.str().c_str());
    }
    
    for(map<string, vector<int> >::iterator iter = this->values_map_int.begin();
	      iter != this->values_map_int.end(); ++iter) {
      (*iter).second[index_position] = 0;
    }
    
    for(map<string, vector<double> >::iterator iter=this->values_map_double.begin();
	      iter != this->values_map_double.end(); ++iter) {
      (*iter).second[index_position] = 0.0;
    }
    
  }

  
  void reset_all_index_key_pairs_to_zero(void) {
    for(int i = 0; i < this->indices.size(); ++i) {
      reset_index_all_key_pairs_to_zero(this->indices[i]);
    }
  }
  
  void set_all_index_for_key(string key_name, int value) {
    for(int i = 0; i < this->indices.size(); ++i) {
      string key_type = this->has_key(key_name);
      if(key_type != "int") {
	      ERROR_PRINT("Tracker.h::set_all_index_for_key, called with an integer and key %s is not an integer value", key_name.c_str());
      }
      this->set_index_key_pair(this->indices[i], key_name, value, false);
    }
  }
  
  void set_all_index_for_key(string key_name, double value) {
    for(int i = 0; i < this->indices.size(); ++i) {
      string key_type = this->has_key(key_name);
      if(key_type != "double") {
	      ERROR_PRINT("Tracker.h::set_all_index_for_key, called with an double and key %s is not an double value", key_name.c_str());
      }
      this->set_index_key_pair(this->indices[i], key_name, value, false);
    }
  }
  
  void set_all_index_for_key(string key_name, string value) {
    for(int i = 0; i < this->indices.size(); ++i){
      string key_type = this->has_key(key_name);
      if(key_type != "string") {
	      ERROR_PRINT("Tracker.h::set_all_index_for_key, called with an string and key %s is not an string value", key_name.c_str());
      }
      this->set_index_key_pair(this->indices[i], key_name, value, false);
    }
  }
      
  //Printers
  string print_key_table(void) {
    stringstream sList;
    
    sList << "Key Table\n";
    sList << "---------------------------------" << std::endl;
    vector<string> atypes = this->_get_allowed_typenames();
    for(int i=0; i < atypes.size(); ++i) {
      vector<string> keys = this->_get_keys(atypes[i]);
      if(keys.size() > 0) {
	      sList << "  " << atypes[i] << " Keys" << std::endl;
	      for(int i = 0; i < keys.size(); ++i) {
	        sList << "\t" << keys[i] << std::endl;
	      }
      }
    }
    return sList.str();
  }   

  string print_key_index_list(string key_name) {
    stringstream returnString;
    
    returnString << "Key: " << key_name << std::endl;
    returnString << "--------------------------------------" << std::endl;
    returnString << "Index\t\tValue" << std::endl;

    string key_type = this->has_key(key_name);
    // Can't figure out how to not do this explicitly yet
    if(key_type == "None") {
      ERROR_PRINT("Tracker.h::print_key_index_list requesting a key %s that does not exist\n", key_name.c_str());
    } else if(key_type == "int") {
      if(this->indices.size() != this->values_map_int[key_name].size()) {
	      ERROR_PRINT("Tracker.h::print_key_index_list there is something wrong with the counts number of indices != number of values for key %s\n", key_name.c_str());
      }
      
      for(int i = 0; i < this->indices.size(); ++i) {
	      returnString << this->indices[i] << "\t\t" << this->values_map_int[key_name][i] << std::endl;
      }
    } else if(key_type == "double") {
      if(this->indices.size() != values_map_double[key_name].size()) {
	      ERROR_PRINT("Tracker.h::print_key_index_list there is something wrong with the counts number of indices != number of values for key %s\n", key_name.c_str());
      }
      
      for(int i = 0; i < this->indices.size(); ++i) {
	      returnString << this->indices[i] << "\t\t" << this->values_map_double[key_name][i] << std::endl;
      }
    } else if(key_type == "string") {
      if(this->indices.size() != this->values_map_string[key_name].size()) {
	      ERROR_PRINT("Tracker.h::print_key_index_list there is something wrong with the counts number of indices != number of values for key %s\n", key_name.c_str());
      }
      
      for(int i = 0; i < this->indices.size(); ++i) {
	      returnString << this->indices[i] << "\t\t" << this->values_map_string[key_name][i] << std::endl;
      }
    } else {
      ERROR_PRINT("Tracker.h::print_key_index_list called with an unrecognized typename for key %s", key_name.c_str());
    }
    
    returnString << "--------------------------------------" << std::endl;

    return returnString.str();
    
  }    
  
  string print_inline_report_format_for_index(T index) {
    int index_pos = this->_index_pos(index);
    if(index_pos == -1) {
      ERROR_PRINT("Tracker.h::print_inline_report_format_for_index asked for index that does not exist");
    }
    stringstream returnStringSt;

    if (index == 0) {
      returnStringSt << this->index_name;
    
      for(map<string, vector<string> >::iterator iter = this->values_map_string.begin();
	  iter != this->values_map_string.end(); ++iter) {
	returnStringSt << "," << (*iter).first;
      }
      for(map<string, vector<int> >::iterator iter = this->values_map_int.begin();
	  iter != this->values_map_int.end(); ++iter){
	returnStringSt << "," << (*iter).first;
      }
      for(map<string, vector<double> >::iterator iter = this->values_map_double.begin();
	  iter != this->values_map_double.end(); ++iter){
	returnStringSt << "," << (*iter).first;
      }
      returnStringSt << "\n";
    }

    returnStringSt << index;
    
    for(map<string, vector<string> >::iterator iter = this->values_map_string.begin();
	      iter != this->values_map_string.end(); ++iter) {
      returnStringSt << "," << (*iter).second[index_pos];
    }
    for(map<string, vector<int> >::iterator iter = this->values_map_int.begin();
	      iter != this->values_map_int.end(); ++iter){
      returnStringSt << "," << (*iter).second[index_pos];
    }
    for(map<string, vector<double> >::iterator iter = this->values_map_double.begin();
	      iter != this->values_map_double.end(); ++iter){
      returnStringSt <<  "," << setprecision(2) << fixed << (*iter).second[index_pos];
    }
    
    string returnString = returnStringSt.str();
    returnString.append("\n");
    
    return returnString;
  }
  
  void output_inline_report_format_for_index(T index, ostream &stream) {
    stream << print_inline_report_format_for_index(index);
  }
  
  void output_inline_report_format_for_index(T index, FILE* outfile) {
    fprintf(outfile, "%s", print_inline_report_format_for_index(index).c_str());
    fflush(outfile);
  }
 
  string print_inline_report_format(void) {
    stringstream returnString;
    
    for(int i = 0; i < this->indices.size(); ++i) {
      returnString << print_inline_report_format_for_index(this->indices[i]);    
    }
    return returnString.str();
  }  
  void output_inline_report_format(ostream& stream) {
    stream << print_inline_report_format();
  }
  
  void output_inline_report_format(FILE* outfile) {
    fprintf(outfile, "%s", print_inline_report_format().c_str());
    fflush(outfile);
  }
  
  string print_csv_report_format_for_index(T index) {
    int index_pos = this->_index_pos(index);
    if(index_pos == -1) {
      ERROR_PRINT("Tracker.h::print_csv_report_format_for_index asked for index that does not exist");
    }
    
    stringstream returnString;
    returnString << index ;
    for(map<string, vector<string> >::iterator iter = this->values_map_string.begin();
	      iter != this->values_map_string.end(); ++iter) {
      returnString << "," << (*iter).second[index_pos];
    }
    for(map<string, vector<int> >::iterator iter = this->values_map_int.begin();
	      iter != this->values_map_int.end(); ++iter){
      returnString << "," << (*iter).second[index_pos];
    }
    for(map<string, vector<double> >::iterator iter = this->values_map_double.begin();
	      iter != this->values_map_double.end(); ++iter) {
      returnString << "," << (*iter).second[index_pos];
    }
    returnString << "\n";

    return returnString.str();
  }
  
  void output_csv_report_format_for_index(T index, ostream& stream) {
    stream << print_csv_report_format_for_index(index);
  }
  
  void output_csv_report_format_for_index(T index, FILE* outfile) {
    fprintf(outfile, "%s", print_csv_report_format_for_index(index).c_str());
    fflush(outfile);
  }
  
  string print_csv_report_format_header(void) {
    stringstream returnString;
    
    returnString << this->index_name;
    for(map<string, vector<string> >::iterator iter = this->values_map_string.begin();
	      iter != this->values_map_string.end(); ++iter) {
      returnString << "," << (*iter).first;
      
    }
    for(map<string, vector<int> >::iterator iter = this->values_map_int.begin();
	      iter != this->values_map_int.end(); ++iter){
      returnString << "," << (*iter).first;
    }
    for(map<string, vector<double> >::iterator iter = this->values_map_double.begin();
	      iter != this->values_map_double.end(); ++iter){
      returnString << "," << (*iter).first;
    }
    returnString << "\n";

    return returnString.str();
  }
  
  void output_csv_report_format_header(FILE* outfile){
    fprintf(outfile, "%s", print_csv_report_format_header().c_str());
    fflush(outfile);
  }
  
  void output_csv_report_format(FILE* outfile){
    output_csv_report_format_header(outfile);
    for(int i = 0; i < this->indices.size(); ++i) {
      output_csv_report_format_for_index(this->indices[i], outfile);
    }
    fflush(outfile);
  }
  
private:
  //Private Variables
  string title;
  string index_name;
  vector<T> indices;
  map<string, vector<int> > values_map_int;
  map<string, vector<string> > values_map_string;
  map<string, vector<double> > values_map_double;

  vector<string> _get_allowed_typenames(void) {
    vector<string> aTypes(allowed_typenames, allowed_typenames + 3);
    return aTypes;
  }

  void _add_new_index(string TypeName){
    if(this->is_allowed_type(TypeName) == false) {
      ERROR_PRINT("Tracker.h::_add_new_index has been called with unsupported TypeName %s, use double, int, or string\n",
		  TypeName.c_str());
    }
     
    if(TypeName == "int") {
      if(this->values_map_int.size() > 0) {
	      for(map< string, vector<int> >::iterator iter = this->values_map_int.begin();
	          iter != this->values_map_int.end(); ++iter){
	        (*iter).second.push_back(0);
	      }
      }
    } else if(TypeName == "double") {
      if(this->values_map_double.size() > 0) {
	      for(map< string, vector<double> >::iterator iter = this->values_map_double.begin();
	          iter != this->values_map_double.end(); ++iter) {
	        (*iter).second.push_back(0);
	      }
      }
    } else if(TypeName  == "string") {
      if(this->values_map_string.size() > 0) {
	      for(map< string, vector<string> >::iterator iter = this->values_map_string.begin();
	          iter != this->values_map_string.end(); ++iter) {
	        (*iter).second.push_back(" ");
	      }
      }
    } else {
      ERROR_PRINT("Tracker.h::add_new_index has been called with unsupported TypeName %s, use double, int, or string\n",
		  TypeName.c_str());
    }
  }
	      
  void _add_new_key(string key_name,string TypeName) {
    if(this->is_allowed_type(TypeName) == false) {
      ERROR_PRINT("Tracker.h::_add_new_keys has been called with unsupported TypeName %s, use double, int, or string\n",
		  TypeName.c_str());
    }
    
    if(TypeName == "int") {
      for(int i =0; i < this->indices.size(); ++i) {
	      this->values_map_int[key_name].push_back(0);
      }
    } else if(TypeName == "double") {
      for(int i =0; i < this->indices.size(); ++i) {
	      this->values_map_double[key_name].push_back(0.0);
      }
    } else if(TypeName == "string") {
      for(int i =0; i < this->indices.size(); ++i) {
	      this->values_map_string[key_name].push_back("A String");
      }
    } else {
      ERROR_PRINT("Tracker.h::_add_new_key got a type name %s for key %s it doesn't know how to handle (use int,double,or string)",
		  key_name.c_str(), TypeName.c_str());
    }
  }
  
  int _index_pos(T index) {
    typename vector<T>::iterator iter_index;
    
    //return index;
    iter_index = find(this->indices.begin(), this->indices.end(), index);
    if(iter_index != this->indices.end()) {
      return distance(this->indices.begin(), iter_index);
    } else {
      return -1;
    }
  }
  
  vector<string> _get_keys(string TypeName) {
    if(this->is_allowed_type(TypeName) == false) {
      ERROR_PRINT("Tracker.h::_get_keys has been called with unsupported TypeName %s, use double, int, or string\n",
		  TypeName.c_str());
    }
    
    vector<string> return_vec;
    if(TypeName == "int") {
      for(map<string, vector<int> >::iterator iter = this->values_map_int.begin();
	        iter != this->values_map_int.end(); ++iter) {
        return_vec.push_back((*iter).first);
      }
    } else if (TypeName == "double") {
      for(map<string, vector<double> >::iterator iter = this->values_map_double.begin();
	        iter != values_map_double.end(); ++iter) {
        return_vec.push_back((*iter).first);
      }
    } else if(TypeName == "string") {
      for(map < string, vector< string > >::iterator iter = this->values_map_string.begin();
	        iter != this->values_map_string.end(); ++iter) {
        return_vec.push_back((*iter).first);
      }
    } else {
      ERROR_PRINT("Tracker.h::_get_keys called with an unrecognized TypeName %s\n", TypeName.c_str());
    }
    return return_vec;
  }  
};

#endif
     

