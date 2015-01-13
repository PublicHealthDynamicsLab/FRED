/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Travel.cc
//

#include <vector>
#include "Travel.h"
#include "Global.h"
#include "Params.h"
#include "Random.h"
#include "Large_Grid.h"
#include "Large_Cell.h"
#include "Person.h"
#include "Utils.h"
#include "Geo_Utils.h"
#include <stdio.h>
#include <vector>
#include <set>

//typedef vector <Person*> pvec;      // vector of person ptrs
typedef set <Person*> pset;      // vector of person ptrs
typedef set<Person*>::iterator piter; // iterator to set of person ptrs
typedef pset * pset_ptr;      // pointer to above type
static pset * traveler_list;       // list of travelers, per day
static pset_ptr * traveler_list_ptr;        // pointers to above lists
static double mean_trip_duration;    // mean days per trip

#include <vector>
static vector <int> src_row;
static vector <int> src_col;
static vector <int> dest_row;
static vector <int> dest_col;
static vector <int> trip_list;
static vector <double> trip_probability;
static int trip_list_size;
static int max_trip_list_size;
static double active_trip_fraction;
static int max_trips_per_day;
static int trips_per_day;
char tripfile[256];

// runtime parameters
static double * Travel_Duration_Cdf;    // cdf for trip duration
static int max_Travel_Duration = 0;    // number of days in cdf

void Travel::setup(char * directory) {
  assert(Global::Enable_Large_Grid && Global::Enable_Travel);
  Global::Large_Cells->read_max_popsize();
  src_row.clear();
  src_col.clear();
  dest_row.clear();
  dest_col.clear();
  trip_list.clear();
  int active_trips = 0;

  // get run-time parameters
  int n;
  Params::get_param_from_string("max_trips_per_day",&max_trips_per_day);
  Params::get_param_from_string("tripfile", tripfile);
  Params::get_param_from_string("travel_duration",&n);
  Travel_Duration_Cdf = new double [n];
  max_Travel_Duration = Params::get_param_vector((char *) "travel_duration",
           Travel_Duration_Cdf) - 1;
 
  if (Global::Verbose > 0) {
    for (int i = 0; i <= max_Travel_Duration; i++)
      fprintf(Global::Statusfp,"travel duration = %d %f ",i,Travel_Duration_Cdf[i]);
    fprintf(Global::Statusfp,"\n");
  }
  mean_trip_duration = (max_Travel_Duration + 1) * 0.5;

  // set up empty vectors of people currently on trips
  // one vector for each day remaining
  traveler_list = new pset[max_Travel_Duration+1];
  for (int i = 0; i <= max_Travel_Duration; i++) {
    traveler_list[i].clear();
  }

  // setup pointers to the above lists, to be advanced each day
  // (so that lists are not copied)
  traveler_list_ptr = new pset_ptr[max_Travel_Duration+1];
  for (int i = 0; i <= max_Travel_Duration; i++) {
    traveler_list_ptr[i] = &traveler_list[i];
  }

  // read the preprocessed trip file
  FILE *fp = Utils::fred_open_file(tripfile);
  if (fp == NULL) { Utils::fred_abort("Help! Can't open tripfile %s\n", tripfile); }
  if (Global::Verbose > 0)
    fprintf(Global::Statusfp, "reading tripfile %s\n", tripfile); fflush(stdout);
  int trips_outside_region = 0;
  max_trip_list_size = 0;
  int r1,c1,r2,c2;
  double pop_threshold = 0.05;
  while (fscanf(fp, "%d %d %d %d", &c1,&r1,&c2,&r2) == 4) {
    max_trip_list_size++;
    Large_Cell * src_cell = Global::Large_Cells->get_grid_cell_with_global_coords(r1,c1);
    Large_Cell * dest_cell = Global::Large_Cells->get_grid_cell_with_global_coords(r2,c2);

    // reject this trip if neither endpoint is in the model region
    if (src_cell == NULL && dest_cell == NULL) continue;
 
    // swap endpoint so that the source cell is in the model region
    if (src_cell == NULL) {
      src_cell = dest_cell;
      dest_cell = NULL;
    }
    assert(src_cell != NULL);

    // get the density of the cell populations relative to actual cell population
    double src_density = src_cell->get_pop_density();
    double dest_density = dest_cell!= NULL? dest_cell->get_pop_density():1.0;

    // attenuate trip probability based on cell pop density
    double trip_prob = src_density*dest_density;

    // reject trip with low trip probability
    if (trip_prob < pop_threshold) continue;

    src_row.push_back(r1);
    src_col.push_back(c1);
    dest_row.push_back(r2);
    dest_col.push_back(c2);
    trip_probability.push_back(trip_prob);
    active_trips++;
   if (Global::Verbose > 1) {
      fprintf(Global::Statusfp, "ACTIVE %d %d %d %d %0.2f\n", c1,r1,c2,r2,trip_prob);
      /*
  int src_pop = src_cell!= NULL? src_cell->get_popsize():-1;
  int src_maxpop = src_cell!= NULL? src_cell->get_max_popsize():-1;
  int dest_pop = dest_cell!= NULL? dest_cell->get_popsize():-1;
  int dest_maxpop = dest_cell!= NULL? dest_cell->get_max_popsize():-1;
  fprintf(Global::Statusfp, "%d %d %0.2f %d %d %0.2f\n", 
  src_pop,src_maxpop,src_density,dest_pop,dest_maxpop, dest_density);
      */
    }

    // count trips that have one endpoint outside region
    if (dest_cell == NULL) { trips_outside_region++; }
  }
  fclose(fp);

  // get the fraction of active trips
  active_trip_fraction = (double) active_trips / (double) max_trip_list_size;

  // the number of trips per day is proportional to active fraction of trips
  trips_per_day = (int) (max_trips_per_day * active_trip_fraction + 0.5);

  if (Global::Verbose > 0) {
    fprintf(Global::Statusfp, "finished reading tripfile\n");
    fprintf(Global::Statusfp, "active_trips %d total_trips %d\n", active_trips, max_trip_list_size);
    fprintf(Global::Statusfp, "trips per day %d \n", trips_per_day);
    fprintf(Global::Statusfp, "trips outside region %d \n", trips_outside_region);
    fflush(Global::Statusfp);
  }
  
  // create a list of indexes for the active trips (this will be shuffled below)
  for (int i = 0; i < active_trips; i++) {
    trip_list.push_back(i);
  }
  trip_list_size = active_trips;

  // save the active trips for possible later use
  /*
  char activefilename[256];
  sprintf(activefilename, "%s/active_trips.txt", directory);
  FILE *outfp = fopen(activefilename,"w");
  for (int i = 0; i < active_trips; i++) {
    fprintf(outfp, "%d %d %d %d\n", src_col[i], src_row[i], dest_col[i], dest_row[i]);
  }
  fclose(outfp);
  */

}


