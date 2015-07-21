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
// File: Neighborhood_Patch.cc
//


#include <new>
#include "Global.h"
#include "Geo.h"
#include "Neighborhood_Layer.h"
#include "Neighborhood_Patch.h"
#include "Place.h"
#include "Neighborhood.h"
#include "Place_List.h"
#include "Random.h"
#include "Household.h"
#include "School.h"
class Person;

void insert_if_unique(place_vector *vec, Place *p) {
  for (place_vector::iterator itr = vec->begin(); itr != vec->end(); itr++) {
    if (*itr == p) return;
  }
  vec->push_back(p);
}

void Neighborhood_Patch::setup(Neighborhood_Layer * grd, int i, int j) {
  grid = grd;
  row = i;
  col = j;
  double patch_size = grid->get_patch_size();
  double grid_min_x = grid->get_min_x();
  double grid_min_y = grid->get_min_y();
  min_x = grid_min_x + (col)*patch_size;
  min_y = grid_min_y + (row)*patch_size;
  max_x = grid_min_x + (col+1)*patch_size;
  max_y = grid_min_y + (row+1)*patch_size;
  center_y = (min_y+max_y)/2.0;
  center_x = (min_x+max_x)/2.0;
  household.clear();
  houses = 0;
  households = 0;
  popsize = 0;
  mean_household_income = 0.0;
  neighborhood = NULL;
}

void Neighborhood_Patch::make_neighborhood( Place::Allocator< Neighborhood > & neighborhood_allocator ) {
  char str[80];
  sprintf(str, "N-%04d-%04d",row,col);
  fred::geo lat = Geo::get_latitude(center_y);
  fred::geo lon = Geo::get_longitude(center_x);

  neighborhood = new ( neighborhood_allocator.get_free() )
    Neighborhood( str, fred::PLACE_SUBTYPE_NONE, lon, lat, 0);
}

void Neighborhood_Patch::add_household(Household *p) {
  houses++;
  household.push_back(p);
  if (Global::Householdfp != NULL) {
    fprintf(Global::Householdfp,"%s %.8f %.8f %f %f house_id: %d row = %d  col = %d  house_number = %d\n",
	    p->get_label(),p->get_longitude(),p->get_latitude(),p->get_x(),p->get_y(), p->get_id(), row, col, houses);
    fflush(Global::Householdfp);
  }
}

void Neighborhood_Patch::record_favorite_places() {
  Household * house;
  Person * per;
  Place * p;
  School * s;
  // FILE *fp;

  // create lists of persons, workplaces, schools (by age)
  person.clear();
  school.clear();
  workplace.clear();
  for(int age = 0; age < Global::ADULT_AGE; age++) {
    school_by_age[age].clear();
  }

  // char filename[FRED_STRING_SIZE];
  // sprintf(filename, "PATCHES/Neighborhood_Patch-%d-%d-households", row, col);
  // fp = fopen(filename, "w");
  for (int i = 0; i < houses; i++) {
    // printf("house %d of %d\n", i, houses); fflush(stdout);
    house = (Household *) household[i];
    house->record_profile();
    this->mean_household_income += house->get_household_income();
    int hsize = house->get_size();
    // fprintf(fp, "%d ", hsize);
    for (int j = 0; j < hsize; j++) {
      per = house->get_housemate(j);
      person.push_back(per);
      p = per->get_activities()->get_workplace();
      if (p != NULL) {
	insert_if_unique(&workplace,p);
      }
      s = static_cast<School *>(per->get_activities()->get_school());
      if (s != NULL) {
	insert_if_unique(&school,s);
	for (int age = 0; age < Global::ADULT_AGE; age++) {
	  if (s->get_students_in_grade(age) > 0) {
	    // school_by_age[age].push_back(s);
	    insert_if_unique(&school_by_age[age],s);
          }
	}
      }
    }
    // fprintf(fp, "\n");
  }
  // fclose(fp);
  popsize = (int) person.size();
  households = houses;
  if (households > 0) {
    mean_household_income /= households;
  }
}

Person * Neighborhood_Patch::select_random_person() {
  if ((int)person.size() == 0) return NULL;
  int i = Random::draw_random_int(0, ((int) person.size())-1);
  return person[i];
}


Place * Neighborhood_Patch::select_random_household() {
  if ((int)household.size() == 0) return NULL;
  int i = Random::draw_random_int(0, ((int) household.size())-1);
  return household[i];
}


Place * Neighborhood_Patch::select_random_workplace() {
  if ((int)workplace.size() == 0) return NULL;
  int i = Random::draw_random_int(0, ((int) workplace.size())-1);
  return workplace[i];
}


Place *Neighborhood_Patch::select_random_school(int age) {
  if ((int)school_by_age[age].size() == 0) return NULL;
  int i = Random::draw_random_int(0, ((int) school_by_age[age].size())-1);
  return school_by_age[age][i];
}

Place * Neighborhood_Patch::select_school(int age) {
  return grid->select_school_in_area(age,row,col);
}

