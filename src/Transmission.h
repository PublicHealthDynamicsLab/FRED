/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
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

  typedef std::map< int, double > Loads; 

    // if primary transmission, infector and place are null.
    // if mutation, place is null.
    Transmission(Person *infector, Place *place, int day) : infector(infector), place(place), exposure_date(day) { 
    }
    ~Transmission() {
      initialLoads->clear();
      delete initialLoads;
    }

  /*
    Person * get_infector() const {
      return infector;
    }
    Place * get_infected_place() const {
      return place;
    }
    int get_exposure_date() const {
      return exposure_date;
    }
  */

    void set_initial_loads( Loads *  _initialLoads) {
      initialLoads = _initialLoads;
    }
    Loads * get_initial_loads() {
      return initialLoads;
    }

  private:
    Person *infector;
    Place *place;
    int exposure_date;
    Loads * initialLoads;
  };

#endif // _FRED_TRANSMISSION_H
