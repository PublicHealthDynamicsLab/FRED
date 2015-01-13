/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: AV_Status.h
//

#ifndef _FRED_AVSTATUS_H
#define _FRED_AVSTATUS_H

#include <stdio.h>
#include <assert.h>
#include <iostream>
#include "Random.h"

class Antiviral;
class Antivirals;
class Health;

using namespace std;

class AV_Health{
public:
  // Creation Operations
  /**
   * Default constructor
   */
  AV_Health();

  /**
   * Constructor that sets the AV start day, the Antiviral, and the Health of this AV_Health object
   *
   * @param _av_day the av start day
   * @param _AV a pointer to the Antiviral object
   * @param _health a pointer to the Health object
   */
  AV_Health(int _av_day, Antiviral* _AV, Health* _health);
  
  //Access Members 
  /**
   * @return the AV start day
   */
  virtual int get_av_start_day()        const {return av_day;}

  /**
   * @return the AV end day
   */
  virtual int get_av_end_day()          const {return av_end_day;}

  /**
   * @return a pointer to the Health object
   */
  virtual Health * get_health()          const {return health;}

  /**
   * @return the disease
   */
  virtual int get_disease()              const {return disease;}

  /**
   * @return a pointer to the AV
   */
  virtual Antiviral * get_antiviral()    const { return AV; }
  
  /**
   * @param day the simulation day to check for
   * @return <code>true</code> if day is between the start and end days, <code>false</code> otherwise
   */
  virtual bool is_on_av(int day) const {
    return ((day >= av_day) && (day <= av_end_day));
  }
  
  /**
   * @return <code>true</code> if av_end_day is not -1, <code>false</code> otherwise
   */
  virtual bool is_effective() const {
    return (av_end_day !=-1);
  }
  
  //Utility Functions
  /**
   * Perform the daily update for this object
   *
   * @param day the simulation day
   */
  virtual void update(int day);

  /**
   * Print out information about this object
   */
  virtual void print() const;

  /**
   * Print out information about this object to the trace file
   */
  virtual void printTrace() const;
  
  
private:
  int av_day;           // Day on which the AV treatment starts
  int av_end_day;       // Day on which the AV treatment ends
  Health* health;       // Pointer to the health class for agent
  int disease;           // Disease for this AV
  Antiviral* AV;        // Pointer to the AV the person took
};

#endif
