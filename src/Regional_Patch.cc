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
// File: Regional_Patch.cc
//
#include <algorithm>
#include <set>

#include "Global.h"
#include "Regional_Patch.h"
#include "Regional_Layer.h"
#include "Geo_Utils.h"
#include "Random.h"
#include "Utils.h"

int Regional_Patch::next_patch_id = 0;

Regional_Patch::Regional_Patch() {
  this->grid = NULL;
  this->popsize = 0;
  this->max_popsize = 0;
  this->pop_density = 0;
  this->id = -1;
}

Regional_Patch::Regional_Patch(Regional_Layer* grd, int i, int j) {
  setup(grd, i, j);
}

void Regional_Patch::setup(Regional_Layer* grd, int i, int j) {
  this->grid = grd;
  this->row = i;
  this->col = j;
  double patch_size = this->grid->get_patch_size();
  double grid_min_x = this->grid->get_min_x();
  double grid_min_y = this->grid->get_min_y();
  this->min_x = grid_min_x + (this->col) * patch_size;
  this->min_y = grid_min_y + (this->row) * patch_size;
  this->max_x = grid_min_x + (this->col + 1) * patch_size;
  this->max_y = grid_min_y + (this->row + 1) * patch_size;
  this->center_y = (this->min_y + this->max_y) / 2.0;
  this->center_x = (this->min_x + this->max_x) / 2.0;
  this->popsize = 0;
  this->max_popsize = 0;
  this->pop_density = 0;
  this->person.clear();
  this->counties.clear();
  this->workplaces.clear();
  this->id = Regional_Patch::next_patch_id++;
  for(int k = 0;k<100;k++){
   this->students_by_age[k].clear();
  }
  this->workers.clear();
}

void Regional_Patch::quality_control() {
  return;
}

