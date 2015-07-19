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

class Infection {

  // thresholds used by determine_transition_dates()
  double trajectory_infectivity_threshold;
  double trajectory_symptomaticity_threshold;

public:
  // if primary infection, infector and place are null.
  // if mutation, place is null.
  Infection(Disease* s, Person* infector, Person* infectee, Place* place, int day);
  ~Infection();

  void chronic_update(int today);

  /**
   * Perform the daily update for this object
   *
   * @param day the simulation day
   */
  void update(int today);

  // general
  /**
   * @return a pointer to the Disease object
   */
  Disease* get_disease() const {
    return this->disease;
  }

  /**
   * @return the a pointer to the Person who was the infector for this Infection
   */
  Person* get_infector() const {
    return this->infector;
  }

  /**
   * @return the pointer to the place where the infection occured
   */
  Place* get_infected_place() const {
    return this->place;
  }

  /**
   * @return the infectee_count
   */
  int get_infectee_count() const {
    return this->infectee_count;
  }

  /**
   * Increment the infectee_count
   * @return the new infectee_count
   */
  int add_infectee() {
    return ++(this->infectee_count);
  }

  /**
   * Print out information about this object
   */
  void print() const;

  /**
   * Print infection information to the log files
   */
  void report_infection(int day) const;

  // chrono

  /**
   * @return the exposure_date
   */
  int get_exposure_date() const {
    return this->exposure_date;
  }

  /**
   * @return the infectious_date
   */
  int get_infectious_date() const {
    return this->infectious_date;
  }

  /**
   * @return the symptomatic_date
   */
  int get_symptomatic_date() const {
    return this->symptomatic_date;
  }

  /**
   * @return the asymptomatic_date
   */
  int get_asymptomatic_date() const {
    return this->asymptomatic_date;
  }

  /**
   * @return the recovery_date
   */
  int get_recovery_date() const {
    return this->recovery_date;
  }

  /**
   * @return the recovery_date + the recovery_period
   */
  int get_susceptible_date() const {
    if(this->recovery_period > -1) {
      return get_recovery_date() + this->recovery_period;
    } else {
      return INT_MAX;
    }
  }

  /**
   * @param period the new susceptibility_period
   */
  int set_susceptibility_period(int period) {
    return this->susceptibility_period = period;
  }

  /**
   * @return exposure_date + susceptibility_period
   */
  int get_unsusceptible_date() const {
    return this->exposure_date + this->susceptibility_period;
  }

  void advance_seed_infection(int days_to_advance);

  /**
   * @param multp the multiplier
   * @param day the simulation day
   */
  void modify_asymptomatic_period(double multp, int cur_day);

  /**
   * @param multp the multiplier
   * @param day the simulation day
   */
  void modify_symptomatic_period(double multp, int cur_day);

  /**
   * @param multp the multiplier
   * @param day the simulation day
   */
  void modify_infectious_period(double multp, int cur_day);

  // parameters
  /**
   * @return <code>true</code> if the agent is infectious, <code>false</code> if not
   */
  bool is_infectious() {
    return this->infectivity > this->trajectory_infectivity_threshold;
  }

  /**
   * @return <code>true</code> if the agent is symptomatic, <code>false</code> if not
   */
  bool is_symptomatic() {
    return this->symptoms > this->trajectory_symptomaticity_threshold;
  }

  /**
   * @return the susceptibility
   */
  double get_susceptibility() const {
    return this->susceptibility;
  }

  /**
   * @return the symptoms
   */
  double get_symptoms() const {
    return this->symptoms;
  }

  /**
   * @param symptoms whether or not the agent has symptoms
   * @param cur_day the simulation day
   */
  void modify_develops_symptoms(bool symptoms, int cur_day);

  /**
   * @param multp the multiplier to be applied to susceptibility
   */
  void modify_susceptibility(double multp) {
    this->susceptibility *= multp;
  }

  /**
   * @param multp the multiplier to be applied to infectivity
   */
  void modify_infectivity(double multp) {
    this->infectivity_multp *= multp;
  }

  /**
   * @param day the simulation day
   * @return the infectivity of this infection for a given day
   */
  double get_infectivity(int day) const {
    day = day - this->exposure_date;
    Trajectory::point point = this->trajectory->get_data_point(day);
    return point.infectivity * this->infectivity_multp;
  }

  /**
   * @param day the simulation day
   * @return the symptoms of this infection for a given day
   */
  double get_symptoms(int day) const {
    day = day - this->exposure_date;
    Trajectory::point point = this->trajectory->get_data_point(day);

    return point.symptomaticity;
  }

  /**
   * @param infectee agent that this agent should try to infect
   * @param transmission
   */
  void transmit(Person* infectee, Transmission &transmission);
  void transmit(Person* infectee, Place* place);

  /**
   * @param transmission a Transmission to add
   */
  //void addTransmission(Transmission* transmission);
  /**
   * @param trajectory the new Trajectory of this Infection
   */
  void setTrajectory(Trajectory* trajectory);

  /**
   * @return a pointer to this Infection's trajectory attribute
   */
  Trajectory* get_trajectory() {
    return this->trajectory;
  }

  int get_num_past_infections();

  Past_Infection* get_past_infection(int i);

  // change the following two to use disease id
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

  bool is_susceptible;
  bool immune_response;
  bool will_be_symptomatic;
  bool infection_is_fatal_today;

  short int latent_period;
  short int asymptomatic_period;
  short int symptomatic_period;
  short int recovery_period;
  short int susceptibility_period;
  short int incubation_period;

  short int age_at_exposure;

  // people involved
  Person* infector;
  Person* host;

  // where infection was caught
  Place* place;

  // number of people infected by this infection
  int infectee_count;

  // parameters
  double susceptibility;
  double infectivity;
  double infectivity_multp;
  double symptoms;

  // trajectory contains vectors that describe the (tentative) infection course
  Trajectory* trajectory;

  // chrono data
  int exposure_date;
  // the transition dates are set in the constructor by determine_transition_dates()
  int infectious_date;
  int symptomatic_date;
  int asymptomatic_date;
  int recovery_date;
  int susceptible_date;

  void determine_transition_dates();

protected:
  // for mocks
  Infection() {
  }
};

#endif // _FRED_INFECTION_H

