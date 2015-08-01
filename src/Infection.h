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

#include "Trajectory.h"

class Disease;
class Person;
class Place;
class Past_Infection;

class Infection {

public:

  // if primary infection, infector and place are null.
  Infection(Disease* disease, Person* infector, Person* infectee, Place* place, int day);

  ~Infection() {
    if (trajectory != NULL) {
      delete trajectory;
    }
  }

  void update(int today);

  void chronic_update(int today);

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
    return place;
  }

  Place* get_infected_place() const {
    return this->place;
  }

  int get_infectee_count() const {
    return this->infectee_count;
  }

  int add_infectee() {
    return ++(this->infectee_count);
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

  int get_unsusceptible_date() const {
    // TODO: do we need to worry about susceptibility after exposure?
    return this->exposure_date /* + this->susceptibility_period */;
  }

  // unused:
  // TODO: do we need to worry about susceptibility after exposure?
  /*
  int set_susceptibility_period(int period) {
    return this->susceptibility_period = period;
  }
  */

  bool is_infectious() const;

  bool is_symptomatic() const;

  double get_susceptibility() const {
    return this->susceptibility;
  }

  double get_symptoms() const {
    return this->symptoms;
  }

  double get_infectivity(int day) const;

  double get_symptoms(int day) const;

  void set_fatal_infection() {
    this->infection_is_fatal_today = true;
  }

  bool is_fatal() const{
    return this->infection_is_fatal_today;
  }

  void setTrajectory(Trajectory* trajectory);

  Trajectory* get_trajectory() const {
    return this->trajectory;
  }

  bool provides_immunity() const {
    return this->immune_response;
  }

  // methods for antivirals

  int get_asymptomatic_date() const {
    return this->asymptomatic_date;
  }

  void modify_susceptibility(double multp) {
    this->susceptibility *= multp;
  }

  void modify_infectivity(double multp) {
    this->infectivity_multp *= multp;
  }

  void advance_seed_infection(int days_to_advance);

  void modify_asymptomatic_period(double multp, int cur_day);

  void modify_symptomatic_period(double multp, int cur_day);

  void modify_infectious_period(double multp, int cur_day);

  void modify_develops_symptoms(bool symptoms, int cur_day);

  int get_num_past_infections();

  Past_Infection* get_past_infection(int i);

  void get_strains(std::vector<int> &strains);

  void mutate(int old_strain, int new_strain, int day);

private:
  // associated disease
  Disease* disease;

  // people involved
  Person* infector;
  Person* host;

  // where infection was caught
  Place* place;

  // the transition dates are set in the constructor by set_transition_dates()
  int exposure_date;

  // person is infectious starting infectious_start_date until infectious_end_date
  int infectious_start_date;
  int infectious_end_date;

  // person is symptomatic starting symptoms_start_date until symptoms_end_date
  int symptoms_start_date;		   // 99999 if never symptomatic
  int symptoms_end_date;		   // 99999 if never symptomatic

  // person is immune from infection starting on exposure_date until immunity_end_date
  int immunity_end_date;       // 99999 if immune forever after recovery

  // the following are used in computing the effect of antivirals:
  int asymptomatic_period;	  // initial number of days asymptomatic
  int symptomatic_period;	   // initial number of days symptomatic

  // person is asymptomatic if infectious but has not symptoms (used?)
  int asymptomatic_date;		   // 99999 if never symptomatic

  // effects on the host
  bool is_susceptible;
  bool immune_response;
  bool will_be_symptomatic;
  bool infection_is_fatal_today;

  // number of people infected by this infection
  int infectee_count;

  // parameters
  double susceptibility;
  double infectivity;
  double infectivity_multp;
  double symptoms;

  // trajectory contains vectors that describe the (tentative) infectivity and symptoms for each day of the infection's course
  Trajectory* trajectory;

  void set_transition_dates();

protected:
  Infection() {}

};

#endif // _FRED_INFECTION_H