void Travel::update_travel(int day) {
  Person * visitor;
  Person * visited;

  if (!Global::Enable_Travel)
    return;

  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "update_travel entered\n");
    fprintf(Global::Statusfp, "trips per day = %d\n", trips_per_day);
    fflush(Global::Statusfp);
  }

  // initiate new trips
  for (int i = 0; i < trips_per_day; i++) {
    select_visitor_and_visited(&visitor, &visited, day);
    
    // check to see if trip was rejected
    if (visitor == NULL && visited == NULL)
      continue;

    if (visitor != NULL) {
      // can't start new trip if traveling
      if (visitor->get_travel_status()) continue;
      if (visited != NULL && visited->get_travel_status()) continue;
  
      // Anuroop
      /*{
        if(visitor && visited){
          double lat1 = visitor->get_household()->get_latitude();
          double lat2 = visited->get_household()->get_latitude();
          if( (lat1 < 45.93 && lat2 > 45.93) || (lat1 > 45.93 && lat2 < 45.93) ) {
            cout << "TRIP MADE: " << lat1 << " " << lat2 << endl;
          }
        }
      }*/
      
      // put traveler in travel status
      visitor->start_traveling(visited);

      // put traveler on list for given number of days to travel
      int duration = draw_from_distribution(max_Travel_Duration, Travel_Duration_Cdf);
      // printf("  for %d days\n", duration); fflush(stdout);
     traveler_list_ptr[duration]->insert(visitor);
    }
    else {
      // TODO: process incoming trips from outside the model region
    }
  }
  
  // process travelers who are returning home
  // printf("returning home:\n"); fflush(stdout);
  for(piter it=traveler_list_ptr[0]->begin(); it != traveler_list_ptr[0]->end(); it++) {
    (*it)->stop_traveling();
  }
  traveler_list_ptr[0]->clear();

  // update days still on travel (after today)
  pset * tmp = traveler_list_ptr[0];
  for (int i = 0; i < max_Travel_Duration; i++) {
    traveler_list_ptr[i] = traveler_list_ptr[i+1];
  }
  traveler_list_ptr[max_Travel_Duration] = tmp;

  if (Global::Verbose > 1) {
    fprintf(Global::Statusfp, "update_travel finished\n");
    fflush(Global::Statusfp);
  }

  return;
}

