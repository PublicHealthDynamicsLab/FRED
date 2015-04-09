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
// File: Activities.h
//
#ifndef _FRED_ACTIVITIES_H
#define _FRED_ACTIVITIES_H

#include <vector>
#include <bitset>
#include <map>

#include "Global.h"
#include "Random.h"
#include "Epidemic.h"
#include "Age_Map.h"

//class Age_Map;
class Person;
class Place;

#define MAX_MOBILITY_AGE 100

// Activity Profiles
#define INFANT_PROFILE 'I'
#define PRESCHOOL_PROFILE 'P'
#define STUDENT_PROFILE 'S'
#define TEACHER_PROFILE 'T'
#define WORKER_PROFILE 'W'
#define WEEKEND_WORKER_PROFILE 'Y'
#define UNEMPLOYED_PROFILE 'U'
#define RETIRED_PROFILE 'R'
#define UNDEFINED_PROFILE 'X'

// Group quarters residents
// Note: hospital patients are all non-staff.
// Note: Staff in group quarters are WORKERS whose workplace is a
// group_quarters, but who households are not group quarters

#define PRISONER_PROFILE 'J'
#define COLLEGE_STUDENT_PROFILE 'C'
#define MILITARY_PROFILE 'M'
#define NURSING_HOME_RESIDENT_PROFILE 'L'

namespace Activity_index {
  enum e {
    HOUSEHOLD_ACTIVITY,
    NEIGHBORHOOD_ACTIVITY,
    SCHOOL_ACTIVITY,
    CLASSROOM_ACTIVITY,
    WORKPLACE_ACTIVITY,
    OFFICE_ACTIVITY,
    HOSPITAL_ACTIVITY,
    AD_HOC_ACTIVITY,
    FAVORITE_PLACES
  };
};


class Activities {

public:

  static const char* activity_lookup(int idx) {
    assert(idx >= 0);
    assert(idx < Activity_index::FAVORITE_PLACES);
    switch(idx) {
      case Activity_index::HOUSEHOLD_ACTIVITY:
        return "Household";
      case Activity_index::NEIGHBORHOOD_ACTIVITY:
        return "Neighborhood";
      case Activity_index::SCHOOL_ACTIVITY:
        return "School";
      case Activity_index::CLASSROOM_ACTIVITY:
        return "Classroom";
      case Activity_index::WORKPLACE_ACTIVITY:
        return "Workplace";
      case Activity_index::OFFICE_ACTIVITY:
        return "Office";
      case Activity_index::HOSPITAL_ACTIVITY:
        return "Hospital";
      case Activity_index::AD_HOC_ACTIVITY:
        return "Ad_Hoc";
      default:
        Utils::fred_abort("Invalid Activity Type", "");
    }
    return NULL;
  }

  /**
   * Setup activities at start of run
   */
  void prepare();

  /**
   * Setup sick leave depending on size of workplace
   */
  void initialize_sick_leave();

  /**
   * Assigns an activity profile to the agent
   */
  void assign_initial_profile(Person* self);

  /**
   * Perform the daily update for an infectious agent
   *
   * @param day the simulation day
   */
  void update_activities_of_infectious_person(Person* self, int day);

  /**
   * Perform the daily update for a non-infectious agent
   *
   * @param day the simulation day
   */
  void add_visitor_to_infectious_places(Person* self, int day);

  /**
   * Perform the daily update to the schedule
   *
   * @param self the agent
   * @param day the simulation day
   */
  void update_schedule(Person* self, int day);

  /**
   * Allows updates to schedule, even when already updated
   * for the day
   *
   * @param self the agent
   * @param day the simulation day
   * @param force should we force the update or not
   */
  void update_schedule(Person* self, int day, bool force);

  /**
   * Decide whether to stay home if symptomatic.
   * May depend on availability of sick leave at work.
   *
   * @param day the simulation day
   */
  void decide_whether_to_stay_home(Person* self, int day);

  /**
   * Decide whether to seek healthcare if symptomatic.
   *
   * @param day the simulation day
   */
  void decide_whether_to_seek_healthcare(Person* self, int day);

