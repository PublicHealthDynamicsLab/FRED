/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
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
#include <vector>
using namespace std;

class Person;
class Date;

class Demographics {
public:

  static const int MAX_AGE = 110;
  static const int MAX_PREGNANCY_AGE = 60;
  static const int MIN_PREGNANCY_AGE = 12;

  static const double MEAN_PREG_DAYS;
  static const double STDDEV_PREG_DAYS;

  ~Demographics();

  /**
   * Perform the daily update for this object
   *
   * @param day the simulation day
   *
   * See also Demographics::update_births and Demographics::update_deaths
   */
  void update(int day);

  void clear_pregnancy( Person * self );

  void update_births( Person * self, int day );

  void update_deaths( Person * self, int day );

  /**
   * @return the number of days the agent has been alive / 365.0
   */
  double get_real_age() const;

  int get_age_in_days() const;
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

  /**
   * Perform the necessary changes to the demographics on an agent's birthday
   */
  void birthday( Person * self, int day );

  /**
   * @return the agent's birth_day_of_year
   */
  short int get_birth_day_of_year() { return this->birth_day_of_year; }

  /**
   * @return the agent's birth_year
   */
  short int get_birth_year() { return this->birth_year; }
  
  void set_number_of_children(int n) { number_of_children = n; }
  int get_number_of_children() { return number_of_children; }

  void terminate( Person * self ) { }

  /**
   * This method is only used one time during initialization to load the birth rate and mortality rate arrays from files
   */
  static void initialize_static_variables();
  static void set_initial_popsize(int popsize) { target_popsize = popsize; }
  static void update_population_dynamics(int day);
  static int get_births_ytd() { return births_ytd; }
  static int get_total_births() { return total_births; }
  static int get_deaths_today() { return deaths_today; }
  static int get_deaths_ytd() { return deaths_ytd; }
  static int get_total_deaths() { return total_deaths; }

  // methods to rebalance household sizes
  static void get_housing_imbalance(int day);
  static int fill_vacancies(int day);
  static void update_housing(int day);
  static void move_college_students_out_of_dorms(int day);
  static void move_college_students_into_dorms(int day);
  static void move_military_personnel_out_of_barracks(int day);
  static void move_military_personnel_into_barracks(int day);
  static void move_inmates_out_of_prisons(int day);
  static void move_inmates_into_prisons(int day);
  static void move_patients_into_nursing_homes(int day);
  static void move_young_adults(int day);
  static void move_older_adults(int day);
  static void report_ages(int day, int house_id);
  static void swap_houses(int day);

private:

  short int init_age;			     // Initial age of the agent
  short int birth_day_of_year;
  short int birth_year;
  short int number_of_children;
  int deceased_sim_day;		   // When the agent (will die) / (died)
  int conception_sim_day;		 // When the agent will conceive
  int due_sim_day;		       // When the agent will give birth
  short int age;			     // Current age of the agent
  char sex;					// Male or female?
  bool pregnant;				// Is the agent pregnant
  bool deceased;				// Is the agent deceased
  short int relationship;   // relationship to the holder (see Global.h)
  short int race;			  // see Global.h for race codes

  static double age_yearly_mortality_rate_male[MAX_AGE + 1];
  static double age_yearly_mortality_rate_female[MAX_AGE + 1];
  static double age_yearly_birth_rate[MAX_PREGNANCY_AGE + 1];
  static double orig_age_yearly_mortality_rate_male[MAX_AGE + 1];
  static double orig_age_yearly_mortality_rate_female[MAX_AGE + 1];
  static double orig_age_yearly_birth_rate[MAX_PREGNANCY_AGE + 1];
  static bool is_initialized;
  static int target_popsize;
  static double population_growth_rate;
  static double max_adjustment_rate;
  static double college_departure_rate;
  static double military_departure_rate;
  static double prison_departure_rate;
  static double youth_home_departure_rate;
  static double adult_home_departure_rate;
  static int births_today;
  static int births_ytd;
  static int total_births;
  static int deaths_today;
  static int deaths_ytd;
  static int total_deaths;
  static int houses;
  static int * beds;
  static int * occupants;

protected:

  friend class Person;
  /**
   * Default constructor
   */
  Demographics();

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


};

#endif // _FRED_DEMOGRAPHICS_H
