/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: School.h
//

#ifndef _FRED_SCHOOL_H
#define _FRED_SCHOOL_H

#include "Place.h"
#include <vector>

#define MAX_GRADE 21
class Classroom;

class School : public Place {
public: 
  School() {}
  ~School() {}
  School( const char*, double, double, Place *, Population *pop );
  void prepare();
  void get_parameters(int diseases);
  int get_group(int disease_id, Person * per);
  double get_transmission_prob(int disease_id, Person * i, Person * s);
  bool should_be_open(int day, int disease_id);
  void apply_global_school_closure_policy(int day, int disease_id);
  void apply_individual_school_closure_policy(int day, int disease_id);
  double get_contacts_per_day(int disease_id);
  void enroll(Person * per);
  void unenroll(Person * per);
  int children_in_grade(int age) {
    if (-1 < age && age < MAX_GRADE) return students_with_age[age];
    else return 0;
  }
  int classrooms_for_age(int age) { return (int) classrooms[age].size(); }
  void print(int disease);
  int get_number_of_rooms();
  void setup_classrooms( Allocator< Classroom > & classroom_allocator );
  Place * assign_classroom(Person *per);

private:
  static double *** school_contact_prob;
  static char school_closure_policy[];
  static int school_closure_day;
  static double school_closure_threshold;
  static double individual_school_closure_threshold;
  static int school_closure_cases;
  static int school_closure_period;
  static int school_closure_delay;
  static bool school_parameters_set;
  static int school_summer_schedule;
  static char school_summer_start[];
  static char school_summer_end[];
  static int school_classroom_size;
  static double * school_contacts_per_day;
  static bool global_closure_is_active;
  static int global_close_date;
  static int global_open_date;

  int students_with_age[MAX_GRADE];
  vector <Place *> classrooms[MAX_GRADE];
  int next_classroom[MAX_GRADE];
  int next_classroom_without_teacher[MAX_GRADE];
  int total_classrooms;
  bool closure_dates_have_been_set;
};

#endif // _FRED_SCHOOL_H
