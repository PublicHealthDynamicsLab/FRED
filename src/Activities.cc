/*
   Copyright 2009 by the University of Pittsburgh
   Licensed under the Academic Free License version 3.0
   See the file "LICENSE" for more information
   */

//
//
// File: Activities.cc
//

#include "Activities.h"
#include "Behavior.h"
#include "Global.h"
#include "Person.h"
#include "Place.h"
#include "Random.h"
#include "Disease.h"
#include "Params.h"
#include "Vaccine_Manager.h"
#include "Manager.h"
#include "Date.h"
#include "Cell.h"
#include "Grid.h"
#include "Geo_Utils.h"
#include "Household.h"
#include "School.h"
#include "Classroom.h"
#include "Workplace.h"
#include "Place_List.h"
#include "Utils.h"
#include "Travel.h"

#define PRESCHOOL_PROFILE 0
#define STUDENT_PROFILE 1
#define TEACHER_PROFILE 2
#define WORKER_PROFILE 3
#define WEEKEND_WORKER_PROFILE 4
#define UNEMPLOYED_PROFILE 5
#define RETIRED_PROFILE 6

bool Activities::is_initialized = false;
bool Activities::is_weekday = false;
double Activities::age_yearly_mobility_rate[MAX_MOBILITY_AGE + 1];
int Activities::day_of_week = 0;

// run-time parameters
double Activities::Community_distance = 20;
double Activities::Community_prob = 0.1;
double Activities::Home_neighborhood_prob = 0.5;
int Activities::Enable_default_sick_behavior = 0;
double Activities::Default_sick_day_prob = 0.0;
double Activities::SLA_mean_sick_days_absent = 0.0;
double Activities::SLU_mean_sick_days_absent = 0.0;
double Activities::SLA_absent_prob = 0.0;
double Activities::SLU_absent_prob = 0.0;
double Activities::Flu_days = 0.0;

// sick leave statistics
int Activities::Sick_days_present = 0;
int Activities::Sick_days_absent = 0;
int Activities::School_sick_days_present = 0;
int Activities::School_sick_days_absent = 0;

Activities::Activities (Person *person, Place *house, Place *school, Place *work) {
  //Create the static arrays one time
  if (!Activities::is_initialized) {
    read_init_files();
    Activities::is_initialized = true;
  }
  self = person;

  for (int i = 0; i < FAVORITE_PLACES; i++) {
    favorite_place[i] = NULL;
  }
  favorite_place[HOUSEHOLD_INDEX] = house;
  favorite_place[SCHOOL_INDEX] = school;
  favorite_place[WORKPLACE_INDEX] = work;
  assert(get_household() != NULL);

  // get the neighbhood from the household
  favorite_place[NEIGHBORHOOD_INDEX] =
    favorite_place[HOUSEHOLD_INDEX]->get_grid_cell()->get_neighborhood();
  if (get_neighborhood() == NULL) {
    printf("Help! NO NEIGHBORHOOD for person %d house %d \n",
        self->get_id(), get_household()->get_id());
  }
  assert(get_neighborhood() != NULL);
  assign_profile();

  // enroll in all the favorite places
  for (int i = 0; i < FAVORITE_PLACES; i++) {
    if (favorite_place[i] != NULL) {
      favorite_place[i]->enroll(self);
    }
  }
  // need to set the daily schedule
  schedule_updated = -1;
  travel_status = false;
  traveling_outside = false;

  // sick leave variables
  my_sick_days_absent = 0;
  my_sick_days_present = 0;
  my_sick_leave_decision_has_been_made = false;
  my_sick_leave_decision = false;
  sick_days_remaining = 0.0;
  sick_leave_available = false;
}

void Activities::prepare() {
  initialize_sick_leave();
}

#define SMALL_COMPANY_MAXSIZE 49
#define MID_COMPANY_MAXSIZE 99
#define MEDIUM_COMPANY_MAXSIZE 499

int Activities::employees_small_with_sick_leave = 0;
int Activities::employees_small_without_sick_leave = 0;
int Activities::employees_med_with_sick_leave = 0;
int Activities::employees_med_without_sick_leave = 0;
int Activities::employees_large_with_sick_leave = 0;
int Activities::employees_large_without_sick_leave = 0;
int Activities::employees_xlarge_with_sick_leave = 0;
int Activities::employees_xlarge_without_sick_leave = 0;