  /**
   * Have agent begin stay in a hospital
   *
   * @param self this agent
   * @param day the simulation day
   * @param length_of_stay how many days to remain hospitalized
   */
  void start_hospitalization(Person* self, int day, int length_of_stay);

  /**
   * Have agent end stay in a hospital
   *
   * @param self this agent
   */
  void end_hospitalization(Person* self);

  /**
   * Decide whether to stay home if symptomatic.
   * If Enable_default_sick_leave_behavior is set, the decision is made only once,
   * and the agent stays home for the entire symptomatic period, or never stays home.
   */
  bool default_sick_leave_behavior();

  /// returns string containing Activities schedule; does
  /// not include trailing newline
  std::string schedule_to_string(Person* self, int day);

  /**
   * Print the Activity schedule
   */
  void print_schedule(Person* self, int day);

  /// returns string containing Activities status; does
  /// not include trailing newline
  std::string to_string(Person* self);
  std::string to_string();

  /**
   * Print out information about this object
   */
  void print(Person* self);

  unsigned char get_deme_id();

  Place* get_favorite_place(int i) {
    if(this->favorite_places_map.find(i) != this->favorite_places_map.end()) {
      return this->favorite_places_map[i];
    } else {
      return NULL;
    }
  }

  void set_favorite_place(int i, Place* p) {
    if(p != NULL) {
      this->favorite_places_map[i] = p;
    } else {
      std::map<int, Place*>::iterator itr;
      itr = this->favorite_places_map.find(i);
      if(itr != this->favorite_places_map.end()) {
        this->favorite_places_map.erase(itr);
      }
    }
  }

  void set_household(Place* p) {
    set_favorite_place(Activity_index::HOUSEHOLD_ACTIVITY, p);
  }

  void set_neighborhood(Place* p) {
    set_favorite_place(Activity_index::NEIGHBORHOOD_ACTIVITY, p);
  }

  void set_school(Place* p) {
    set_favorite_place(Activity_index::SCHOOL_ACTIVITY, p);
  }

  void set_classroom(Place* p) {
    set_favorite_place(Activity_index::CLASSROOM_ACTIVITY, p);
  }

  void set_workplace(Place* p) {
    set_favorite_place(Activity_index::WORKPLACE_ACTIVITY, p);
  }

  void set_office(Place* p) {
    set_favorite_place(Activity_index::OFFICE_ACTIVITY, p);
  }

  void set_hospital(Place* p) {
    set_favorite_place(Activity_index::HOSPITAL_ACTIVITY, p);
  }

  void set_ad_hoc(Place* p) {
    set_favorite_place(Activity_index::AD_HOC_ACTIVITY, p);
  }

  void move_to_new_house(Person* self, Place* house);
  void change_household(Person* self, Place* place);
  void change_school(Person* self, Place* place);
  void change_workplace(Person* self, Place* place, int include_office = 1);

  Place* get_temporary_household() {
    if(this->tmp_favorite_places_map->find(Activity_index::HOUSEHOLD_ACTIVITY)
        != this->tmp_favorite_places_map->end()) {
      return (*this->tmp_favorite_places_map)[Activity_index::HOUSEHOLD_ACTIVITY];
    } else {
      return NULL;
    }
  }

  /**
   * @return a pointer to this agent's Household
   *
   * If traveling, this is the household that is currently
   * being visited
   */
  Place* get_household() {
    if(Global::Enable_Hospitals && this->is_hospitalized) {
      return get_temporary_household();
    } else {
      return get_favorite_place(Activity_index::HOUSEHOLD_ACTIVITY);
    }
  }

  /**
   * @return a pointer to this agent's permanent Household
   *
   * If traveling, this is the Person's permanent residence,
   * NOT the household being visited
   */
  Place* get_permanent_household() {
    if(this->is_traveling && this->is_traveling_outside) {
      return get_temporary_household();
    } else if(Global::Enable_Hospitals && this->is_hospitalized) {
      return get_temporary_household();
    } else {
      return get_household();
    }
  }

