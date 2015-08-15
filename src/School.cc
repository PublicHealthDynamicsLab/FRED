/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: School.cc
//
#include "School.h"
#include "Classroom.h"
#include "Date.h"
#include "Disease.h"
#include "Disease_List.h"
#include "Global.h"
#include "Params.h"
#include "Person.h"
#include "Place_List.h"
#include "Population.h"
#include "Random.h"
#include "Utils.h"
#include "Tracker.h"

//Private static variables that will be set by parameter lookups
double* School::school_contacts_per_day;
double*** School::school_contact_prob;
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

//Private static variable to assure we only lookup parameters once
bool School::school_parameters_set = false;
bool School::global_closure_is_active = false;
int School::global_close_date = 0;
int School::global_open_date = 0;

School::School() {
  this->type = Place::SCHOOL;
  this->subtype = fred::PLACE_SUBTYPE_NONE;
  get_parameters(Global::Diseases.get_number_of_diseases());
  for(int i = 0; i < GRADES; ++i) {
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

School::School(const char* lab, fred::place_subtype _subtype, fred::geo lon, fred::geo lat) {
  this->type = Place::SCHOOL;
  this->subtype = _subtype;
  setup(lab, lon, lat);
  get_parameters(Global::Diseases.get_number_of_diseases());
  for(int i = 0; i < GRADES; ++i) {
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
  for(int g = 0; g < GRADES; ++g) {
    this->orig_students_in_grade[g] = this->students_in_grade[g];
  }
}

void School::get_parameters(int diseases) {
  if(School::school_parameters_set) {
    return;
  }

  School::school_contacts_per_day = new double[diseases];
  School::school_contact_prob = new double**[diseases];

  char param_str[80];
  for(int disease_id = 0; disease_id < diseases; ++disease_id) {
    Disease* disease = Global::Diseases.get_disease(disease_id);
    sprintf(param_str, "%s_school_contacts", disease->get_disease_name());
    Params::get_param((char *) param_str, &School::school_contacts_per_day[disease_id]);
    sprintf(param_str, "%s_school_prob", disease->get_disease_name());
    int n = Params::get_param_matrix(param_str, &School::school_contact_prob[disease_id]);

    if(Global::Verbose > 0) {
      printf("\nSchool_contact_prob before normalization:\n");
      for(int i  = 0; i < n; i++)  {
	for(int j  = 0; j < n; j++) {
	  printf("%f ", School::school_contact_prob[disease_id][i][j]);
	}
	printf("\n");
      }
      printf("\ncontact rate: %f\n", School::school_contacts_per_day[disease_id]);
    }

    // normalize contact parameters
    // find max contact prob
    double max_prob = 0.0;
    for(int i  = 0; i < n; i++)  {
      for(int j  = 0; j < n; j++) {
	if (School::school_contact_prob[disease_id][i][j] > max_prob) {
	  max_prob = School::school_contact_prob[disease_id][i][j];
	}
      }
    }

    // convert max contact prob to 1.0
    if (max_prob > 0) {
      for(int i  = 0; i < n; i++)  {
	for(int j  = 0; j < n; j++) {
	  School::school_contact_prob[disease_id][i][j] /= max_prob;
	}
      }
      // compensate contact rate
      School::school_contacts_per_day[disease_id] *= max_prob;
    }

    if(Global::Verbose > 0) {
      printf("\nSchool_contact_prob after normalization:\n");
      for(int i  = 0; i < n; i++)  {
	for(int j  = 0; j < n; j++) {
	  printf("%f ", School::school_contact_prob[disease_id][i][j]);
	}
	printf("\n");
      }
      printf("\ncontact rate: %f\n", School::school_contacts_per_day[disease_id]);
    }
    // end normalization
  }

  Params::get_param_from_string("school_classroom_size", &School::school_classroom_size);

  // summer school parameters
  Params::get_param_from_string("school_summer_schedule", &School::school_summer_schedule);
  Params::get_param_from_string("school_summer_start", School::school_summer_start);
  Params::get_param_from_string("school_summer_end", School::school_summer_end);
  sscanf(School::school_summer_start, "%d-%d", &School::summer_start_month, &School::summer_start_day);
  sscanf(School::school_summer_end, "%d-%d", &School::summer_end_month, &School::summer_end_day);

  // school closure parameters
  Params::get_param_from_string("school_closure_policy", School::school_closure_policy);
  Params::get_param_from_string("school_closure_duration", &School::school_closure_duration);
  Params::get_param_from_string("school_closure_delay", &School::school_closure_delay);
  Params::get_param_from_string("school_closure_day", &School::school_closure_day);
  Params::get_param_from_string("min_school_closure_day", &School::min_school_closure_day);
  Params::get_param_from_string("school_closure_ar_threshold", &School::school_closure_threshold);
  Params::get_param_from_string("individual_school_closure_ar_threshold",
				&School::individual_school_closure_threshold);
  Params::get_param_from_string("school_closure_cases", &School::school_closure_cases);

  // aliases for parameters
  int Weeks;
  Params::get_param_from_string("Weeks", &Weeks);
  if(Weeks > -1) {
    School::school_closure_duration = 7 * Weeks;
  }

  int Cases;
  Params::get_param_from_string("Cases", &Cases);
  if(Cases > -1) {
    School::school_closure_cases = Cases;
  }

  School::school_parameters_set = true;
}

int School::get_group(int disease_id, Person* per) {
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

double School::get_transmission_prob(int disease_id, Person* i, Person* s) {
  // i = infected agent
  // s = susceptible agent
  int row = get_group(disease_id, i);
  int col = get_group(disease_id, s);
  double tr_pr = School::school_contact_prob[disease_id][row][col];
  return tr_pr;
}

void School::close(int day, int day_to_close, int duration) {
  this->close_date = day_to_close;
  this->open_date = close_date + duration;
  this->closure_dates_have_been_set = true;

  // log this school closure decision
  if(Global::Verbose > 0) {
    printf("SCHOOL %s CLOSURE decision day %d close_date %d duration %d open_date %d\n",
	   this->label, day, this->close_date, duration, this->open_date);
  }
}


bool School::is_open(int day) {
  bool open = (day < this->close_date || this->open_date <= day);
  if (!open) {
    FRED_VERBOSE(0,"Place %s is closed on day %d\n", this->label, day);
  }
  return open;
}

bool School::should_be_open(int day, int disease_id) {
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
	fprintf(Global::Statusfp, "School %s closed for summer\n", label);
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
    apply_global_school_closure_policy(day, disease_id);
    return is_open(day);
  }

  // individual school closure policy in effect
  if(strcmp(School::school_closure_policy, "individual") == 0) {
    apply_individual_school_closure_policy(day, disease_id);
    return is_open(day);
  }

  // if school_closure_policy is not recognized, then open
  return true;
}

void School::apply_global_school_closure_policy(int day, int disease_id) {

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
      Disease* disease = Global::Diseases.get_disease(disease_id);
      if(School::school_closure_threshold <= disease->get_symptomatic_attack_rate()) {
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
      Disease* disease = Global::Diseases.get_disease(disease_id);
      printf("GLOBAL SCHOOL CLOSURE pop_ar %5.2f local_cases = %d / %d (%5.2f)\n",
	     disease->get_symptomatic_attack_rate(), get_total_cases(disease_id),
	     get_size(), get_symptomatic_attack_rate(disease_id));
    }
  }
}

void School::apply_individual_school_closure_policy(int day, int disease_id) {

  // don't apply any policy prior to School::min_school_closure_day
  if(day <= School::min_school_closure_day) {
    return;
  }

  // don't apply any policy before the epidemic reaches a noticeable threshold
  Disease* disease = Global::Diseases.get_disease(disease_id);
  if(disease->get_symptomatic_attack_rate() < School::school_closure_threshold) {
    return;
  }

  bool close_this_school = false;

  // if school_closure_cases > -1 then close if this number of cases occurs
  if(School::school_closure_cases != -1) {
    close_this_school = (School::school_closure_cases <= get_total_cases(disease_id));
  } else {
    // close if attack rate threshold is reached
    close_this_school = (School::individual_school_closure_threshold <= get_symptomatic_attack_rate(disease_id));
  }

  if(close_this_school) {
    // set close and open dates for this school (only once)
    close(day,day + School::school_closure_delay, School::school_closure_duration);

    // log this school closure decision
    if(Global::Verbose > 0) {
      Disease* disease = Global::Diseases.get_disease(disease_id);
      printf("LOCAL SCHOOL CLOSURE pop_ar %.3f local_cases = %d / %d (%.3f)\n",
	     disease->get_symptomatic_attack_rate(), get_total_cases(disease_id),
	     get_size(), get_symptomatic_attack_rate(disease_id));
    }
  }
}

double School::get_contacts_per_day(int disease_id) {
  return School::school_contacts_per_day[disease_id];
}

int School::enroll(Person* person) {

  // call base class method:
  int return_value = Place::enroll(person);

  FRED_VERBOSE(1,"Enroll person %d age %d in school %d %s new size %d\n",
	       person->get_id(), person->get_age(), this->id, this->label, this->get_size());
  if(person->is_teacher()) {
    this->staff_size++;
  } else {
    int age = person->get_age();
    int grade = ((age < GRADES) ? age : GRADES - 1);
    assert(grade > 0);
    this->students_in_grade[grade]++;
    if(grade > max_grade) {
      this->max_grade = grade;
    }
    person->set_grade(grade);
  }

  return return_value;
}

void School::unenroll(int pos) {
  int size = enrollees.size();
  assert(0 <= pos && pos < size);
  Person *removed = enrollees[pos];
  int grade = removed->get_grade();
  FRED_VERBOSE(1,"UNENROLL person %d age %d grade %d, is_teacher %d from school %d %s Size = %d\n",
	       removed->get_id(), removed->get_age(), grade, removed->is_teacher()?1:0, this->id, this->label, get_size());

  // call the base class method
  Place::unenroll(pos);

  if(removed->is_teacher() || grade == 0) {
    this->staff_size--;
  } else {
    assert(0 < grade && grade <= max_grade);
    this->students_in_grade[grade]--;
  }
  removed->set_grade(0);
  FRED_VERBOSE(1,"UNENROLLED from school %d %s size = %d\n", this->id, this->label, get_size());
}

void School::print(int disease_id) {
  fprintf(Global::Statusfp, "Place %d label %s type %c ", this->id, this->label, this->type);
  for(int g = 0; g < GRADES; ++g) {
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
  for(int a = 0; a < GRADES; ++a) {
    int n = this->students_in_grade[a];
    if(n == 0)
      continue;
    int rooms = n / School::school_classroom_size;
    if(n % School::school_classroom_size) {
      rooms++;
    }
    total_rooms += rooms;
  }
  return total_rooms;
}

void School::setup_classrooms(Allocator<Classroom> &classroom_allocator) {
  if(School::school_classroom_size == 0) {
    return;
  }

  for(int a = 0; a < GRADES; ++a) {
    int n = this->students_in_grade[a];
    if(n == 0) {
      continue;
    }
    int rooms = n / School::school_classroom_size;
    if(n % School::school_classroom_size) {
      rooms++;
    }

    FRED_STATUS(1, "school %d %s grade %d number %d rooms %d\n", id, label, a, n, rooms);

    for(int c = 0; c < rooms; ++c) {
      char new_label[128];
      sprintf(new_label, "%s-%02d-%02d", this->get_label(), a, c + 1);

      Classroom* clsrm = new (classroom_allocator.get_free()) Classroom(new_label,
									fred::PLACE_SUBTYPE_NONE,
									this->get_longitude(),
									this->get_latitude());
      clsrm->set_school(this);

      this->classrooms[a].push_back(clsrm);
    }
  }
}

Place* School::select_classroom_for_student(Person* per) {
  if(School::school_classroom_size == 0) {
    return NULL;
  }
  int grade = per->get_age();
  if(GRADES <= grade) {
    grade = GRADES - 1;
  }
  if(Global::Verbose > 1) {
    fprintf(Global::Statusfp, "assign classroom for student %d in school %d %s grade %d\n",
	    per->get_id(), this->id, this->label, grade);
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
  for(int g = 1; g < GRADES; ++g) {
    if(this->orig_students_in_grade[g] > 0) {
      printf("SCHOOL %s grade %d orig %d current %d\n", 
	     this->get_label(), g, this->orig_students_in_grade[g], this->students_in_grade[g]);
    }
  }
  fflush(stdout);
}
