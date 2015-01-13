/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Activities.h
//

#ifndef _FRED_ACTIVITIES_H
#define _FRED_ACTIVITIES_H

#include <vector>
#include <bitset>

#include "Global.h"
#include "Random.h"
#include "Epidemic.h"

class Person;
class Place;

#define HOUSEHOLD_INDEX 0
#define NEIGHBORHOOD_INDEX 1
#define SCHOOL_INDEX 2
#define CLASSROOM_INDEX 3
#define WORKPLACE_INDEX 4
#define OFFICE_INDEX 5
#define FAVORITE_PLACES 6
#define MAX_MOBILITY_AGE 100

class Activities {
public:
  Activities (Person *person, Place *house, Place *school, Place *work);

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
  void assign_profile();

  /**
   * Perform the daily update for an infectious agent
   *
   * @param day the simulation day
   */
  void update_infectious_activities(int day, int dis);

  /**
   * Perform the daily update for a susceptible agent
   *
   * @param day the simulation day
   */
  void update_susceptible_activities(int day, int dis);

  /*
   * Appends disease-place-person items to vector passed in as reference
   */
  void get_susceptible_places_to_visit( int day, int disease_id,
      Epidemic::Place_Person_List & list, int thread_id );

  /**
   * Perform the daily update to the schedule
   *
   * @param day the simulation day
   */
  void update_schedule(int day);

  /**
   * Decide whether to stay home if symptomatic.
   * May depend on availability of sick leave at work.
   *
   * @param day the simulation day
   */
  void decide_whether_to_stay_home(int day);

  /**
   * Decide whether to stay home if symptomatic.
   * If Enable_default_sick_leave_behavior is set, the decision is made only once,
   * and the agent stays home for the entire symptomatic period, or never stays home.
   */
  bool default_sick_leave_behavior();

  /**
   * Print the Activity schedule
   */
  void print_schedule(int day);

  /**
   * Print out information about this object
   */
  void print();

  /**
   * @return a pointer to this agent's Household
   */
  Place * get_household() { return favorite_place[HOUSEHOLD_INDEX]; }

  /**
   * @return a pointer to this agent's Neighborhood
   */
  Place * get_neighborhood() const { return favorite_place[NEIGHBORHOOD_INDEX]; }

  /**
   * @return a pointer to this agent's School
   */
  Place * get_school() { return favorite_place[SCHOOL_INDEX]; }

  /**
   * @return a pointer to this agent's Classroom
   */
  Place * get_classroom() { return favorite_place[CLASSROOM_INDEX]; }

  /**
   * @return a pointer to this agent's Workplace
   */
  Place * get_workplace() { return favorite_place[WORKPLACE_INDEX]; }

  /**
   * @return a pointer to this agent's Office
   */
  Place * get_office() { return favorite_place[OFFICE_INDEX]; }

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
   * Update the agent's profile
   */
  void update_profile();

  /**
   * Update the household mobility of the agent</br>
   * Note: this is not used unless <code>Global::Enable_Mobility</code> is set to <code>true</code>
   */
  void update_household_mobility();

  /**
   * Unenroll from all the favorite places
   */
  void terminate();

  /**
   * Increments the incidence count for a given disease and its strains for all of the agent's favorite places
   */
  void addIncidence(int disease, std::vector<int> strains);

  /**
   * Increments the prevalence count for a given disease and its strains for all of the agent's favorite places
   */
  void addPrevalence(int disease, std::vector<int> strains);

  /**
   * The agent begins traveling.  The favorite places for this agent are stored, and it gets a new schedule
   * based on the agent it is visiting.
   *
   * @param visited a pointer to the Person object being visited
   * @see Activities::store_favorite_places()
   */
  void start_traveling(Person *visited);

  /**
   * The agent stops traveling and returns to its original favorite places
   * @see Activities::restore_favorite_places()
   */
  void stop_traveling();

  /**
   * @return <code>true</code> if the agent is traveling, <code>false</code> otherwise
   */
  bool get_travel_status() { return travel_status; }
  
  /**
   * Return the number of other agents in an agent's neighborhood, school,
   * and workplace.
   */
  int get_degree();

  int get_household_size();
  int get_group_size(int index);
  bool is_sick_leave_available() { return sick_leave_available; }
  int get_sick_days_absent() { return my_sick_days_absent; }
  int get_sick_days_present() { return my_sick_days_present; }
  static void update(int day);
  static void end_of_run();
  static void before_run();
  static int employees_small_with_sick_leave;
  static int employees_small_without_sick_leave;
  static int employees_med_with_sick_leave;
  static int employees_med_without_sick_leave;
  static int employees_large_with_sick_leave;
  static int employees_large_without_sick_leave;
  static int employees_xlarge_with_sick_leave;
  static int employees_xlarge_without_sick_leave;

private:
  Person * self;        // pointer to person using having this activities
  int profile;          // index of usual visit pattern
  Place * favorite_place[FAVORITE_PLACES];    // list of expected places
  std::bitset< FAVORITE_PLACES > on_schedule; // true iff favorite place is on schedule
  int schedule_updated;     // date of last schedule update
  bool travel_status;       // true if traveling
  bool traveling_outside;       // true if traveling outside modeled area
  Place ** tmp_favorite_place;  // list of favorite places, stored while traveling

  // individual sick day variables
  int my_sick_days_absent;
  int my_sick_days_present;
  double sick_days_remaining;
  bool sick_leave_available;
  bool my_sick_leave_decision_has_been_made;
  bool my_sick_leave_decision;

  // static variables
  static bool is_initialized; // true if static arrays have been initialized
  static bool is_weekday;     // true if current day is Monday .. Friday
  static int day_of_week;     // day of week index, where Sun = 0, ... Sat = 6
  static double age_yearly_mobility_rate[MAX_MOBILITY_AGE + 1];

  // run-time parameters
  static double Community_distance;     // size of community (in km)
  static double Community_prob;         // prob of visiting Community per day
  static double Home_neighborhood_prob; // prob of visiting home neighborhood per day

  static int Enable_default_sick_behavior;
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

  // sick days statistics
  static int Sick_days_present;
  static int Sick_days_absent;
  static int School_sick_days_present;
  static int School_sick_days_absent;

  /**
   * Reads the yearly_mobility_rate_file set in params.def
   * Note: this is not used unless <code>Global::Enable_Mobility</code> is set to <code>true</code>
   */
  void read_init_files();

protected:

  /**
   * Default constructor
   */
  friend class Person;
  Activities() { }

  /**
   * Place this agent's favorite places into a temporary location
   */
  void store_favorite_places();

  /**
   * Copy the favorite places from the temporary location, then reclaim the allocated memory of the temporary storage
   */
  void restore_favorite_places();
};


#endif // _FRED_ACTIVITIES_H