void Activities::initialize_sick_leave() {
  int workplace_size = 0;
  my_sick_days_absent = 0;
  my_sick_days_present = 0;
  my_sick_leave_decision_has_been_made = false;
  my_sick_leave_decision = false;
  sick_days_remaining = 0.0;
  sick_leave_available = false;

  if (favorite_place[WORKPLACE_INDEX] != NULL)
    workplace_size = favorite_place[WORKPLACE_INDEX]->get_size();

  // is sick leave available?
  if (workplace_size > 0) {
    if (workplace_size <= SMALL_COMPANY_MAXSIZE) {
      sick_leave_available = (RANDOM() < 0.53);
      if (sick_leave_available)
        Activities::employees_small_with_sick_leave++;
      else
        Activities::employees_small_without_sick_leave++;
    }
    else if (workplace_size <= MID_COMPANY_MAXSIZE) {
      sick_leave_available = (RANDOM() < 0.58);
      if (sick_leave_available)
        Activities::employees_med_with_sick_leave++;
      else
        Activities::employees_med_without_sick_leave++;
    }
    else if (workplace_size <= MEDIUM_COMPANY_MAXSIZE) {
      sick_leave_available = (RANDOM() < 0.70);
      if (sick_leave_available)
        Activities::employees_large_with_sick_leave++;
      else
        Activities::employees_large_without_sick_leave++;
    }
    else {
      sick_leave_available = (RANDOM() < 0.85);
      if (sick_leave_available)
        Activities::employees_xlarge_with_sick_leave++;
      else
        Activities::employees_xlarge_without_sick_leave++;
    }
  }
  else
    sick_leave_available = false;

  // compute sick days remaining (for flu)
  sick_days_remaining = 0.0;
  if (sick_leave_available) {
    if (RANDOM() < Activities::SLA_absent_prob) {
      sick_days_remaining = Activities::SLA_mean_sick_days_absent + Activities::Flu_days;
    }
  }
  else {
    if (RANDOM() < Activities::SLU_absent_prob) {
      sick_days_remaining = Activities::SLU_mean_sick_days_absent + Activities::Flu_days;
    }
    else if (RANDOM() < Activities::SLA_absent_prob - Activities::SLU_absent_prob) {
      sick_days_remaining = Activities::Flu_days;
    }
  }
}

void Activities::before_run() {
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp,"employees at small workplaces with sick leave: %d\n",
        Activities::employees_small_with_sick_leave);
    fprintf(Global::Statusfp,"employees at small workplaces without sick leave: %d\n",
        Activities::employees_small_without_sick_leave);
    fprintf(Global::Statusfp,"employees at med workplaces with sick leave: %d\n",
        Activities::employees_med_with_sick_leave);
    fprintf(Global::Statusfp,"employees at med workplaces without sick leave: %d\n",
        Activities::employees_med_without_sick_leave);
    fprintf(Global::Statusfp,"employees at large workplaces with sick leave: %d\n",
        Activities::employees_large_with_sick_leave);
    fprintf(Global::Statusfp,"employees at large workplaces without sick leave: %d\n",
        Activities::employees_large_without_sick_leave);
    fprintf(Global::Statusfp,"employees at xlarge workplaces with sick leave: %d\n",
        Activities::employees_xlarge_with_sick_leave);
    fprintf(Global::Statusfp,"employees at xlareg workplaces without sick leave: %d\n",
        Activities::employees_xlarge_without_sick_leave);
    fflush(Global::Statusfp);
  }
}

void Activities::assign_profile() {
  int age = self->get_age();
  if (age < Global::SCHOOL_AGE && get_school() == NULL)
    profile = PRESCHOOL_PROFILE;    // child at home
  else if (age < Global::SCHOOL_AGE && get_school() != NULL)
    profile = STUDENT_PROFILE;      // child in preschool
  else if (get_school() != NULL)
    profile = STUDENT_PROFILE;      // student
  // else if (get_school() != NULL)
  // profile = TEACHER_PROFILE;      // teacher
  else if (Global::RETIREMENT_AGE <= age && RANDOM() < 0.5)
    profile = RETIRED_PROFILE;      // retired
  else
    profile = WORKER_PROFILE;     // worker

  // weekend worker
  if (profile == WORKER_PROFILE && RANDOM() < 0.2) {
    profile = WEEKEND_WORKER_PROFILE;   // 20% weekend worker
  }

  // unemployed
  if ((profile == WORKER_PROFILE ||
        profile == WEEKEND_WORKER_PROFILE) && RANDOM() < 0.1) {
    profile = UNEMPLOYED_PROFILE;   // 10% unemployed
  }
}

