/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: AV_Manager.h
//

#ifndef _FRED_AV_MANAGER_H
#define _FRED_AV_MANAGER_H

#include "Antivirals.h"
#include "Manager.h"

#define AV_POLICY_PERCENT_SYMPT 0
#define AV_POLICY_GIVE_EVERYONE 1

class Population;
class Person;
class Policy; 

class AV_Manager: public Manager {
public:

  /**
   * Default constructor. Does not set 'do_av' bool, thereby disabling antirals.
   */

  AV_Manager();

  /**
   * Constructor that sets the Population to which this AV_Manager is tied.
   * This constructor also checks to see if the number of antivirals given in the
   * params file greater than one and, if so, sets the 'do_av' bool.
   */
  AV_Manager(Population *_pop);
  
  //Parameters
  /**
   * @return  <code>true</code> if antivirals are being disseminated <code>false</code> otherwise
   */
  bool do_antivirals()             const { return do_av; }

  /**
   * @return overall_start_day
   */
  int get_overall_start_day()      const { return overall_start_day; }

  /**
   * @return a pointer to current_av
   */
  Antiviral* get_current_av()      const { return current_av; }
  
  //Paramters
  /**
   * @return a pointer to this manager's Antiviral package
   */
  Antivirals* get_antivirals()     const { return av_package; }

  /**
   * @return a count of this manager's antivirals
   * @see Antivirals::get_number_antivirals()
   */
  int get_num_antivirals()         const { return av_package->get_number_antivirals(); }

  /**
   * @return <code>true</code> if policies are set, <code>false</code> otherwise
   */
  bool get_are_policies_set()      const { return are_policies_set; }
  
  // Manager Functions
  /**
   * Push antivirals to agents, needed for prophylaxis
   *
   * @param day the simulation day
   */
  void disseminate(int day);
  
  // Utility Functions
  /**
   * Perform the daily update for this object
   *
   * @param day the simulation day
   */
  void update(int day);

  /**
   * Put this object back to its original state
   */
  void reset();

  /**
   * Print out information about this object
   */
  void print();
  
private:
  bool do_av;                      //Whether or not antivirals are being disseminated
  Antivirals* av_package;          //The package of avs available to this manager
  // Parameters 
  int overall_start_day;           //Day to start the av procedure

  /**
   * Member to set the policy of all of the Antivirals
   */
  void set_policies();
  bool are_policies_set;         //Ensure that the policies for AVs have been set.
  
  Antiviral* current_av;           //NEED TO ELIMINATE, HIDDEN to IMPLEMENTATION
};

#endif
