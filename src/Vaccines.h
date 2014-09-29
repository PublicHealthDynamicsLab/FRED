/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Vaccines.h
//

#ifndef _FRED_VACCINES_H
#define _FRED_VACCINES_H

class Vaccine;
class Vaccine_Dose;

class Vaccines {
  // Vaccines is a class used to describe a group of Vaccine Classes
public:
  // Creation Operations
  Vaccines() { }
  void setup();
  
  Vaccine *get_vaccine(int i) const { return vaccines[i];}
  
  vector <int> which_vaccines_applicable(int age) const;
  int pick_from_applicable_vaccines(int age) const;
  int get_total_vaccines_avail_today() const;
  
  
  //utility Functions
  void print() const;
  void print_current_stocks() const;
  void update(int day);
  void reset();
private:
  vector < Vaccine* > vaccines;
}; 

#endif
