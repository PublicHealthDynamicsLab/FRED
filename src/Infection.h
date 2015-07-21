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

#include "Global.h"
#include <map>
#include "Trajectory.h"
#include "IntraHost.h"
#include <limits.h>

class Health;
class Person;
class Place;
class Disease;
class Antiviral;
class Health;
class IntraHost;
class Transmission;
class Past_Infection;

#define BIFURCATING 0
#define SEQUENTIAL 1

typedef map<int,double> viral_load_t;

class Infection {

  // thresholds used by determine_transition_dates()
  double trajectory_infectivity_threshold;
  double trajectory_symptomaticity_threshold;

public:
  // if primary infection, infector and place are null.
  Infection(Disease* s, Person* infector, Person* infectee, Place* place, int day);
  ~Infection();

  void chronic_update(int today);

  void update(int today);

  Disease* get_disease() const {
    return this->disease;
  }

  Person* get_infector() const {
    return this->infector;
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

  int get_infectious_date() const {
    return this->infectious_date;
  }

  int get_symptomatic_date() const {
    return this->symptomatic_date;
  }

  int get_asymptomatic_date() const {
    return this->asymptomatic_date;
  }

  int get_recovery_date() const {
    return this->recovery_date;
  }

  int get_susceptible_date() const {
    if(this->recovery_period > -1) {
      return get_recovery_date() + this->recovery_period;
    } else {
      return INT_MAX;
    }
  }

  int set_susceptibility_period(int period) {
    return this->susceptibility_period = period;
  }

  int get_unsusceptible_date() const {
    return this->exposure_date + this->susceptibility_period;
  }

  void advance_seed_infection(int days_to_advance);

  void modify_asymptomatic_period(double multp, int cur_day);

  void modify_symptomatic_period(double multp, int cur_day);

  void modify_infectious_period(double multp, int cur_day);

  bool is_infectious() {
    return this->infectivity > this->trajectory_infectivity_threshold;
  }

  bool is_symptomatic() {
    return this->symptoms > this->trajectory_symptomaticity_threshold;
  }

  double get_susceptibility() const {
    return this->susceptibility;
  }

  double get_symptoms() const {
    return this->symptoms;
  }

  void modify_develops_symptoms(bool symptoms, int cur_day);

  void modify_susceptibility(double multp) {
    this->susceptibility *= multp;
  }

  void modify_infectivity(double multp) {
    this->infectivity_multp *= multp;
  }

  double get_infectivity(int day) const {
    day = day - this->exposure_date;
    Trajectory::point point = this->trajectory->get_data_point(day);
    return point.infectivity * this->infectivity_multp;
  }

  double get_symptoms(int day) const {
    day = day - this->exposure_date;
    Trajectory::point point = this->trajectory->get_data_point(day);
    return point.symptomaticity;
  }

  void setTrajectory(Trajectory* trajectory);

  Trajectory* get_trajectory() {
    return this->trajectory;
  }

  int get_num_past_infections();

  Past_Infection* get_past_infection(int i);

  // TODO: change the following two to use disease id
  int get_num_vaccinations();
  Past_Infection* get_vaccine_health(int i);

  bool provides_immunity() {
    return this->immune_response;
  }

  void get_strains(std::vector<int> &strains) {
    return this->trajectory->get_all_strains(strains);
  }

  int get_age_at_exposure() {
    return this->age_at_exposure;
  }

  Person* get_host() {
    return this->host;
  }

  void mutate(int old_strain, int new_strain, int day) {
    this->trajectory->mutate(old_strain, new_strain, day);
  }

  void set_fatal_infection() {
    this->infection_is_fatal_today = true;
  }

  bool is_fatal() {
    return this->infection_is_fatal_today;
  }

  Place * get_place() {
    return place;
  }

private:
  // associated disease
  Disease* disease;

  // people involved
  Person* infector;
  Person* host;

  // where infection was caught
  Place* place;

  // chronological data
  int exposure_date;
  short int age_at_exposure;
  short int latent_period;
  short int asymptomatic_period;
  short int symptomatic_period;
  short int recovery_period;
  short int susceptibility_period;
  short int incubation_period;

  // the transition dates are set in the constructor by determine_transition_dates()
  int infectious_date;
  int symptomatic_date;
  int asymptomatic_date;
  int recovery_date;
  int susceptible_date;

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

  void determine_transition_dates();

protected:
  Infection() {}

};

#endif // _FRED_INFECTION_H

