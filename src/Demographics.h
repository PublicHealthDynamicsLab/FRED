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
// File: Demographics.h
//

#ifndef _FRED_DEMOGRAPHICS_H
#define _FRED_DEMOGRAPHICS_H

#include "Global.h"
#include "Events.h"
#include <vector>
#include <map>

using namespace std;

class Person;
class Date;

class Demographics {
public:

  static const int MAX_AGE = 100;
  static const int MIN_PREGNANCY_AGE = 12;
  static const int MAX_PREGNANCY_AGE = 60;
  static const double MEAN_PREG_DAYS;
  static const double STDDEV_PREG_DAYS;

  // default constructor and destructor
  Demographics();
  ~Demographics();

  /**
   * setup for two-phase construction; sets all of the attributes of a Demographics object
   * @param self pointer to the Person object with which this Demographics object is associated
   * @param _age
   * @param _sex (M or F)
   * @param day the simulation day
   * @param is_newborn needed to know how to set the date of birth
   */
  void setup( Person * self, short int _age, char _sex, short int _race,
	      short int rel, int day, bool is_newborn = false );

  static void initialize_static_variables();

  void initialize_demographic_dynamics(Person *self);

  /**
   * Perform the daily update for this object
   *
   * @param day the simulation day
   *
   */
  static void update(int day);

  /**
   * @return the number of days the agent has been alive / 365.25
   */
  double get_real_age() const;

  /**
   * @return the agent's age
   */
  short int get_age()      { return age; }

  /**
   * @return the agent's sex
   */
  const char get_sex() const { return sex; }

  /**
   * @return <code>true</code> if the agent is pregnant, <code>false</code> otherwise
   */
  const bool is_pregnant() const { return pregnant; }

  void unset_pregant() { pregnant = false; }

  /**
   * @return <code>true</code> if the agent is deceased, <code>false</code> otherwise
   */
  const bool is_deceased() const { return deceased; }

  /**
   * Print out information about this object
   */
  void print();

  /**
   * @return the agent's init_age
   */
  short int get_init_age() const { return init_age; }

  /**
   * @return the agent's race
   */
  short int get_race() const           { return race; }

  void set_relationship(int rel) { relationship = rel; }

  const int get_relationship() const { return relationship; }

  /**
   * @return <code>true</code> if the agent is a householder, <code>false</code> otherwise
   */
  bool is_householder() { return relationship == Global::HOUSEHOLDER; }

  void make_householder() { relationship = Global::HOUSEHOLDER; }

  int get_day_of_year_for_birthday_in_nonleap_year();

  /**
   * Perform the necessary changes to the demographics on an agent's birthday
   */
  void birthday( Person * self, int day );

  void set_number_of_children(int n) { number_of_children = n; }
  int get_number_of_children() { return number_of_children; }

  void terminate( Person * self ) { }

  int get_conception_sim_day() { return conception_sim_day; }
  void set_conception_sim_day(int day) { conception_sim_day = day; }
  int get_maternity_sim_day() { return maternity_sim_day; }
  void set_maternity_sim_day(int day) { maternity_sim_day = day; }
  void set_pregnant() { pregnant = true; }
  void unset_pregnant() { pregnant = false; }

  static int get_births_ytd() { return births_ytd; }
  static int get_total_births() { return total_births; }
  static int get_deaths_today() { return deaths_today; }
  static int get_deaths_ytd() { return deaths_ytd; }
  static int get_total_deaths() { return total_deaths; }

  // event handlers:
  static void conception_handler( int day, Person * self );
  static void maternity_handler( int day, Person * self );
  static void mortality_handler( int day, Person * self );
  void cancel_conception( Person * self );
  void become_pregnant( int day, Person * self );
  void cancel_pregnancy( Person * self );
  void update_birth_stats( int day, Person * self );
  void die(int day, Person * self);

  // birthday lists
  static void add_to_birthday_list(Person * person);
  static void delete_from_birthday_list(Person * person);
  static void update_people_on_birthday_list(int day);

  static void add_conception_event(int day, Person *person) {
    conception_queue->add_event(day, person);
  }

  static void delete_conception_event(int day, Person *person) {
    conception_queue->delete_event(day, person);
  }

  static void add_maternity_event(int day, Person *person) {
    maternity_queue->add_event(day, person);
  }

  static void delete_maternity_event(int day, Person *person) {
    maternity_queue->delete_event(day, person);
  }

  static void add_mortality_event(int day, Person *person) {
    mortality_queue->add_event(day, person);
  }

  static void delete_mortality_event(int day, Person *person) {
    mortality_queue->delete_event(day, person);
  }

  static void report(int day); 

private:

  static Events * conception_queue;
  static Events * maternity_queue;
  static Events * mortality_queue;

  short int init_age;			     // Initial age of the agent
  short int age;			     // Current age of the agent
  short int number_of_children;			// number of births
  short int relationship; // relationship to the householder (see Global.h)
  short int race;			  // see Global.h for race codes
  char sex;					// Male or female?
  bool pregnant;			       // is the agent pregnant?
  bool deceased;				// Is the agent deceased

  // all sim_day values assume simulation starts on day 0
  int birthday_sim_day;		  // agent's birthday in simulation time
  int deceased_sim_day;		   // When the agent (will die) / (died)
  int conception_sim_day;	  // When the agent will become pregnant
  int maternity_sim_day;	       // When the agent will give birth

  static int births_today;
  static int births_ytd;
  static int total_births;
  static int deaths_today;
  static int deaths_ytd;
  static int total_deaths;

  static std::vector <Person*> birthday_vecs[367]; //0 won't be used | day 1 - 366
  static std::map<Person*, int> birthday_map;

protected:

  friend class Person;

};

#endif // _FRED_DEMOGRAPHICS_H
