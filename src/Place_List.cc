/*
   Copyright 2009 by the University of Pittsburgh
   Licensed under the Academic Free License version 3.0
   See the file "LICENSE" for more information
   */

//
//
// File: Place_List.cc
//

#include <math.h>
#include <set>
#include <new>
#include <iostream>

#include "Place_List.h"
#include "Population.h"
#include "Disease.h"
#include "Global.h"
#include "Place.h"
#include "School.h"
#include "Classroom.h"
#include "Workplace.h"
#include "Office.h"
#include "Neighborhood.h"
#include "Household.h"
#include "Hospital.h"
#include "Params.h"
#include "Person.h"
#include "Grid.h"
#include "Large_Grid.h"
#include "Small_Grid.h"
#include "Geo_Utils.h"
#include "Travel.h"
#include "Seasonality.h"
#include "Random.h"
#include "Utils.h"

// Place_List::quality_control implementation is very large,
// include from separate .cc file:
#include "Place_List_Quality_Control.cc"

void Place_List::get_parameters() {
}

void Place_List::read_places() {
  char s[80];
  char line[1024];
  char place_type;
  fred::geo lon, lat;
  fred::geo min_lat, max_lat, min_lon, max_lon;
  char location_file[256];
  bool use_gzip = false;
  FILE *fp = NULL;

  // vector to hold init data
  std::vector< Place_Init_Data > pidv;

  // vectors to hold household income
  std::vector< string > household_labels;
  std::vector< int > household_incomes;

  FRED_STATUS(0, "read places entered\n", "");

  // to compute the region's bounding box
  min_lat = min_lon = 999;
  max_lat = max_lon = -999;

  // initialize counts to zero
  place_type_counts[ HOUSEHOLD ] = 0;       // 'H'
  place_type_counts[ SCHOOL ] = 0;          // 'S'
  place_type_counts[ WORKPLACE ] = 0;       // 'W'
  place_type_counts[ HOSPITAL ] = 0;        // 'M'
  place_type_counts[ NEIGHBORHOOD ] = 0;    // 'N'
  place_type_counts[ CLASSROOM ] = 0;       // 'C'
  place_type_counts[ OFFICE ] = 0;          // 'O'
  place_type_counts[ COMMUNITY ] = 0;       // 'X'

  if (strcmp(Global::Synthetic_population_id, "none") == 0) {

    // read original FRED location file

    Params::get_param_from_string("locfile", locfile);
    strcpy(location_file, locfile);
    fp = Utils::fred_open_file(location_file);

    if (fp == NULL) {
      // try to find the gzipped version
      char location_file_gz[256];
      sprintf(location_file_gz, "%s.gz", location_file);
      if (Utils::fred_open_file(location_file_gz)) {
	char cmd[256];
	use_gzip = true;
	sprintf(cmd, "gunzip -c %s > %s", location_file_gz, location_file);
	system(cmd);
	fp = Utils::fred_open_file(location_file);
      }
      // gunzip didn't work ...
      if (fp == NULL) {
	Utils::fred_abort("location_file %s not found\n", location_file);
      }
    }

    // assign location ids sequentially, start with zero
    // but actual assignment of place id is now done in add_place
    
    while (fgets(line, 255, fp) != NULL) {
      
      // skip white-space-only lines
      int i = 0;
      while (i < 255 && line[i] != '\0' && isspace(line[i])) i++;
      if (line[i] == '\0') continue;
      
      // skip comment lines
      if (line[0] == '#') continue;
      
      if (sscanf(line, "%s %c %f %f", s, &place_type, &lat, &lon) == 4) {
	pidv.push_back( Place_Init_Data( s, place_type, lat, lon ) );
	++( place_type_counts[ place_type ] );
      }
      else {
	Utils::fred_abort( "Help! Bad location format: %s\n", line );
      }
    }
    fclose(fp);
  }
  else {

    // read ver 2.0 synthetic population files
    char newline[1024];
    char hh_id[32];
    char serialno[32];
    char stcotrbg[32];
    char hh_race[32];
    char hh_income[32];
    char hh_size[32];
    char hh_age[32];
    char latitude[32];
    char longitude[32];
    int income;
    char workplace_id[32];
    char num_workers_assigned[32];
    char school_id[32];
    char name[64];
    char stabbr[32];
    char address[64];
    char city[32];
    char county[32];
    char zip[32];
    char zip4[32];
    char nces_id[32];
    char total[32];
    char prek[32];
    char kinder[32];
    char gr01_gr12[32];
    char ungraded[32];
    char source[32];
    char stco[32];

    household_labels.clear();
    household_incomes.clear();

    // read household locations
    sprintf(location_file, "%s/%s/%s_synth_households.txt", 
	    Global::Synthetic_population_directory,
	    Global::Synthetic_population_id,
	    Global::Synthetic_population_id);
    fp = Utils::fred_open_file(location_file);
    fgets(line, 1024, fp);
    while (fgets(line, 1024, fp) != NULL) {
      Utils::replace_csv_missing_data(newline,line,"0");
      // printf("%s%s",line,newline); fflush(stdout); exit(0);
      strcpy(hh_id,strtok(newline,","));
      strcpy(serialno,strtok(NULL,","));
      strcpy(stcotrbg,strtok(NULL,","));
      strcpy(hh_race,strtok(NULL,","));
      strcpy(hh_income,strtok(NULL,","));
      strcpy(hh_size,strtok(NULL,","));
      strcpy(hh_age,strtok(NULL,","));
      strcpy(latitude,strtok(NULL,","));
      strcpy(longitude,strtok(NULL,",\n"));

      place_type = 'H';
      sprintf(s, "%c%s", place_type, hh_id);
      sscanf(latitude, "%f", &lat);
      sscanf(longitude, "%f", &lon);

      pidv.push_back( Place_Init_Data( s, place_type, lat, lon ) );
      ++( place_type_counts[ place_type ] );
      // printf ("%s %c %f %f\n", s, place_type,lat,lon); fflush(stdout);

      // save household incomes for later processing
      sscanf(hh_income, "%d", &income);
      household_incomes.push_back(income);
      household_labels.push_back(string(s));
    }
    fclose(fp);

    // read workplace locations
    sprintf(location_file, "%s/%s/%s_workplaces.txt", 
	    Global::Synthetic_population_directory,
	    Global::Synthetic_population_id,
	    Global::Synthetic_population_id);
    fp = Utils::fred_open_file(location_file);
    fgets(line, 255, fp);
    while (fgets(line, 255, fp) != NULL) {
      Utils::replace_csv_missing_data(newline,line,"0");
      strcpy(workplace_id,strtok(newline,","));
      strcpy(num_workers_assigned,strtok(NULL,","));
      strcpy(latitude,strtok(NULL,","));
      strcpy(longitude,strtok(NULL,",\n"));

      place_type = 'W';
      sprintf(s, "%c%s", place_type, workplace_id);
      sscanf(latitude, "%f", &lat);
      sscanf(longitude, "%f", &lon);

      pidv.push_back( Place_Init_Data( s, place_type, lat, lon ) );
      ++( place_type_counts[ place_type ] );
      // printf ("%s %c %f %f\n", s, place_type,lat,lon); fflush(stdout);
    }
    fclose(fp);

    // read school locations
    sprintf(location_file, "%s/%s/%s_schools.txt", 
	    Global::Synthetic_population_directory,
	    Global::Synthetic_population_id,
	    Global::Synthetic_population_id);
    fp = Utils::fred_open_file(location_file);
    fgets(line, 255, fp);
    while (fgets(line, 255, fp) != NULL) {
      Utils::replace_csv_missing_data(newline,line,"0");
      strcpy(school_id,strtok(newline,","));
      strcpy(name,strtok(NULL,","));
      strcpy(stabbr,strtok(NULL,","));
      strcpy(address,strtok(NULL,","));
      strcpy(city,strtok(NULL,","));
      strcpy(county,strtok(NULL,","));
      strcpy(zip,strtok(NULL,","));
      strcpy(zip4,strtok(NULL,","));
      strcpy(nces_id,strtok(NULL,","));
      strcpy(total,strtok(NULL,","));
      strcpy(prek,strtok(NULL,","));
      strcpy(kinder,strtok(NULL,","));
      strcpy(gr01_gr12,strtok(NULL,","));
      strcpy(ungraded,strtok(NULL,","));
      strcpy(latitude,strtok(NULL,","));
      strcpy(longitude,strtok(NULL,","));
      strcpy(source,strtok(NULL,","));
      strcpy(stco,strtok(NULL,",\n"));

      place_type = 'S';
      sprintf(s, "%c%s", place_type, school_id);
      sscanf(latitude, "%f", &lat);
      sscanf(longitude, "%f", &lon);

      pidv.push_back( Place_Init_Data( s, place_type, lat, lon ) );
      ++( place_type_counts[ place_type ] );
      // printf ("%s %c %f %f\n", s, place_type,lat,lon); fflush(stdout);
    }
    fclose(fp);
  }

  if (use_gzip) {
    // remove the uncompressed file
    unlink(location_file);
  }

  // sort the place init data by type, location ( using Place_Init_Data::operator< )
  std::sort( pidv.begin(), pidv.end() );

  // HOUSEHOLD in-place allocator
  Place::Allocator< Household > household_allocator;
  household_allocator.reserve( place_type_counts[ HOUSEHOLD ] );
  // SCHOOL in-place allocator
  Place::Allocator< School > school_allocator;
  school_allocator.reserve( place_type_counts[ SCHOOL ] );
  // WORKPLACE in-place allocator
  Place::Allocator< Workplace > workplace_allocator;
  workplace_allocator.reserve( place_type_counts[ WORKPLACE ] );
  // HOSPITAL in-place allocator
  Place::Allocator< Hospital > hospital_allocator;
  hospital_allocator.reserve( place_type_counts[ HOSPITAL ] );
  
  // fred-specific place types initialized elsewhere (setup_offices, setup_classrooms)

  // more temporaries
  Place * place = NULL;
  Place * container = NULL;

  // loop through sorted init data and create objects using Place_Allocator
  for ( int i = 0; i < pidv.size(); ++i ) {
      strcpy( s, pidv[ i ].s );
      place_type = pidv[ i ].place_type;
      lon = pidv[ i ].lon;
      lat = pidv[ i ].lat;

      if (place_type == HOUSEHOLD && lat != 0.0) {
        if (lat < min_lat) min_lat = lat;
        if (max_lat < lat) max_lat = lat;
      }
      if (place_type == HOUSEHOLD && lon != 0.0) {
        if (lon < min_lon) min_lon = lon;
        if (max_lon < lon) max_lon = lon;
      }
      if (place_type == HOUSEHOLD) {
        place = new ( household_allocator.get_free() )
          Household( s, lon, lat, container, &Global::Pop );
      }
      else if (place_type == SCHOOL) {
        place = new ( school_allocator.get_free() )
          School( s, lon, lat, container, &Global::Pop );
      }
      else if (place_type == WORKPLACE) {
        place = new ( workplace_allocator.get_free() )
          Workplace( s, lon, lat, container, &Global::Pop );
      }
      else if (place_type == HOSPITAL) {
        place = new ( hospital_allocator.get_free() )
          Hospital( s, lon, lat, container, &Global::Pop );
      }
      else {
        Utils::fred_abort("Help! bad place_type %c\n", place_type); 
      }
      
      if (place == NULL) {
        Utils::fred_abort( "Help! allocation failure for the %dth entry in location file (s=%s, type=%c)\n",
            i,s,place_type ); 
      }

      place = NULL;
  }

  // since everything was allocated in contiguous blocks, we can use pointer arithmetic
  // call to add_preallocated_places also ensures that all allocations were used for
  // successful additions to the place list
  add_preallocated_places< Household >( HOUSEHOLD, household_allocator ); 
  add_preallocated_places< School    >( SCHOOL,    school_allocator    ); 
  add_preallocated_places< Workplace >( WORKPLACE, workplace_allocator ); 
  add_preallocated_places< Hospital  >( HOSPITAL,  hospital_allocator  ); 

  // set the household income
  if (strcmp(Global::Synthetic_population_id,"none")!=0) {
    for (int i = 0; i < household_incomes.size(); i++) {
      Household * h = (Household *) get_place_from_label((char*) household_labels[i].c_str());
      h->set_household_income(household_incomes[i]);
      FRED_VERBOSE( 1, "INC: %s %c %f %f %d\n", h->get_label(), h->get_type(),
		    h->get_latitude(), h->get_longitude(), h->get_household_income());
    }
  }

  FRED_STATUS(0, "finished reading %d locations, now creating additional FRED locations\n", next_place_id);

  if (Global::Use_Mean_Latitude) {
    // Make projection based on the location file.
    fred::geo mean_lat = (min_lat + max_lat) / 2.0;
    Geo_Utils::set_km_per_degree(mean_lat);
    printf("min_lat: %f  max_lat: %f  mean_lat: %f\n", min_lat, max_lat, mean_lat);
  }
  else {
    // DEFAULT: Use mean US latitude (see Geo_Utils.cc)
  }

  // create geographical grids
  if (Global::Enable_Large_Grid) {
    Global::Large_Cells = new Large_Grid(min_lon, min_lat, max_lon, max_lat);
    // get coordinates of large grid as alinged to global grid
    min_lon = Global::Large_Cells->get_min_lon();
    min_lat = Global::Large_Cells->get_min_lat();
    max_lon = Global::Large_Cells->get_max_lon();
    max_lat = Global::Large_Cells->get_max_lat();
    // Initialize global seasonality object
    if (Global::Enable_Seasonality) {
      Global::Clim = new Seasonality(Global::Large_Cells);
    }
  }
  // Grid/Cells contain neighborhoods
  Global::Cells = new Grid(min_lon, min_lat, max_lon, max_lat);
  // one neighborhood per cell
  int number_of_neighborhoods = Global::Cells->get_number_of_cells();
  // create allocator for neighborhoods
  Place::Allocator< Neighborhood > neighborhood_allocator;
  // reserve enough space for all neighborhoods
  neighborhood_allocator.reserve( number_of_neighborhoods );
  FRED_STATUS( 0, "Allocated space for %7d neighborhoods\n", number_of_neighborhoods );
  // pass allocator to Grid::setup (which then passes to Cell::make_neighborhood)
  Global::Cells->setup( neighborhood_allocator );
  // add Neighborhoods in one contiguous block
  add_preallocated_places< Neighborhood >( NEIGHBORHOOD, neighborhood_allocator );

  if (Global::Enable_Small_Grid)
    Global::Small_Cells = new Small_Grid(min_lon, min_lat, max_lon, max_lat);

  int number_places = (int) places.size();
  for (int p = 0; p < number_places; p++) {
    if (places[p]->get_type() == HOUSEHOLD) {
      Place *place = places[p];
      fred::geo lat = place->get_latitude();
      fred::geo lon = place->get_longitude();
      Cell * grid_cell = (Cell *) Global::Cells->get_grid_cell_from_lat_lon(lat,lon);

      FRED_CONDITIONAL_VERBOSE( 1, grid_cell == NULL,
          "Help: household %d has bad grid_cell,  lat = %f  lon = %f\n",
          place->get_id(),lat,lon); 

      assert(grid_cell != NULL);

      grid_cell->add_household(place);
      place->set_grid_cell(grid_cell);
    }
  }

  FRED_STATUS(0, "read places finished: Places = %d\n", (int) places.size());

  // Added by Anuroop
  /*ofstream fplace("results/locations_3deme");
    for(int i=0; i<places.size(); i++) {
    fplace << places[i]->get_latitude() << " " << places[i]->get_longitude() << endl;
    }
    fplace.close();
    exit(0);*/
}

