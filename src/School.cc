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
// File: School.cc
//

#include "Classroom.h"
#include "Date.h"
#include "Condition.h"
#include "Condition_List.h"
#include "Global.h"
#include "Mixing_Group.h"
#include "Params.h"
#include "Person.h"
#include "Place_List.h"
#include "Population.h"
#include "Random.h"
#include "School.h"
#include "Utils.h"
#include "Tracker.h"

//Private static variables that will be set by parameter lookups
double School::contacts_per_day;
double** School::prob_transmission_per_contact;
int School::school_classroom_size = 0;
char School::school_closure_policy[80] = "undefined";
int School::school_closure_day = 0;
int School::min_school_closure_day = 0;
double School::school_closure_threshold = 0.0;
double School::individual_school_closure_threshold = 0.0;
int School::school_closure_cases = -1;
int School::school_closure_duration = 0;
int School::school_closure_delay = 0;
int School::school_summer_schedule = 0;
char School::school_summer_start[8];
char School::school_summer_end[8];
int School::summer_start_month = 0;
int School::summer_start_day = 0;
int School::summer_end_month = 0;
int School::summer_end_day = 0;
int School::total_school_pop = 0;
int School::pop_income_Q1 = 0;
int School::pop_income_Q2 = 0;
int School::pop_income_Q3 = 0;
int School::pop_income_Q4 = 0;
bool School::global_closure_is_active = false;
int School::global_close_date = 0;
int School::global_open_date = 0;

School::School() : Place() {
  this->set_type(Place::TYPE_SCHOOL);
  this->set_subtype(Place::SUBTYPE_NONE);
  this->intimacy = 0.025;
  for(int i = 0; i < Global::GRADES; ++i) {
    this->students_in_grade[i] = 0;
    this->orig_students_in_grade[i] = 0;
    this->next_classroom[i] = 0;
    this->classrooms[i].clear();
  }
  this->classroom.clear();
  this->closure_dates_have_been_set = false;
  this->staff_size = 0;
  this->max_grade = -1;
  this->county_index = -1;
  this->income_quartile = -1;
}


School::School(const char* lab, char _subtype, fred::geo lon, fred::geo lat) : Place(lab, lon, lat) {
  this->set_type(Place::TYPE_SCHOOL);
  this->set_subtype(_subtype);
  this->intimacy = 0.025;
  for(int i = 0; i < Global::GRADES; ++i) {
    this->students_in_grade[i] = 0;
    this->orig_students_in_grade[i] = 0;
    this->next_classroom[i] = 0;
    this->classrooms[i].clear();
  }
  this->closure_dates_have_been_set = false;
  this->staff_size = 0;
  this->max_grade = -1;
  this->county_index = -1;
  this->income_quartile = -1;
}

void School::prepare() {
  assert(Global::Pop.is_load_completed());
  // call base class function to perform preparations common to all Places 
  Place::prepare();

  // record original size in each grade
  for(int g = 0; g < Global::GRADES; ++g) {
    this->orig_students_in_grade[g] = this->students_in_grade[g];
  }
}


