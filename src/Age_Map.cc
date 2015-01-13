/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: AgeMap.cpp
//

#include <iostream>
#include <iomanip>

#include "Age_Map.h"
#include "Params.h"
#include "Utils.h"

using namespace std;

Age_Map::Age_Map() {}

Age_Map::Age_Map(string name) {
  Name = name + " Age Map";
}

void Age_Map::read_from_input(string Input, int i){
  stringstream Input_string;
  Input_string << Input << "[" << i << "]";  
  this->read_from_input(Input_string.str());
}

void Age_Map::read_from_input(string Input, int i, int j){
  stringstream Input_string;
  Input_string << Input << "[" << i << "][" << j << "]";
  this->read_from_input(Input_string.str());
}


// will leave this interface in, as it will automatically determine whether the 
// input variable is indexed or not.
void Age_Map::read_from_input(string Input) {
  Name = Input + " Age Map";
  
  char ages_string[255];
  char values_string[255];
  
  if(Input.find("[") != string::npos) {
    // Need Special parsing if this is an array from input
    // Allows Disease specific values
    string input_tmp;
    string number;
    size_t found = Input.find_first_of("[");
    size_t found2 = Input.find_last_of("]");
    input_tmp.assign(Input.begin(),Input.begin()+found);
    number.assign(Input.begin()+found+1,Input.begin()+found2);
    sprintf(ages_string,"%s_ages[%s]",input_tmp.c_str(),number.c_str());
    sprintf(values_string,"%s_values[%s]",input_tmp.c_str(),number.c_str());
  }
  else {
    sprintf(ages_string,"%s_ages",Input.c_str());
    sprintf(values_string,"%s_values",Input.c_str());
  }
  
  vector < int > ages_tmp;
  int na  = Params::get_param_vector(ages_string,ages_tmp);
  
  if(na % 2) {
    Utils::fred_abort("Error parsing Age_Map: %s: Must be an even number of age entries\n", Input.c_str()); 
  }
  
  for(int i=0;i<na; i+=2){
    vector <int> ages_tmp2(2);
    ages_tmp2[0] = ages_tmp[i];
    ages_tmp2[1] = ages_tmp[i+1];
    ages.push_back( ages_tmp2 );
  }
  
  Params::get_param_vector(values_string,values);
  
  if(quality_control() != true)
    Utils::fred_abort("");
  return;
}

int Age_Map::get_minimum_age(void) const {
  int min = 9999999;
  for(unsigned int i=0;i<ages.size();i++){
    if(ages[i][0] < min) min = ages[i][0];
  }
  return min;
}

int Age_Map::get_maximum_age(void) const {
  int max = -1;
  for(unsigned int i=0; i< ages.size();i++){
    if(ages[i][1] > max) max = ages[i][1];
  }
  return max;
}

void Age_Map::add_value(int lower_age, int upper_age,double val) {
  vector < int > ages_tmp(2);
  ages_tmp[0] = lower_age;
  ages_tmp[1] = upper_age;
  ages.push_back(ages_tmp);
  values.push_back(val);  
}


double Age_Map::find_value(int age) const {
  
  //  if(age >=0 && age < ages[0]) return values[0];
  for(unsigned int i=0;i<values.size();i++)
    if(age >= ages[i][0] && age <= ages[i][1])
      return values[i];
  
  return 0.0;
}


void Age_Map::print() const {
  cout << "\n" << Name << "\n";
  for(unsigned int i=0;i<ages.size();i++){
    cout << "age "<< setw(2) << ages[i][0] << " to " << setw(3) << ages[i][1] << ": " 
    << setw(7) << setprecision(4) << fixed << values[i] << "\n";
  }
  cout << "\n";
}

bool Age_Map::quality_control() const {
  // First check to see there are a proper number of values for each age
  if(ages.size()!=values.size()){
    cout <<"Help! Age_Map: " << Name << ": Must have the same number of age groups and values\n";
    cout <<"Number of Age Groups = " << ages.size() << "  Number of Values = " << values.size() << "\n";
    return false;
  }
  
  // Next check that the ages groups are correct, the low and high ages are right
  for(unsigned int i=0; i < ages.size(); i++) {
    if(ages[i][0] > ages[i][1] ) {
      cout <<"Help! Age_Map: " << Name << ": Age Group " << i << " invalid, low age higher than high\n";
      cout <<" Low Age = "<< ages[i][0] << "  High Age = "<< ages[i][1] << "\n";
      return false;
    }
  }
  
  // Make sure the age groups are mutually exclusive
  for(unsigned int i=0; i < ages.size(); i++) {
    int lowage = ages[i][0];
    int highage = ages[i][1];
    
    for(unsigned int j=0;j<ages.size();j++) {
      if(j!=i){
        if((lowage >= ages[j][0] && lowage <= ages[j][1]) ||
           (highage >= ages[j][0] && highage <= ages[j][1])) {
          cout << "Help! Age_Map: age group " << i << " not mutually exclusive with age group " << j << "\n";
          cout << lowage << " " << highage << " " << ages[j][0] << " " << ages[j][1] << "\n";
          return false;
        }
      }
    }
  }
  
  return true;
}
