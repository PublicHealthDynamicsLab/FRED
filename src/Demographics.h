/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Demographics.h
//

#ifndef _FRED_DEMOGRAPHICS_H
#define _FRED_DEMOGRAPHICS_H

#include "Global.h"
class Person;
class Date;

class Demographics {
public:

  static const int MAX_AGE = 110;
  static const int MAX_PREGNANCY_AGE = 60;
  static const int MIN_PREGNANCY_AGE = 12;

  static const double MEAN_PREG_DAYS;
  static const double STDDEV_PREG_DAYS;

  /**
   * Default constructor
   */
  Demographics();

  /**
   * Constructor that sets all of the attributes of a Demographics object
   * @param _self the Person object with which this Demographics object is associated
   * @param _age
   * @param _sex (M or F)
   * @param day the simulation day
   * @param is_newborn needed to know how to set the date of birth
   */
  Demographics(Person* _self, short int _age, char _sex, short int _race,
	       short int rel, int day, bool is_newborn = false);

  ~Demographics();

  /**
   * Perform the daily update for this object
   *
   * @param day the simulation day
   *
   * See also Demographics::update_births and Demographics::update_deaths
   */
  void update(int day);

  void update_births( int day );

  void update_deaths( int day );

  /**
   * @return the number of days the agent has been alive / 365.0
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
  short int get_init_age() const           { return init_age; }

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

  /**
   * Perform the necessary changes to the demographics on an agent's birthday
   */
  void birthday(int day);

  /**
   * @return the agent's birth_day_of_year
   */
  short int get_birth_day_of_year() { return this->birth_day_of_year; }

  /**
   * @return the agent's birth_year
   */
  short int get_birth_year() { return this->birth_year; }
  
  void terminate(){}

  /**
   * This method is only used one time during initialization to load the birth rate and mortality rate arrays from files
   */
  static void read_init_files();

private:
  Person *self;			  // Pointer to the person class belongs
  short int init_age;		  // Initial age of the agent
  short int birth_day_of_year;
  short int birth_year;
  short int deceased_sim_day;        // When the agent (will die) / (died)
  short int conception_sim_day;      // When the agent will conceive
  short int due_sim_day;             // When the agent will give birth
  short int age;                     // Current age of the agent
  char sex;                    // Male or female?
  bool pregnant;               // Is the agent pregnant
  bool deceased;               // Is the agent deceased
  short int relationship;   // relationship to the holder (see Global.h)
  short int race;			  // see Global.h for race codes

  static double age_yearly_mortality_rate_male[MAX_AGE + 1];
  static double age_yearly_mortality_rate_female[MAX_AGE + 1];
  static double age_yearly_birth_rate[MAX_PREGNANCY_AGE + 1];
  static double age_daily_birth_rate[MAX_PREGNANCY_AGE + 1];
  static bool is_initialized;

};

#endif // _FRED_DEMOGRAPHICS_H