void Activities::update(int day) {

  // decide if this is a weekday:
  Activities::day_of_week = Global::Sim_Current_Date->get_day_of_week();
  Activities::is_weekday = (0 < Activities::day_of_week && Activities::day_of_week < 6);

  // print out absenteeism/presenteeism counts
  if (Global::Verbose > 0 && day > 0) {
    printf("DAY %d ABSENTEEISM: work absent %d present %d %0.2f  school absent %d present %d %0.2f\n", day-1,
        Activities::Sick_days_absent,
        Activities::Sick_days_present,
        (double) (Activities::Sick_days_absent) /(double)(1+Activities::Sick_days_absent+Activities::Sick_days_present),
        Activities::School_sick_days_absent,
        Activities::School_sick_days_present,
        (double) (Activities::School_sick_days_absent) /(double)(1+Activities::School_sick_days_absent+Activities::School_sick_days_present));
  }

  // keep track of global activity counts
  Activities::Sick_days_present = 0;
  Activities::Sick_days_absent = 0;
  Activities::School_sick_days_present = 0;
  Activities::School_sick_days_absent = 0;
}


void Activities::update_infectious_activities(int day, int dis) {
  // skip scheduled activities if traveling abroad
  if (traveling_outside)
    return;

  // get list of places to visit today
  update_schedule(day);

  // if symptomatic, decide whether or not to stay home
  if (self->is_symptomatic()) {
    decide_whether_to_stay_home(day);
  }

  for (int i = 0; i < FAVORITE_PLACES; i++) {
    if (on_schedule[i]) {
      assert(favorite_place[i] != NULL);
      favorite_place[i]->add_infectious(dis, self);
    }
  }
}


void Activities::update_susceptible_activities(int day, int dis) {
  // skip scheduled activities if traveling abroad
  if (traveling_outside)
    return;

  // get list of places to visit today
  update_schedule(day);

  for (int i = 0; i < FAVORITE_PLACES; i++) {
    if (on_schedule[i]) {
      assert(favorite_place[i] != NULL);
      if (favorite_place[i]->is_infectious(dis)) {
        favorite_place[i]->add_susceptible(dis, self);
      }
    }
  }
}

void Activities::update_schedule(int day) {
  // update this schedule only once per day
  if (day <= schedule_updated)
    return;
  schedule_updated = day;

  on_schedule.reset();

  // always visit the household
  on_schedule[HOUSEHOLD_INDEX] = true;

  // provisionally visit the neighborhood
  on_schedule[NEIGHBORHOOD_INDEX] = true;

  // weekday vs weekend provisional activity
  if (Activities::is_weekday) {
    if (favorite_place[SCHOOL_INDEX] != NULL)
      on_schedule[SCHOOL_INDEX] = true;
    if (favorite_place[CLASSROOM_INDEX] != NULL)
      on_schedule[CLASSROOM_INDEX] = true;
    if (favorite_place[WORKPLACE_INDEX] != NULL)
      on_schedule[WORKPLACE_INDEX] = true;
    if (favorite_place[OFFICE_INDEX] != NULL)
      on_schedule[OFFICE_INDEX] = true;
  }
  else {
    if (profile == WEEKEND_WORKER_PROFILE || profile == STUDENT_PROFILE) {
      if (favorite_place[WORKPLACE_INDEX] != NULL)
        on_schedule[WORKPLACE_INDEX] = true;
      if (favorite_place[OFFICE_INDEX] != NULL)
        on_schedule[OFFICE_INDEX] = true;
    }
  }

  // skip work at background absenteeism rate
  if (Global::Work_absenteeism > 0.0 && on_schedule[WORKPLACE_INDEX]) {
    if (RANDOM() < Global::Work_absenteeism) {
      on_schedule[WORKPLACE_INDEX] = false;
      on_schedule[OFFICE_INDEX] = false;
    }
  }

  // skip school at background school absenteeism rate
  if (Global::School_absenteeism > 0.0 && on_schedule[SCHOOL_INDEX]) {
    if (RANDOM() < Global::School_absenteeism)
      on_schedule[SCHOOL_INDEX] = false;
    on_schedule[CLASSROOM_INDEX] = false;
  }

  // decide whether to stay home if symptomatic moved to
  // update_infectious_activities.  Since in general if a person is
  // symptomatic they will also be infectious, this should be fine.
  // If later it is decided that a person could be symptomatic without
  // being infectious, then we would have to change the updates so that
  // they are applied to all infected people (with a check to see if 
  // infectious before adding to infectious places and a check to see if 
  // symptomatic before deciding to stay home
  //
  // if (self->is_symptomatic()) {
  //   decide_whether_to_stay_home(day);
  // }

  // decide which neighborhood to visit today
  if (on_schedule[NEIGHBORHOOD_INDEX]) {
    favorite_place[NEIGHBORHOOD_INDEX] =
      favorite_place[HOUSEHOLD_INDEX]->select_neighborhood(Activities::Community_prob,
          Activities::Community_distance,
          Activities::Home_neighborhood_prob);
  }

  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "update_schedule on day %d\n", day);
    print_schedule(day);
    fflush(Global::Statusfp);
  }
}

