/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: StrainTable.cc
//

#include "StrainTable.h"
#include "Params.h"
#include "Disease.h"
#include "Strain.h"
#include "Global.h"
#include <vector>
#include <map>

using namespace std;

StrainTable::StrainTable() {
  strains = NULL;
  originalStrains = 1;
}

StrainTable::~StrainTable() {
  delete strains;
}

void StrainTable::setup(Disease *d) {
  disease = d;
  int diseaseId = disease->get_id();

  int numStrains = 1;
  Params::get_indexed_param((char *) "num_strains", diseaseId, &numStrains);
  originalStrains = numStrains;

  if(Global::Verbose > 0) printf("Reading %d strains for disease %d\n", numStrains, diseaseId);

  strains = new vector<Strain *>();

  for(int is = 0; is < numStrains; is++) {
    Strain *s = new Strain();
    s->setup(is, disease);
    strains->push_back(s);
  }
}

void StrainTable::reset() {
  strains->clear();
  setup(disease);
}

void StrainTable::add(Strain *s) {
  strains->push_back(s);
}

int StrainTable::add(vector<int> &strain_data, double transmissibility, int parent) {
  // TODO Hash it!!

  int n = strain_data.size();
  for(int s=0; s<strains->size(); s++) {
    Strain *str = strains->at(s);
    if(str->get_num_data_elements() != n) continue;
    bool matched = true;
    for(int i=0; i<n; i++){
      if(str->get_data_element(i) != strain_data.at(i)) {
        matched = false; break;
      }
    }
    if(matched) return s;
  }

  Strain *s = new Strain();
  n = strains->size();
  vector<int> *data = new vector<int>;
  for(unsigned int i=0; i<strain_data.size(); ++i){
    data->push_back(strain_data[i]);
  }
  s->setup(n, disease, data, transmissibility, strains->at(parent));
  add(s);
  return n;
}

double StrainTable::get_transmissibility(int id) {
  return strains->at(id)->get_transmissibility();
}

int StrainTable :: get_num_strain_data_elements(int strain)    
{
  //if(strain >= strains->size()) return 0;
  return strains->at(strain)->get_num_data_elements();
}

int StrainTable :: get_strain_data_element(int strain, int i)
{
  //if(strain >= strains->size()) return -1;
  return strains->at(strain)->get_data_element(i);
}
    
void StrainTable :: printStrain(int strain_id, stringstream &out)
{
  if(strain_id >= strains->size()) return;
  strains->at(strain_id)->print_alternate(out);
}

int StrainTable :: get_substitutions(int strain_id) {
  if(strain_id >= strains->size()) return -1;
  return strains->at(strain_id)->get_substitutions();
}

