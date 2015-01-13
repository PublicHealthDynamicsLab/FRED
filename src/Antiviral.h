/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Antiviral.h
//
#ifndef _FRED_ANTIVIRAL_H
#define _FRED_ANTIVIRAL_H

#include "Global.h"
#include <vector>
#include <stdio.h>
#include <iostream>

using namespace std;

class Disease;
class Health;
class Policy;
class AV_Health;

/**
 * Antiviral is a class to hold all of the parameters to describe a
 * single antiviral
 */
class Antiviral {

public:
  //Creation
  /**
   * Constructor that sets all of the attributes of an Antiviral object
   */
  Antiviral(int _disease, int _course_length, double _reduce_infectivity,
            double _reduce_susceptibility, double _reduce_asymp_period,
            double _reduce_sympt_period, double _prob_symptoms,
            int _initial_stock, int _total_avail, int _additional_per_day,
            double _efficacy, double* _av_cousre_start_day,
            int _max_av_course_start_day, int _start_day, bool _prophylaxis,
            double _percent_symptomatics);

  ~Antiviral() {
    if (av_course_start_day) delete[] av_course_start_day;
  }

  //Parameter Access Members
  /**
   * @return this Antiviral's disease
   */
  int     get_disease()                const { return disease;}

  /**
   * @return this Antiviral's reduce_infectivity
   */
  double  get_reduce_infectivity()    const { return reduce_infectivity;}

  /**
   * @return this Antiviral's reduce_susceptibility
   */
  double  get_reduce_susceptibility() const { return reduce_susceptibility;}

  /**
   * @return this Antiviral's reduce_asymp_period
   */
  double  get_reduce_asymp_period()   const { return reduce_asymp_period;}

  /**
   * @return this Antiviral's reduce_symp_period
   */
  double  get_reduce_symp_period()    const { return reduce_symp_period;}

  /**
   * @return this Antiviral's prob_symptoms
   */
  double  get_prob_symptoms()         const { return prob_symptoms;}

  /**
   * @return this Antiviral's course_length
   */
  int     get_course_length()         const { return course_length;}

  /**
   * @return this Antiviral's percent_symptomatics
   */
  double  get_percent_symptomatics()  const { return percent_symptomatics;}

  /**
   * @return this Antiviral's efficacy
   */
  double  get_efficacy()              const { return efficacy;}

  /**
   * @return this Antiviral's start_day
   */
  int     get_start_day()             const { return start_day;}

  /**
   * @return <code>true</code> if this Antiviral's is prophylaxis, <code>false</code> otherwise
   */
  bool    is_prophylaxis()            const { return prophylaxis;}

  // Roll operators
  /**
   * Randomly determine if will be symptomatic (determined by prob_symptoms)
   *
   * @return 1 if roll is successful, 0 if false
   */
  int roll_will_have_symp()           const;

  /**
   * Randomly determine if will be effective (determined by efficacy)
   *
   * @return 1 if roll is successful, 0 if false
   */
  int roll_efficacy()                 const;

  /**
   * Randomly determine the day to start (<code>draw_from_distribution(max_av_course_start_day, av_course_start_day)</code>)
   *
   * @return the number of days drawn
   */
  int roll_course_start_day()         const;

  // Logistics Functions
  /**
   * @return the initial_stock
   */
  int get_initial_stock()         const { return initial_stock;}

  /**
   * @return the total_avail
   */
  int get_total_avail()           const { return total_avail;}

  /**
   * @return the reserve
   */
  int get_current_reserve()       const { return reserve;}

  /**
   * @return the stock
   */
  int get_current_stock()         const { return stock;}

  /**
   * @return the additional_per_day
   */
  int get_additional_per_day()    const { return additional_per_day;}

  /**
   * @param amount how much to add to stock
   */
  void add_stock( int amount ) {
    if(amount < reserve) {
      stock += amount;
      reserve -= amount;
    }
    else {
      stock += reserve;
      reserve = 0;
    }
  }

