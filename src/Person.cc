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
// File: Person.cc
//

#include "Person.h"

#include "Activities.h"
#include "Age_Map.h"
#include "Classroom.h"
#include "Demographics.h"
#include "Condition.h"
#include "Global.h"
#include "Health.h"
#include "Household.h"
#include "Mixing_Group.h"
#include "Neighborhood.h"
#include "Office.h"
#include "Place.h"
#include "Population.h"
#include "Random.h"
#include "School.h"
#include "Workplace.h"

#include <cstdio>
#include <vector>
#include <sstream>

Person::Person() {
  this->id = -1;
  this->index = -1;
  this->exposed_household_index = -1;
  this->eligible_to_migrate = true;
  this->native = true;
  this->original = false;
}

Person::~Person() {
}

void Person::setup(int _index, int _id, int age, char sex,
		   int race, int rel, Place* house, Place* school, Place* work,
		   int day, bool today_is_birthday) {
  FRED_VERBOSE(1, "Person::setup() id %d age %d house %s school %s work %s\n",
	       _id, age, house->get_label(), school? school->get_label():"NULL", work?work->get_label():"NULL");
  int myFIPS;
  this->index = _index;
  this->id = _id;
  this->demographics.setup(this, age, sex, race, rel, day, today_is_birthday);
  this->health.setup(this);
  this->activities.setup(this, house, school, work);

  myFIPS = house->get_county_fips();
  if(today_is_birthday) {
    FRED_VERBOSE(1, "Baby index %d id %d age %d born on day %d household = %s  new_size %d orig_size %d\n",
		 _index, this->id, age, day, house->get_label(), house->get_size(), house->get_orig_size());
  }
}

void Person::print(FILE* fp, int condition_id) {
  if(fp == NULL) {
    return;
  }
  fprintf(fp, "%d id %7d  a %3d  s %c r %d ",
          condition_id, id,
          this->demographics.get_age(),
          this->demographics.get_sex(),
          this->demographics.get_race());
  fprintf(fp, "exp: %2d ",
          this->health.get_exposure_date(condition_id));
  fprintf(fp, "infected_at %c %6d ",
          get_infected_mixing_group_type(condition_id),
	  get_infected_mixing_group_id(condition_id));
  fprintf(fp, "infector %d ", get_infector(condition_id)->get_id());
  fprintf(fp, "infectees %d ", get_infectees(condition_id));
  fprintf(fp,"\n");
  fflush(fp);
}

void Person::update_household_counts(int day, int condition_id) {
  // this is only called for people with an active infection.
  Mixing_Group* hh = this->get_household();
  if(hh == NULL) {
    if(Global::Enable_Hospitals && this->is_hospitalized() && this->get_permanent_household() != NULL) {
      hh = this->get_permanent_household();
    }
  }
  if(hh != NULL) {
    this->health.update_mixing_group_counts(day, condition_id, hh);
  }
}

void Person::update_school_counts(int day, int condition_id) {
  // this is only called for people with an active infection.
  Mixing_Group* school = this->get_school();
  if(school != NULL) {
    this->health.update_mixing_group_counts(day, condition_id, school);
  }
}

void Person::become_immune(int condition_id) {
  if(this->health.is_susceptible(condition_id)) {
    this->health.become_immune(condition_id);
  }
}

Person* Person::give_birth(int day) {
  int age = 0;
  char sex = (Random::draw_random(0.0, 1.0) < 0.5 ? 'M' : 'F');
  int race = get_race();
  int rel = Global::CHILD;
  Place* house = this->get_household();
  if(house == NULL) {
    if(Global::Enable_Hospitals && this->is_hospitalized() && this->get_permanent_household() != NULL) {
      house = this->get_permanent_household();
    }
  }
  /*
    if (house == NULL) {
    printf("Mom %d has no household\n", this->get_id()); fflush(stdout);
    }
  */
  assert(house != NULL);
  Place* school = NULL;
  Place* work = NULL;
  bool today_is_birthday = true;
  Person* baby = Global::Pop.add_person_to_population(age, sex, race, rel,
					house, school, work, day, today_is_birthday);

  if(Global::Enable_Population_Dynamics) {
    baby->get_demographics()->initialize_demographic_dynamics(baby);
  }

  this->demographics.update_birth_stats(day, this);
  FRED_VERBOSE(1, "mother %d baby %d\n", this->get_id(), baby->get_id());

  return baby;
}

string Person::to_string() {

  stringstream tmp_string_stream;
  // (i.e *ID* Age Sex Race Household School Classroom Workplace Office Neighborhood Hospital Ad_Hoc Relationship)
  tmp_string_stream << this->id << " " << this->get_age() << " " <<  this->get_sex() << " " ;
  tmp_string_stream << this->get_race() << " " ;
  tmp_string_stream << Place::get_place_label(this->get_household()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_school()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_classroom()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_workplace()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_office()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_neighborhood()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_hospital()) << " ";
  tmp_string_stream << Place::get_place_label(this->get_ad_hoc()) << " ";
  tmp_string_stream << this->get_relationship();

  return tmp_string_stream.str();
}

void Person::terminate(int day) {
  FRED_VERBOSE(1, "terminating person %d\n", id);
  this->health.terminate(day);
  this->activities.terminate();
  this->demographics.terminate(this);
  
  if (Global::Enable_Transmission_Network || Global::Enable_Sexual_Partner_Network) {
    this->relationships.terminate(day, Global::Sexual_Partner_Network);
  }
}

double Person::get_x() {
  Place* hh = this->get_household();

  if(hh == NULL) {
    if(Global::Enable_Hospitals && this->is_hospitalized() && this->get_permanent_household() != NULL) {
      hh = this->get_permanent_household();
    }
  }

  if(hh == NULL) {
    return 0.0;
  } else {
    return hh->get_x();
  }
}

double Person::get_y() {
  Place* hh = this->get_household();

  if(hh == NULL) {
    if(Global::Enable_Hospitals && this->is_hospitalized() && this->get_permanent_household() != NULL) {
      hh = this->get_permanent_household();
    }
  }

  if(hh == NULL) {
    return 0.0;
  } else {
    return hh->get_y();
  }
}

char* Person::get_household_structure_label() {
  return get_household()->get_household_structure_label();
}

int Person::get_income() {
  return get_household()->get_household_income();
}

long int Person::get_household_census_tract_fips() {
  return get_household()->get_census_tract_fips();
}

int Person::get_household_county_fips() {
  return get_household()->get_county_fips();
}

