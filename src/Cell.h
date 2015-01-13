/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Cell.h
//

#ifndef _FRED_CELL_H
#define _FRED_CELL_H

#include <vector>
#include <string.h>
#include "Abstract_Cell.h"
#include "Place.h"
#include "Global.h"
class Person;
class Grid;
class Neighborhood;

class Cell : public Abstract_Cell {
public:

  /**
   * Default constructor
   */
  Cell() {}
  ~Cell() {}

  /**
   * Set all of the attributes for the Cell
   *
   * @param grd the Grid to which this cell belongs
   * @param i the row of this Cell in the Grid
   * @param j the column of this Cell in the Grid
   * @param xmin the minimum x value for this cell
   * @param xmax the maximum x value for this cell
   * @param ymin the minimum y value for this cell
   * @param ymax the minimum y value for this cell
   *
   */
  void setup(Grid * grd, int i, int j, double xmin, double xmax, double ymin, double ymax);

  /**
   * Print out information about this object
   */
  void print();

  /**
   * Print out the x and y of this Cell as an ordered pair
   */
  void print_coord();

  /**
   * Used during debugging to verify that code is functioning properly.
   *
   */
  void quality_control();

  /**
   * Determines distance from this Cell to another.  Note, that it is distance from the
   * <strong>center</strong> of this Cell to the <strong>center</strong> of the Cell in question.
   *
   * @param the grid Cell to check against
   * @return the distance from this grid Cell to the one in question
   */
  double distance_to_grid_cell(Cell *grid_cell2);

  // specific to Cell grid:
  /**
   * Setup the neighborhood in this Cell
   */
  void make_neighborhood( Place::Allocator< Neighborhood > & neighborhood_allocator );

  /**
   * Add household to this Cell's household vector
   */
  void add_household(Place *p);

  /**
   * Create lists of persons, workplaces, schools (by age)
   */
  void record_favorite_places();

  /**
   * Select either this neighborhood or a neighborhood taken from one of this Cell's neighboring Cells in the Grid.
   *
   * @return a pointer to a Neighborhood
   */
  Place * select_neighborhood(double community_prob, double community_distance, double local_prob);

  /**
   * @return a pointer to a random Person in this Cell
   */
  Person * select_random_person();

  /**
   * @return a pointer to a random Household in this Cell
   */
  Place * select_random_household();

  /**
   * @return a pointer to a random Workplace in this Cell
   */
  Place * select_random_workplace();

  /**
   * @return a pointer to a random School in this Cell
   */
  Place * select_random_school(int age);

  /**
   * @return a count of houses in this Cell
   */
  int get_houses() { return houses;}

  /**
   * @return list of households in this grid cell.
   */
  vector <Place *> get_households() { return household; }

  /**
   * @return a pointer to this Cell's Neighborhood
   */
  Place * get_neighborhood() { return neighborhood; }

  /**
   * Add a person to this Cell's Neighborhood
   * @param per a pointer to the Person to add
   */
  void enroll(Person *per) { neighborhood->enroll(per); }

  /**
   * @return the count of occupied houses in the Cell
   */
  int get_occupied_houses() { return occupied_houses; }

  /**
   * Increment the count of occupied houses in the Cell
   */
  void add_occupied_house() { occupied_houses++; }

  /**
   * Decrement the count of occupied houses in the Cell
   */
  void subtract_occupied_house() { occupied_houses--; }

  /**
   * @return the target_households
   */
  int get_target_households() { return target_households; }

  /**
   * @return the target_popsize
   */
  int get_target_popsize() { return target_popsize; }

protected:
  Cell ** neighbor_cells;
  Grid * grid;

  int houses;
  Place * neighborhood;
  vector <Place *> household;
  vector <Place *> school[20];
  vector <Place *> workplace;
  vector <Person *> person;

  // target grid_cell variables;
  int target_households;
  int target_popsize;
  int occupied_houses;

  fred::Mutex mutex;
};

#endif // _FRED_CELL_H
