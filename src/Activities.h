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

#include <bitset>
#include <map>
#include <vector>

using namespace std;

#include "Condition_List.h"
#include "Global.h"
#include "Person_Network_Link.h"
#include "Person_Place_Link.h"

class Age_Map;
class Activities_Tracking_Data;
class Hospital;
class Mixing_Group;
class Network;
class Person;
class Person_Network_Link;
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

#define SEEK_HC "Seek_hc"
#define PRIMARY_HC_UNAV "Primary_hc_unav"
#define HC_ACCEP_INS_UNAV "Hc_accep_ins_unav"
#define HC_UNAV "Hc_unav"
#define ER_VISIT "ER_visit"
#define DIABETES_HC_UNAV "Diabetes_hc_unav"
#define ASTHMA_HC_UNAV "Asthma_hc_unav"
#define HTN_HC_UNAV "HTN_hc_unav"
#define MEDICAID_UNAV "Medicaid_unav"
#define MEDICARE_UNAV "Medicare_unav"
#define PRIVATE_UNAV "Private_unav"
#define UNINSURED_UNAV "Uninsured_unav"

#define SICK_DAYS_PRESENT "sick_days_pres"
#define SICK_DAYS_ABSENT "sick_days_abs"
#define SCHL_SICK_DAYS_PRESENT "schl_sick_days_pres"
#define SCHL_SICK_DAYS_ABSENT "schl_sick_days_abs"

#define TOT_EMP_DAYS_USED_FOR_CHILD "tot_emp_days_used_for_child"
#define TOT_EMP_SL_DAYS_USED_FOR_CHILD "tot_emp_sl_days_used_for_child"
#define TOT_EMP_DAYS_USED "tot_emp_days_used"
#define TOT_EMP_MISS_WORK "tot_emp_miss_work"
#define TOT_EMP_SICK_DAYS_USED "tot_emp_sl_days_used"
#define TOT_EMP_USING_SICK_LEAVE "tot_emp_using_sl"
#define TOT_EMP_SICK_DAYS_PRES "tot_emp_sl_days_pres"
#define TOT_EMP_SYMP_WORKDAYS "tot_emp_symp_workdays"
#define TOT_EMP_SYMP_NONWORKDAYS "tot_emp_symp_nonworkdays"

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
    DAILY_ACTIVITY_LOCATIONS
  };
};

class Activities {

public:

  static const char* activity_lookup(int idx) {
    assert(idx >= 0);
    assert(idx < Activity_index::DAILY_ACTIVITY_LOCATIONS);
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
  void assign_initial_profile();

  /**
   * Perform the daily update for an infectious agent
   *
   * @param day the simulation day
   */
  void update_activities_of_infectious_person(int sim_day);

  /**
   * Perform the daily update to the schedule
   *
   * @param day the simulation day
   */
  void update_schedule(int sim_day);

  /**
   * Decide whether to stay home if symptomatic.
   * May depend on availability of sick leave at work.
   *
   * @param sim_day the simulation day
   */
  void decide_whether_to_stay_home(int sim_day);

  /**
   * Decide whether to seek healthcare if symptomatic.
   *
   * @param sim_day the simulation day
   */
  void decide_whether_to_seek_healthcare(int sim_day);

  /**
   * Have agent begin stay in a hospital
   *
   * @param day the simulation day
   * @param length_of_stay how many days to remain hospitalized
   */
  void start_hospitalization(int sim_day, int length_of_stay);

  /**
   * Have agent end stay in a hospital
   *
   * @param self this agent
   */
  void end_hospitalization();

  /**
   * Decide whether to stay home if symptomatic.
   * If Enable_default_sick_leave_behavior is set, the decision is made only once,
   * and the agent stays home for the entire symptomatic period, or never stays home.
   */
  bool default_sick_leave_behavior();

  /// returns string containing Activities schedule; does
  /// not include trailing newline
  std::string schedule_to_string(int sim_day);

  /**
   * Print the Activity schedule
   */
  void print_schedule(int sim_day);

  /// returns string containing Activities status; does
  /// not include trailing newline
  std::string to_string();

  /**
   * Print out information about this object
   */
  void print();

  unsigned char get_deme_id();

  Place* get_daily_activity_location(int i) {
    return this->link[i].get_place();
  }

  std::vector<Place*> get_daily_activity_locations() {
    std::vector<Place*> faves;
    faves.clear();
    for(int i = 0; i < Activity_index::DAILY_ACTIVITY_LOCATIONS; ++i) {
      Place* place = get_daily_activity_location(i);
      if (place != NULL) {
	      faves.push_back(place);
      }
    }
    return faves;
  }

  void set_daily_activity_location(int i, Place* place);

  void set_household(Place* p) {
    set_daily_activity_location(Activity_index::HOUSEHOLD_ACTIVITY, p);
  }

  void set_neighborhood(Place* p) {
    set_daily_activity_location(Activity_index::NEIGHBORHOOD_ACTIVITY, p);
  }

  void reset_neighborhood() {
    set_daily_activity_location(Activity_index::NEIGHBORHOOD_ACTIVITY, this->home_neighborhood);
  }

  void set_school(Place* p) {
    set_daily_activity_location(Activity_index::SCHOOL_ACTIVITY, p);
  }

  void set_classroom(Place* p) {
    set_daily_activity_location(Activity_index::CLASSROOM_ACTIVITY, p);
  }

  void set_workplace(Place* p) {
    set_daily_activity_location(Activity_index::WORKPLACE_ACTIVITY, p);
  }

  void set_office(Place* p) {
    set_daily_activity_location(Activity_index::OFFICE_ACTIVITY, p);
  }

  void set_hospital(Place* p) {
    set_daily_activity_location(Activity_index::HOSPITAL_ACTIVITY, p);
  }

  void set_ad_hoc(Place* p) {
    set_daily_activity_location(Activity_index::AD_HOC_ACTIVITY, p);
  }

  void move_to_new_house(Place* house);
  void change_household(Place* place);
  void change_school(Place* place);
  void change_workplace(Place* place, int include_office = 1);

  Place* get_stored_household() {
    return this->stored_daily_activity_locations[Activity_index::HOUSEHOLD_ACTIVITY];
  }

  /**
   * @return a pointer to this agent's permanent Household
   *
   * If traveling, this is the Person's permanent residence,
   * NOT the household being visited
   */
  Place* get_permanent_household() {
    if(this->is_traveling && this->is_traveling_outside) {
      return get_stored_household();
    } else if(Global::Enable_Hospitals && this->is_hospitalized) {
      return get_stored_household();
    } else {
      return get_household();
    }
  }

  /**
   * @return a pointer to this agent's Household
   */
  Place* get_household() {
    return get_daily_activity_location(Activity_index::HOUSEHOLD_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's Neighborhood
   */
  Place* get_neighborhood() {
    return get_daily_activity_location(Activity_index::NEIGHBORHOOD_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's School
   */
  Place* get_school() {
    return get_daily_activity_location(Activity_index::SCHOOL_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's Classroom
   */
  Place* get_classroom() {
    return get_daily_activity_location(Activity_index::CLASSROOM_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's Workplace
   */
  Place* get_workplace() {
    return get_daily_activity_location(Activity_index::WORKPLACE_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's Office
   */
  Place* get_office() {
    return get_daily_activity_location(Activity_index::OFFICE_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's Hospital
   */
  Place* get_hospital() {
    return get_daily_activity_location(Activity_index::HOSPITAL_ACTIVITY);
  }

  /**
   * @return a pointer to this agent's Ad Hoc location
   */
  Place* get_ad_hoc() {
    return get_daily_activity_location(Activity_index::AD_HOC_ACTIVITY);
  }

  /**
   * Assign the agent to a School
   */
  void assign_school();

  /**
   * Assign the agent to a Classroom
   */
  void assign_classroom();

  /**
   * Assign the agent to a Workplace
   */
  void assign_workplace();

  /**
   * Assign the agent to an Office
   */
  void assign_office();

  /**
   * Assign the agent to a Hospital
   */
  void assign_hospital(Place* place);

  /**
   * Assign the agent to an Ad Hoc Location
   * Note: this is meant to be assigned at will
   */
  void assign_ad_hoc_place(Place* place);

  void unassign_ad_hoc_place();

  /**
   * Find a Primary Healthcare Facility and assign it to the agent
   * @param self the agent who needs to find a Primary care facility
   */
  void assign_primary_healthcare_facility();

  Hospital* get_primary_healthcare_facility() {
    return this->primary_healthcare_facility;
  }

  /**
   * Update the agent's profile
   */
  void update_profile();

  /**
   * withdraw from all activities
   */
  void terminate();

  /**
   * The agent begins traveling.  The daily activity locations for this agent are stored, and it gets a new schedule
   * based on the agent it is visiting.
   *
   * @param visited a pointer to the Person object being visited
   * @see Activities::store_daily_activity_locations()
   */
  void start_traveling(Person* visited);

  /**
   * The agent stops traveling and returns to its original daily activity locations
   * @see Activities::restore_daily_activity_locations()
   */
  void stop_traveling();

  /**
   * @return <code>true</code> if the agent is traveling, <code>false</code> otherwise
   */
  bool get_travel_status() {
    return this->is_traveling;
  }

  bool become_a_teacher(Place* school);

  /**
   * Return the number of other agents in an agent's neighborhood, school,
   * and workplace.
   */
  int get_degree();

  int get_group_size(int index);

  bool is_sick_leave_available() {
    return this->sick_leave_available;
  }

  static void update(int day);
  static void end_of_run();
  static void before_run();
  static void report(int day);

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
    return this->profile == COLLEGE_STUDENT_PROFILE;
  }

  bool is_military_base_resident() {
    return this->profile == MILITARY_PROFILE;
  }

  bool is_nursing_home_resident() {
    return this->profile == NURSING_HOME_RESIDENT_PROFILE;
  }

  bool is_hospital_staff();

  bool is_prison_staff();

  bool is_college_dorm_staff();

  bool is_military_base_staff();

  bool is_nursing_home_staff();

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

  int get_visiting_health_status(Place* place, int sim_day, int condition_id);

  void set_return_from_travel_sim_day(int sim_day) {
    this->return_from_travel_sim_day = sim_day;
  }

  int get_return_from_travel_sim_day() {
    return this->return_from_travel_sim_day;
  }

  void create_network_link_to(Person* person, Network* network);

  void create_network_link_from(Person* person, Network* network);

  void destroy_network_link_to(Person* person, Network* network);

  void destroy_network_link_from(Person* person, Network* network);

  void add_network_link_to(Person* person, Network* network);

  void add_network_link_from(Person* person, Network* network);

  void delete_network_link_to(Person* person, Network* network);

  void delete_network_link_from(Person* person, Network* network);

  void join_network(Network* network);

  bool is_enrolled_in_network(Network* network);

  void print_network(FILE* fp, Network* network);

  bool is_connected_to(Person* person, Network* network);

  bool is_connected_from(Person* person, Network* network);

  int get_out_degree(Network* network);

  int get_in_degree(Network* network);

  void clear_network(Network* network);
  
  Person* get_end_of_link(int n, Network* network);

  static int get_index_of_sick_leave_dist(Person* per);

private:

  // pointer to owner
  Person* myself;

  // links to daily activity locations
  Person_Place_Link* link;

  // links to networks of people
  std::vector<Person_Network_Link*> networks;

  std::bitset<Activity_index::DAILY_ACTIVITY_LOCATIONS> on_schedule; // true iff daily activity location is on schedule

  // list of daily activity locations, stored while traveling
  Place** stored_daily_activity_locations;

  //Primary Care Location
  Hospital* primary_healthcare_facility;

  Place* home_neighborhood;

  // daily activity schedule:
  int schedule_updated;                       // date of last schedule update
  bool is_traveling;                         // true if traveling
  bool is_traveling_outside;                 // true if traveling outside modeled area

  char profile;                              // activities profile type
  bool is_hospitalized;
  bool is_isolated;
  int return_from_travel_sim_day;
  int sim_day_hospitalization_ends;
  int grade;

  // Do I have paid sick leave
  bool sick_leave_available;
  // Only matters if have sick_leave_available
  float sick_days_remaining;

  bool my_sick_leave_decision_has_been_made;
  bool my_sick_leave_decision;

  // static variables
  static bool is_initialized; // true if static arrays have been initialized
  static bool is_weekday;     // true if current day is Monday .. Friday
  static int day_of_week;     // day of week index, where Sun = 0, ... Sat = 6

  // run-time parameters
  static bool Enable_default_sick_behavior;
  static double Default_sick_day_prob;
  // prob of staying home if sick leave is available
  static double SLA_absent_prob;
  // prob of staying home if sick leave is unavailable
  static double SLU_absent_prob;
  // extra sick days for flu
  static double Flu_days;
  
  static int HAZEL_seek_hc_ramp_up_days;
  static double HAZEL_seek_hc_ramp_up_mult;

  static Age_Map* Hospitalization_prob;
  static Age_Map* Outpatient_healthcare_prob;
  static double Hospitalization_visit_housemate_prob;
  
  static Activities_Tracking_Data Tracking_data;

  // sick days statistics
  static double Standard_sicktime_allocated_per_child;

  static const int WP_SIZE_DIST = 1;
  static const int HH_INCOME_QTILE_DIST = 2;
  static int Sick_leave_dist_method;
  static std::vector<double> WP_size_sl_prob_vec;
  static std::vector<double> WP_size_sl_days_vec;
  static std::vector<double> HH_income_qtile_sl_prob_vec;
  static double HH_income_qtile_mean_sl_days_available;

  // Statistics for childhood presenteeism study
  static double Sim_based_prob_stay_home_not_needed;
  static double Census_based_prob_stay_home_not_needed;

  // school change statistics
  static int entered_school;
  static int left_school;

  void clear_daily_activity_locations();
  void enroll_in_daily_activity_location(int i);
  void enroll_in_daily_activity_locations();
  void update_enrollee_index(Mixing_Group* mixing_group, int new_index);
  void unenroll_from_daily_activity_location(int i);
  void unenroll_from_daily_activity_locations();
  void store_daily_activity_locations();
  void restore_daily_activity_locations();
  int get_daily_activity_location_id(int i);
  const char* get_daily_activity_location_label(int i);
  bool is_present(int sim_day, Place* place);

protected:

  /**
   * Default constructor
   */
  friend class Person;
  Activities();
  void setup(Person* self, Place* house, Place* school, Place* work);

};

struct Activities_Tracking_Data {
  // Run-wide totals (cumulative)
  int total_employees_days_used;
  int total_employees_missing_work;
  int total_employees_sick_leave_days_used;
  int total_employees_taking_sick_leave;
  int total_employees_sick_days_present;
  int total_employees_symptomatic_workdays;
  int total_employees_symptomatic_nonworkdays;
  double total_employees_days_used_for_child;
  double total_employees_sl_days_used_for_child;

  int entered_school;
  int left_school;

  // Statistics for presenteeism study
  // These are cumulative totals by sick leave groupings (e.g. workplace size level)
  vector<int> employees_with_sick_leave;
  vector<int> employees_without_sick_leave;
  vector<int> employees_days_used;
  vector<int> employees_taking_day_off;
  vector<int> employees_sick_leave_days_used;
  vector<int> employees_taking_sick_leave_day_off;
  vector<int> employees_sick_days_present;
  vector<double> employees_days_used_for_child;
  vector<double> employees_sl_days_used_for_child;
  vector<int> employees_days_at_work_with_symp_child_at_home;

  Activities_Tracking_Data() {
    this->total_employees_days_used = 0;
    this->total_employees_missing_work = 0;
    this->total_employees_sick_leave_days_used = 0;
    this->total_employees_taking_sick_leave = 0;
    this->total_employees_sick_days_present = 0;
    this->total_employees_symptomatic_workdays = 0;
    this->total_employees_symptomatic_nonworkdays = 0;
    this->total_employees_days_used_for_child = 0.0;
    this->total_employees_sl_days_used_for_child = 0.0;

    this->entered_school = 0;
    this->left_school = 0;
    this->employees_with_sick_leave.clear();
    this->employees_without_sick_leave.clear();
    this->employees_days_used.clear();
    this->employees_taking_day_off.clear();
    this->employees_sick_leave_days_used.clear();
    this->employees_taking_sick_leave_day_off.clear();
    this->employees_sick_days_present.clear();
    this->employees_days_used_for_child.clear();
    this->employees_sl_days_used_for_child.clear();
    this->employees_days_at_work_with_symp_child_at_home.clear();
  }
};

#endif // _FRED_ACTIVITIES_H
