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

class Disease;
class Person;
class Place;

class Infection {

public:

  // if primary infection, infector and place are null.
  Infection(Disease* disease, Person* infector, Person* host, Place* place, int day);

  ~Infection() {}

  void update(int today);

  Disease* get_disease() const {
    return this->disease;
  }

  Person* get_host() const {
    return this->host;
  }

  Person* get_infector() const {
    return this->infector;
  }

  Place * get_place() const {
    return this->place;
  }

  void print() const;

  void report_infection(int day) const;

  int get_exposure_date() const {
    return this->exposure_date;
  }

  int get_infectious_start_date() const {
    return this->infectious_start_date;
  }

  int get_infectious_end_date() const {
    return this->infectious_end_date;
  }

  int get_symptoms_start_date() const {
    return this->symptoms_start_date;
  }

  int get_symptoms_end_date() const {
    return this->symptoms_end_date;
  }

  int get_immunity_end_date() const {
    return this->immunity_end_date;
  }

  bool is_infectious(int day) const;

  bool is_symptomatic(int day) const;

  double get_infectivity(int day) const;

  double get_symptoms(int day) const;

  bool is_fatal() const { return false; }

  bool provides_immunity() { return true; }

  // methods for antivirals

  void modify_infectivity(double multp) {} 

  void advance_seed_infection(int days_to_advance) {}

  void modify_infectious_period(double multp, int cur_day) {}

  void modify_symptomatic_period(double multp, int cur_day) {}

  double get_susceptibility() { return 1.0; }

  void modify_asymptomatic_period(double multp, int cur_day) {}

  void modify_develops_symptoms(bool symptoms, int cur_day) {}

protected:

  // method to set the transition dates
  void set_transition_dates();

  // associated disease
  Disease* disease;

  // people involved
  Person* infector;
  Person* host;

  // where infection was caught
  Place* place;

  // date of infection (all dates are in sim_days)
  int exposure_date;

  // person is infectious starting infectious_start_date until infectious_end_date
  int infectious_start_date;
  int infectious_end_date;

  // person is symptomatic starting symptoms_start_date until symptoms_end_date
  int symptoms_start_date;		      // -1 if never symptomatic
  int symptoms_end_date;		      // -1 if never symptomatic

  // person is immune from infection starting on exposure_date until immunity_end_date
  int immunity_end_date;	  // -1 if immune forever after recovery

protected:
  Infection() {}

};

#endif // _FRED_INFECTION_H

