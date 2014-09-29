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
// File: Antivirals.cc
//

#include <stdio.h>
#include <new>

using namespace std;
#include "Antivirals.h"
#include "Antiviral.h"
#include "Global.h"
#include "Params.h"
#include "Person.h"
#include "Health.h"

Antivirals::Antivirals(){
  int nav;
  
  Params::get_param_from_string("number_antivirals",&nav);
  
  for(int iav=0;iav<nav;iav++){
    int Disease, CorLength, InitSt, TotAvail, PerDay;
    double RedInf, RedSusc, RedASympPer, RedSympPer, ProbSymp, Eff, PerSympt;
    int StrtDay,Proph;
    bool isProph;
    
    Params::get_indexed_param("av_disease",iav,&Disease);
    Params::get_indexed_param("av_initial_stock",iav,&InitSt);
    Params::get_indexed_param("av_total_avail",iav,&TotAvail);
    Params::get_indexed_param("av_additional_per_day",iav,&PerDay);
    //get_indexed_param("av_percent_resistance",iav,&Eff);
    Eff = 1.0; // Not implemented yet
    Params::get_indexed_param("av_course_length",iav,&CorLength);
    Params::get_indexed_param("av_reduce_infectivity",iav,&RedInf);
    Params::get_indexed_param("av_reduce_susceptibility",iav,&RedSusc);
    Params::get_indexed_param("av_reduce_symptomatic_period",iav,&RedSympPer);
    Params::get_indexed_param("av_reduce_asymptomatic_period",iav,&RedASympPer);
    Params::get_indexed_param("av_prob_symptoms",iav,&ProbSymp);
    Params::get_indexed_param("av_start_day",iav,&StrtDay);
    Params::get_indexed_param("av_prophylaxis",iav,&Proph);
    if(Proph == 1) isProph= true;
    else isProph = false;
    Params::get_indexed_param("av_percent_symptomatics",iav,&PerSympt);
    int n;
    Params::get_indexed_param("av_course_start_day",iav,&n);
    double* AVCourseSt= new double [n];
    int MaxAVCourseSt = Params::get_indexed_param_vector("av_course_start_day",iav, AVCourseSt) -1;
    
    AVs.push_back(new Antiviral(Disease, CorLength, RedInf, 
                                RedSusc, RedASympPer, RedSympPer,
                                ProbSymp, InitSt, TotAvail, PerDay, 
                                Eff, AVCourseSt, MaxAVCourseSt,
                                StrtDay, isProph, PerSympt) );
    
  }
  print();
  quality_control(Global::Diseases);
}

int Antivirals::get_total_current_stock() const {
  int sum = 0;
  for(unsigned int i=0;i<AVs.size();i++) sum += AVs[i]->get_current_stock();
  return sum;
}

vector < Antiviral* > Antivirals::find_applicable_AVs(int disease) const {
  vector <Antiviral* > avs;
  for(unsigned int iav=0;iav< AVs.size();iav++){
    if(AVs[iav]->get_disease() == disease && AVs[iav]->get_current_stock() != 0){
      avs.push_back(AVs[iav]);
    }
  }
  return avs;
}

vector < Antiviral* > Antivirals::prophylaxis_AVs() const {
  vector <Antiviral*> avs;
  for(unsigned int iav=0;iav< AVs.size();iav++){
    if(AVs[iav]->is_prophylaxis()){
      avs.push_back(AVs[iav]);
    }
  }
  return avs;
}

void Antivirals::print() const {
  cout << "\nAntiviral Package \n";
  cout << "There are "<< AVs.size() << " antivirals to choose from. \n";
  for(unsigned int iav=0;iav<AVs.size(); iav++){
    cout << "\nAntiviral #" << iav << "\n";
    AVs[iav]->print();
  }
  cout << "\n\n";
}

void Antivirals::print_stocks() const {
  for(unsigned int iav = 0; iav < AVs.size(); iav++){
    cout <<"\n Antiviral #" << iav;
    AVs[iav]->print_stocks();
    cout <<"\n";
  }
}

void Antivirals::quality_control(int ndiseases) const {  
  for(unsigned int iav = 0;iav < AVs.size();iav++) {
    if (Global::Verbose > 1) {
      AVs[iav]->print();
    }
    if(AVs[iav]->quality_control(ndiseases)){
      cout << "\nHelp! AV# "<<iav << " failed Quality\n";
      exit(1);
    }
  }
}

void Antivirals::update(int day) {
  for(unsigned int iav =0;iav < AVs.size(); iav++)
    AVs[iav]->update(day);
}

void Antivirals::report(int day) const {
  // STB - To Do
}

void Antivirals::reset(){
  for(unsigned int iav = 0; iav < AVs.size(); iav++)
    AVs[iav]->reset();
}