void Place_List::prepare() {

  FRED_STATUS(0, "prepare places entered\n","");

  int number_places = places.size();
  for (int p = 0; p < number_places; p++) {
    places[p]->prepare();
  }

  FRED_STATUS(0, "prepare places finished\n","");

}

void Place_List::update(int day) {

  FRED_STATUS(1, "update places entered\n","");

  if (Global::Enable_Seasonality) {
    Global::Clim->update(day);
  }
  int number_places = places.size();
  #pragma omp parallel for
  for ( int p = 0; p < number_places; ++p ) {
    places[ p ]->update( day );
  }

  FRED_STATUS(1, "update places finished\n", "");
}

Place * Place_List::get_place_from_label(char *s) {
  if (strcmp(s, "-1") == 0) return NULL;
  string str;
  str.assign(s);
  if (place_label_map.find(s) != place_label_map.end())
    return places[place_label_map[s]];
  else {
    FRED_VERBOSE(1, "Help!  can't find place with label = %s\n", s);
    return NULL;
  }
}

int Place_List::add_place( Place * p ) {
  int index = -1;
  // p->print(0);
  // assert(place_map.find(p->get_d()) == place_map.end());
  //
  FRED_CONDITIONAL_WARNING( p->get_id() != -1,
      "Place id (%d) was overwritten!", p->get_id() ); 
  assert( p->get_id() == -1 );

    string str;
    str.assign( p->get_label() );

    if ( place_label_map.find(str) == place_label_map.end() ) {
 
      p->set_id( get_new_place_id() );
     
      places.push_back(p);

      place_map[ p->get_id() ] = places.size() - 1;

      place_label_map[ str ] = places.size() - 1;


      // printf("places now = %d\n", (int)(places.size())); fflush(stdout);
     
      // TODO workplaces vector won't be needed once all places stored and labeled in bloque
      if (Global::Enable_Local_Workplace_Assignment && p->is_workplace()) {
        workplaces.push_back(p);
      }

    }
    else {
      printf("Warning: duplicate place label found: ");
      p->print(0);
    }

  return index;
}

