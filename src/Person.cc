/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Person.cc
//

#include "Person.h"

#include "Activities.h"
#include "Demographics.h"
#include "Global.h"
#include "Health.h"
#include "Behavior.h"
#include "Place.h"
#include "Household.h"
#include "Disease.h"
#include "Population.h"
#include "AV_Health.h"
#include "Age_Map.h"
#include "Date.h"
#include "Place_List.h"
#include "Transmission.h"
#include "Utils.h"

#include <cstdio>
#include <vector>
#include <sstream>
class Household;

Person::Person() {
  id = -1;
  index = -1;
}

Person::~Person() {
}

Person::Person(int _index, int _id, int age, char sex,
	       int race, int rel, Place *house, Place *school, Place *work,
	       int day, bool today_is_birthday) {

  index = _index;
  id = _id;
  demographics = Demographics(this, age, sex, race, rel, day, today_is_birthday);
  health = Health( this );
  activities = Activities( this, house, school, work );
  behavior = Behavior( this );

  for (int disease = 0; disease < Global::Diseases; disease++) {
    Disease* dis = Global::Pop.get_disease(disease);
    if (!dis->get_residual_immunity()->is_empty()) {
      double residual_immunity_prob = dis->get_residual_immunity()->find_value(age);
      // printf("RESID: age %d prob %f\n",age,residual_immunity_prob);
      if (RANDOM() < residual_immunity_prob) {
        become_immune(dis);
      }
    }
  }
}

void Person::print(FILE *fp, int disease) {
  if (fp == NULL) return;
  fprintf(fp, "%d id %7d  a %3d  s %c %d",
          disease, id,
          demographics.get_age(),
          demographics.get_sex(),
          demographics.get_race());
  fprintf(fp, "exp: %2d  inf: %2d  rem: %2d ",
          health.get_exposure_date(disease),
    health.get_infectious_date(disease),
    health.get_recovered_date(disease));
  fprintf(fp, "sympt: %d ", health.get_symptomatic_date(disease));

  fprintf(fp, "places %d ", FAVORITE_PLACES);
  fprintf(fp, "infected_at %c %6d ",
          health.get_infected_place_type(disease),
    health.get_infected_place(disease));
  Person * infector = health.get_infector(disease);
  int infector_id;
  if (infector == NULL) 
    infector_id = -1;
  else 
    infector_id = infector->get_id();
  fprintf(fp, "infector %d ", infector_id);
  fprintf(fp, "infectees %d ", health.get_infectees(disease));
  fprintf(fp, "antivirals: %2d ",health.get_number_av_taken());
  for(int i=0;i<health.get_number_av_taken();i++)
    fprintf(fp," %2d",health.get_av_health(i)->get_av_start_day());
  fprintf(fp,"\n");
  fflush(fp);
}

void Person::become_immune(Disease* disease) {
  int disease_id = disease->get_id();
  if(health.is_susceptible(disease_id)){
    health.become_immune(disease);
  }
}

void Person::set_changed(){
  Global::Pop.set_changed(this);
}

void Person::infect(Person *infectee, int disease, Transmission *transmission) {
  health.infect( infectee, disease, transmission );
}

Person * Person::give_birth(int day) {
  int id = Global::Pop.get_next_id();
  int age = 0;
  char sex = (URAND(0.0, 1.0) < 0.5 ? 'M' : 'F');
  int race = get_race();
  int rel = Global::CHILD;
  Place * house = get_household();
  Place * school = NULL;
  Place * work = NULL;
  bool today_is_birthday = true;
  Person * baby = Global::Pop.add_person( id, age, sex, race, rel,
           house, school, work, day, today_is_birthday );
  return baby;
}

void Person::addIncidence(int disease, vector<int> strains) {
  activities.addIncidence(disease, strains);
}

void Person::addPrevalence(int disease, vector<int> strains) {
  activities.addPrevalence(disease, strains);
}

string Person::to_string() {

  stringstream tmp_string_stream;
  // (i.e *ID* Age Sex Household School *Classroom* Workplace *Office* Relationship)
  tmp_string_stream << this->id << " " << this->get_age() << " " <<  this->get_sex() << " " ;
  tmp_string_stream << this->get_race() << " " ;
  Place *tmp_place = this->get_household();
  if(tmp_place == NULL)
    tmp_string_stream << "-1 ";
  else
    tmp_string_stream << tmp_place->get_label() << " ";

  tmp_place = this->get_school();
  if(tmp_place == NULL)
    tmp_string_stream << "-1 ";
  else
    tmp_string_stream << tmp_place->get_label() << " ";

  tmp_place = this->get_classroom();
  if(tmp_place == NULL)
    tmp_string_stream << "-1 ";
  else
    tmp_string_stream << tmp_place->get_label() << " ";

  tmp_place = this->get_workplace();
  if(tmp_place == NULL)
    tmp_string_stream << "-1 ";
  else
    tmp_string_stream << tmp_place->get_label() << " ";

  tmp_place = this->get_office();
  if(tmp_place == NULL)
    tmp_string_stream << "-1 ";
  else
    tmp_string_stream << tmp_place->get_label() << " ";

  tmp_string_stream << this->get_relationship();

  return tmp_string_stream.str();
}

void Person::terminate() {
  Utils::fred_verbose(1, "terminating person %d\n", id);
  behavior.terminate();
  activities.terminate();
  health.terminate();
  demographics.terminate();
}

