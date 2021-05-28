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
// File: Demographics.cc
//
#include <limits>

#include "Demographics.h"
#include "Property.h"
#include "Person.h"
#include "Global.h"
#include "Date.h"
#include "Utils.h"

class Global;
bool Demographics::enable_aging = false;

// birth and death stats
int Demographics::births_today = 0;
int Demographics::births_ytd = 0;
int Demographics::total_births = 0;
int Demographics::deaths_today = 0;
int Demographics::deaths_ytd = 0;
int Demographics::total_deaths = 0;
std::vector<int> Demographics::admin_codes;

void Demographics::initialize_static_variables() {

  Property::disable_abort_on_failure();
  Property::get_property("enable_aging", &Demographics::enable_aging);
  Property::set_abort_on_failure();

  // create file pointers if needed
  if(Global::Enable_Population_Dynamics || Demographics::enable_aging) {
    int run = Global::Simulation_run_number;
    char filename[FRED_STRING_SIZE];
    char directory[FRED_STRING_SIZE];
    sprintf(directory, "%s/RUN%d", Global::Simulation_directory, run);

    sprintf(filename, "%s/births.txt", directory);
    Global::Birthfp = fopen(filename, "w");
    if(Global::Birthfp == NULL) {
      Utils::fred_abort("Can't open %s\n", filename);
    }

    sprintf(filename, "%s/deaths.txt", directory);
    Global::Deathfp = fopen(filename, "w");
    if(Global::Deathfp == NULL) {
      Utils::fred_abort("Can't open %s\n", filename);
    }
  }
}


void Demographics::update(int day) {
  // reset counts of births and deaths
  Demographics::births_today = 0;
  Demographics::deaths_today = 0;
}


void Demographics::report(int day) {
  char filename[FRED_STRING_SIZE];
  FILE* fp = NULL;

  // get the current year
  int year = Date::get_year();

  sprintf(filename, "%s/ages-%d.txt", Global::Simulation_directory, year);
  fp = fopen(filename, "w");

  int n0, n5, n18, n65;
  int count[20];
  int total = 0;
  n0 = n5 = n18 = n65 = 0;
  // age distribution
  for(int c = 0; c < 20; ++c) {
    count[c] = 0;
  }
  for(int p = 0; p < Person::get_population_size(); ++p) {
    Person* person = Person::get_person(p);
    int a = person->get_age();
    if(a < 5) {
      n0++;
    } else if(a < 18) {
      n5++;
    } else if(a < 65) {
      n18++;
    } else {
      n65++;
    }
    int n = a / 5;
    if(n < 20) {
      count[n]++;
    } else {
      count[19]++;
    }
    total++;
  }
  // fprintf(fp, "\nAge distribution: %d people\n", total);
  for(int c = 0; c < 20; ++c) {
    fprintf(fp, "age %2d to %d: %6d %e %d\n", 5 * c, 5 * (c + 1) - 1, count[c],
	    (1.0 * count[c]) / total, total);
  }
  /*
    fprintf(fp, "AGE 0-4: %d %.2f%%\n", n0, (100.0 * n0) / total);
    fprintf(fp, "AGE 5-17: %d %.2f%%\n", n5, (100.0 * n5) / total);
    fprintf(fp, "AGE 18-64: %d %.2f%%\n", n18, (100.0 * n18) / total);
    fprintf(fp, "AGE 65-100: %d %.2f%%\n", n65, (100.0 * n65) / total);
    fprintf(fp, "\n");
  */
  fclose(fp);

}


int Demographics::find_admin_code(int n) {
  int size = Demographics::admin_codes.size();
  for(int i = 0; i < size; ++i) {
    if(Demographics::admin_codes[i] == n) {
      return i;
    }
  }
  return -1;
}


void Demographics::terminate(Person* self) {
  int day = Global::Simulation_Day;
  FRED_STATUS(1, "Demographics::terminate day %d person %d age %d\n",
	      day, self->get_id(), self->get_age());

  // update death stats
  Demographics::deaths_today++;
  Demographics::deaths_ytd++;
  Demographics::total_deaths++;

  if(Global::Deathfp != NULL) {
    // report deaths
    fprintf(Global::Deathfp, "day %d person %d age %d\n",
	    day, self->get_id(), self->get_age());
    fflush(Global::Deathfp);
  }
}