void Activities::decide_whether_to_stay_home(int day) {
  assert (self->is_symptomatic());
  bool stay_home = false;

  if (self->is_adult()) {
    if (Activities::Enable_default_sick_behavior) {
      stay_home = default_sick_leave_behavior();
    }
    else {
      if (on_schedule[WORKPLACE_INDEX]) {
        // it is a work day
        if (sick_days_remaining > 0.0) {
          stay_home = (RANDOM() < sick_days_remaining);
          sick_days_remaining--;
        }
        else {
          stay_home = false;
        }
      }
      else {
        // it is a not work day
        stay_home = (RANDOM() < Activities::Default_sick_day_prob);
      }
    }
  }
  else {
    // sick child: use default sick behavior, for now.
    stay_home = default_sick_leave_behavior();
  }

  // record work absent/present decision if it is a workday
  if (on_schedule[WORKPLACE_INDEX]) {
    if (stay_home) {
      Activities::Sick_days_absent++;
      my_sick_days_absent++;
    }
    else {
      Activities::Sick_days_present++;
      my_sick_days_present++;
    }
  }

  // record school absent/present decision if it is a school day
  if (on_schedule[SCHOOL_INDEX]) {
    if (stay_home) {
      Activities::School_sick_days_absent++;
      my_sick_days_absent++;
    }
    else {
      Activities::School_sick_days_present++;
      my_sick_days_present++;
    }
  }

  if (stay_home) {
    // withdraw to household
    on_schedule[WORKPLACE_INDEX] = false;
    on_schedule[OFFICE_INDEX] = false;
    on_schedule[SCHOOL_INDEX] = false;
    on_schedule[CLASSROOM_INDEX] = false;
    on_schedule[NEIGHBORHOOD_INDEX] = false;
    // printf("agent %d staying home on day %d\n", self->get_id(), day);
  }
}

bool Activities::default_sick_leave_behavior() {
  bool stay_home = false;
  if (my_sick_leave_decision_has_been_made) {
    stay_home = my_sick_leave_decision;
  }
  else {
    stay_home = (RANDOM() < Activities::Default_sick_day_prob);
    my_sick_leave_decision = stay_home;
    my_sick_leave_decision_has_been_made = true;
  }
  return stay_home;
}


void Activities::print_schedule(int day) {
  fprintf(Global::Statusfp, "day %d schedule for person %d  ", day, self->get_id());
  for (int p = 0; p < FAVORITE_PLACES; p++) {
    fprintf(Global::Statusfp, on_schedule[p]? "+" : "-");
    if (favorite_place[p] == NULL) {
      fprintf(Global::Statusfp, "-1 ");
    } else {
      fprintf(Global::Statusfp, "%d ", favorite_place[p]->get_id());
    }
  }
  fprintf(Global::Statusfp, "\n");
  fflush(Global::Statusfp);
}

void Activities::print() {
  fprintf(Global::Statusfp, "Activities for person %d: ", self->get_id());
  for (int p = 0; p < FAVORITE_PLACES; p++) {
    if (favorite_place[p] != NULL)
      fprintf(Global::Statusfp, "%d ", favorite_place[p]->get_id());
    else {
      fprintf(Global::Statusfp, "-1 ");
    }
  }
  fprintf(Global::Statusfp, "\n");
  fflush(Global::Statusfp);
}

