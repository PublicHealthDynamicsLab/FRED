/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Transmission.h
//

#ifndef _FRED_TRANSMISSION_H
#define _FRED_TRANSMISSION_H

class Disease;
class Mixing_Group;

class Transmission {

public:
  
  virtual ~Transmission() {}

  /**
   * This static factory method is used to get an instance of a
   * Transmission object of the specified subclass.
   *
   * @param a string containing the requested Transmission mode type
   * @return a pointer to a specific Transmission object
   */

  static Transmission* get_new_transmission(char* transmission_mode);
  static void get_parameters();
  virtual void setup(Disease* disease) = 0;
  virtual void spread_infection(int day, int disease_id, Mixing_Group* mixing_group) = 0;

protected:

  // static seasonal transmission parameters
  static double Seasonal_Reduction;
  static double* Seasonality_multiplier;
};


#endif // _FRED_TRANSMISSION_H