Place * Neighborhood_Patch::select_school_in_neighborhood(int age, double threshold) {
  FRED_VERBOSE(1,"select_school_in_neighborhood entered age %d row %d col %d\n",age,row,col);
  int size = (int)school_by_age[age].size();
  if (size == 0) {
    FRED_VERBOSE(1,"Found no schools for age %d\n",age);
    return NULL;
  }
  FRED_VERBOSE(1,"Found %d schools for age %d\n",size,age);
  double max_vacancies = -9999999.0;
  int max_vacancies_index = -1;
  for (int i = 0; i < size; i++) {
    Place * p = school_by_age[age][i];
    School * s = static_cast<School *>(p);
    double vacancies;
    if (s->get_orig_students_in_grade(age) > 0)
      vacancies = (double)(s->get_orig_students_in_grade(age) - s->get_students_in_grade(age))/(double)s->get_orig_students_in_grade(age);
    else
      vacancies = -1.0 * s->get_students_in_grade(age);
    // avoid moves into places that are already overfilled
    if (vacancies > max_vacancies && vacancies > threshold) {
      max_vacancies = vacancies;
      max_vacancies_index = i;
    }
  }
  if (max_vacancies_index < 0) {
    FRED_VERBOSE(1,"select_school_in_neighborhood entered age %d row %d col %d\n",age,row,col);
    FRED_VERBOSE(1,"Found no schools with vacancies for age %d\n",age);
    return NULL;
  }
  Place * school = school_by_age[age][max_vacancies_index];
  School * s = static_cast<School *>(school);
  FRED_VERBOSE(1,"SELECT_SCHOOL: age %d %s size %d orig_in_grade %d curr_in_grade %d max_vacancies %0.4f\n", 
	       age, s->get_label(), s->get_size(), s->get_orig_students_in_grade(age),
	       s->get_students_in_grade(age),max_vacancies);
  return school;
}


void  Neighborhood_Patch::find_schools_for_age(int age, place_vector * schools) {
  FRED_VERBOSE(1,"find_schools_for_age %d row %d col %d\n",age,row,col);
  int size = (int)school_by_age[age].size();
  for (int i = 0; i < size; i++) {
    Place * p = school_by_age[age][i];
    insert_if_unique(schools, p);
  }
}


Place * Neighborhood_Patch::select_workplace() {
  return grid->select_workplace_in_area(row,col);
}

Place * Neighborhood_Patch::select_workplace_in_neighborhood() {
  FRED_VERBOSE(1,"select_workplace_in_neighborhood entered row %d col %d\n",row,col);
  int size = (int)workplace.size();
  if (size == 0) {
    FRED_VERBOSE(1,"Found no workplaces\n");
    return NULL;
  }
  FRED_VERBOSE(1,"Found %d workplaces\n",size);
  double max_vacancies = -9999999;
  int max_vacancies_index = -1;
  for(int i = 0; i < size; i++) {
    Place * p = workplace[i];
    double vacancies = 0.0;
    if(p->get_orig_size() > 0) {
      vacancies = (double)(p->get_orig_size() - p->get_size())/(double)p->get_orig_size();
    } else {
      vacancies = -1.0 * p->get_size();
    }
    
    // avoid moves into places that are already 50% overfilled
    if (vacancies > max_vacancies && vacancies > -0.5) {
      max_vacancies = vacancies;
      max_vacancies_index = i;
    }
  }
  if(max_vacancies_index < 0) {
    FRED_VERBOSE(1,"select_workplace_in_neighborhood entered row %d col %d\n",row,col);
    FRED_VERBOSE(1,"Found no workplaces with vacancies\n");
    return NULL;
  }
  assert(max_vacancies_index >= 0);
  Place * p = workplace[max_vacancies_index];
  FRED_VERBOSE(1,"SELECT_WORKPLACE: %s orig %d curr %d max_vacancies %0.4f\n", 
	       p->get_label(), p->get_orig_size(), p->get_size(), max_vacancies);
  return p;
}


void Neighborhood_Patch::quality_control() {
  return;
  if(person.size() > 0) {
    fprintf(Global::Statusfp,
	    "PATCH row = %d col = %d  pop = %d  houses = %d work = %d schools = %d by_age ",
	    row,col,(int)person.size(),(int)household.size(),(int)workplace.size(),(int)school.size());
    for(int age = 0; age < 20; age++) {
      fprintf(Global::Statusfp, "%d ", (int)school_by_age[age].size());
    }
    fprintf(Global::Statusfp, "\n");
    if(Global::Verbose > 0) {
      for (int i = 0; i < school.size(); i++) {
	School * s = static_cast<School *>(school[i]);
	fprintf(Global::Statusfp, "School %d: %s by_age: ", i, s->get_label());
	for(int a = 0; a < 19; a++) {
	  fprintf(Global::Statusfp, " %d:%d,%d ", a, s->get_students_in_grade(a), s->get_orig_students_in_grade(a));
	}
	fprintf(Global::Statusfp, "\n");
      }
      fflush(Global::Statusfp);
    }
  }
}


double Neighborhood_Patch::distance_to_patch(Neighborhood_Patch *p2) {
  double x1 = center_x;
  double y1 = center_y;
  double x2 = p2->get_center_x();
  double y2 = p2->get_center_y();
  return sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}