void Activities::assign_school() {
  int age = self->get_age();
  // if (age < Global::SCHOOL_AGE || Global::ADULT_AGE <= age) return;
  Cell *grid_cell = favorite_place[HOUSEHOLD_INDEX]->get_grid_cell();
  assert(grid_cell != NULL);
  Place *p = grid_cell->select_random_school(age);
  if (p != NULL) {
    favorite_place[SCHOOL_INDEX] = p;
    favorite_place[CLASSROOM_INDEX] = NULL;
    assign_classroom();
    return;
  }
  else {
    favorite_place[SCHOOL_INDEX] = NULL;
    favorite_place[CLASSROOM_INDEX] = NULL;
    return;
  }

  int src_pop = grid_cell->get_target_popsize();
  int row = grid_cell->get_row();
  int col = grid_cell->get_col();
  int level = 1;
  vector<Cell *> nbrs;
  while(level < 100) {
    nbrs.clear();
    for(int j=-level; j<level; j++)
      nbrs.push_back( Global::Cells->get_grid_cell(row-level, col+j) );
    for(int j=-level; j<level; j++)
      nbrs.push_back( Global::Cells->get_grid_cell(row+j, col+level) );
    for(int j=-level; j<level; j++)
      nbrs.push_back( Global::Cells->get_grid_cell(row+level, col+j) );
    for(int j=-level; j<level; j++)
      nbrs.push_back( Global::Cells->get_grid_cell(row+j, col-level) );

    int target_pop = 0;
    for(unsigned int i=0; i<nbrs.size(); i++) {
      target_pop += nbrs.at(i)->get_target_popsize();
    }
    // Radiation model (Simini et al, 2011)
    // Approximated to concentric squares instead of concentric circles
    double prob = 1.0 / (1.0 + ((double) target_pop) / src_pop);
    double prob_per_person = prob / target_pop;

    int attempts = 0;
    for(unsigned int i=0; i<nbrs.size(); i++) {
      Cell *nbr = nbrs.at(i);
      if(nbr == NULL) continue;
      else attempts++;
      double r = RANDOM();
      r = 0;
      if(r < prob_per_person * nbr->get_target_popsize()) {
        p = nbr->select_random_school(age);
        if(p != NULL) {
          favorite_place[SCHOOL_INDEX] = p;
          favorite_place[CLASSROOM_INDEX] = NULL;
          assign_classroom();
          return;
        }
      }
    }
    if(attempts == 0) break;
    level++;
  }

  /*
     int trials = 0;
     while (trials < 100) {
     grid_cell = (Cell *) Global::Cells->select_random_grid_cell();
     p = grid_cell->select_random_school(age);
     if (p != NULL) {
     favorite_place[SCHOOL_INDEX] = p;
     favorite_place[CLASSROOM_INDEX] = NULL;
     assign_classroom();

     return;
     }
     trials++;
     }*/
  Utils::fred_abort("assign_school: can't locate school for person %d\n", self->get_id());
  //FRED_WARNING("assign_school: can't locate school for person %d\n", self->get_id());
}

void Activities::assign_classroom() {
  if (favorite_place[SCHOOL_INDEX] != NULL &&
      favorite_place[CLASSROOM_INDEX] == NULL) {
    Place * place =
      ((School *) favorite_place[SCHOOL_INDEX])->assign_classroom(self);
    if (place != NULL) place->enroll(self);
    favorite_place[CLASSROOM_INDEX] = place;
  }
}

void Activities::assign_workplace() {
  int age = self->get_age();
  // if (age < Global::ADULT_AGE) return;
  Cell *grid_cell = favorite_place[HOUSEHOLD_INDEX]->get_grid_cell();
  assert(grid_cell != NULL);
  Place *p = grid_cell->select_random_workplace();
  if (p != NULL) {
    favorite_place[WORKPLACE_INDEX] = p;
    favorite_place[OFFICE_INDEX] = NULL;
    assign_office();
    return;
  }
  else {
    favorite_place[WORKPLACE_INDEX] = NULL;
    favorite_place[OFFICE_INDEX] = NULL;
    return;
  }


  int src_pop = grid_cell->get_target_popsize();
  int row = grid_cell->get_row();
  int col = grid_cell->get_col();
  int level = 1;
  vector<Cell *> nbrs;
  while(level < 100) {
    nbrs.clear();
    for(int j=-level; j<level; j++)
      nbrs.push_back( Global::Cells->get_grid_cell(row-level, col+j) );
    for(int j=-level; j<level; j++)
      nbrs.push_back( Global::Cells->get_grid_cell(row+j, col+level) );
    for(int j=-level; j<level; j++)
      nbrs.push_back( Global::Cells->get_grid_cell(row+level, col+j) );
    for(int j=-level; j<level; j++)
      nbrs.push_back( Global::Cells->get_grid_cell(row+j, col-level) );

    int target_pop = 0;
    for(unsigned int i=0; i<nbrs.size(); i++) {
      target_pop += nbrs.at(i)->get_target_popsize();
    }
    // Radiation model (Simini et al, 2011)
    // Approximated to concentric squares instead of concentric circles
    double prob = 1.0 / (1.0 + ((double) target_pop) / src_pop);
    double prob_per_person = prob / target_pop;

    int attempts = 0;
    for(unsigned int i=0; i<nbrs.size(); i++) {
      Cell *nbr = nbrs.at(i);
      if(nbr == NULL) continue;
      else attempts++;
      double r = RANDOM();
      r = 0;
      if(r < prob_per_person * nbr->get_target_popsize()) {
        p = nbr->select_random_workplace();
        if(p != NULL) {
          favorite_place[WORKPLACE_INDEX] = p;
          favorite_place[OFFICE_INDEX] = NULL;
          assign_office();
          return;
        }
      }
    }
    if(attempts == 0) break;
    level++;
  }

  /*
     int trials = 0;
     while (trials < 100) {
     grid_cell = (Cell *) Global::Cells->select_random_grid_cell();
     p = grid_cell->select_random_workplace();
     if (p != NULL) {
     favorite_place[WORKPLACE_INDEX] = p;
     favorite_place[OFFICE_INDEX] = NULL;
     assign_office();
     return;
     }
     trials++;
     }*/
  //Utils::fred_abort("assign_workplace: can't locate workplace for person %d\n", self->get_id());
  FRED_WARNING("assign_workplace: can't locate workplace for person %d\n", self->get_id());
}

