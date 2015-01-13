/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Large_Cell.cc
//

#include "Global.h"
#include "Large_Cell.h"
#include "Large_Grid.h"
#include "Random.h"

void Large_Cell::setup(Large_Grid * grd, int i, int j, double xmin, double xmax, double ymin, double ymax) {
  grid = grd;
  row = i;
  col = j;
  min_x = xmin;
  max_x = xmax;
  min_y = ymin;
  max_y = ymax;
  center_y = (min_y+max_y)/2.0;
  center_x = (min_x+max_x)/2.0;
  neighbors = (Large_Cell **) grid->get_neighbors(i,j);
  popsize = 0;
  max_popsize = 0;
  pop_density = 0;
  person.clear();
}

void Large_Cell::print() {
  printf("Large_Cell %d %d: %f, %f, %f, %f\n",row,col,min_x,max_x,min_y,max_y);
  for (int i = 0; i < 9; i++) {
    if (neighbors[i] == NULL) { printf("NULL ");}
    else {neighbors[i]->print_coord();}
    printf("\n");
  }
}

void Large_Cell::print_coord() {
  printf("(%d, %d)",row,col);
}

void Large_Cell::quality_control() {
  return;
}

double Large_Cell::distance_to_grid_cell(Large_Cell *p2) {
  double x1 = center_x;
  double y1 = center_y;
  double x2 = p2->get_center_x();
  double y2 = p2->get_center_y();
  return sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}


Person *Large_Cell::select_random_person() {
  if ((int)person.size() == 0) return NULL;
  int i = IRAND(0, ((int) person.size())-1);

  //if (Global::Verbose > 2) {
    //fprintf(Global::Statusfp, "ABC%d,%d\n", i, person[i]->get_id());
    //fflush(Global::Statusfp);
    //fprintf(stderr, "ABC%d,%d\n", i, person[i]->get_id());
    //fflush(stderr);
  //}
  return person[i];
}


void Large_Cell::set_max_popsize(int n) {
  max_popsize = n; 
  pop_density = (double) popsize/ (double) n;
  
  // the following reflects noise in the estimated population in the preprocessing routine
  if (pop_density > 0.8) pop_density = 1.0;

  /*
    if (pop_density < 1.0) { printf("WARNING: cell %d %d pop %d max_pop %d frac %f\n",
    col,row,popsize,max_popsize,pop_density);
    fflush(stdout);
  */
}

void Large_Cell::unenroll(Person *per) {
  int id = per->get_id();
  vector<Person *>::iterator it;
  for(it = person.begin(); it != person.end(); it++) {
    if(id == (*it)->get_id()) break;
  }
  if(it != person.end()) {
    person.erase(it);
  }
}