void School::get_parameters() {

  Params::get_param("school_contacts", &School::contacts_per_day);
  int n = Params::get_param_matrix((char *)"school_trans_per_contact", &School::prob_transmission_per_contact);
  if(Global::Verbose > 1) {
    printf("\nSchool_contact_prob:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      printf("%f ", School::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
  }

  // normalize contact parameters
  // find max contact prob
  double max_prob = 0.0;
  for(int i  = 0; i < n; ++i)  {
    for(int j  = 0; j < n; ++j) {
      if (School::prob_transmission_per_contact[i][j] > max_prob) {
	      max_prob = School::prob_transmission_per_contact[i][j];
      }
    }
  }

  // convert max contact prob to 1.0
  if(max_prob > 0) {
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      School::prob_transmission_per_contact[i][j] /= max_prob;
      }
    }
    // compensate contact rate
    School::contacts_per_day *= max_prob;
  }

  if(Global::Verbose > 0) {
    printf("\nSchool_contact_prob after normalization:\n");
    for(int i  = 0; i < n; ++i)  {
      for(int j  = 0; j < n; ++j) {
	      printf("%f ", School::prob_transmission_per_contact[i][j]);
      }
      printf("\n");
    }
    printf("\ncontact rate: %f\n", School::contacts_per_day);
  }
  // end normalization

  Params::get_param("school_classroom_size", &School::school_classroom_size);

  // summer school parameters
  Params::get_param("school_summer_schedule", &School::school_summer_schedule);
  Params::get_param("school_summer_start", School::school_summer_start);
  Params::get_param("school_summer_end", School::school_summer_end);
  sscanf(School::school_summer_start, "%d-%d", &School::summer_start_month, &School::summer_start_day);
  sscanf(School::school_summer_end, "%d-%d", &School::summer_end_month, &School::summer_end_day);

  // school closure parameters
  Params::get_param("school_closure_policy", School::school_closure_policy);
  Params::get_param("school_closure_duration", &School::school_closure_duration);
  Params::get_param("school_closure_delay", &School::school_closure_delay);
  Params::get_param("school_closure_day", &School::school_closure_day);
  Params::get_param("min_school_closure_day", &School::min_school_closure_day);
  Params::get_param("school_closure_ar_threshold", &School::school_closure_threshold);
  Params::get_param("individual_school_closure_ar_threshold",
				&School::individual_school_closure_threshold);
  Params::get_param("school_closure_cases", &School::school_closure_cases);

  // aliases for parameters
  int Weeks;
  Params::get_param("Weeks", &Weeks);
  if(Weeks > -1) {
    School::school_closure_duration = 7 * Weeks;
  }

  int cases;
  Params::get_param("Cases", &cases);
  if(cases > -1) {
    School::school_closure_cases = cases;
  }
}

int School::get_group(int condition_id, Person* per) {
  int age = per->get_age();
  if(age < 12) {
    return 0;
  } else if(age < 16) {
    return 1;
  } else if(per->is_student()) {
    return 2;
  } else {
    return 3;
  }
}

double School::get_transmission_prob(int condition_id, Person* i, Person* s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(condition_id, i);
  int col = get_group(condition_id, s);
  double tr_pr = School::prob_transmission_per_contact[row][col];
  return tr_pr;
}

void School::close(int day, int day_to_close, int duration) {
  this->close_date = day_to_close;
  this->open_date = close_date + duration;
  this->closure_dates_have_been_set = true;

  // log this school closure decision
  if(Global::Verbose > 0) {
    printf("SCHOOL %s CLOSURE decision day %d close_date %d duration %d open_date %d\n",
	   this->get_label(), day, this->close_date, duration, this->open_date);
  }
}


bool School::is_open(int day) {
  bool open = (day < this->close_date || this->open_date <= day);
  if(!open) {
    FRED_VERBOSE(0, "Place %s is closed on day %d\n", this->get_label(), day);
  }
  return open;
}

bool School::should_be_open(int day, int condition_id) {
  // no students
  if(get_size() == 0) {
    return false;
  }

  // summer break
  if(School::school_summer_schedule > 0) {
    int month = Date::get_month();
    int day_of_month = Date::get_day_of_month();
    if((month == School::summer_start_month && School::summer_start_day <= day_of_month) ||
       (month == School::summer_end_month && day_of_month <= School::summer_end_day) ||
       (School::summer_start_month < month && month < School::summer_end_month)) {
      if(Global::Verbose > 1) {
	      fprintf(Global::Statusfp, "School %s closed for summer\n", this->get_label());
	      fflush(Global::Statusfp);
      }
      return false;
    }
  }

  // stick to previously made decision to close
  if(this->closure_dates_have_been_set) {
    return is_open(day);
  }

  // global school closure policy in effect
  if(strcmp(School::school_closure_policy, "global") == 0) {
    apply_global_school_closure_policy(day, condition_id);
    return is_open(day);
  }

  // individual school closure policy in effect
  if(strcmp(School::school_closure_policy, "individual") == 0) {
    apply_individual_school_closure_policy(day, condition_id);
    return is_open(day);
  }

  // if school_closure_policy is not recognized, then open
  return true;
}

