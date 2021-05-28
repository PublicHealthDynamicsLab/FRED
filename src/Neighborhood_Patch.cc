/*
 * This file is part of the FRED system.
 *
 * Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette, Shawn Brown, 
 * Roni Rosenfield, Alona Fyshe, David Galloway, Nathan Stone, Jay DePasse, 
 * Anuroop Sriram, and Donald Burke
 * All rights reserved.
 *
 * Copyright (c) 2013-2019, University of Pittsburgh, John Grefenstette, Robert Frankeny,
 * David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
 * All rights reserved.
 *
 * FRED is distributed on the condition that users fully understand and agree to all terms of the 
 * End User License Agreement.
 *
 * FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.
 *
 * See the file "LICENSE" for more information.
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
#include "Person.h"
#include "Place.h"
#include "Place_Type.h"
#include "Random.h"
#include "Household.h"


void insert_if_unique(place_vector* vec, Place* p) {
  for (place_vector::iterator itr = vec->begin(); itr != vec->end(); ++itr) {
    if(*itr == p) {
      return;
    }
  }
  vec->push_back(p);
}

Neighborhood_Patch::Neighborhood_Patch() {
  this->grid = NULL;
  this->row = -1;
  this-> col = -1;
  this->min_x = 0.0;
  this->min_y = 0.0;
  this->max_x = 0.0;
  this->max_y = 0.0;
  this->center_y = 0.0;
  this->center_x = 0.0;
  this->popsize = 0;
  this->neighborhood = NULL;
  this->admin_code = 0;
}

void Neighborhood_Patch::setup(Neighborhood_Layer* grd, int i, int j) {
  this->grid = grd;
  this->row = i;
  this->col = j;
  double patch_size = grid->get_patch_size();
  double grid_min_x = grid->get_min_x();
  double grid_min_y = grid->get_min_y();
  this->min_x = grid_min_x + (col)*patch_size;
  this->min_y = grid_min_y + (row)*patch_size;
  this->max_x = grid_min_x + (col+1)*patch_size;
  this->max_y = grid_min_y + (row+1)*patch_size;
  this->center_y = (min_y+max_y)/2.0;
  this->center_x = (min_x+max_x)/2.0;
  this->popsize = 0;
  this->neighborhood = NULL;
  this->admin_code = 0;
  int number_of_place_types = Place_Type::get_number_of_place_types();
  this->places = new place_vector_t [ number_of_place_types ];
    for (int i = 0; i < number_of_place_types; i++) {
    this->places[i].clear();
  }
  this->schools_attended_by_neighborhood_residents.clear();
  this->workplaces_attended_by_neighborhood_residents.clear();
}

void Neighborhood_Patch::prepare() {
  this->neighborhood->prepare();
  this->neighborhood->set_elevation(0.0);
  int houses = get_number_of_households();
  if (houses > 0) {
    double sum = 0.0;
    for(int i = 0; i < houses; ++i) {
      double elevation = get_household(i)->get_elevation();
      sum += elevation;
    }
    double mean = sum / (double) houses;
    this->neighborhood->set_elevation(mean);
  }

  // find median income
  int size = houses;
  std::vector<int> income_list;
  income_list.reserve(size);
  for(int i = 0; i < houses; ++i) {
    income_list[i] = get_household(i)->get_income();
  }
  std::sort(income_list.begin(), income_list.end());
  int median = income_list[size/2];
  this->neighborhood->set_income(median);

}


void Neighborhood_Patch::make_neighborhood(int type) {
  char label[80];
  sprintf(label, "N-%04d-%04d", this->row, this->col);
  fred::geo lat = Geo::get_latitude(this->center_y);
  fred::geo lon = Geo::get_longitude(this->center_x);
  this->neighborhood = Place::add_place(label, type,
						Place::SUBTYPE_NONE,
						lon, lat, 0.0,
						this->admin_code);
}

void Neighborhood_Patch::add_place(Place* place) {
  int type_id = place->get_type_id();
  this->places[type_id].push_back(place);
  if (this->admin_code == 0) {
    this->admin_code = place->get_admin_code();
  }
  FRED_VERBOSE(1, 
	       "NEIGHBORHOOD_PATCH: add place %d %s type_id %d lat %.8f lon %.8f  row %d  col %d  place_number %d\n",
	       place->get_id(), 
	       place->get_label(), type_id,
	       place->get_longitude(), place->get_latitude(),
	       row, col, (int) this->places[type_id].size());
}

void Neighborhood_Patch::record_activity_groups() {
  Household* house;
  Person* per;
  Place* p;
  Place* s;

  // create lists of persons, workplaces, schools (by age)
  this->person.clear();
  this->schools_attended_by_neighborhood_residents.clear();
  this->workplaces_attended_by_neighborhood_residents.clear();
  for(int age = 0; age < Global::ADULT_AGE; age++) {
    this->schools_attended_by_neighborhood_residents_by_age[age].clear();
  }

  // char filename[FRED_STRING_SIZE];
  // sprintf(filename, "PATCHES/Neighborhood_Patch-%d-%d-households", row, col);
  // fp = fopen(filename, "w");
  int houses = get_number_of_households();
  for(int i = 0; i < houses; ++i) {
    // printf("house %d of %d\n", i, houses); fflush(stdout);
    house = static_cast<Household*>(get_household(i));
    int hsize = house->get_size();
    // fprintf(fp, "%d ", hsize);
    // printf("hsize = %d \n", hsize); fflush(stdout);
    for(int j = 0; j < hsize; ++j) {
      // printf("hsize = %d j = %d\n", hsize, j); fflush(stdout);
      per = house->get_member(j);
      person.push_back(per);
      p = per->get_workplace();
      if (p != NULL) {
	insert_if_unique(&workplaces_attended_by_neighborhood_residents,p);
      }
      // printf("hsize = %d j = %d\n", hsize, j); fflush(stdout);
      s = per->get_school();
      if(s != NULL) {
	// printf("school %s size %d\n", s->get_label(), s->get_size());
	insert_if_unique(&this->schools_attended_by_neighborhood_residents,s);
	// printf("school %s size %d insert ok\n", s->get_label(), s->get_size());
	for(int age = 0; age < Global::ADULT_AGE; ++age) {
	  // printf("school %s size %d age %d size-age %d\n", s->get_label(), s->get_size(), age, s->get_original_size_by_age(age)); fflush(stdout);
	  if(s->get_original_size_by_age(age) > 0) {
	    insert_if_unique(&schools_attended_by_neighborhood_residents_by_age[age],s);
          }
	  // printf("school %s size %d age %d size-age %d insert ok\n", s->get_label(), s->get_size(), age, s->get_original_size_by_age(age)); fflush(stdout);
	}
      }
      // printf("hsize = %d j = %d\n", hsize, j); fflush(stdout);
    }
    // fprintf(fp, "\n");
  }
  // fclose(fp);
  this->popsize = static_cast<int>(this->person.size());
}

Place* Neighborhood_Patch::select_random_household() {
  int n = get_number_of_households(); 
  if(n == 0) {
    return NULL;
  }
  int i = Random::draw_random_int(0, n-1);
  return get_household(i);
}


void Neighborhood_Patch::quality_control() {
  if(Global::Quality_control > 1 && this->person.size() > 0) {
    fprintf(Global::Statusfp,
	    "PATCH row = %d col = %d  pop = %d  houses = %d work = %d schools = %d by_age ",
	    this->row, this->col, static_cast<int>(this->person.size()), get_number_of_households(), get_number_of_workplaces(), get_number_of_schools());
    for(int age = 0; age < 20; ++age) {
      fprintf(Global::Statusfp, "%d ", static_cast<int>(this->schools_attended_by_neighborhood_residents_by_age[age].size()));
    }
    fprintf(Global::Statusfp, "\n");
    if(Global::Verbose > 0) {
      for(int i = 0; i < this->schools_attended_by_neighborhood_residents.size(); ++i) {
	Place* s = this->schools_attended_by_neighborhood_residents[i];
	fprintf(Global::Statusfp, "School %d: %s by_age: ", i, s->get_label());
	for(int a = 0; a < 19; ++a) {
	  fprintf(Global::Statusfp, " %d:%d,%d ", a, s->get_size_by_age(a), s->get_original_size_by_age(a));
	}
	fprintf(Global::Statusfp, "\n");
      }
      fflush(Global::Statusfp);
    }
  }
}


double Neighborhood_Patch::get_elevation(double lat, double lon) {

  int size = this->elevation_sites.size();
  // printf("Patch %d %d elevation sites %d\n", this->row, this->col, size);
  if (size > 0) {

    // use FCC interpolation from
    // http://www.softwright.com/faq/support/Terrain%20Elevation%20Interpolation.html

    // find the four elevation points closest to the target point
    elevation_t* A = NULL;
    elevation_t* B = NULL;
    elevation_t* C = NULL;
    elevation_t* D = NULL;
    for (int i = 1; i < size; i++) {
      elevation_t* e = this->elevation_sites[i];
      if ((lat <= e->lat && e->lon <= lon) && (A==NULL || (e->lat <= A->lat && A->lon <= e->lon))) {
	A = e;
      }
      if ((lat <= e->lat && lon < e->lon) && (B==NULL || (e->lat <= B->lat && e->lon <= B->lon))) {
	B = e;
      }
      if ((e->lat <= lat && e->lon <= lon) && (C==NULL || (C->lat <= e->lat && C->lon <= e->lon))) {
	C = e;
      }
      if ((e->lat <= lat && lon < e->lon) && (D==NULL || (D->lat <= e->lat && e->lon <= D->lon))) {
	D = e;
      }
    }  
    assert(A!=NULL || B!=NULL || C!=NULL || D!=NULL);

    // interpolate on the lines AB and CD
    double elev_E = -9999;
    double lat_E = -999;
    if (A==NULL) {
      if (B!=NULL) {
	elev_E = B->elevation;
	lat_E = B->lat;
      }
    }
    else if (B==NULL) {
      if (A!=NULL) {
	elev_E = A->elevation;
	lat_E = A->lat;
      }
    }
    else {
      elev_E = ((B->lon - lon)*A->elevation + (lon - A->lon)*B->elevation)/(B->lon - A->lon);
      lat_E = A->lat;
    }

    double elev_F = -9999;
    double lat_F = -999;
    if (C==NULL) {
      if (D!=NULL) {
	elev_F = D->elevation;
	lat_F = D->lat;
      }
    }
    else if (D==NULL) {
      if (C!=NULL) {
	elev_F = C->elevation;
	lat_F = C->lat;
      }
    }
    else {
      elev_F = ((D->lon - lon)*C->elevation + (lon - C->lon)*D->elevation)/(D->lon - C->lon);
      lat_F = C->lat;
    }

    // interpolate on the line EF
    double elev_G;
    if (elev_E < 0) {
      elev_G = elev_F;
    }
    else if (elev_F < 0) {
      elev_G = elev_E;
    }
    else {
      if (lat_E > lat_F) {
	elev_G = ((lat_E - lat)*elev_F + (lat - lat_F)*elev_E)/(lat_E - lat_F);
      }
      else {
	elev_G = elev_F;
      }
    }
    if (elev_G < -9000) {
      printf("HELP! lat_E %f lat_F %f elev_E %f elev_F %f elev_G %f\n", lat_E,lat_F,elev_E,elev_F,elev_G); fflush(stdout);
      assert(elev_G >= -9000);
    }
    return elev_G;

    // use closest elevation site
    double min_dist = Geo::xy_distance(lat, lon, this->elevation_sites[0]->lat, this->elevation_sites[0]->lon);
    int min_dist_index = 0;
    for (int i = 1; i < size; i++) {
      double dist = Geo::xy_distance(lat, lon, this->elevation_sites[i]->lat, this->elevation_sites[i]->lon);
      if (dist < min_dist) {
	min_dist = dist;
	min_dist_index = i;
      }
    }
    return this->elevation_sites[min_dist_index]->elevation;
  }
  else {
    return 0.0;
  }
}

place_vector_t Neighborhood_Patch::get_places_at_distance(int type_id, int dist) {
  place_vector_t results;
  place_vector_t tmp;
  results.clear();
  tmp.clear();
  Neighborhood_Patch* next_patch;

  if (dist == 0) {
    int r = this->row;
    int c = this->col;
    FRED_VERBOSE(1, "get_patch X %d Y %d | dist = %d | x %d y %d\n", this->col, this->row, dist, c, r);
    next_patch = Global::Neighborhoods->get_patch(r, c);
    if (next_patch != NULL) {
      tmp = next_patch->get_places(type_id);
      // append results from one patch to overall results
      results.insert(std::end(results), std::begin(tmp), std::end(tmp));
    }
    return results;
  }

  for (int c = this->col - dist; c <= this->col + dist; c++) {
    int r = this->row - (dist - abs(c - this->col));;
    FRED_VERBOSE(1, "get_patch X %d Y %d | dist = %d | x %d y %d\n", this->col, this->row, dist, c, r);
    next_patch = Global::Neighborhoods->get_patch(r, c);
    if (next_patch != NULL) {
      tmp = next_patch->get_places(type_id);
      // append results from one patch to overall results
      results.insert(std::end(results), std::begin(tmp), std::end(tmp));
    }

    if (dist > abs(c - this->col)) {
      r = this->row + (dist - abs(c - this->col));;
      FRED_VERBOSE(1, "get_patch X %d Y %d | dist = %d | x %d y %d\n", this->col, this->row, dist, c, r);
      next_patch = Global::Neighborhoods->get_patch(r, c);
      if (next_patch != NULL) {
	tmp = next_patch->get_places(type_id);
	// append results from one patch to overall results
	results.insert(std::end(results), std::begin(tmp), std::end(tmp));
      }
    }
  }
  return results;
}
