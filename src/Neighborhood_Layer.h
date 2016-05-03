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
// File: Neighborhood_Layer.h
//

#ifndef _FRED_NEIGHBORHOOD_LAYER_H
#define _FRED_NEIGHBORHOOD_LAYER_H

#include <vector>
using namespace std;

#include "Place.h"
#include "Abstract_Grid.h"

typedef std::vector<int> offset_t;
typedef std::vector<double> gravity_cdf_t;
typedef std::vector<Place *> place_vector_t;

class Neighborhood_Patch;
class Neighborhood;

class Neighborhood_Layer : public Abstract_Grid {
public:
  Neighborhood_Layer();
  ~Neighborhood_Layer() {}

  void setup();
  void setup( Place::Allocator< Neighborhood > & neighborhood_allocator );

  /**
   * @param row the row where the Patch is located
   * @param col the column where the Patch is located
   * @return a pointer to the Patch at the row and column requested
   */
  void prepare();
  Neighborhood_Patch * get_patch(int row, int col);
  Neighborhood_Patch * get_patch(fred::geo lat, fred::geo lon);
  Neighborhood_Patch * select_random_patch(double x0, double y0, double dist);
  Neighborhood_Patch * select_random_neighbor(int row, int col);
  Place * select_school_in_area(int age, int row, int col);
  Place * select_workplace_in_area(int row, int col);

  /**
   * @return a pointer to a random patch in this layer
   */
  Neighborhood_Patch * select_random_patch();

  /**
   * Used during debugging to verify that code is functioning properly.
   */
  void quality_control();
  void quality_control(double min_x, double min_y);

  /**
   * @brief Get all people living within a specified radius of a point.
   *
   * Finds patches overlapping radius_in_km from point, then finds all households in those patches radius_in_km distance from point.
   * Returns list of people in those households radius_in_km from point.
   * Used in Epidemic::update for geographical seeding.
   *
   * @author Jay DePasse
   * @param lat the latitude of the center of the circle
   * @param lon the longitude of the center of the circle
   * @param radius_in_km the distance from the center of the circle
   * @return a Vector of places
   */
  vector < Place * > get_households_by_distance(fred::geo lat, fred::geo lon, double radius_in_km);

  /**
   * Make each patch in this layer store its daily activity locations, and then set popsize and households
   */
  void record_daily_activity_locations();

  /**
   * @return the popsize
   */
  int get_popsize() {
    return this->popsize;
  }

  /**
   * @return the households
   */
  int get_households() {
    return this->households;
  }

  /**
   * Write the household distributions to a file
   *
   * @param dir the directory where the file will go
   * @param date_string the date of the write (used for filename)
   * @param run the run id (used for filename)
   */
  int get_number_of_neighborhoods();

  void setup_gravity_model();

  void setup_null_gravity_model();

  void print_gravity_model();

  void print_distances();
  
  Place * select_destination_neighborhood(Place * src_neighborhood);

  Place * select_destination_neighborhood_by_gravity_model(Place * src_neighborhood);

  Place * select_destination_neighborhood_by_old_model(Place * src_neighborhood);

  void register_place(Place *place);

protected:
  Neighborhood_Patch ** grid;		 // Rectangular array of patches
  int popsize;
  int households;
  int age_distrib[100];

  // data used by neighborhood gravity model
  offset_t ** offset;
  gravity_cdf_t ** gravity_cdf;
  int max_offset;
  vector < pair <double,int> >sort_pair;

  // runtime parameters for neighborhood gravity model
  bool Enable_neighborhood_gravity_model;
  double max_distance;
  double min_distance;
  int max_destinations;
  double pop_exponent;
  double dist_exponent;

  // runtime parameters for old neighborhood model (deprecated)
  double Community_distance;			// deprecated
  double Community_prob;			// deprecated
  double Home_neighborhood_prob;		// deprecated
};

#endif // _FRED_NEIGHBORHOOD_LAYER_H