void School::apply_global_school_closure_policy(int day, int condition_id) {

  // Only test triggers for school closure if not global closure is not already activated
  if(School::global_closure_is_active == false) {
    // Setting school_closure_day > -1 overrides other global triggers.
    // Close schools if the closure date has arrived (after a delay)
    if(School::school_closure_day > -1) {
      if(School::school_closure_day <= day) {
        // the following only happens once
        School::global_close_date = day + School::school_closure_delay;
        School::global_open_date = day + School::school_closure_delay
	        + School::school_closure_duration;
	      School::global_closure_is_active = true;
      }
    } else {
      // Close schools if the global symptomatic attack rate has reached the threshold (after a delay)
      Condition* condition = Global::Conditions.get_condition(condition_id);
      if(School::school_closure_threshold <= condition->get_symptomatic_attack_rate()) {
        // the following only happens once
        School::global_close_date = day + School::school_closure_delay;
        School::global_open_date = day + School::school_closure_delay
	        + School::school_closure_duration;
	      School::global_closure_is_active = true;
      }
    }
  }
  if(School::global_closure_is_active) {
    // set close and open dates for this school (only once)
    close(day,School::global_close_date, School::school_closure_duration);

    // log this school closure decision
    if(Global::Verbose > 0) {
      Condition* condition = Global::Conditions.get_condition(condition_id);
      printf("GLOBAL SCHOOL CLOSURE pop_ar %5.2f local_cases = %d / %d (%5.2f)\n",
	     condition->get_symptomatic_attack_rate(), get_total_cases(condition_id),
	     get_size(), get_symptomatic_attack_rate(condition_id));
    }
  }
}

void School::apply_individual_school_closure_policy(int day, int condition_id) {

  // don't apply any policy prior to School::min_school_closure_day
  if(day <= School::min_school_closure_day) {
    return;
  }

  // don't apply any policy before the epidemic reaches a noticeable threshold
  Condition* condition = Global::Conditions.get_condition(condition_id);
  if(condition->get_symptomatic_attack_rate() < School::school_closure_threshold) {
    return;
  }

  bool close_this_school = false;

  // if school_closure_cases > -1 then close if this number of cases occurs
  if(School::school_closure_cases != -1) {
    close_this_school = (School::school_closure_cases <= get_total_cases(condition_id));
  } else {
    // close if attack rate threshold is reached
    close_this_school = (School::individual_school_closure_threshold <= get_symptomatic_attack_rate(condition_id));
  }

  if(close_this_school) {
    // set close and open dates for this school (only once)
    close(day,day + School::school_closure_delay, School::school_closure_duration);

    // log this school closure decision
    if(Global::Verbose > 0) {
      Condition* condition = Global::Conditions.get_condition(condition_id);
      printf("LOCAL SCHOOL CLOSURE pop_ar %.3f local_cases = %d / %d (%.3f)\n",
	     condition->get_symptomatic_attack_rate(), get_total_cases(condition_id),
	     get_size(), get_symptomatic_attack_rate(condition_id));
    }
  }
}

double School::get_contacts_per_day(int condition_id) {
  return School::contacts_per_day;
}

int School::enroll(Person* person) {

  // call base class method:
  int return_value = Place::enroll(person);

  FRED_VERBOSE(1,"day %d Enroll person %d age %d in school %d %s new size %d\n",
	       Global::Simulation_Day, person->get_id(), person->get_age(),
	       this->get_id(), this->get_label(), this->get_size());
  if(person->is_teacher()) {
    this->staff_size++;
  } else {
    int age = person->get_age();
    int grade = ((age < Global::GRADES) ? age : Global::GRADES - 1);
    assert(grade > 0);
    this->students_in_grade[grade]++;
    if(grade > this->max_grade) {
      this->max_grade = grade;
    }
    person->set_grade(grade);
  }

  return return_value;
}

void School::unenroll(int pos) {
  int size = this->enrollees.size();
  assert(0 <= pos && pos < size);
  Person* removed = this->enrollees[pos];
  int grade = removed->get_grade();
  FRED_VERBOSE(1,"day %d UNENROLL person %d age %d grade %d, is_teacher %d from school %d %s Size = %d\n",
	       Global::Simulation_Day, removed->get_id(), removed->get_age(), grade,
	       removed->is_teacher()?1:0, this->get_id(), this->get_label(), this->get_size());

  // call the base class method
  Mixing_Group::unenroll(pos);

  if(removed->is_teacher() || grade == 0) {
    this->staff_size--;
  } else {
    assert(0 < grade && grade <= this->max_grade);
    this->students_in_grade[grade]--;
  }
  removed->set_grade(0);
  FRED_VERBOSE(1,"day %d UNENROLLED from school %d %s size = %d\n", 
	       Global::Simulation_Day, this->get_id(), this->get_label(), this->get_size());
}