int Place_List::get_number_of_places( char place_type ) {
  assert( place_type_counts.find( place_type ) !=
          place_type_counts.end() );
  return place_type_counts[ place_type ];
}

void Place_List::setup_classrooms() {

  FRED_STATUS(0, "setup classrooms entered\n","");

  int number_classrooms = 0;
  int number_places = places.size();
  int number_schools = 0;

  //#pragma omp parallel for reduction(+:number_classrooms)
  for (int p = 0; p < number_places; p++) {
    if (places[p]->get_type() == SCHOOL) {
      School * school = (School *) places[p];
      number_classrooms += school->get_number_of_rooms();
      ++( number_schools );
    }
  }

  Place::Allocator< Classroom > classroom_allocator;
  classroom_allocator.reserve( number_classrooms );

  FRED_STATUS( 0, "Allocating space for %d classrooms in %d schools (out of %d total places)\n",
      number_classrooms, number_schools, number_places );

  for (int p = 0; p < number_places; p++) {
    if (places[p]->get_type() == SCHOOL) {
      School * school = (School *) places[p];
      school->setup_classrooms( classroom_allocator );
    }
  }

  add_preallocated_places< Classroom >( CLASSROOM, classroom_allocator );

  FRED_STATUS(0, "setup classrooms finished\n","");
}

