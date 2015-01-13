/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
// File: Transmission.h
//

#ifndef _FRED_TRANSMISSION_H
#define _FRED_TRANSMISSION_H

#include "Global.h"
#include <map>

class Person;
class Place;

class Transmission {
  public:
    // if primary transmission, infector and place are null.
    // if mutation, place is null.
    Transmission(Person *infector, Place *place, int day) : infector(infector), place(place), exposure_date(day) { 
      force_success = false;
    }
    ~Transmission() {
      initialLoads->clear();
      delete initialLoads;
    }

    // general
    /**
     * @return a pointer to the Person object that is the infector
     */
    Person * get_infector() const {
      return infector;
    }

    /**
     * @return a pointer to the Place object where the Transmission occured
     */
    Place * get_infected_place() const {
      return place;
    }

    /**
     * @param initialLoads the new initialLoads
     */
    void set_initial_loads(std::map<int, double> * _initialLoads) {
      initialLoads = _initialLoads;
    }

    /**
     * @return the map of initial loads for this Transmission
     */
    std::map<int, double> * get_initial_loads() {
      return initialLoads;
    }

    /**
     * Print out information about this object
     */
    void print() const;

    // chrono
    /**
     * @return the exposure_date
     */
    int get_exposure_date() const {
      return exposure_date;
    }

    bool get_forcing() {
      return force_success;
    }

    void set_forcing(bool force_success) {
      this->force_success = force_success;
    }
    
  private:
    Person *infector;
    Place *place;
    int exposure_date;
    bool force_success;

    std::map<int, double> *initialLoads;
  };

#endif // _FRED_TRANSMISSION_H
