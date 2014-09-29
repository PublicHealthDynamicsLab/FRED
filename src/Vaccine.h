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
// File: VaccineDose.h
//

#ifndef _FRED_VACCINE_H
#define _FRED_VACCINE_H

#include "Global.h"
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <vector>

using namespace std;

class Vaccine_Dose;

class Vaccine{
public:
  // Creation
  Vaccine(string _name, int _id, int _disease, 
          int _total_avail, int _additional_per_day, 
          int _start_day, int num_strains, int *_strains);
  ~Vaccine();
  
  void add_dose(Vaccine_Dose* dose);
  
  int get_disease()                 const { return disease; }
  int get_ID()                     const { return id; }
  int get_number_doses()           const { return doses.size(); }
  Vaccine_Dose* get_dose(int i)    const { return doses[i]; }
  
  // Logistics Functions
  int get_initial_stock()          const { return initial_stock; }
  int get_total_avail()            const { return total_avail; }
  int get_current_reserve()        const { return reserve; }
  int get_current_stock()          const { return stock; }
  int get_additional_per_day()     const { return additional_per_day; }
  void add_stock( int add ){ 
    if(add <= reserve){
      stock   += add;
      reserve -= add;
    }
    else{
      stock   += reserve;
      reserve  = 0;
    }
  }
  
  void remove_stock( int remove ) {
    stock-=remove;
    if(stock < 0) stock = 0;
  }

  int get_num_strains() {
    return num_strains;
  }
  
  int get_strain(int i);

  //Utility Functions
  void print() const;
  void update(int day);
  void reset();
  
private:
  string name;
  int id;                              // Which in the number of vaccines is it
  int disease;                          // Which Disease is this vaccine for
  int number_doses;                    // How many doses does the vaccine need.
  //int ages[2];                         // Applicable Ages
  vector < Vaccine_Dose* > doses;       // Data structure to hold the efficacy of each dose.
  
  int initial_stock;                   // How much available at the beginning
  int total_avail;                     // How much total in reserve
  int stock;                           // How much do you currently have
  int reserve;                         // How much is still left to system
  int additional_per_day;              // How much can be introduced into the system on a given day
  
  int start_day;                       // When to start production

  int *strains;
  int num_strains;
  
  // for statistics
  int number_delivered;
  int number_effective;
  
protected:
  Vaccine() { }
};

#endif
