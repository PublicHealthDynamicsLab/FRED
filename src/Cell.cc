/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Cell.cc
//


/*
#include <stdio.h>
#include <string>
using namespace std;
*/
#include <new>

#include "Global.h"
#include "Grid.h"
#include "Cell.h"
#include "Place.h"
#include "Neighborhood.h"
#include "Place_List.h"
#include "Random.h"
#include "Household.h"
#include "School.h"
class Person;

void Cell::setup(Grid * grd, int i, int j, double xmin, double xmax, double ymin, double ymax) {
  grid = grd;
  min_x = xmin;
  max_x = xmax;
  min_y = ymin;
  max_y = ymax;
  row = i;
  col = j;
  center_y = (min_y+max_y)/2.0;
  center_x = (min_x+max_x)/2.0;
  neighbor_cells = (Cell **) grid->get_neighbors(i,j);
  houses = 0;
  occupied_houses = 0;
  household.clear();
  //make_neighborhood();
}

void Cell::make_neighborhood( Place::Allocator< Neighborhood > & neighborhood_allocator ) {
  char str[80];
  fred::geo lat, lon;
  sprintf(str, "N-%04d-%04d",row,col);
  grid->translate_to_lat_lon(center_x,center_y,&lat,&lon);

  neighborhood = new ( neighborhood_allocator.get_free() )
    Neighborhood( str, lon, lat, 0, &Global::Pop );
  
  // moved add_place call to Place_List so that all neighborhoods can be added to the
  // Place_List bloque at once
  //
  //Global::Places.add_place(neighborhood);
}

void Cell::add_household(Place *p) {
  houses++;
  household.push_back(p);
  if (Global::Householdfp != NULL) {
    fred::geo lat, lon;
    double x, y;
    lat = p->get_latitude();
    lon = p->get_longitude();
    grid->translate_to_cartesian(lat,lon,&x,&y);
    fprintf(Global::Householdfp,"%s %f %f %f %f house_id: %d row = %d  col = %d  house_number = %d\n",
      p->get_label(),lon,lat, x,y, p->get_id(), row, col, houses);
    fflush(Global::Householdfp);
  }
}

void Cell::record_favorite_places() {
  Household * house;
  Person * per;
  Place * p;
  School * s;
  // FILE *fp;

  // create lists of persons, workplaces, schools (by age)
  person.clear();
  workplace.clear();
  for (int age = 0; age < Global::ADULT_AGE; age++) school[age].clear();

  // char filename[256];
  // sprintf(filename, "PATCHES/Cell-%d-%d-households", row, col);
  // fp = fopen(filename, "w");
  for (int i = 0; i < houses; i++) {
    // printf("house %d of %d\n", i, houses); fflush(stdout);
    house = (Household *) household[i];
    house->record_profile();
    int hsize = house->get_size();
    // fprintf(fp, "%d ", hsize);
    for (int j = 0; j < hsize; j++) {
      per = house->get_housemate(j);
      person.push_back(per);
      p = per->get_activities()->get_workplace();
      if (p != NULL) workplace.push_back(p);
      s = (School *) per->get_activities()->get_school();
      if (s != NULL) {
  for (int age = 0; age < Global::ADULT_AGE; age++) {
    if (s->children_in_grade(age) > 0)
      school[age].push_back(s);
  }
      }
    }
    // fprintf(fp, "\n");
  }
  // fclose(fp);
  // set target values
  target_popsize = (int) person.size();
  target_households = houses;
}


Place * Cell::select_neighborhood(double community_prob,
          double community_distance, double local_prob) {

  Cell * grid_cell;
  double r = RANDOM();

  // select a random cell with community_prob
  if (r < community_prob) {
    grid_cell = grid->select_random_grid_cell(center_x, center_y, community_distance);
  }
  else if (r < community_prob + local_prob) {
    // select local grid_cell with local_prob
    grid_cell = this;
  }
  else {
    // select randomly from among immediate neighbors
    int n = IRAND_0_7();
    if (n>3) n++;        // excludes local grid_cell
    grid_cell = neighbor_cells[n];
  }
  if (grid_cell == NULL) grid_cell = this; // fall back to local grid_cell
  return grid_cell->get_neighborhood();
}


Person *Cell::select_random_person() {
  if ((int)person.size() == 0) return NULL;
  int i = IRAND(0, ((int) person.size())-1);
  return person[i];
}


Place *Cell::select_random_household() {
  if ((int)household.size() == 0) return NULL;
  int i = IRAND(0, ((int) household.size())-1);
  return household[i];
}


Place *Cell::select_random_workplace() {
  if ((int)workplace.size() == 0) return NULL;
  int i = IRAND(0, ((int) workplace.size())-1);
  return workplace[i];
}


Place *Cell::select_random_school(int age) {
  if ((int)school[age].size() == 0) return NULL;
  int i = IRAND(0, ((int) school[age].size())-1);
  return school[age][i];
}


void Cell::quality_control() {
  return;
  fprintf(Global::Statusfp,
    "PATCH row = %d col = %d  pop = %d  houses = %d work = %d schools = ",
    row,col,(int)person.size(),(int)household.size(),(int)workplace.size());
  for (int age = 0; age < 20; age++) {
    fprintf(Global::Statusfp, "%d ", (int)school[age].size());
  }
  fprintf(Global::Statusfp, "\n");
  fflush(Global::Statusfp);
}


void Cell::print() {
  printf("Cell %d %d: %f, %f, %f, %f\n",row,col,min_x,max_x,min_y,max_y);
  for (int i = 0; i < 9; i++) {
    if (neighbor_cells[i] == NULL) { printf("NULL ");}
    else {neighbor_cells[i]->print_coord();}
    printf("\n");
  }
}

void Cell::print_coord() {
  printf("(%d, %d)",row,col);
}

double Cell::distance_to_grid_cell(Cell *p2) {
  double x1 = center_x;
  double y1 = center_y;
  double x2 = p2->get_center_x();
  double y2 = p2->get_center_y();
  return sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}
