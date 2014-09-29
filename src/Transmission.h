/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
// File: Transmission.h
//

#ifndef _FRED_TRANSMISSION_H
#define _FRED_TRANSMISSION_H

#include <map>

#include "Global.h"

class Person;
class Place;


class Transmission {
  public:

    // used to initialize trajectories, be careful!
    // will be deleted on destruction of Transmission
    // TODO this is not a good way to handle this.  Need to
    // look over Anuroop's code and come up with a better way
    typedef std::map< int, double > Loads; 

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
    void set_initial_loads( Loads *  _initialLoads) {
      initialLoads = _initialLoads;
    }

    /**
     * @return the map of initial loads for this Transmission
     */
    Loads * get_initial_loads() {
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

    Loads * initialLoads;
  };

#endif // _FRED_TRANSMISSION_H
