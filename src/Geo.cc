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
// File: Geo.cc
//

#include "Geo.h"

const double Geo::DEG_TO_RAD = 0.017453292519943295769236907684886; // PI/180

// see http://andrew.hedges.name/experiments/haversine/
const double Geo::EARTH_RADIUS = 6373.0; // earth's radius in kilometers
const double Geo::KM_PER_DEG_LAT = 111.325; // assuming spherical earth

// US Mean latitude-longitude (http://www.travelmath.com/country/United+States)
const double MEAN_US_LON = -97.0; // near Wichita, KS
const double MEAN_US_LAT = 38.0; // near Wichita, KS

// from http://www.ariesmar.com/degree-latitude.php
const double Geo::MEAN_US_KM_PER_DEG_LON = 87.832; // at 38 deg N
const double Geo::MEAN_US_KM_PER_DEG_LAT = 110.996; // 

double Geo::km_per_deg_longitude = Geo::MEAN_US_KM_PER_DEG_LON;
double Geo::km_per_deg_latitude = Geo::MEAN_US_KM_PER_DEG_LAT;

void Geo::set_km_per_degree(fred::geo lat) {
  lat *= Geo::DEG_TO_RAD;
  double cosine = cos(lat);
  Geo::km_per_deg_longitude = cosine * Geo::KM_PER_DEG_LAT;
  Geo::km_per_deg_latitude = Geo::KM_PER_DEG_LAT;
  // printf("lat %.8f cosine %.8f km_per_deg_lat %.8f\n", lat, cosine, Geo::km_per_deg_longitude);
}

double Geo::haversine_distance (fred::geo lon1, fred::geo lat1, fred::geo lon2, fred::geo lat2) {
  // convert to radians
  lat1 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;
  fred::geo latH = sin(0.5 * (lat2 - lat1));
  latH *= latH;
  fred::geo lonH = sin(0.5 * (lon2 - lon1));
  lonH *= lonH;
  double a = latH + cos(lat1) * cos(lat2) * lonH;
  double c = 2 * atan2( sqrt(a), sqrt(1 - a));
  double dist = EARTH_RADIUS * c;
  return dist;
}

double Geo::spherical_cosine_distance (fred::geo lon1, fred::geo lat1, fred::geo lon2, fred::geo lat2) {
  // convert to radians
  lat1 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;
  return acos(sin(lat1)*sin(lat2)+cos(lat1)*cos(lat2)*cos(lon2-lon1))*EARTH_RADIUS;
}

double Geo::spherical_projection_distance (fred::geo lon1, fred::geo lat1, fred::geo lon2, fred::geo lat2) {
  // convert to radians
  lat1 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;
  double dlat = (lat2-lat1);
  dlat *= dlat;
  double dlon = (lon2-lon1);
  double tmp = cos(0.5*(lat1+lat2))*dlon;
  tmp *= tmp;
  return EARTH_RADIUS*sqrt(dlat+tmp);
}

/*

  int main () {
  fred::geo lon1 = -79;
  fred::geo lat1 = 40;
  int N = 0;
  for (fred::geo lon = -80.0; lon <= -79.0; lon += 0.01) {
  for (fred::geo lat = 40; lat < 40.1; lat += 0.01 ) {
  double d1 = haversine_distance(lon1,lat1,lon,lat);
  double d2 = spherical_cosine_distance(lon1,lat1,lon,lat);
  double d3 = spherical_projection_distance(lon1,lat1,lon,lat);
  N++;
  printf("%6.3f %5.2f %8.3f %8.3f %f\n", lon, lat, d1, d3, d1-d3);
  }
  }
  printf("N = %d\n", N);
  }

*/

