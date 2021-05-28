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
// File: Fred_API.cc
//

#include <stdio.h>
#include <assert.h>
#include <vector>
#include <string>
#include <cstring>

#include "Global.h"

using namespace std;

// include default RNG
#include <stdlib.h>
double get_urand() {
  double val = random();
  return val / (1.0 * RAND_MAX);
}


void update();


// keys
std::vector<std::string> keys;

// values
std::vector<std::string> values;


double get_value(string key) {
  int size = keys.size();
  for (int i = 0; i < size; i++) {
    if (keys[i] == key) {
      double val;
      if (key == "sex") {
	char chr;
	sscanf(values[i].c_str(), "%c", &chr);
	val = (chr == 'M');
      }
      else {
	sscanf(values[i].c_str(), "%lf", &val);
      }
      return val;
    }
  }
  return 0.0;
}


void set_value(string key, double val) {
  int size = keys.size();
  for (int i = 0; i < size; i++) {
    if (keys[i] == key) {
      char vstr[64];
      sprintf(vstr, "%f", val);
      values[i] = string(vstr);
      return;
    }
  }
  return;
}


char dir[FRED_STRING_SIZE];

int main(int argc, char* argv[]) {

  // read working directory from command line
  if(argc > 1) {
    strcpy(dir, argv[1]);
  } else {
    strcpy(dir, "");
  }
  // printf("dir = |%s|\n", dir);

  // requests filename
  char requests[FRED_STRING_SIZE];
  sprintf(requests, "%s/requests", dir);

  // open request file
  FILE* reqfp = fopen(requests, "r");
  if (reqfp == NULL) {
    //abort
  }

  char next_filename[FRED_STRING_SIZE];
  char requestfile[FRED_STRING_SIZE];
  
  while (fscanf(reqfp, "%s", next_filename)==1) {

    // request filename
    sprintf(requestfile, "%s/%s", dir, next_filename);

    // open request file
    FILE* fp = fopen(requestfile, "r");
    if (fp == NULL) {
      // abort
    }

    // clear keys and values
    keys.clear();
    values.clear();

    char key[FRED_STRING_SIZE];
    char val[FRED_STRING_SIZE];
    while (fscanf(fp, "%s = %s ", key, val) == 2) {
      keys.push_back(string(key));
      values.push_back(string(val));
      // printf("%s == %s\n", key,val);
    }
    fclose(fp);
    if (keys.size() == 0) {
      continue;
    }

    update();

    // set results filename
    char results_file[FRED_STRING_SIZE];
    int id = get_value("person");
    sprintf(results_file, "%s/results.%d", dir, id);
    
    // open results file
    fp = fopen(results_file, "w");
    int size = keys.size();
    for (int i = 0; i < size; i++) {
      fprintf(fp, "%s = %s\n", keys[i].c_str(), values[i].c_str());
    }
    fclose(fp);
  }

  // send ready signal
  char ready_file[FRED_STRING_SIZE];
  sprintf(ready_file, "%s/results_ready", dir);
  FILE* fp = fopen(ready_file, "w");
  fclose(fp);

  return 0;
}


////////////////////////////////////////////////////////
//
// DO NOT MODIFY ANY CODE ABOVE THIS POINT
//
// MODIFY THE FOLLOWING AS NEEDED TO UPDATE VALUES OF
// ANY PERSONAL VARIABLES
//
////////////////////////////////////////////////////////


void update() {

  // select FRED variables using format CONDITION.variable
  // and get current values
  int day = get_value("day");
  int id = get_value("person");
  int age = get_value("age");
  double vload = get_value("HIV.vload");
  double cd4 = get_value("HIV.cd4");

  // update as needed
  // (any variables not explicitly set retain their old values)

  if (cd4 == 0) {
    // set initial value for CD4 count and viral load
    // (replace this with a call to the external model api)
    set_value("HIV.cd4", 500.0);
    set_value("HIV.vload", 10000.0);

  }
  else {
    // update value for CD4 count and viral load
    // (replace this with a call to the external model api)
    set_value("HIV.cd4", get_value("HIV.cd4")*0.99);
    set_value("HIV.vload", get_value("HIV.voad")*1.01);
  }
  
  // set random seed based on day and person id
  srandom(1 + id + day);

  // begin HAART with some probability
  double r = get_urand();
  if (r < 0.1) {
    set_value("HIV.begin", 1.0);
  }

  // assume case fatality is 1% per month
  r = get_urand();
  if (r < 0.01) {
    set_value("HIV.case_fatality", 1.0);
  }
  else {
    // assume other fatality is (0.025 * age)% per month
    r = get_urand();
    if (r < 0.00025*age) {
      set_value("HIV.other_fatality", 1.0);
    }
  }

  // debugging
  /*
    char filename[FRED_STRING_SIZE];
    sprintf(filename, "%s/rands", dir);
    FILE* fp = fopen(filename, "a");
    fprintf(fp, "day = %d r = %f\n", day, r);
    fclose(fp);
  */
  
}