  /**
   * @param remove how much to remove from stock
   */
  void remove_stock(int remove) {
    stock -= remove;

    if(stock < 0) stock = 0;
  }

  // Utility Functions
  /**
   * Perform the daily update for this object
   *
   * @param day the simulation day
   */
  void update(int day);

  /**
   * Print out information about this object
   */
  void print() const;

  /**
   * Put this object back to its original state
   */
  void reset();

  /**
   * Print out a daily report
   *
   * @param day the simulation day
   */
  void report(int day) const;

  /**
   * Used during debugging to verify that code is functioning properly. <br />
   * Currently, this checks the parsing of the AVs, and it returns 1 if there is a problem
   *
   * @param ndiseases the bumber of diseases
   * @return 1 if there is a problem, 0 otherwise
   */
  int quality_control(int ndiseases) const;

  /**
   * Print out current stock information
   */
  void print_stocks() const;

  //Effect the Health of Person
  /**
   * Used to alter the Health of an agent
   *
   * @param h pointer to a Health object
   * @param cur_day the simulation day
   * @param av_health pointer to a specific AV_Health object
   */
  void effect(Health *h, int cur_day, AV_Health* av_health);

  /**
   * Modify the susceptibility of an agent (through that agent's Health)
   *
   * @param health pointer to a Health object
   * @param disease which disease
   */
  void modify_susceptiblilty(Health *health, int disease);

  /**
   * Modify the infectivity of an agent (through that agent's Health)
   *
   * @param health pointer to a Health object
   * @param disease which disease
   */
  void modify_infectivity(Health *health, int disease);

  /**
   * Modify the infectivity of an agent (through that agent's Health)
   *
   * @param health pointer to a Health object
   * @param disease which disease
   * @param strain which strain of the disease
   */
  void modify_infectivity_strain(Health *health, int disease, int strain);

  /**
   * Modify the symptomaticity of an agent (through that agent's Health)
   *
   * @param health pointer to a Health object
   * @param disease which disease
   * @param cur_day the simulation day
   */
  void modify_symptomaticity(Health *health, int disease, int cur_day);

  // Policies members
  // Antivirals need a policy associated with them to determine who gets them.
  /**
   * Set the distribution policy for this Antiviral
   *
   * @param p pointer to the new Policy
   */
  void set_policy(Policy* p)                {policy = p;}

  /**
   * @return this Antiviral's distribution policy
   */
  Policy* get_policy() const                {return policy;}

  // To Be depricated
  void add_given_out(int amount)            {given_out+=amount;}
  void add_ineff_given_out(int amount)      {ineff_given_out+=amount;}

private:
  int disease;
  int course_length;               // How many days mush one take the AV
  double reduce_infectivity;       // What percentage does it reduce infectivity
  double reduce_susceptibility;    // What percentage does it reduce susceptability
  double reduce_infectious_period; // What percentage does AV reduce infectious period
  double percent_symptomatics;     // Percentage of symptomatics receiving this drug
  double reduce_asymp_period;      // What percentage does it reduce the asymptomatic period
  double reduce_symp_period;       // What percentage does it reduce the symptomatic period
  double prob_symptoms;            // What is the probability of being symptomatic
  double efficacy;                 // The effectiveness of the AV (resistance)
  int max_av_course_start_day;     // Maximum start day
  double* av_course_start_day;     // Probabilistic AV start

private:
  //Logistics
  int initial_stock;               // Amount of AV at day 0
  int stock;                       // Amount of AV currently available
  int additional_per_day;          // Amount of AV added to the system per day
  int total_avail;                 // The total amount available to the simulation
  int reserve;                     // Amount of AV left unused
  int start_day;                   // Day that AV is available to the system


  //Policy variables
  bool prophylaxis;                // Is this AV allowed for prophylaxis
  int percent_of_symptomatics;     // Percent of symptomatics that get this AV as treatment

  // For Statistics
  int given_out;
  int ineff_given_out;

  //Policy
  Policy* policy;

  //To Do... Hospitalization based policy
protected:
  Antiviral() { }
};
#endif // _FRED_ANTIVIRAL_H