  /**
   * @return a pointer to this agent's Neighborhood
   */
  Place* get_neighborhood() {
    return get_favorite_place(Activity_index::NEIGHBORHOOD_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's School
   */
  Place* get_school() {
    return get_favorite_place(Activity_index::SCHOOL_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's Classroom
   */
  Place* get_classroom() {
    return get_favorite_place(Activity_index::CLASSROOM_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's Workplace
   */
  Place* get_workplace() {
    return get_favorite_place(Activity_index::WORKPLACE_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's Office
   */
  Place* get_office() {
    return get_favorite_place(Activity_index::OFFICE_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's Hospital
   */
  Place* get_hospital() {
    return get_favorite_place(Activity_index::HOSPITAL_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's Ad Hoc location
   */
  Place* get_ad_hoc() {
    return get_favorite_place(Activity_index::AD_HOC_ACTIVITY);
  }

  /**
   * Assign the agent to a School
   */
  void assign_school(Person* self);

  /**
   * Assign the agent to a Classroom
   */
  void assign_classroom(Person* self);

  /**
   * Assign the agent to a Workplace
   */
  void assign_workplace(Person* self);

  /**
   * Assign the agent to an Office
   */
  void assign_office(Person* self);

  /**
   * Assign the agent to a Hospital
   */
  void assign_hospital(Person* self, Place* place);

  /**
   * Assign the agent to an Ad Hoc Location
   * Note: this is meant to be assigned at will
   */
  void assign_ad_hoc_place(Person* self, Place* place);

  void unassign_ad_hoc_place(Person * self);

  /**
   * Update the agent's profile
   */
  void update_profile(Person* self);

  /**
   * Unenroll from all the favorite places
   */
  void terminate(Person* self);

  /**
   * The agent begins traveling.  The favorite places for this agent are stored, and it gets a new schedule
   * based on the agent it is visiting.
   *
   * @param visited a pointer to the Person object being visited
   * @see Activities::store_favorite_places()
   */
  void start_traveling(Person* self, Person* visited);

  /**
   * The agent stops traveling and returns to its original favorite places
   * @see Activities::restore_favorite_places()
   */
  void stop_traveling(Person* self);

  /**
   * @return <code>true</code> if the agent is traveling, <code>false</code> otherwise
   */
  bool get_travel_status() {
    return this->is_traveling;
  }

  bool become_a_teacher(Person* self, Place* school);

  /**
   * Return the number of other agents in an agent's neighborhood, school,
   * and workplace.
   */
  int get_degree();

  int get_household_size();
  int get_group_size(int index) {
    int size = 0;
    if(get_favorite_place(index) != NULL) {
      size = get_favorite_place(index)->get_size();
    }
    return size;
  }

//  float get_sick_days_remaining() {
//    return this->sick_days_remaining;
//  }

  bool is_sick_leave_available() {
    return this->sick_leave_available;
  }

  int get_sick_days_absent() {
    return this->my_sick_days_absent;
  }

  int get_sick_days_present() {
    return this->my_sick_days_present;
  }

  static void update(int day);
  static void end_of_run();
  static void before_run();

  void set_profile(char _profile) {
    this->profile = _profile;
  }

  bool is_teacher() {
    return this->profile == TEACHER_PROFILE;
  }

  bool is_student() {
    return this->profile == STUDENT_PROFILE;
  }

  bool is_college_student() {
    return this->profile == COLLEGE_STUDENT_PROFILE;
  }

  bool is_prisoner() {
    return this->profile == PRISONER_PROFILE;
  }

  bool is_college_dorm_resident() {
    return profile == COLLEGE_STUDENT_PROFILE;
  }

  bool is_military_base_resident() {
    return this->profile == MILITARY_PROFILE;
  }

  bool is_nursing_home_resident() {
    return this->profile == NURSING_HOME_RESIDENT_PROFILE;
  }

  bool is_hospital_staff() {
    bool ret_val = false;
    
    if(this->profile == WORKER_PROFILE || this->profile == WEEKEND_WORKER_PROFILE) {
      if(get_workplace() != NULL && get_household() != NULL) {
        if(get_workplace()->is_hospital() &&
           !get_household()->is_hospital()) {
          ret_val = true;
        }
      }
    }
    
    return ret_val;
  }

  bool is_prison_staff() {
    bool ret_val = false;
    
    if(this->profile == WORKER_PROFILE || this->profile == WEEKEND_WORKER_PROFILE) {
      if(get_workplace() != NULL && get_household() != NULL) {
        if(get_workplace()->is_prison() &&
           !get_household()->is_prison()) {
          ret_val = true;
        }
      }
    }
    
    return ret_val;
  }

  bool is_college_dorm_staff() {
    bool ret_val = false;
    
    if(this->profile == WORKER_PROFILE || this->profile == WEEKEND_WORKER_PROFILE) {
      if(get_workplace() != NULL && get_household() != NULL) {
        if(get_workplace()->is_college() &&
           !get_household()->is_college()) {
          ret_val = true;
        }
      }
    }
    
    return ret_val;
  }

  bool is_military_base_staff() {
    bool ret_val = false;
    
    if(this->profile == WORKER_PROFILE || this->profile == WEEKEND_WORKER_PROFILE) {
      if(get_workplace() != NULL && get_household() != NULL) {
        if(get_workplace()->is_military_base() &&
           !get_household()->is_military_base()) {
          ret_val = true;
        }
      }
    }
    
    return ret_val;
  }

  bool is_nursing_home_staff() {
    bool ret_val = false;
    
    if(this->profile == WORKER_PROFILE || this->profile == WEEKEND_WORKER_PROFILE) {
      if(get_workplace() != NULL && get_household() != NULL) {
        if(get_workplace()->is_nursing_home() &&
           !get_household()->is_nursing_home()) {
          ret_val = true;
        }
      }
    }
    
    return ret_val;
  }

  static void initialize_static_variables();

  char get_profile() {
    return this->profile;
  }

  int get_grade() {
    return this->grade;
  }
  
  void set_grade(int n) {
    this->grade = n;
  }

  int get_visiting_health_status(Person* self, Place* place, int day, int disease_id);

  void update_activities_while_traveling(Person* self, int day);

  void set_return_from_travel_sim_day(int day) {
    this->return_from_travel_sim_day = day;
  }

  int get_return_from_travel_sim_day() {
    return this->return_from_travel_sim_day;
  }

  static void print_stats(int day);

private:

  // current favorite places
  std::map<int, Place*> favorite_places_map;
  // list of favorite places, stored while traveling
  std::map<int, Place*> * tmp_favorite_places_map;

  Place* home_neighborhood;
  std::bitset<Activity_index::FAVORITE_PLACES> on_schedule; // true iff favorite place is on schedule
  int schedule_updated;                       // date of last schedule update
  bool is_traveling;                         // true if traveling
  bool is_traveling_outside;                 // true if traveling outside modeled area
  char profile;                              // activities profile type
  bool is_hospitalized;
  bool is_isolated;
  int return_from_travel_sim_day;
  int sim_day_hospitalization_ends;
  int grade;

  // individual sick day variables
  short int my_sick_days_absent;
  short int my_sick_days_present;
  float sick_days_remaining;
  bool sick_leave_available;
  bool my_sick_leave_decision_has_been_made;
  bool my_sick_leave_decision;

  // static variables
  static bool is_initialized; // true if static arrays have been initialized
  static bool is_weekday;     // true if current day is Monday .. Friday
  static int day_of_week;     // day of week index, where Sun = 0, ... Sat = 6

  // run-time parameters
  static bool Enable_default_sick_behavior;
  static double Default_sick_day_prob;
  // mean number of sick days taken if sick leave is available
  static double SLA_mean_sick_days_absent;
  // mean number of sick days taken if sick leave is unavailable
  static double SLU_mean_sick_days_absent;
  // prob of taking sick days if sick leave is available
  static double SLA_absent_prob;
  // prob of taking sick days if sick leave is unavailable
  static double SLU_absent_prob;
  // extra sick days for fle
  static double Flu_days;
  
  static Age_Map* Hospitalization_prob;
  static Age_Map* Outpatient_healthcare_prob;
  static double Hospitalization_visit_housemate_prob;

  // sick days statistics
  static int Sick_days_present;
  static int Sick_days_absent;
  static int School_sick_days_present;
  static int School_sick_days_absent;
  static double Standard_sicktime_allocated_per_child;

  static const int WP_SIZE_DIST = 1;
  static const int HH_INCOME_QTILE_DIST = 2;
  static int Sick_leave_dist_method;
  static std::vector<double> WP_size_sl_prob_vec;
  static std::vector<double> HH_income_qtile_sl_prob_vec;

  // Statistics for presenteeism study
  static int Employees_small_with_sick_leave;
  static int Employees_small_without_sick_leave;
  static int Employees_med_with_sick_leave;
  static int Employees_med_without_sick_leave;
  static int Employees_large_with_sick_leave;
  static int Employees_large_without_sick_leave;
  static int Employees_xlarge_with_sick_leave;
  static int Employees_xlarge_without_sick_leave;

  // Statistics for childhood presenteeism study
  static double Sim_based_prob_stay_home_not_needed;
  static double Census_based_prob_stay_home_not_needed;

  // Statistics for Healthcare deficits
  static int Seeking_healthcare;
  static int Primary_healthcare_unavailable;
  static int Healthcare_accepting_insurance_unavailable;
  static int Healthcare_unavailable;

  // school change statistics
  static int entered_school;
  static int left_school;

  void clear_favorite_places() {
    this->favorite_places_map.clear();
  }

  void enroll_in_favorite_places(Person* self) {
    for(int i = 0; i < Activity_index::FAVORITE_PLACES; ++i) {
      if(get_favorite_place(i) != NULL) {
        get_favorite_place(i)->enroll(self);
      }
    }
  }

  void unenroll_from_favorite_places(Person* self) {
    for(int i = 0; i < Activity_index::FAVORITE_PLACES; ++i) {
      if(get_favorite_place(i) != NULL) {
        get_favorite_place(i)->unenroll(self);
      }
    }
    clear_favorite_places();
  }

  void make_favorite_places_infectious(Person* self, int dis) {
    for(int i = 0; i < Activity_index::FAVORITE_PLACES; ++i) {
      if(this->on_schedule[i]) {
        assert(get_favorite_place(i) != NULL);
        get_favorite_place(i)->add_infectious(dis, self);
      }
    }
  }

  void join_susceptible_lists_at_favorite_places(Person* self, int dis) {
    for(int i = 0; i < Activity_index::FAVORITE_PLACES; ++i) {
      if(this->on_schedule[i]) {
        assert(get_favorite_place(i) != NULL);
        if(get_favorite_place(i)->is_infectious(dis)) {
          get_favorite_place(i)->add_susceptible(dis, self);
        }
      }
    }
  }

  void join_nonsusceptible_lists_at_favorite_places(Person* self, int dis) {
    for(int i = 0; i < Activity_index::FAVORITE_PLACES; ++i) {
      if(this->on_schedule[i]) {
        assert(get_favorite_place(i) != NULL);
        if(get_favorite_place(i)->is_infectious(dis)) {
          get_favorite_place(i)->add_nonsusceptible(dis, self);
        }
      }
    }
  }

  /**
   * Place this agent's favorite places into a temporary location
   */

  void store_favorite_places() {
    this->tmp_favorite_places_map = new std::map<int, Place*>();
    *this->tmp_favorite_places_map = this->favorite_places_map;
  }

  /**
   * Copy the favorite places from the temporary location, then reclaim
   * the allocated memory of the temporary storage
   */

  void restore_favorite_places() {
    this->favorite_places_map = *this->tmp_favorite_places_map;
    delete this->tmp_favorite_places_map;
  }

  int get_place_id(int p) {
    return get_favorite_place(p) == NULL ? -1 : get_favorite_place(p)->get_id();
  }

  const char* get_place_label(int p) {
    return get_favorite_place(p) == NULL ? "NULL" : get_favorite_place(p)->get_label();
  }

protected:

  /**
   * Default constructor
   */
  friend class Person;
  Activities();
  void setup(Person* self, Place* house, Place* school, Place* work);

};

#endif // _FRED_ACTIVITIES_H
