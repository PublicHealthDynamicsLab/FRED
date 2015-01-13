/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: StrainTable.h
//

#ifndef _FRED_StrainTable_H
#define _FRED_StrainTable_H

#include <vector>
#include <map>
#include <fstream>

class Strain;
class Disease;

using namespace std;

class Disease;
class Strain;

class StrainTable {
  public:
    StrainTable();
    ~StrainTable();

    void setup(Disease *d); // Initial strains
    void reset();

    void add(Strain *s);
    int add(vector<int> &strain_data, double transmissibility, int parent);
    double get_transmissibility(int id);

    int get_num_strains() { return strains->size(); }

    int get_num_strain_data_elements(int strain);
    int get_strain_data_element(int strain, int i);
    void printStrain(int strain_id, stringstream &out);
  
    int get_substitutions(int strain_id);
  private:
    Disease *disease;
    vector<Strain *> *strains;
    int originalStrains;
  };

#endif // _FRED_StrainTable_H
