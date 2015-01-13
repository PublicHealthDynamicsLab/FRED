/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Strain.h
//

#ifndef _FRED_STRAIN_H
#define _FRED_STRAIN_H

#include "Global.h"
#include "Epidemic.h"
#include <map>
#include <string>

using namespace std;

class Person;
class Disease;
class Age_Map;

class Strain {
  public:
    Strain();
    ~Strain();

    void reset();
    void setup(int s, Disease *d);
    void setup(int strain, Disease *disease, vector<int> *data, double trans, Strain *parent);
    void print();
    void print_alternate(stringstream &out);

    int get_id() {
      return id;
      }
    double get_transmissibility() {
      return transmissibility;
      }

    int get_num_data_elements();
    
    int get_data_element(int i);
  
    int get_substitutions();

  private:
    Strain *parent;
    int id;
    double transmissibility;
    Disease *disease;
    vector<int> *strain_data;
    int substitutions;
  };

#endif // _FRED_STRAIN_H
