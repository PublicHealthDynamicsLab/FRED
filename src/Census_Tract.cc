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
// File: Census_Tract.cc
//

#include "Census_Tract.h"
#include "County.h"
#include "Person.h"
#include "Place.h"
#include "Random.h"

Census_Tract::~Census_Tract() {
}

Census_Tract::Census_Tract(long long int _admin_code) : Admin_Division(_admin_code) {

  this->workplaces_attended.clear();
  for (int i = 0; i < Global::GRADES; i++) {
    this->schools_attended[i].clear();
  }

  // get the county associated with this tract, creating a new one if necessary
  int county_admin_code = get_admin_division_code() / 1000000;
  County* county = County::get_county_with_admin_code(county_admin_code);
  this->higher = county;
  county->add_subdivision(this);
}


void Census_Tract::setup() {
  set_school_probabilities();
  set_workplace_probabilities();
  FRED_VERBOSE(1, "CENSUS_TRACT %lld setup: population = %d  households = %d  workplaces attended = %d\n",
	       get_admin_division_code(),
	       get_population_size(),
	       (int) this->households.size(),
	       (int) this->workplaces_attended.size());
  for (int i = 0; i < Global::GRADES; i++) {
    if (this->schools_attended[i].size() > 0) {
      FRED_VERBOSE(1, "CENSUS_TRACT %lld setup: school attended for grade %d = %d\n",
		   get_admin_division_code(), i, (int) this->schools_attended[i].size());
    }
  }
}


void Census_Tract::update(int day) {
}



// METHODS FOR SELECTING NEW SCHOOLS

void Census_Tract::set_school_probabilities() {

  FRED_VERBOSE(1, "set_school_probablities for admin_code %lld\n", get_admin_division_code());

  int total[Global::GRADES];
  school_id_lookup.clear();
  for (int g = 0; g < Global::GRADES; g++) {
    schools_attended[g].clear();
    school_probabilities[g].clear();
    school_counts[g].clear();
    total[g] = 0;
  }

  // get number of people in this census_tract attending each school
  // at the start of the simulation
  int houses = this->households.size();
  for (int i = 0; i < houses; i++) {
    Place* hh = this->households[i];
    int hh_size = hh->get_size();
    for (int j = 0; j < hh_size; j++) {
      Person* person = hh->get_member(j);
      Place* school = person->get_school();
      int grade = person->get_age();
      if (school != NULL && grade < Global::GRADES) {
	// add this person to the count for this school
	// printf("school %s grade %d person %d\n", school->get_label(), grade, person->get_id()); fflush(stdout);
	int school_id = school->get_id();
	if (school_counts[grade].find(school_id) == school_counts[grade].end()) {
	  std::pair<int,int> new_school_count(school_id,1);
	  school_counts[grade].insert(new_school_count);
	  if (school_id_lookup.find(school_id) == school_id_lookup.end()) {
	    std::pair<int,Place*> new_school_id_map(school_id,school);
	    school_id_lookup.insert(new_school_id_map);
	  }
	}
	else {
	  school_counts[grade][school_id]++;
	}
	total[grade]++;
      } // endif school != NULL
    } // foreach Housemate
  } // foreach household

  // convert to probabilities
  for (int g = 0; g < Global::GRADES; g++) {
    if (total[g] > 0) {
      for (attendance_map_itr_t itr = school_counts[g].begin(); itr != school_counts[g].end(); ++itr) {
	int school_id = itr->first;
	int count = itr->second;
	Place* school = school_id_lookup[school_id];
	schools_attended[g].push_back(school);
	double prob = (double) count / (double) total[g];
	school_probabilities[g].push_back(prob);
	FRED_VERBOSE(1,"school %s admin_code %lld grade %d attended by %d prob %f\n",
		     school->get_label(), school->get_county_admin_code(), g, count, prob);
      }
    }
  }
}