double Regional_Patch::distance_to_patch(Regional_Patch* p2) {
  double x1 = this->center_x;
  double y1 = this->center_y;
  double x2 = p2->get_center_x();
  double y2 = p2->get_center_y();
  return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

Person* Regional_Patch::select_random_person() {
  if((int)this->person.size() == 0)
    return NULL;
  int i = IRAND(0, ((int )this->person.size()) - 1);

  return this->person[i];
}

void Regional_Patch::set_max_popsize(int n) {
  this->max_popsize = n;
  this->pop_density = (double)this->popsize / (double)n;

  // the following reflects noise in the estimated population in the preprocessing routine
  if(this->pop_density > 0.8) {
    this->pop_density = 1.0;
  }

}

Person* Regional_Patch::select_random_student(int age_) {
  if((int)this->students_by_age[age_].size() == 0)
    return NULL;
  int i = IRAND(0, ((int )this->students_by_age[age_].size()) - 1);

  return this->students_by_age[age_][i];
}

Person* Regional_Patch::select_random_worker() {
  if((int)this->workers.size() == 0) {
    return NULL;
  }
  int i = IRAND(0, ((int )this->workers.size()) - 1);
  
  return this->workers[i];
}


void Regional_Patch::unenroll(Person* per) {
  // <-------------------------------------------------------------- Mutex
  fred::Scoped_Lock lock(this->mutex);
  if(this->person.size() > 1) {
    std::vector<Person *>::iterator iter;
    iter = std::find(this->person.begin(), this->person.end(), per);
    if(iter != this->person.end()) {
      std::swap((*iter), this->person.back());
      this->person.erase(this->person.end() - 1);
    }
    assert(std::find(this->person.begin(), this->person.end(), per) == this->person.end());
  } else {
    this->person.clear();
  }
}


#if SQLITE

Transaction* Regional_Patch::collect_patch_stats(int day, int disease_id) {
  Patch_Report* report = new Patch_Report(day, this->id, disease_id);
  report->collect(this->person);
  return report;
}

#endif

Place* Regional_Patch::get_nearby_workplace(Place* place, int staff) {
  // printf("get_workplace_near_place entered\n"); print(); fflush(stdout);
  double x = Geo_Utils::get_x(place->get_longitude());
  double y = Geo_Utils::get_y(place->get_latitude());

  // allow staff size variation by 25%
  int min_staff = (int)(0.75 * staff);
  if(min_staff < 1)
    min_staff = 1;
  int max_staff = (int)(0.5 + 1.25 * staff);
  FRED_VERBOSE(1, " staff %d %d %d \n", min_staff, staff, max_staff);

  // find nearest workplace that has right number of employees
  double min_dist = 1e99;
  Place* nearby_workplace = this->grid->get_nearby_workplace(this->row, this->col, x, y, min_staff,
      max_staff, &min_dist);
  if(nearby_workplace == NULL)
    return NULL;
  assert(nearby_workplace != NULL);
  double x2 = Geo_Utils::get_x(nearby_workplace->get_longitude());
  double y2 = Geo_Utils::get_y(nearby_workplace->get_latitude());
  FRED_VERBOSE(1, "nearby workplace %s %f %f size %d dist %f\n", nearby_workplace->get_label(),
      x2, y2, nearby_workplace->get_size(), min_dist);

  return nearby_workplace;
}

Place* Regional_Patch::get_closest_workplace(double x, double y, int min_size, int max_size,
    double* min_dist) {
  // printf("get_closest_workplace entered for patch %d %d\n", row, col); fflush(stdout);
  Place* closest_workplace = NULL;
  int number_workplaces = this->workplaces.size();
  for(int j = 0; j < number_workplaces; j++) {
    Place* workplace = this->workplaces[j];
    if (workplace->is_group_quarters()) {
      continue;
    }
    int size = workplace->get_size();
    if(min_size <= size && size <= max_size) {
      double x2 = Geo_Utils::get_x(workplace->get_longitude());
      double y2 = Geo_Utils::get_y(workplace->get_latitude());
      double dist = sqrt((x - x2) * (x - x2) + (y - y2) * (y - y2));
      if(dist < 20.0 && dist < *min_dist) {
        *min_dist = dist;
        closest_workplace = workplace;
        // printf("closer = %s size = %d min_dist = %f\n", closest_workplace->get_label(), size, *min_dist);
        // fflush(stdout);
      }
    }
  }
  return closest_workplace;
}

void Regional_Patch::add_workplace(Place* workplace) {
  this->workplaces.push_back(workplace);
}

void Regional_Patch::swap_county_people(){
  if(counties.size()>1){
    double percentage = 0.1;
    int people_swapped = 0;
    int people_to_reassign_place = (int) (percentage * this->person.size());
    FRED_VERBOSE(1,"People to reassign : %d \n", people_to_reassign_place);
    for(int k = 0; k < people_to_reassign_place; ++k) {
      Person* p = this->select_random_person();
      Person* p2;
      if(p!=NULL) {
	      if(p->is_student()) {
	        int age_ = 0;
	        age_ = p->get_age();
	        if(age_>100) {
	          age_=100;
	        }
	        if(age_ < 0) {
	          age_=0;
	        }
	        p2 = select_random_student(age_);
	        if(p2 !=NULL) {
	          Place* s1 = p->get_school();
	          Place* s2 = p2->get_school();
	          Household* h1 =  (Household*)p->get_household();
	          int c1 = h1->get_county_index();
	          int h1_county = Global::Places.get_fips_of_county_with_index(c1);
	          Household* h2 =  (Household*)p2->get_household();
	          int c2 = h2->get_county_index();
	          int h2_county = Global::Places.get_fips_of_county_with_index(c2);
	          if(h1_county != h2_county) {
	            p->change_school(s2);
	            p2->change_school(s1);
	            FRED_VERBOSE(0,"SWAPSCHOOLS\t%d\t%d\t%s\t%s\t%lg\t%lg\t%lg\t%lg\n",p->get_id(),p2->get_id(),p->get_school()->get_label(),p2->get_school()->get_label(),p->get_school()->get_latitude(),p->get_school()->get_longitude(),p2->get_school()->get_latitude(),p2->get_school()->get_longitude());
	            printf("SWAPSCHOOLS\t%d\t%d\t%s\t%s\t%lg\t%lg\t%lg\t%lg\n",p->get_id(),p2->get_id(),p->get_school()->get_label(),p2->get_school()->get_label(),p->get_school()->get_latitude(),p->get_school()->get_longitude(),p2->get_school()->get_latitude(),p2->get_school()->get_longitude());
	            people_swapped++;
	          }
	        }
	      } else if (p->get_workplace()!=NULL) {
	        p2 = select_random_worker();
	        if(p2 != NULL) {
	          Place* w1 = p->get_workplace();
	          Place* w2 = p2->get_workplace();
	          Household* h1 = (Household*)p->get_household();
	          int c1 = h1->get_county_index();
	          int h1_county = Global::Places.get_fips_of_county_with_index(c1);
	          Household* h2 = (Household*)p2->get_household();
	          int c2 = h2->get_county_index();
	          int h2_county = Global::Places.get_fips_of_county_with_index(c2);
	          if(h1_county != h2_county) {
	            p->change_workplace(w2);
	            p2->change_workplace(w1);
	            FRED_VERBOSE(0,"SWAPWORKS\t%d\t%d\t%s\t%s\t%lg\t%lg\t%lg\t%lg\n",p->get_id(),p2->get_id(),p->get_workplace()->get_label(),p2->get_workplace()->get_label(),p->get_workplace()->get_latitude(),p->get_workplace()->get_longitude(),p2->get_workplace()->get_latitude(),p2->get_workplace()->get_longitude());
	            printf("SWAPWORKS\t%d\t%d\t%s\t%s\t%lg\t%lg\t%lg\t%lg\n",p->get_id(),p2->get_id(),p->get_workplace()->get_label(),p2->get_workplace()->get_label(),p->get_workplace()->get_latitude(),p->get_workplace()->get_longitude(),p2->get_workplace()->get_latitude(),p2->get_workplace()->get_longitude());
	            people_swapped++;
	          }
	        }
	      }
      }
    }
    FRED_VERBOSE(0,"People Swapped:: %d out of %d\n",people_swapped,people_to_reassign_place);
    printf("People Swapped:: %d out of %d\n",people_swapped,people_to_reassign_place);
  }
  return;
}


unsigned char Regional_Patch::get_deme_id() {
  unsigned char deme_id = 0;
  int max_deme_count = 0;
  std::map<unsigned char, int>::iterator itr = this->demes.begin();
  for(; itr != this->demes.end(); ++itr) {
    if((*itr).second > max_deme_count) {
      max_deme_count = (*itr).second;
      deme_id = (*itr).first;
    }
  }
  return deme_id;
}