void Travel::select_visitor_and_visited(Person **v1, Person **v2, int day) {
  //static int trip_counter = 0;
  // Anuroop
  static long trip_counter = 0;
  Person * visitor = NULL;
  Person * visited = NULL;

  if (Global::Verbose > 2) {
    fprintf(Global::Statusfp, "select_visitor entered ");
    fprintf(Global::Statusfp, "trip_counter %ld trip_list_size %d\n",
      trip_counter, trip_list_size);
    fflush(Global::Statusfp);
  }

  // shuffle trip list each time through the list
  if (trip_counter % trip_list_size == 0) FYShuffle<int>(trip_list);

  // get the next trip in the list
  int trip = trip_list[trip_counter % trip_list_size];
  trip_counter++;

  if (Global::Verbose > 2) {
  //if(day >= 430) 
    fprintf(Global::Statusfp, "trip_index %d trip_counter %ld\n",
      trip, trip_counter);
    fprintf(Global::Statusfp, "src_sizes: %zu %zu dest_sizes %zu %zu\n",
      src_row.size(), src_col.size(), dest_row.size(), dest_col.size());
    fflush(Global::Statusfp);
  }
  
  // Anuroop
  if(src_row.size() <= trip || src_col.size() <= trip || 
      dest_row.size() <= trip || dest_col.size() <= trip) {
    fprintf(Global::Statusfp, "Illegal trip: %d\n", trip);
    fflush(Global::Statusfp);
    *v1 = NULL, *v2 = NULL;
    return;
  }

 // extract rows and cols for cells at endpoints
  int r1 = src_row[trip];
  int c1 = src_col[trip];
  int r2 = dest_row[trip];
  int c2 = dest_col[trip];
  double trip_pr = trip_probability[trip];
  //if (Global::Verbose > 2) {
  //  fprintf(Global::Statusfp,"TRIP %d %d %d %d %.2f\n", c1,r1,c2,r2,trip_pr);
  //  fflush(Global::Statusfp);
  //}

  // reject this trip based on trip probability
  if (trip_pr < RANDOM()) {
    if (Global::Verbose > 2) {
      fprintf(Global::Statusfp,"REJECT: %d %d %d %d %.2f\n",c1,r1,c2,r2,trip_pr);
      fflush(Global::Statusfp);
    }
    *v1 = NULL;
    *v2 = NULL;
    return;
  }

  // get the endpoint cells for  the trip
  Large_Cell * src_cell = Global::Large_Cells->get_grid_cell_with_global_coords(r1,c1);
  Large_Cell * dest_cell = Global::Large_Cells->get_grid_cell_with_global_coords(r2,c2);

  // select visitor and visited persons
  if (src_cell != NULL) {visitor = src_cell->select_random_person();}
  if (dest_cell != NULL) {visited = dest_cell->select_random_person();}

  // randomly pick the direction of the trip
  if (RANDOM() < 0.5) {
    Person * tmp = visitor;
    visitor = visited;
    visited = tmp;
  }

  *v1 = visitor;
  *v2 = visited;
  return;
}

void Travel::terminate_person(Person *person) {
  for(unsigned int i = 0; i <= max_Travel_Duration; i++) {
    piter it = traveler_list[i].find(person);
    if(it != traveler_list[i].end()) {
      traveler_list[i].erase(it);
      break;
    }
  }
}

void Travel::quality_control(char * directory) {
}