Place* Census_Tract::select_new_school(int grade) {

  // pick the school with largest vacancy rate in this grade
  Place* selected = NULL;
  double max_vrate = 0.0;
  for (int i = 0; i < this->schools_attended[grade].size(); ++i) {
    Place* school = this->schools_attended[grade][i];
    double target = school->get_original_size_by_age(grade);
    double vrate = (target-school->get_size_by_age(grade))/target;
    if (vrate > max_vrate) {
      selected = school;
      max_vrate = vrate;
    }
  }
  if (selected != NULL) {
    return selected;
  }

  FRED_VERBOSE(1,"WARNING: NO SCHOOL VACANCIES found on day %d in admin_code = %lld grade = %d schools = %d\n",
	       Global::Simulation_Day, get_admin_division_code(), grade, (int) (this->schools_attended[grade].size()));

 // pick from the attendance distribution
  double r = Random::draw_random();
  double sum = 0.0;
  for (int i = 0; i < this->school_probabilities[grade].size(); ++i) {
    sum += this->school_probabilities[grade][i];
    if (r < sum) {
      return this->schools_attended[grade][i];
    }
  }
  FRED_VERBOSE(1,"WARNING: NO SCHOOL FOUND on day %d in admin_code = %lld grade = %d schools = %d r = %f sum = %f\n",
	       Global::Simulation_Day, get_admin_division_code(), grade,
	       (int) (this->school_probabilities[grade].size()), r, sum);

  // fall back to selecting school from County
  return NULL;
}


// METHODS FOR SELECTING NEW WORKPLACES

void Census_Tract::set_workplace_probabilities() {

  // list of workplaces attended by people in this census_tract
  typedef std::unordered_map<int,int> attendance_map_t;
  typedef std::unordered_map<int,Place*> wid_map_t;
  typedef attendance_map_t::iterator attendance_map_itr_t;

  workplaces_attended.clear();
  workplace_probabilities.clear();

  // get number of people in this county attending each workplace
  // at the start of the simulation
  int houses = this->households.size();
  attendance_map_t workplace_counts;
  wid_map_t wid_to_workplace;
  workplace_counts.clear();
  wid_to_workplace.clear();
  int total = 0;
  for (int i = 0; i < houses; i++) {
    Place* hh = this->households[i];
    int hh_size = hh->get_size();
    for (int j = 0; j < hh_size; j++) {
      Person* person = hh->get_member(j);
      Place* workplace = person->get_workplace();
      if (workplace != NULL) {
	int wid = workplace->get_id();
	if (workplace_counts.find(wid) == workplace_counts.end()) {
	  std::pair<int,int> new_workplace_count(wid,1);
	  workplace_counts.insert(new_workplace_count);
	  std::pair<int,Place*> new_wid_map(wid,workplace);
	  wid_to_workplace.insert(new_wid_map);
	}
	else {
	  workplace_counts[wid]++;
	}
	total++;
      }
    }
  }
  if (total == 0) {
    return;
  }

  // convert to probabilities
  for (attendance_map_itr_t itr = workplace_counts.begin(); itr != workplace_counts.end(); ++itr) {
    int wid = itr->first;
    int count = itr->second;
    Place* workplace = wid_to_workplace[wid];
    workplaces_attended.push_back(workplace);
    double prob = (double) count / (double) total;
    workplace_probabilities.push_back(prob);
    FRED_VERBOSE(1,"workplace %s admin_code %lld  attended by %d prob %f\n",
		 workplace->get_label(), workplace->get_census_tract_admin_code(), count, prob);
  }

}

Place* Census_Tract::select_new_workplace() {
  double r = Random::draw_random();
  double sum = 0.0;
  for (int i = 0; i < this->workplace_probabilities.size(); ++i) {
    sum += this->workplace_probabilities[i];
    if (r < sum) {
      return this->workplaces_attended[i];
    }
  }
  return NULL;
}

//////////////////////////////
//
// STATIC METHODS
//
//////////////////////////////


// static variables
std::vector<Census_Tract*> Census_Tract::census_tracts;
std::unordered_map<long long int,Census_Tract*> Census_Tract::lookup_map;

Census_Tract* Census_Tract::get_census_tract_with_admin_code(long long int census_tract_admin_code) {
  Census_Tract* census_tract = NULL;
  std::unordered_map<long long int,Census_Tract*>::iterator itr;
  itr = Census_Tract::lookup_map.find(census_tract_admin_code);
  if (itr == Census_Tract::lookup_map.end()) {
    census_tract = new Census_Tract(census_tract_admin_code);
    Census_Tract::census_tracts.push_back(census_tract);
    Census_Tract::lookup_map[census_tract_admin_code] = census_tract;
  }
  else {
    census_tract = itr->second;
  }
  return census_tract;
}

void Census_Tract::setup_census_tracts() {
  // set each census tract's school and workplace attendance probabilities
  for(int i = 0; i < get_number_of_census_tracts(); ++i) {
    Census_Tract::census_tracts[i]->setup();
  }
}

