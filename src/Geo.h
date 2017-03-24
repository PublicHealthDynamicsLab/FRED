/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Geo.h
//

#ifndef FRED_GEO_H_
#define FRED_GEO_H_

#include "Global.h"
#include <cmath>

using namespace std;


class Geo{
public:

  static const double DEG_TO_RAD;		// PI/180
  static double km_per_deg_longitude;
  static double km_per_deg_latitude;

  /**
   * Sets the kilometers per degree longitude at a given latitiude
   *
   * @param lat the latitude to set KM / degree
   */
  static void set_km_per_degree(fred::geo lat);

  /**
   * @param lon1
   * @param lat1
   * @param lon2
   * @param lat2
   *
   * @return the haversine distance between the two points on the Earth's surface
   */
  static double haversine_distance (fred::geo lon1, fred::geo lat1, fred::geo lon2, fred::geo lat2);

  /**
   * @param lon1
   * @param lat1
   * @param lon2
   * @param lat2
   *
   * @return the spherical cosine distance between the two points on the Earth's surface
   */
  static double spherical_cosine_distance (fred::geo lon1, fred::geo lat1, fred::geo lon2, fred::geo lat2);

  /**
   * @param lon1
   * @param lat1
   * @param lon2
   * @param lat2
   *
   * @return the spherical projection distance between the two points on the Earth's surface
   */
  static double spherical_projection_distance (fred::geo lon1, fred::geo lat1, fred::geo lon2, fred::geo lat2);

  static double get_x(fred::geo lon) { 
    return (lon + 180.0)*km_per_deg_longitude;
  }
  static double get_y(fred::geo lat) { 
    return (lat + 90.0) * km_per_deg_latitude;
  }
  static double get_longitude(double x) { 
    return (double) (x / km_per_deg_longitude - 180.0);
  }
  static double get_latitude(double y) {
    return (double) (y / km_per_deg_latitude - 90.0);
  }

  static double xy_distance(fred::geo lat1, fred::geo lon1, fred::geo lat2, fred::geo lon2) {
    double x1 = get_x(lon1); double y1 = get_y(lat1);
    double x2 = get_x(lon2); double y2 = get_y(lat2);
    return sqrt((x1-x2)*(x1-x2)+(y1-y2)*(y1-y2));
  }
  
  static double xsize_to_degree_longitude(double xsize) {
    return (xsize / km_per_deg_longitude);
  }
  
  static double ysize_to_degree_latitude(double ysize) {
    return (ysize / km_per_deg_latitude);
  }
  
  
private:
  // see http://andrew.hedges.name/experiments/haversine/
  static const double EARTH_RADIUS;	 // earth's radius in kilometers
  static const double KM_PER_DEG_LAT;	     // assuming spherical earth
  
  // US Mean latitude-longitude (http://www.travelmath.com/country/United+States)
  static const double MEAN_US_LON;		// near Wichita, KS
  static const double MEAN_US_LAT;		// near Wichita, KS
  
  // from http://www.ariesmar.com/degree-latitude.php
  static const double MEAN_US_KM_PER_DEG_LON;		// at 38 deg N
  static const double MEAN_US_KM_PER_DEG_LAT; // 

};

#endif /* FRED_GEO_H_ */