void Place_List::setup_offices() {

  FRED_STATUS(0, "setup offices entered\n","");

  int number_offices = 0;
  int number_places = places.size();
 
  #pragma omp parallel for reduction(+:number_offices)
  for (int p = 0; p < number_places; p++) {
    if (places[p]->get_type() == WORKPLACE) {
      Workplace * workplace = (Workplace *) places[p];
      number_offices += workplace->get_number_of_rooms();
    }
  }

  Place::Allocator< Office > office_allocator;
  office_allocator.reserve( number_offices );

  for (int p = 0; p < number_places; p++) {
    if (places[p]->get_type() == WORKPLACE) {
      Workplace * workplace = (Workplace *) places[p];
      workplace->setup_offices( office_allocator );
    }
  }
  // add offices in one contiguous block to Place_List
  add_preallocated_places< Office >( OFFICE, office_allocator );

  FRED_STATUS(0, "setup offices finished\n","");
}

Place * Place_List::get_random_workplace() {
  int size = workplaces.size();
  if (size > 0) {
    return workplaces[IRAND(0,size-1)];
  }
  else
    return NULL;
}

void Place_List::end_of_run() {
  if (Global::Verbose > 1) {
    int number_places = places.size();
    for (int p = 0; p < number_places; p++) {
      Place *place = places[p];
      fprintf(Global::Statusfp,"PLACE REPORT: id %d type %c size %d days_inf %d attack_rate %5.2f\n",
          place->get_id(), place->get_type(), place->get_size(),
          place->get_days_infectious(), 100.0*place->get_attack_rate());
    }
  }
}



