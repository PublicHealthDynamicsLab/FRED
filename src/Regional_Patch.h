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
// File: Regional_Patch.h
//
#ifndef _FRED_REGIONAL_PATCH_H
#define _FRED_REGIONAL_PATCH_H

#include <vector>
#include <set>

#include "Person.h"
#include "Abstract_Patch.h"
#include "Global.h"
#include "Utils.h"
#include "Household.h"
#include "Place_List.h"
#include "Place.h"

typedef vector <Person*> person_vec;	     //Vector of person pointers
typedef vector <Place *> place_vec;	      //Vector of place pointers

class Regional_Layer;

#if SQLITE
#include "DB.h"

struct Patch_Report : public Transaction {

  int day, patch_id, disease_id, naive, total;

  Patch_Report(int _day, int _patch_id, int _disease_id) :
      day(_day), patch_id(_patch_id), disease_id(_disease_id) {
    naive = 0;
    total = 0;
  }

  void collect(std::vector<Person *> & person) {
    std::vector<Person *>::iterator iter = person.begin();
    while(iter != person.end()) {
      Person & p = *(*iter);
      if(!((p.is_infectious(disease_id)) && (p.get_exposure_date(disease_id) >= 0)
          && (p.get_num_past_infections(disease_id) > 0))) {
        ++naive;
      }
      ++total;
      ++iter;
    }
  }

  const char * initialize(int statement_number) {
    if(statement_number < n_stmts) {
      if(statement_number == 0) {
        return "create table if not exists patch_stats \
            ( day integer, patch integer, naive integer, total integer );";
      }
    } else {
      Utils::fred_abort("Index of requested statement is out of range!", "");
    }
    return NULL;
  }

  void prepare() {
    // statement number; reused for each statement's creation
    int S = 0;
    // define the statement and value variables
    n_stmts = 1;
    // new array of strings to hold statements

    statements = new std::string[n_stmts];
    // array giving the number of values expected for each statement
    n_values = new int[n_stmts];
    // array of vectors of string arrays  
    values = new std::vector<string *>[n_stmts];
    // <------------------------------------------------------------------------------------- prepare first statement
    S = 0;
    statements[S] = std::string("insert into patch_stats values ($DAY,$PATCH,$NAIVE,$TOTAL);");
    n_values[S] = 4;
    std::string * values_array = new std::string[n_values[S]];
    std::stringstream ss;
    ss << this->day;
    values_array[0] = ss.str();
    ss.str("");
    ss << patch_id;
    values_array[1] = ss.str();
    ss.str("");
    ss << naive;
    values_array[2] = ss.str();
    ss.str("");
    ss << total;
    values_array[3] = ss.str();
    ss.str("");
    values[S].push_back(values_array);
  }
};
#endif

class Regional_Patch : public Abstract_Patch {
public:
  Regional_Patch(Regional_Layer * grd, int i, int j);
  Regional_Patch();
  ~Regional_Patch() {
  }
  void setup(Regional_Layer * grd, int i, int j);
  void quality_control();
  double distance_to_patch(Regional_Patch *patch2);
  void add_person(Person * p) {
    // <-------------------------------------------------------------- Mutex
    fred::Scoped_Lock lock(this->mutex);
    this->person.push_back(p);
    if(Global::Enable_Vector_Layer) {
      Household * h =  (Household *) p->get_household();
      int c = h ->get_county();
      int h_county = Global::Places.get_county_with_index(c);
      this->counties.insert(h_county);
      if(p->is_student()){
	int age_ = 0;
	age_ = p->get_age();
	if(age_>100)
	  age_=100;
	if(age_<0)
	  age_=0;
	students_by_age[age_].push_back(p);
      }
      if(p->get_workplace()!=NULL){
	workers.push_back(p);
      }
    }
    ++this->demes[p->get_deme_id()];
    ++this->popsize;
  }

  int get_popsize() {
    return this->popsize;
  }

  Person * select_random_person();
  Person * select_random_student(int age_);
  Person * select_random_worker();
  void set_max_popsize(int n);
  int get_max_popsize() {
    return this->max_popsize;
  }

  double get_pop_density() {
    return this->pop_density;
  }

  void unenroll(Person *pers);
  void add_workplace(Place *place);
  Place * get_nearby_workplace(Place *place, int staff);
  Place * get_closest_workplace(double x, double y, int min_size, int max_size, double * min_dist);

  int get_id() {
    return this->id;
  }

  void swap_county_people();

  unsigned char get_deme_id();

#if SQLITE
  Transaction * collect_patch_stats(int day, int disease_id);
#endif

protected:
  fred::Mutex mutex;
  Regional_Layer * grid;
  int popsize;
  vector<Person *> person;
  std::set<int > counties;
  int max_popsize;
  double pop_density;
  int id;
  static int next_patch_id;
  place_vec workplaces;
  person_vec students_by_age[100];
  person_vec workers;
  std::map<unsigned char, int> demes;
};

#endif // _FRED_REGIONAL_PATCH_H