void Activities::assign_office() {
  if (favorite_place[WORKPLACE_INDEX] != NULL &&
      favorite_place[OFFICE_INDEX] == NULL &&
      Workplace::get_max_office_size() > 0) {
    Place * place =
      ((Workplace *) favorite_place[WORKPLACE_INDEX])->assign_office(self);
    if (place != NULL) place->enroll(self);
    else { 
      if (Global::Verbose > 0) {
        printf("Warning! No office assigned for person %d workplace %d\n",
            self->get_id(), favorite_place[WORKPLACE_INDEX]->get_id());
      }
    }
    favorite_place[OFFICE_INDEX] = place;
  }
}

int Activities::get_group_size(int index) {
  int size = 0;
  if (favorite_place[index] != NULL)
    size = favorite_place[index]->get_size();
  return size;
}

int Activities::get_degree() {
  int degree;
  int n;
  degree = 0;
  n = get_group_size(NEIGHBORHOOD_INDEX);
  if (n > 0) degree += (n-1);
  n = get_group_size(SCHOOL_INDEX);
  if (n > 0) degree += (n-1);
  n = get_group_size(WORKPLACE_INDEX);
  if (n > 0) degree += (n-1);
  return degree;
}

void Activities::update_profile() {
  int age = self->get_age();

  if (profile == PRESCHOOL_PROFILE && Global::SCHOOL_AGE <= age && age < Global::ADULT_AGE) {
    // start school
    profile = STUDENT_PROFILE;
    // select a school based on age and neighborhood
    assign_school();
    if (Global::Verbose>1) {
      fprintf(Global::Statusfp,
          "changed behavior profile to STUDENT: id %d age %d sex %c\n",
          self->get_id(), age, self->get_sex());
      print();
      fflush(Global::Statusfp);
    }
    return;
  }

  if (profile == STUDENT_PROFILE && age < Global::ADULT_AGE) {
    // select a school based on age and neighborhood
    School * s = (School *) favorite_place[SCHOOL_INDEX];
    Classroom * c = (Classroom *) favorite_place[CLASSROOM_INDEX];
    if (c == NULL || c->get_age_level() == age) {
      // no change
      if (Global::Verbose>1) {
        fprintf(Global::Statusfp,
            "KEPT CLASSROOM ASSIGNMENT: id %d age %d sex %c ",
            self->get_id(), age, self->get_sex());
        fprintf(Global::Statusfp, "%s %s | ",
            s->get_label(), c->get_label());
        print();
        fflush(Global::Statusfp);
      }
      return;
    } else if (s != NULL && s->classrooms_for_age(age) > 0) {
      // pick a new classrooms in current school
      favorite_place[CLASSROOM_INDEX] = NULL;
      assign_classroom();
      if (Global::Verbose>1) {
        fprintf(Global::Statusfp,
            "CHANGED CLASSROOM ASSIGNMENT: id %d age %d sex %c ",
            self->get_id(), age, self->get_sex());
        if (s != NULL && c != NULL) {
          fprintf(Global::Statusfp, "from %s %s to %s %s | ",
              s->get_label(), c->get_label(),
              favorite_place[SCHOOL_INDEX]->get_label(),
              favorite_place[CLASSROOM_INDEX]->get_label());
        } else {
          fprintf(Global::Statusfp, "from NULL NULL to %s %s | ",
              favorite_place[SCHOOL_INDEX]->get_label(),
              favorite_place[CLASSROOM_INDEX]->get_label());
        }
        print();
        fflush(Global::Statusfp);
      }
    } else {
      // pick a new school and classroom
      assign_school();
      if (Global::Verbose>1) {
        fprintf(Global::Statusfp,
            "CHANGED SCHOOL ASSIGNMENT: id %d age %d sex %c ",
            self->get_id(), age, self->get_sex());
        if (s != NULL && c != NULL) {
          fprintf(Global::Statusfp, "from %s %s to %s %s | ",
              s->get_label(), c->get_label(),
              favorite_place[SCHOOL_INDEX]->get_label(),
              favorite_place[CLASSROOM_INDEX]->get_label());
        } else {
          fprintf(Global::Statusfp, "from NULL NULL to %s %s | ",
              favorite_place[SCHOOL_INDEX]->get_label(),
              (favorite_place[CLASSROOM_INDEX] == NULL)?
              "NULL" : favorite_place[CLASSROOM_INDEX]->get_label());
        }
        print();
        fflush(Global::Statusfp);
      }
    }
    return;
  }

  if (profile == STUDENT_PROFILE && Global::ADULT_AGE <= age) {
    // leave school
    favorite_place[SCHOOL_INDEX] = NULL;
    favorite_place[CLASSROOM_INDEX] = NULL;
    // get a job
    profile = WORKER_PROFILE;
    assign_workplace();
    initialize_sick_leave();
    if (Global::Verbose>1) {
      fprintf(Global::Statusfp,
          "changed behavior profile to WORKER: id %d age %d sex %c\n",
          self->get_id(), age, self->get_sex());
      print();
      fflush(Global::Statusfp);
    }
    return;
  }

  if (profile != RETIRED_PROFILE && Global::RETIREMENT_AGE <= age) {
    if (RANDOM() < 0.5) {
      // quit working
      favorite_place[WORKPLACE_INDEX] = NULL;
      favorite_place[OFFICE_INDEX] = NULL;
      initialize_sick_leave(); // no sick leave available if retired
      profile = RETIRED_PROFILE;
      if (Global::Verbose>1) {
        fprintf(Global::Statusfp,
            "changed behavior profile to RETIRED: age %d age %d sex %c\n",
            self->get_id(), age, self->get_sex());
        print();
        fflush(Global::Statusfp);
      }
    }
    return;
  }
}

