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
// File: Regional_Patch.cc
//
#include <algorithm>

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

Regional_Patch::Regional_Patch(Regional_Layer * grd, int i, int j) {
  setup(grd, i, j);
}

void Regional_Patch::setup(Regional_Layer * grd, int i, int j) {
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
  this->workplaces.clear();
  this->id = Regional_Patch::next_patch_id++;
}

void Regional_Patch::quality_control() {
  return;
}

double Regional_Patch::distance_to_patch(Regional_Patch *p2) {
  double x1 = this->center_x;
  double y1 = this->center_y;
  double x2 = p2->get_center_x();
  double y2 = p2->get_center_y();
  return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

Person *Regional_Patch::select_random_person() {
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

void Regional_Patch::unenroll(Person *per) {
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

Transaction * Regional_Patch::collect_patch_stats(int day, int disease_id) {
  Patch_Report * report = new Patch_Report(day, this->id, disease_id);
  report->collect(this->person);
  return report;
}

#endif

Place *Regional_Patch::get_nearby_workplace(Place *place, int staff) {
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
  Place * nearby_workplace = this->grid->get_nearby_workplace(this->row, this->col, x, y, min_staff,
      max_staff, &min_dist);
  if(nearby_workplace == NULL)
    return NULL;
  assert(nearby_workplace != NULL);
  double x2 = Geo_Utils::get_x(nearby_workplace->get_longitude());
  double y2 = Geo_Utils::get_y(nearby_workplace->get_latitude());
  FRED_VERBOSE(0, "nearby workplace %s %f %f size %d dist %f\n", nearby_workplace->get_label(),
      x2, y2, nearby_workplace->get_size(), min_dist);

  return nearby_workplace;
}

Place * Regional_Patch::get_closest_workplace(double x, double y, int min_size, int max_size,
    double * min_dist) {
  // printf("get_closest_workplace entered for patch %d %d\n", row, col); fflush(stdout);
  Place * closest_workplace = NULL;
  int number_workplaces = this->workplaces.size();
  for(int j = 0; j < number_workplaces; j++) {
    Place *workplace = this->workplaces[j];
    if (workplace->is_group_quarters())
      continue;
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

void Regional_Patch::add_workplace(Place *workplace) {
  this->workplaces.push_back(workplace);
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

