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
// File: Census_Tract.cc
//

#include "Census_Tract.h"
#include "Global.h"
#include "Household.h"
#include "Person.h"
#include "Random.h"
#include "School.h"
#include "Workplace.h"
#include "Utils.h"

Census_Tract::~Census_Tract() {
}

Census_Tract::Census_Tract(long int _fips) {
  this->fips = _fips;
  this->tot_current_popsize = 0;
  this->households.clear();
  this->workplaces_attended.clear();
  for (int i = 0; i < Global::GRADES; i++) {
    this->schools_attended[i].clear();
  }
}


void Census_Tract::setup() {
  set_school_probabilities();
  set_workplace_probabilities();
  FRED_VERBOSE(1, "CENSUS_TRACT %ld setup: population = %d  households = %d  workplaces attended = %d\n",
	       this->fips,
	       this->tot_current_popsize,
	       (int) this->households.size(),
	       (int) this->workplaces_attended.size());
  for (int i = 0; i < Global::GRADES; i++) {
    if (this->schools_attended[i].size() > 0) {
      FRED_VERBOSE(1, "CENSUS_TRACT %ld setup: school attended for grade %d = %d\n",
		   this->fips, i, (int) this->schools_attended[i].size());
    }
  }
}


bool Census_Tract::increment_popsize(Person* person) {
  int age = person->get_age();
  char sex = person->get_sex();
  if(age > Demographics::MAX_AGE) {
    age = Demographics::MAX_AGE;
  }
  if(age >= 0) {
    this->tot_current_popsize++;
    return true;
  }
  return false;
}

bool Census_Tract::decrement_popsize(Person* person) {
  int age = person->get_age();
  char sex = person->get_sex();
  if(age > Demographics::MAX_AGE) {
    age = Demographics::MAX_AGE;
  }
  if(age >= 0) {
    this->tot_current_popsize--;
    return true;
  }
  return false;
}

void Census_Tract::update(int day) {
}


void Census_Tract::report_census_tract_population() {
}


// METHODS FOR SELECTING NEW SCHOOLS

void Census_Tract::set_school_probabilities() {

  FRED_VERBOSE(1, "set_school_probablities for fips %ld\n", this->fips);

  int total[Global::GRADES];
  sid_to_school.clear();
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
    Household* hh = this->households[i];
    int hh_size = hh->get_size();
    for (int j = 0; j < hh_size; j++) {
      Person* person = hh->get_enrollee(j);
      School* school = person->get_school();
      int grade = person->get_age();
      if (school != NULL && grade < Global::GRADES) {
	// add this person to the count for this school
	// printf("school %s grade %d person %d\n", school->get_label(), grade, person->get_id()); fflush(stdout);
	int sid = school->get_id();
	if (school_counts[grade].find(sid) == school_counts[grade].end()) {
	  std::pair<int,int> new_school_count(sid,1);
	  school_counts[grade].insert(new_school_count);
	  if (sid_to_school.find(sid) == sid_to_school.end()) {
	    std::pair<int,School*> new_sid_map(sid,school);
	    sid_to_school.insert(new_sid_map);
	  }
	}
	else {
	  school_counts[grade][sid]++;
	}
	total[grade]++;
      } // endif school != NULL
    } // foreach housemate
  } // foreach household

  // convert to probabilities
  for (int g = 0; g < Global::GRADES; g++) {
    if (total[g] > 0) {
      for (attendance_map_itr_t itr = school_counts[g].begin(); itr != school_counts[g].end(); ++itr) {
	int sid = itr->first;
	int count = itr->second;
	School* school = sid_to_school[sid];
	schools_attended[g].push_back(school);
	double prob = (double) count / (double) total[g];
	school_probabilities[g].push_back(prob);
	FRED_VERBOSE(1,"school %s fips %ld grade %d attended by %d prob %f\n",
		     school->get_label(), school->get_county_fips(), g, count, prob);
      }
    }
  }
}




School* Census_Tract::select_new_school(int grade) {

  // pick the school with largest vacancy rate in this grade
  School* selected = NULL;
  double max_vrate = 0.0;
  for (int i = 0; i < this->schools_attended[grade].size(); ++i) {
    School* school = this->schools_attended[grade][i];
    double target = school->get_orig_students_in_grade(grade);
    double vrate = (target-school->get_students_in_grade(grade))/target;
    if (vrate > max_vrate) {
      selected = school;
      max_vrate = vrate;
    }
  }
  if (selected != NULL) {
    return selected;
  }

  FRED_VERBOSE(1,"WARNING: NO SCHOOL VACANCIES found on day %d in fips = %ld grade = %d schools = %d\n",
	       Global::Simulation_Day, this->fips, grade, (int) (this->schools_attended[grade].size()));

 // pick from the attendance distribution
  double r = Random::draw_random();
  double sum = 0.0;
  for (int i = 0; i < this->school_probabilities[grade].size(); ++i) {
    sum += this->school_probabilities[grade][i];
    if (r < sum) {
      return this->schools_attended[grade][i];
    }
  }
  FRED_VERBOSE(1,"WARNING: NO SCHOOL FOUND on day %d in fips = %ld grade = %d schools = %d r = %f sum = %f\n",
	       Global::Simulation_Day, this->fips, grade,
	       (int) (this->school_probabilities[grade].size()), r, sum);

  // fall back to selecting school from County
  return NULL;
}


// METHODS FOR SELECTING NEW WORKPLACES

void Census_Tract::set_workplace_probabilities() {

  // list of workplaces attended by people in this census_tract
  typedef std::unordered_map<int,int> attendance_map_t;
  typedef std::unordered_map<int,Workplace*> wid_map_t;
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
    Household* hh = this->households[i];
    int hh_size = hh->get_size();
    for (int j = 0; j < hh_size; j++) {
      Person* person = hh->get_enrollee(j);
      Workplace* workplace = person->get_workplace();
      if (workplace != NULL) {
	int wid = workplace->get_id();
	if (workplace_counts.find(wid) == workplace_counts.end()) {
	  std::pair<int,int> new_workplace_count(wid,1);
	  workplace_counts.insert(new_workplace_count);
	  std::pair<int,Workplace*> new_wid_map(wid,workplace);
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
    Workplace* workplace = wid_to_workplace[wid];
    workplaces_attended.push_back(workplace);
    double prob = (double) count / (double) total;
    workplace_probabilities.push_back(prob);
    FRED_VERBOSE(1,"workplace %s fips %ld  attended by %d prob %f\n",
		 workplace->get_label(), workplace->get_census_tract_fips(), count, prob);
  }

}

Workplace* Census_Tract::select_new_workplace() {
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