static int mobility_count[MAX_MOBILITY_AGE + 1];
static int mobility_moved[MAX_MOBILITY_AGE + 1];
static int mcount = 0;

void Activities::update_household_mobility() {
  if (!Global::Enable_Mobility) return;
  if (Global::Verbose>1) {
    fprintf(Global::Statusfp, "update_household_mobility entered with mcount = %d\n", mcount);
    fflush(Global::Statusfp);
  }

  if (mcount == 0) {
    for (int i = 0; i <= MAX_MOBILITY_AGE; i++) {
      mobility_count[i] = mobility_moved[i] = 0;
    }
  }

  int age = self->get_age();
  mobility_count[age]++;
  mcount++;

  Household * household = (Household *) self->get_household();
  if (self->is_householder()) {
    if (RANDOM() < Activities::age_yearly_mobility_rate[age]) {
      int size = household->get_size();
      for (int i = 0; i < size; i++) {
        Person *p = household->get_housemate(i);
        mobility_moved[p->get_age()]++;
      }
    }
  }

  int popsize = Global::Pop.get_pop_size();
  if (mcount == popsize) {
    double mobility_rate[MAX_MOBILITY_AGE + 1];
    FILE *fp;
    fp = fopen("mobility.out", "w");
    for (int i = 0; i <= MAX_MOBILITY_AGE; i++) {
      mobility_rate[i] = 0;
      if (mobility_count[i]> 0) mobility_rate[i] = (1.0*mobility_moved[i])/mobility_count[i];
      fprintf(fp, "%d %d %d %f\n", i, mobility_count[i], mobility_moved[i], mobility_rate[i]);
    }
    fclose(fp);
  }
}

void Activities::addIncidence(int disease, vector<int> strains) {
  for (int i = 0; i < FAVORITE_PLACES; i++) {
    if(favorite_place[i] == NULL) continue;
    favorite_place[i]->modify_incidence_count(disease, strains, 1);
  }
}

void Activities::addPrevalence(int disease, vector<int> strains) {
  for (int i = 0; i < FAVORITE_PLACES; i++) {
    if(favorite_place[i] == NULL) continue;
    favorite_place[i]->modify_prevalence_count(disease, strains, 1);
  }
}


