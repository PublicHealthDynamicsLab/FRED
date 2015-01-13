/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Strain.cc
//

#include "Strain.h"

#include <iostream>
#include <stdio.h>
#include <new>
#include <string>
#include <sstream>
#include <cstdlib>

using namespace std;

#include "Global.h"
#include "Params.h"
#include "Population.h"
#include "Random.h"
#include "Age_Map.h"
#include "Epidemic.h"
#include "Timestep_Map.h"
#include "Disease.h"

Strain::Strain() {
  transmissibility = -1.0;
  disease = NULL;
  strain_data = NULL;
}

Strain::~Strain() {
  if(strain_data != NULL) delete strain_data;
}

void Strain::setup(int strain, Disease *disease, vector<int> *data, double trans, Strain *parent) {
  id = strain;
  this->disease = disease;
  strain_data = data;
  transmissibility = trans;
  //if(Global::Verbose > 0) print();
  this->parent = parent;
  if(parent == NULL) substitutions = 0;
  else { 
    substitutions = parent->get_substitutions() + 1;
  }
}

void Strain::setup(int strain, Disease *disease) {
  vector<int> *data = new vector<int>;
  int numAminoAcids;
  Params::get_param((char *) "num_amino_acids", &numAminoAcids);
  // HACK
  int ns;

  int distance = 3;

  Params::get_param((char *) "num_strains[0]", &ns);
  for(int i=0; i<numAminoAcids; ++i){
    int aacid = IRAND(0,63);
    if(strain == 0){
      aacid = 0;
    }
    else if(strain == 1){
      if(i < distance) aacid = 1;
      else aacid = 0;
    }
    data->push_back(aacid);
  }
  double trans;
  //Params::get_double_indexed_param("transmissibility", disease->get_id(), strain, &trans);
  trans = 1.0;

  setup(strain, disease, data, trans, NULL);

  printf("Strain setup finished\n");
  fflush(stdout);

  //if (Global::Verbose > 0) print();
}

void Strain::print_alternate(stringstream &out) {
  //out << "ID: " << id << " Ntide Sequence: ";
  int pid = -1;
  if(parent) pid = parent->get_id();
  out << id << " : " << pid << " : ";
  for(unsigned int i=0; i < strain_data->size(); ++i){
    out << strain_data->at(i) << " ";
  }
}

void Strain::print() {
  cout << "New Strain: " << id << " Trans: " << transmissibility << " Data: ";
  for(unsigned int i=0; i < strain_data->size(); ++i){
    cout << strain_data->at(i) << " ";
  } cout << endl;
}

int Strain::get_num_data_elements()
{
  return strain_data->size();
}

int Strain::get_data_element(int i)
{
  return strain_data->at(i);
}

int Strain::get_substitutions()
{
  return substitutions;
}