void School::print(int condition_id) {
  fprintf(Global::Statusfp, "Place %d label %s type %c ", this->get_id(), this->get_label(), this->get_type());
  for(int g = 0; g < Global::GRADES; ++g) {
    fprintf(Global::Statusfp, "%d students in grade %d | ", this->students_in_grade[g], g);
  }
  fprintf(Global::Statusfp, "\n");
  fflush(Global::Statusfp);
}

int School::get_number_of_rooms() {
  int total_rooms = 0;
  if(School::school_classroom_size == 0) {
    return 0;
  }
  for(int a = 0; a < Global::GRADES; ++a) {
    int n = this->students_in_grade[a];
    if(n == 0) {
      continue;
    }
    int rooms = n / School::school_classroom_size;
    if(n % School::school_classroom_size) {
      rooms++;
    }
    total_rooms += rooms;
  }
  return total_rooms;
}

void School::setup_classrooms() {
  if(School::school_classroom_size == 0) {
    return;
  }
  FRED_VERBOSE(1, "setup_classroom for School %d %s\n", this->get_id(), this->get_label());

  for(int a = 0; a < Global::GRADES; ++a) {
    int n = this->students_in_grade[a];
    if(n == 0) {
      continue;
    }
    int rooms = n / School::school_classroom_size;
    if(n % School::school_classroom_size) {
      rooms++;
    }
    FRED_STATUS(1, "School %d %s grade %d number %d rooms %d\n", 
		this->get_id(), this->get_label(), a, n, rooms);
    for(int c = 0; c < rooms; ++c) {
      char new_label[128];
      sprintf(new_label, "%s-%02d-%02d", this->get_label(), a, c + 1);
      Classroom* classroom = static_cast<Classroom *>(Global::Places.add_place(new_label, 
									       Place::TYPE_CLASSROOM, 
									       Place::SUBTYPE_NONE,
									       this->get_longitude(),
									       this->get_latitude(),
									       this->get_census_tract_fips()));
      classroom->set_school(this);
      this->classrooms[a].push_back(classroom);
      this->classroom.push_back(classroom);
    }
  }
  FRED_VERBOSE(1, "setup_classroom finished for School %d %s\n", this->get_id(), this->get_label());
}

Place* School::select_classroom_for_student(Person* per) {
  if(School::school_classroom_size == 0) {
    return NULL;
  }
  int grade = per->get_age();
  if(Global::GRADES <= grade) {
    grade = Global::GRADES - 1;
  }
  if(Global::Verbose > 1) {
    fprintf(Global::Statusfp, "assign classroom for student %d in school %d %s grade %d\n",
	    per->get_id(), this->get_id(), this->get_label(), grade);
    fflush(Global::Statusfp);
  }

  if(this->classrooms[grade].size() == 0) {
    return NULL;
  }

  // pick next classroom for this grade, round-robin
  int room = this->next_classroom[grade];
  if(room < (int) this->classrooms[grade].size() - 1) {
    this->next_classroom[grade]++;
  } else {
    this->next_classroom[grade] = 0;
  }

  // pick next classroom for this grade at random
  // int room = Random::draw_random_int(0,(classrooms[grade].size()-1));

  if(Global::Verbose > 1) {
    fprintf(Global::Statusfp, "room = %d %s %d\n", room, this->classrooms[grade][room]->get_label(),
	    this->classrooms[grade][room]->get_id());
    fflush(Global::Statusfp);
  }
  return this->classrooms[grade][room];
}

void School::print_size_distribution() {
  for(int g = 1; g < Global::GRADES; ++g) {
    if(this->orig_students_in_grade[g] > 0) {
      printf("SCHOOL %s grade %d orig %d current %d\n", 
	     this->get_label(), g, this->orig_students_in_grade[g], this->students_in_grade[g]);
    }
  }
  fflush(stdout);
}
