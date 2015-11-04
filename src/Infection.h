/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
// File: Infection.h
//
#ifndef _FRED_INFECTION_H
#define _FRED_INFECTION_H

#include <map>
using namespace std;

// TODO: find the proper place for this:
typedef std::map<int, double> Loads;

class Disease;
class Person;
class Mixing_Group;

#define NEVER (-1)


class Infection {

public:

  // if primary infection, infector and place are null.
  Infection(Disease* disease, Person* infector, Person* host, Mixing_Group* mixing_group, int day);

  virtual ~Infection() {}

  /**
   * This static factory method is used to get an instance of a specific
   * Infection that tracks patient-specific data that depends on the
   * natural history model associated with the disease.
   *
   * @param a pointer to the disease causing this infection.
   * @return a pointer to a specific Infection object of a possible derived class
   */
  
  static Infection* get_new_infection(Disease* disease, Person* infector, Person* host, Mixing_Group* mixing_group, int day);


  /*
   * The Infection base class defines a SEIR(S) model.  For other
   * models, define the following virtual methods in a dervived class.
   */

  virtual void setup();

  virtual void update(int day);

  virtual double get_infectivity(int day);

  virtual double get_symptoms(int day);

  virtual void print();

  virtual void report_infection(int day);

  // methods for antivirals
  virtual bool provides_immunity() {
    return true;
  }

  virtual void modify_infectivity(double multp) {} 

  virtual void advance_seed_infection(int days_to_advance) {}
  virtual void modify_infectious_period(double multp, int cur_day) {}
  virtual void modify_symptomatic_period(double multp, int cur_day) {}
  virtual double get_susceptibility() {
    return 1.0;
  }

  virtual void modify_asymptomatic_period(double multp, int cur_day) {}
  virtual void modify_develops_symptoms(bool symptoms, int cur_day) {}

  Disease* get_disease() {
    return this->disease;
  }

  Person* get_host() {
    return this->host;
  }

  Person* get_infector() {
    return this->infector;
  }

  Mixing_Group* get_mixing_group() {
    return this->mixing_group;
  }

  int get_exposure_date() {
    return this->exposure_date;
  }

  int get_infectious_start_date() {
    return this->infectious_start_date;
  }

  int get_infectious_end_date() {
    return this->infectious_end_date;
  }

  int get_symptoms_start_date() {
    return this->symptoms_start_date;
  }

  int get_symptoms_end_date() {
    return this->symptoms_end_date;
  }

  int get_immunity_end_date() {
    return this->immunity_end_date;
  }

  bool is_infectious(int day) {
    if (this->infectious_start_date != NEVER) {
      return (this->infectious_start_date <= day && day < this->infectious_end_date);
    } else {
      return false;
    }
  }

  bool is_symptomatic(int day) {
    if(this->symptoms_start_date != NEVER) {
      return (this->symptoms_start_date <= day && day < this->symptoms_end_date);
    } else {
      return false;
    }
  }

  void set_fatal_infection() {
    this->infection_is_fatal_today = true;
  }

  bool is_fatal(int day) {
    return this->infection_is_fatal_today;
  }

protected:

  // associated disease
  Disease* disease;

  // people involved
  Person* infector;
  Person* host;

  // where infection was caught
  Mixing_Group* mixing_group;

  // date of infection (all dates are in sim_days)
  int exposure_date;

  // person is infectious starting infectious_start_date until infectious_end_date
  int infectious_start_date;
  int infectious_end_date;

  // person is symptomatic starting symptoms_start_date until symptoms_end_date
  bool will_develop_symptoms;
  int symptoms_start_date;		      // -1 if never symptomatic
  int symptoms_end_date;		      // -1 if never symptomatic

  // person is immune from infection starting on exposure_date until immunity_end_date
  int immunity_end_date;	  // -1 if immune forever after recovery

  // is infection fata today?
  bool infection_is_fatal_today;
};

#endif // _FRED_INFECTION_H

