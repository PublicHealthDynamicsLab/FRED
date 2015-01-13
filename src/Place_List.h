/*
 Copyright 2009 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Place_List.h
//

#ifndef _FRED_PLACE_LIST_H
#define _FRED_PLACE_LIST_H

#include <vector>
#include <map>
//TODO test performance of unordered_map for lookup by label
//#include <tr1/unordered_map>
#include <cstring>

#include "Global.h"
#include "Place.h"
#include "Utils.h"

using namespace std;

class School;
class Neighborhood;
class Household;
class Office;
class Hospital;
class Classroom;
class Workplace;

class Place_List {
public:
  
  Place_List() {
    places.clear();
    workplaces.clear();
    next_place_id = 0;
    init_place_type_name_lookup_map();
  }

  void read_places();
  void prepare();
  void update(int day);
  void quality_control(char * directory);
  void get_parameters();
  Place * get_place_from_label(char *s);
  Place * get_place_at_position(int i) { return places[i]; }


  // TODO have to assign id consistently.  don't set id in place constructor, instead
  // initialize it with -1, then set the id in add_place.

  int get_number_of_places() { return places.size(); }
 
  int get_number_of_places( char place_type );
  //int get_max_id() { return max_id; }
  
  int get_new_place_id() {
    int id = next_place_id;
    ++( next_place_id );
    return id;
  }

  void setup_classrooms();
  void setup_offices();
  Place * get_random_workplace();
  void end_of_run();

private:

  std::map< char, string > place_type_name_lookup_map;

  void init_place_type_name_lookup_map() {
    place_type_name_lookup_map[ NEIGHBORHOOD ]  = "NEIGHBORHOOD";
    place_type_name_lookup_map[ HOUSEHOLD ]     = "HOUSEHOLD";
    place_type_name_lookup_map[ SCHOOL ]        = "SCHOOL";
    place_type_name_lookup_map[ CLASSROOM ]     = "CLASSROOM";
    place_type_name_lookup_map[ WORKPLACE ]     = "WORKPLACE";
    place_type_name_lookup_map[ OFFICE ]        = "OFFICE";
    place_type_name_lookup_map[ HOSPITAL ]      = "HOSPITAL";
    place_type_name_lookup_map[ COMMUNITY ]     = "COMMUNITY";
  }

  string lookup_place_type_name( char place_type ) {
    assert( place_type_name_lookup_map.find( place_type ) !=
            place_type_name_lookup_map.end() );
    return place_type_name_lookup_map[ place_type ];
  }

  int add_place( Place * p );

  template< typename Place_Type >
  void add_preallocated_places( char place_type, Place::Allocator< Place_Type > & pal ) {
    // make sure that the expected number of places were allocated
    assert( pal.get_number_of_contiguous_blocks_allocated() == 1 );
    assert( pal.get_number_of_remaining_allocations() == 0 );

    int places_added = 0; 
    Place_Type * place = pal.get_base_pointer();
    int places_allocated = pal.size();
   
    for ( int i = 0; i < places_allocated; ++i ) {
      if ( add_place ( place ) ) {
        ++( places_added );
      }
      ++( place );
    }
    FRED_STATUS( 0, "Added %7d %16s places to Place_List\n",
        places_added, lookup_place_type_name( place_type ).c_str() );
    FRED_CONDITIONAL_WARNING( places_added != places_allocated,
        "%7d %16s places were added to the Place_List, but %7d were allocated\n",
        places_added, lookup_place_type_name( place_type ).c_str(),
        places_allocated );
    // update/set place_type_counts for this place_type
    place_type_counts[ place_type ] = places_added;
  } 

  // map to hold counts for each place type
  std::map< char, int > place_type_counts;

  int next_place_id;

  char locfile[80];

  struct Place_Init_Data {
    char s[ 80 ];
    char place_type;
    fred::geo lat, lon;
    Place_Init_Data( char _s[], char _place_type, fred::geo _lat, fred::geo _lon ):
      place_type( _place_type ), lat( _lat ), lon( _lon ) {
        strcpy( s, _s );
    };
    bool operator< ( const Place_Init_Data & other ) const {
      if ( place_type != other.place_type ) {
        return place_type < other.place_type;
      }
      else {
        return lat < other.lat;
      }
    }
  };

  vector <Place *> places;
  vector <Place *> workplaces;

  map<int, int> place_map;
  map<string, int> place_label_map;

  //int max_id;
};


#endif // _FRED_PLACE_LIST_H
