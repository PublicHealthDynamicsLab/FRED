/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Small_Cell.cc
//

#include "Global.h"
#include "Small_Cell.h"
#include "Small_Grid.h"
#include "Random.h"

void Small_Cell::setup(Small_Grid * grd, int i, int j, double xmin, double xmax, double ymin, double ymax) {
  grid = grd;
  row = i;
  col = j;
  min_x = xmin;
  max_x = xmax;
  min_y = ymin;
  max_y = ymax;
  center_y = (min_y+max_y)/2.0;
  center_x = (min_x+max_x)/2.0;
  neighbors = (Small_Cell **) grid->get_neighbors(i,j);
}

void Small_Cell::print() {
  printf("Small_Cell %d %d: %f, %f, %f, %f\n",row,col,min_x,max_x,min_y,max_y);
  for (int i = 0; i < 9; i++) {
    if (neighbors[i] == NULL) { printf("NULL ");}
    else {neighbors[i]->print_coord();}
    printf("\n");
  }
}

void Small_Cell::print_coord() {
  printf("(%d, %d)",row,col);
}

void Small_Cell::quality_control() {
  return;
}

double Small_Cell::distance_to_grid_cell(Small_Cell *p2) {
  double x1 = center_x;
  double y1 = center_y;
  double x2 = p2->get_center_x();
  double y2 = p2->get_center_y();
  return sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
}





