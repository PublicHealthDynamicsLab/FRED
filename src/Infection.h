/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
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

extern int Infection_model;

class Infection {
public:
  // if primary infection, infector and place are null.
  // if mutation, place is null.
  Infection(Disease *s, Person *infector, Person *infectee, Place* place, int day);
  ~Infection();

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
  Disease * get_disease() const { return disease; }

  /**
   * @return the a pointer to the Person who was the infector for this Infection
   */
  Person * get_infector() const { return infector; }

  /**
   * @return the pointer to the place where the infection occured
   */
  Place * get_infected_place() const { return place; }

  /**
   * @return the infectee_count
   */
  int get_infectee_count() const { return infectee_count; }

  /**
   * Increment the infectee_count
   * @return the new infectee_count
   */
  int add_infectee() { return ++infectee_count; }


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
  int get_exposure_date() const { return exposure_date; }

  /**
   * @return the infectious_date - offset
   */
  int get_infectious_date() const { return infectious_date - offset; }

  /**
   * @return the symptomatic_date - offset
   */
  int get_symptomatic_date() const { return symptomatic_date - offset; }

  /**
   * @return the asymptomatic_date - offset
   */
  int get_asymptomatic_date() const { return asymptomatic_date - offset; }

  /**
   * @return the recovery_date - offset
   */
  int get_recovery_date() const { return recovery_date - offset; }

  /**
   * @return the recovery_date + the recovery_period
   */
  int get_susceptible_date() const {
    if (recovery_period > -1) {
      return get_recovery_date() + recovery_period;
    } else {
      return INT_MAX;
    }
  }

  /**
   * @param period the new susceptibility_period
   */
  int set_susceptibility_period(int period) {
    return susceptibility_period = period;
  }

  /**
   * @return exposure_date + susceptibility_period
   */
  int get_unsusceptible_date() const {
    return exposure_date + susceptibility_period;
  }

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
  bool is_infectious() { return infectivity > trajectory_infectivity_threshold; }

  /**
   * @return <code>true</code> if the agent is symptomatic, <code>false</code> if not
   */
  bool is_symptomatic() { return symptoms > trajectory_symptomaticity_threshold; }

  /**
   * @return the susceptibility
   */
  double get_susceptibility() const { return susceptibility; }

  /**
   * @return the symptoms
   */
  double get_symptoms() const { return symptoms; }

  /**
   * @param symptoms whether or not the agent has symptoms
   * @param cur_day the simulation day
   */
  void modify_develops_symptoms(bool symptoms, int cur_day);

  /**
   * @param multp the multiplier to be applied to susceptibility
   */
  void modify_susceptibility(double multp) { susceptibility *= multp; }

  /**
   * @param multp the multiplier to be applied to infectivity
   */
  void modify_infectivity(double multp) { infectivity_multp *= multp; }

  /**
   * @param day the simulation day
   * @return the infectivity of this infection for a given day
   */
  double get_infectivity(int day) const {
    day = day - exposure_date + offset;
    Trajectory::point point = trajectory->get_data_point(day);
    return point.infectivity * infectivity_multp;
  }

  /**
   * @param infectee agent that this agent should try to infect
   * @param transmission
   */
  void transmit(Person *infectee, Transmission *transmission);

  /**
   * @param transmission a Transmission to add
   */
  //void addTransmission(Transmission *transmission);

  /**
   * @param trajectory the new Trajectory of this Infection
   */
  void setTrajectory(Trajectory *trajectory);

  //
  /**
   * @param s the disease we are trying to infect a person with
   * @param host the host Person
   * @param day the simulation day
   * @returns an infection for the given host and disease with exposed date and recovered date both equal to day (instant resistance to the given disease);
   */
  static Infection * get_dummy_infection(Disease *s, Person *host, int day);

  /**
   * @return a pointer to this Infection's trajectory attribute
   */
  Trajectory * get_trajectory() { return trajectory; }

  int get_num_past_infections();
  
  Past_Infection *get_past_infection(int i);

  // change the following two to use disease id
  int get_num_vaccinations();
  Past_Infection *get_vaccine_health(int i);

  bool provides_immunity() { return immune_response; }

  void get_strains(std::vector<int> &strains) { return trajectory->get_all_strains(strains); }

  int get_age_at_exposure() { return age_at_exposure; }

  Person *get_host() { return host; }

  void mutate(int old_strain, int new_strain, int day) { trajectory->mutate(old_strain, new_strain, day); }

private:
  // associated disease
  Disease *disease;
  int id;
  bool isSusceptible;

  // chrono data
  int exposure_date;

  int latent_period;
  int asymptomatic_period;
  int symptomatic_period;
  int recovery_period;
  int susceptibility_period;

  // people involved
  Person *infector;
  Person *host;

  // where infection was caught
  Place *place;

  // number of people infected by this infection
  int infectee_count;

  // parameters
  bool will_be_symptomatic;
  double susceptibility;
  double infectivity;
  double infectivity_multp;
  double symptoms;
  int infection_model;

  // trajectory contains vectors that describe the (tentative) infection course
  Trajectory * trajectory;
  // thresholds used by determine_transition_dates()
  double trajectory_infectivity_threshold;
  double trajectory_symptomaticity_threshold;

  int incubation_period;
  // the transition dates are set in the constructor by determine_transition_dates()
  int infectious_date;
  int symptomatic_date;
  int asymptomatic_date;
  int recovery_date;
  int susceptible_date;

  void determine_transition_dates();

  // offset is used for dummy infections to advance to the first infectious day
  // this offset is applied in the get_*_date() methods defined in the header
  // as well as in the update() method when evaluating the trajectory
  bool dummy;
  int offset;

  bool immune_response;
  int age_at_exposure;

  std::vector <Transmission *> transmissions;

protected:
  // for mocks
  Infection() { }
};

#endif // _FRED_INFECTION_H