void Activities::read_init_files() {
  Params::get_param_from_string("community_distance", &Activities::Community_distance);
  Params::get_param_from_string("community_prob", &Activities::Community_prob);
  Params::get_param_from_string("home_neighborhood_prob", &Activities::Home_neighborhood_prob);

  Params::get_param_from_string("enable_default_sick_behavior", &Activities::Enable_default_sick_behavior);
  Params::get_param_from_string("sick_day_prob", &Activities::Default_sick_day_prob);

  Params::get_param_from_string("SLA_mean_sick_days_absent", &Activities::SLA_mean_sick_days_absent);
  Params::get_param_from_string("SLU_mean_sick_days_absent", &Activities::SLU_mean_sick_days_absent);
  Params::get_param_from_string("SLA_absent_prob", &Activities::SLA_absent_prob);
  Params::get_param_from_string("SLU_absent_prob", &Activities::SLU_absent_prob);
  Params::get_param_from_string("flu_days", &Activities::Flu_days);

  if (!Global::Enable_Mobility) return;
  char yearly_mobility_rate_file[256];
  if (Global::Verbose) {
    fprintf(Global::Statusfp, "read activities init files entered\n"); fflush(Global::Statusfp);
  }
  Params::get_param_from_string("yearly_mobility_rate_file", yearly_mobility_rate_file);
  // read mobility rate file and load the values into the mobility_rate_array
  FILE *fp = Utils::fred_open_file(yearly_mobility_rate_file);
  if (fp == NULL) {
    fprintf(Global::Statusfp, "Activities init_file %s not found\n", yearly_mobility_rate_file);
    exit(1);
  }
  for (int i = 0; i <= MAX_MOBILITY_AGE; i++) {
    int age;
    double mobility_rate;
    if (fscanf(fp, "%d %lf",
          &age, &mobility_rate) != 2) {
      fprintf(Global::Statusfp, "Help! Read failure for age %d\n", i);
      Utils::fred_abort("");
    }
    Activities::age_yearly_mobility_rate[age] = mobility_rate;
  }
  fclose(fp);
  if (Global::Verbose) {
    fprintf(Global::Statusfp, "finished reading Activities init_file = %s\n", yearly_mobility_rate_file);
    for (int i = 0; i <= MAX_MOBILITY_AGE; i++) {
      fprintf(Global::Statusfp, "%d %f\n", i, Activities::age_yearly_mobility_rate[i]);
    }
    fflush(Global::Statusfp);
  }
}

void Activities::store_favorite_places() {
  tmp_favorite_place = new Place* [FAVORITE_PLACES];
  for (int i = 0; i < FAVORITE_PLACES; i++) {
    tmp_favorite_place[i] = favorite_place[i];
  }
}

void Activities::restore_favorite_places() {
  for (int i = 0; i < FAVORITE_PLACES; i++) {
    favorite_place[i] = tmp_favorite_place[i];
  }
  delete[] tmp_favorite_place;
}

void Activities::start_traveling(Person * visited) {
  if (visited == NULL) {
    traveling_outside = true;
  }
  else {
    store_favorite_places();
    for (int i = 0; i < FAVORITE_PLACES; i++) {
      favorite_place[i] = NULL;
    }
    favorite_place[HOUSEHOLD_INDEX] = visited->get_household();
    favorite_place[NEIGHBORHOOD_INDEX] = visited->get_neighborhood();
    if (profile == WORKER_PROFILE) {
      favorite_place[WORKPLACE_INDEX] = visited->get_workplace();
      favorite_place[OFFICE_INDEX] = visited->get_office();
    }

    //Anuroop: Should this person enroll in visited's favorite places??
  }
  travel_status = true;
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "start traveling: id = %d\n", self->get_id());
    fflush(Global::Statusfp);
  }
}

void Activities::stop_traveling() {
  if (!traveling_outside) {
    restore_favorite_places();
  }
  travel_status = false;
  traveling_outside = false;
  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "stop traveling: id = %d\n", self->get_id());
    fflush(Global::Statusfp);
  }
}

void Activities::terminate() {
  // Person was enrolled in only his original 
  // favorite places, not his host's places while travelling
  if(travel_status && ! traveling_outside) 
    restore_favorite_places();

  // unenroll from all the favorite places
  for (int i = 0; i < FAVORITE_PLACES; i++) {
    if (favorite_place[i] != NULL) {
      favorite_place[i]->unenroll(self);
    }
  }
}

void Activities::end_of_run() {
}
