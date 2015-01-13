/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */
//
//
// File: Geo_Utils.cc
//

#include "Geo_Utils.h"

double Geo_Utils::km_per_deg_longitude = MEAN_US_KM_PER_DEG_LON;
double Geo_Utils::km_per_deg_latitude = MEAN_US_KM_PER_DEG_LAT;
// Geo_Utils::km_per_deg_longitude = Geo_Utils::ALLEG_KM_PER_DEG_LON;
// Geo_Utils::km_per_deg_latitude = Geo_Utils::ALLEG_KM_PER_DEG_LAT;

const double Geo_Utils::DEG_TO_RAD = 0.017453292519943295769236907684886; // PI/180

// see http://andrew.hedges.name/experiments/haversine/
const double Geo_Utils::EARTH_RADIUS = 6373.0; // earth's radius in kilometers
const double Geo_Utils::KM_PER_DEG_LAT = 111.325; // assuming spherical earth

// Allegheny VALUES - for regression test
const double Geo_Utils::ALLEG_KM_PER_DEG_LON = 84.83063; // assuming spherical earth
const double Geo_Utils::ALLEG_KM_PER_DEG_LAT = 111.04326; // assuming spherical earth

// US Mean latitude-longitude (http://www.travelmath.com/country/United+States)
const fred::geo MEAN_US_LON = -97.0; // near Wichita, KS
const fred::geo MEAN_US_LAT = 38.0; // near Wichita, KS

// from http://www.ariesmar.com/degree-latitude.php
const double Geo_Utils::MEAN_US_KM_PER_DEG_LON = 87.832; // at 38 deg N
const double Geo_Utils::MEAN_US_KM_PER_DEG_LAT = 110.996; // 


void Geo_Utils::set_km_per_degree(fred::geo lat) {
  lat *= Geo_Utils::DEG_TO_RAD;
  km_per_deg_longitude = cos(lat) * KM_PER_DEG_LAT;
  km_per_deg_latitude = KM_PER_DEG_LAT;
}

double Geo_Utils::haversine_distance (fred::geo lon1, fred::geo lat1, fred::geo lon2, fred::geo lat2) {
  // convert to radians
  lat1 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;
  double latH = sin(0.5 * (lat2 - lat1));
  latH *= latH;
  double lonH = sin(0.5 * (lon2 - lon1));
  lonH *= lonH;
  double a = latH + cos(lat1) * cos(lat2) * lonH;
  double c = 2 * atan2( sqrt(a), sqrt(1 - a));
  double dist = EARTH_RADIUS * c;
  return dist;
}

double Geo_Utils::spherical_cosine_distance (fred::geo lon1, fred::geo lat1, fred::geo lon2, fred::geo lat2) {
  // convert to radians
  lat1 *= DEG_TO_RAD;
  lon1 *= DEG_TO_RAD;
  lat2 *= DEG_TO_RAD;
  lon2 *= DEG_TO_RAD;
  return acos(sin(lat1)*sin(lat2)+cos(lat1)*cos(lat2)*cos(lon2-lon1))*EARTH_RADIUS;
}


double Geo_Utils::spherical_projection_distance (fred::geo lon1, fred::geo lat1, fred::geo lon2, fred::geo lat2) {
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

void Geo_Utils::translate_to_cartesian(fred::geo lat, fred::geo lon, double *x, double *y,
               fred::geo min_lat, fred::geo min_lon) {
  *x = (lon - min_lon) * km_per_deg_longitude;
  *y = (lat - min_lat) * km_per_deg_latitude;
}


void Geo_Utils::translate_to_lat_lon(double x, double y, fred::geo *lat, fred::geo *lon,
             fred::geo min_lat, fred::geo min_lon) {
  *lon = min_lon + x / km_per_deg_longitude;
  *lat = min_lat + y / km_per_deg_latitude;
}




/*

int main () {
  double lon1 = -79;
  double lat1 = 40;
  int N = 0;
  for (double lon = -80.0; lon <= -79.0; lon += 0.01) {
    for (double lat = 40; lat < 40.1; lat += 0.01 ) {
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

