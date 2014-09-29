/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_FERGEVOLUTION_H
#define _FRED_FERGEVOLUTION_H

#include <map>
#include <set>
#include <vector>
#include <string>
#include <fstream>

#include "Evolution.h"
#include "Transmission.h"
#include "MSEvolution.h"
#include "Age_Map.h"
#include "Global.h"
#include "Date.h"
#include "State.h"
#include "Utils.h"

#include "DB.h"
#include <sqlite3.h>

class Infection;
class AV_Health;
class Antiviral;
class Infection;
class Health;
class Disease;
class Trajectory;
class Piecewise_Linear;

/*
 * FergEvolution_Report derives from Transaction and implements methods from
 * its parent class that are used by DB to commit transactions to an sqlite3
 * database (asynchronously).
 *
 * Also implements a merge method used by the State class to combine the results
 * from several (e.g., one per thread) instances of the reporting class.  The
 * individual (lock-free) instances are created when State<> is instantiated
 * using the logging class as a template parameter.
 *
 * Though this seems like a lot of work, this system (as opposed to dumping
 * to a flat file) was chosen due to the unacceptable slowdown due to the locking
 * needed to keep the output from multiple threads from becoming hopelessly 
 * interleaved.  Additionally, the calculation of anitgenic & genetic distance
 * can be performed by the backgroud database thread (as part of the prepare()
 * method), allowing these values to be calculated during the run without any
 * performance penalty.
 *
 */
//template< int num_clusters >
struct FergEvolution_Report : public Transaction {

  static const int window_size = 10;

  // <----------------------------------------------------------------------- Helper Structs
  struct Parent_Child {
    int parent, child;
    Parent_Child( int _parent, int _child ) : parent( _parent ), child( _child ) { }
    bool operator< ( const Parent_Child & other ) const {
      if ( parent < other.parent ) {
        return true;
      }
      else if ( parent == other.parent ) {
        return child < other.child;
      }
      else {
        return false;
      }
    }
  };
  // Daily: stores data for each of the past seven days
  class Daily {

    typedef int Window[ window_size ];
    Window * incidence;
    Window * mutated;
    int n;

    public:

    Daily() : incidence( NULL ), mutated( NULL ) { }
    void init( int N ) {
      n = N;
      incidence = new Window[ N * N ];
      mutated = new Window[ N ];
      for ( int c = 0; c < N; ++c ) {
        for ( int d = 0; d < window_size; ++d ) {
          for ( int cc = 0; cc < N; ++cc ) {
            incidence[ c + cc ][ d ] = 0;
          }
          mutated[ c ][ d ] = 0;
        }
      }
    }
    ~Daily() {
      if ( incidence ) delete[] incidence;
      if ( mutated ) delete[] mutated;
    }
    void add_incidence( int infector_patch, int infectee_patch, int day, int increment ) {
      incidence[ ( n * infectee_patch ) + infector_patch ][ day ] += increment;
    }
    int get_incidence( int infector_patch, int infectee_patch, int day ) const {
      return incidence[ ( n * infectee_patch ) + infector_patch ][ day ];
    }
    int get_incidence( int incident_patch, int day ) const {
      int patch_incidence = 0;
      for ( int c = 0; c < n; ++c ) {
        patch_incidence += incidence[ ( n * incident_patch ) + c ][ day ];
      }
      return patch_incidence;
    }
    void add_mutated( int patch, int day, int increment ) {
      mutated[ patch ][ day ] += increment;
    }
    int get_mutated( int patch, int day ) const {
      return mutated[ patch ][ day ];
    }
  };
  struct Extended_Daily {
    typedef int Window[ window_size ];
    Window * exposed;
    Window * prevalence;
    Window * next_exposed;
    Window * next_prevalence;
    int num_clusters;

    Extended_Daily() : exposed( NULL ), prevalence( NULL ), next_exposed( NULL ), next_prevalence( NULL ) { }

    void init( int N ) {
      num_clusters = N;
      exposed = new Window[ N ];
      prevalence = new Window[ N ];
      next_exposed = new Window[ N ];
      next_prevalence = new Window[ N ];
      for ( int c = 0; c < N; ++c ) {
        for ( int d = 0; d < window_size; ++d ) {
          exposed[ c ][ d ] = 0;
          prevalence[ c ][ d ] = 0;
          next_exposed[ c ][ d ] = 0;
          next_prevalence[ c ][ d ] = 0;
        }
      }
    }
    bool shift_window( int N ) {
      if ( exposed ) delete[] exposed;
      exposed = next_exposed;
      next_exposed = new Window[ N ];
      if ( prevalence ) delete[] prevalence;
      prevalence = next_prevalence;
      next_prevalence = new Window[ N ];
      bool in_use = false;
      for ( int c = 0; c < N; ++c ) {
        for ( int d = 0; d < window_size; ++d ) {
          next_exposed[ c ][ d ] = 0;
          next_prevalence[ c ][ d ] = 0;
          if ( exposed[ c ][ d ] > 0 || prevalence[ c ][ d ] > 0 ) {
            in_use = true;
          }
        }
      }
      return in_use;
    }

    void add( Window * window, int patch, int offset, int length ) {
      for ( int d = offset; d < offset + length; ++d ) {
        assert( patch < num_clusters );
        assert( d < window_size );
        ++( window[ patch ][ d ] );
      }
    }

    void add_exposed( int patch, int offset, int length ) {
      if ( offset < window_size ) {
        add( exposed, patch, offset, offset + length < window_size ? length : window_size - offset );
      }
      if ( offset + length >= window_size ) {
        add( next_exposed, patch, 0, ( 2 * window_size ) - ( offset + length ) );
      }
    }

    void add_prevalence( int patch, int offset, int length ) {
      if ( offset < window_size ) {
        add( prevalence, patch, offset, offset + length < window_size ? length : window_size - offset );
      }
      if ( offset + length >= window_size ) {
        add( next_prevalence, patch, 0, ( 2 * window_size ) - ( offset + length ) );
      }
    }


    ~Extended_Daily() {
      if ( exposed ) delete[] exposed;
      if ( prevalence ) delete[] prevalence;
      if ( next_exposed ) delete[] next_exposed;
      if ( next_prevalence ) delete[] next_prevalence;
    }
  };
  // <----------------------------------------------------------------------- Member Variables
  // the disease provides access to the Strain_Table for these strains
  Disease * disease;
  // Daily values stored in a map, keyed by strain id
  typedef std::map< int, Daily > Daily_Strain_Map; 
  Daily_Strain_Map daily_strain_map;
  // Extended Daily values (exposed, prevalence) stored in a map, keyed by strain id
  // The extended daily values are calculated from the projected trajectory
  typedef std::map< int, Extended_Daily > Extended_Daily_Strain_Map; 
  Extended_Daily_Strain_Map extended_daily_strain_map;
  // mutations
  typedef std::set< Parent_Child > Novel_Mutations;
  typedef Novel_Mutations Novel_Mutations_Array[ window_size ];
  Novel_Mutations_Array * daily_novel_mutations;
  // date of submission (i.e., the end of the week)
  int days[ window_size ];
  int days_index;
  typedef std::string Date_Array_Type;
  int dates_array[ window_size ];
  // number of patches
  int num_clusters;
  // sum of prevalence for all patches
  int total_activity;
  // <----------------------------------------------------------------------- Constructor
  FergEvolution_Report() {
    num_clusters = 0;
    daily_novel_mutations = NULL;
  }
  FergEvolution_Report( int _num_clusters ) {
    init( _num_clusters );
  }
  // <----------------------------------------------------------------------- Init method
  void init( int _num_clusters ) {
    num_clusters = _num_clusters;
    daily_novel_mutations = new Novel_Mutations_Array[ num_clusters ];
    clear();
  }
  // <----------------------------------------------------------------------- Destructor
  ~FergEvolution_Report() {
    if ( daily_novel_mutations ) delete[] daily_novel_mutations;
    //daily_strain_map.clear();
    //extended_daily_strain_map.clear();
  }
  // <----------------------------------------------------------------------- Public Methods
  void set_disease( Disease * _disease ) {
    disease = _disease;
  }
  int get_total_activity() { return total_activity; }
  // Merge the contents of supplied FergEvolution_Report with self
  void merge( const FergEvolution_Report & other ) {
    // iterator over the daily and weekly objects for each strain in the state to 
    // be merged with this one
    Daily_Strain_Map::const_iterator iter = other.daily_strain_map.begin();
    // iterate over each state and merge its daily and weekly subtotals
    while ( iter != other.daily_strain_map.end() ) {
      int strain = (*iter).first;
      const Daily & daily = (*iter).second;
      // if this strain isn't in the daily map, create entries for it
      if ( daily_strain_map.find( strain ) == daily_strain_map.end() ) {
        daily_strain_map[ strain ].init( num_clusters );
      }
 
      Extended_Daily_Strain_Map::const_iterator other_extended_daily_iter = other.extended_daily_strain_map.find( strain );
      if ( other_extended_daily_iter != other.extended_daily_strain_map.end() ) {
        if ( extended_daily_strain_map.find( strain ) == extended_daily_strain_map.end() ) {
          extended_daily_strain_map[ strain ].init( num_clusters );
        }
      }
     
      // loop over geographic clusters
      for ( int c = 0; c < num_clusters; ++c ) {
        // loop over recorded days
        for ( int d = 0; d <= other.days_index; ++d ) {
          assert( days[ d ] == -1 || days[ d ] == other.days[ d ] );
          days[ d ] = other.days[ d ];
          for ( int cc = 0; cc < num_clusters; ++cc ) {
            daily_strain_map[ strain ].add_incidence( c, cc, d, daily.get_incidence( c, cc, d ) );
          }
          daily_strain_map[ strain ].add_mutated( c, d, daily.get_mutated( c, d ) );
 
          if ( other_extended_daily_iter != other.extended_daily_strain_map.end() ) {
            extended_daily_strain_map[ strain ].exposed[ c ][ d ] += other_extended_daily_iter->second.exposed[ c ][ d ];
            extended_daily_strain_map[ strain ].prevalence[ c ][ d ] += other_extended_daily_iter->second.prevalence[ c ][ d ];
            total_activity += other_extended_daily_iter->second.prevalence[ c ][ d ];
          }

          daily_novel_mutations[ c ][ d ].insert(
              other.daily_novel_mutations[ c ][ d ].begin(),
              other.daily_novel_mutations[ c ][ d ].end() );
          
          dates_array[ d ] = other.dates_array[ d ];
        }
      }
      ++iter;
    }
    days_index = days_index < other.days_index ? other.days_index : days_index;
  }
  // Clear daily and weekly maps, reset days and days_index
  void clear() {
    total_activity = 0;
    daily_strain_map.clear();
    // shift the window for extended daily records
    Extended_Daily_Strain_Map::iterator iter = extended_daily_strain_map.begin();
    while ( iter != extended_daily_strain_map.end() ) {
      // shift_window returns true only if there's data in the next window
      // if empty, erase the strain
      if ( !( (*iter).second.shift_window( num_clusters ) ) ) {
        extended_daily_strain_map.erase( iter++ );
      }
      else {
        ++iter;
      }
    }
    days_index = 0;
    // configure date indexer and load the date strings for the next seven days
    Date * date_object = Global::Sim_Current_Date->clone();
    int sim_day = Date::days_between( Global::Sim_Start_Date, date_object );
    delete date_object;
    for ( int day = 0; day < window_size; ++day ) {
      days[ day ] = -1;
      dates_array[ day ] = sim_day;
      for ( int c = 0; c < num_clusters; ++c ) {
        daily_novel_mutations[ c ][ day ].clear();
      }
      ++sim_day;
    }
  }
  // Advance day
  void advance_day() {
    ++days_index;
    assert( days_index > 0 && days_index < window_size );
  }
  // Get current day
  int get_day() {
    return days_index;
  }
  // Report initial strain
  void report_root_strain() {
    int root_strain = 0;
    if ( daily_strain_map.find( root_strain ) == daily_strain_map.end() ) {
      daily_strain_map[ root_strain ].init( num_clusters );

      daily_novel_mutations[ 0 ][ 0 ].insert(
          Parent_Child( root_strain, root_strain ) );
    }
  }
  // Report case
  void report_incidence( int infector_patch, int infectee_patch, int strain ) {
    // if this strain isn't in the daily map, create entries for it
    if ( daily_strain_map.find( strain ) == daily_strain_map.end() ) {
      daily_strain_map[ strain ].init( num_clusters );
    }
    daily_strain_map[ strain ].add_incidence( infector_patch, infectee_patch, days_index, 1 ); 
  }
  // Report mutated strain
  void report_mutation( int cluster, int child_strain, int parent_strain, bool novel ) {
    // if this strain isn't in the daily map, create entries for it
    if ( daily_strain_map.find( child_strain ) == daily_strain_map.end() ) {
      daily_strain_map[ child_strain ].init( num_clusters );
    }
    daily_strain_map[ child_strain ].add_mutated( cluster, days_index, 1 ); 
    // report novel mutations
    if ( novel ) {
      daily_novel_mutations[ cluster ][ days_index ].insert(
          Parent_Child( parent_strain, child_strain ) ); 
    }
  }
  // Report exposure (extended daily)
  void report_exposure( int cluster, int strain, int duration ) {
    // if this strain isn't in the extended daily map, create entries for it
    if ( extended_daily_strain_map.find( strain ) == extended_daily_strain_map.end() ) {
      extended_daily_strain_map[ strain ].init( num_clusters );
    }
    extended_daily_strain_map[ strain ].add_exposed( cluster, days_index, duration );
  }
  // Report prevalence (extended daily)
  void report_prevalence( int cluster, int strain, int offset, int duration ) {
    // if this strain isn't in the extended daily map, create entries for it
    if ( extended_daily_strain_map.find( strain ) == extended_daily_strain_map.end() ) {
      extended_daily_strain_map[ strain ].init( num_clusters );
    }
    extended_daily_strain_map[ strain ].add_prevalence( cluster, days_index + offset, duration );
  }

  // <------------------------------------------------------------------------- Implementation of pure virtual interface method
  //                                                                            for Transaction class
  const char * initialize( int statement_number ) {
    if ( statement_number < n_stmts ) {
      if ( statement_number == 0 ) {
        return "create table if not exists strain_incidence \
            ( day integer, patch integer, strain integer, incidence integer );";
      }
      if ( statement_number == 1 ) {
        return "create table if not exists novel_mutations \
            ( day integer, patch integer, parent_strain integer, child_strain integer );";
      }
      if ( statement_number == 2 ) {
        return "create table if not exists strains \
            ( strain integer, genotype text );";
      }
      if ( statement_number == 3 ) {
        return "create table if not exists strain_prevalence \
            ( day integer, patch integer, strain integer, prevalence integer );";
      }
      if ( statement_number == 4 ) {
        return "create table if not exists strain_exposed \
            ( day integer, patch integer, strain integer, exposed integer );";
      }
    }
    else {
      Utils::fred_abort( "Index of requested statement is out of range!", "" );
    }
    
    return NULL;
  }

  void prepare() {
    // statement number; reused for each statement's creation
    int S = 0;
    // define the statement and value variables
    n_stmts = 5;
    // new array of strings to hold statements
    statements = new std::string[ n_stmts ];
    // array giving the number of values expected for each statement
    n_values = new int[ n_stmts ];
    // array of vectors of string arrays  
    values = new std::vector< string * >[ n_stmts ];
    // <------------------------------------------------------------------------------------- prepare first statement
    S = 0;
    statements[ S ] = std::string( "insert into strain_incidence values ($DAY,$CELL,$STRAIN,$INCIDENCE);" );
    n_values[ S ] = 4;
    // store values for first statement ( bound to statement during execute() )
    Daily_Strain_Map::const_iterator daily_iter = daily_strain_map.begin();
    while ( daily_iter != daily_strain_map.end() ) {
      int strain = (*daily_iter).first;
      const Daily & daily = (*daily_iter).second;
      for ( int c = 0; c < num_clusters; ++c ) {
        for ( int d = 0; d < window_size; ++d ) {
          if ( daily.get_incidence( c, d ) > 0 ) {
            std::string * values_array = new std::string[ n_values[ S ] ]; 
            std::stringstream ss;
            ss << dates_array[ d ];
            values_array[ 0 ] = ss.str(); ss.str("");
            ss << c;
            values_array[ 1 ] = ss.str(); ss.str("");
            ss << strain;
            values_array[ 2 ] = ss.str(); ss.str("");
            ss << daily.get_incidence( c, d ); 
            values_array[ 3 ] = ss.str(); ss.str("");
            values[ S ].push_back( values_array );
          }
        }
      }
      ++daily_iter;
    }

    // <------------------------------------------------------------------------------------- prepare second statement
    S = 1;
    statements[ S ] = std::string( "insert into novel_mutations values ($DAY,$CELL,$PARENT_STRAIN,$CHILD_STRAIN);" );
    n_values[ S ] = 4;
    for ( int c = 0; c < num_clusters; ++c ) {
      for ( int d = 0; d < window_size; ++d ) {    
        Novel_Mutations::const_iterator daily_novel_iter = daily_novel_mutations[ c ][ d ].begin();
        while ( daily_novel_iter != daily_novel_mutations[ c ][ d ].end() ) {
          std::string * values_array = new std::string[ n_values[ S ] ]; 
          std::stringstream ss;
          ss << dates_array[ d ];
          values_array[ 0 ] = ss.str(); ss.str("");
          ss << c;
          values_array[ 1 ] = ss.str(); ss.str("");
          ss << (*daily_novel_iter).parent;
          values_array[ 2 ] = ss.str(); ss.str("");
          ss << (*daily_novel_iter).child;
          values_array[ 3 ] = ss.str(); ss.str("");
          values[ S ].push_back( values_array );
          ++daily_novel_iter;
        }
      }
    }
    // <------------------------------------------------------------------------------------- prepare third statement
    S = 2;
    statements[ S ] = std::string( "insert into strains values ($STRAIN,$GENOTYPE);" );
    n_values[ S ] = 2;
    for ( int c = 0; c < num_clusters; ++c ) {
      for ( int d = 0; d < window_size; ++d ) {    
        Novel_Mutations::const_iterator daily_novel_iter = daily_novel_mutations[ c ][ d ].begin();
        while ( daily_novel_iter != daily_novel_mutations[ c ][ d ].end() ) {
          std::string * values_array = new std::string[ n_values[ S ] ]; 
          std::stringstream ss;
          ss <<  (*daily_novel_iter).child;
          values_array[ 0 ] = ss.str(); ss.str("");
          values_array[ 1 ] = disease->get_strain_data_string( (*daily_novel_iter).child );
          values[ S ].push_back( values_array );
          ++daily_novel_iter;
        }
      }
    }

    // <------------------------------------------------------------------------------------- prepare statement
    S = 3;
    statements[ S ] = std::string( "insert into strain_prevalence values ($DAY,$CELL,$STRAIN,$PREVALENCE);" );
    n_values[ S ] = 4;
    // store values for first statement ( bound to statement during execute() )
    Extended_Daily_Strain_Map::const_iterator ext_daily_iter = extended_daily_strain_map.begin();
    while ( ext_daily_iter != extended_daily_strain_map.end() ) {
      int strain = (*ext_daily_iter).first;
      const Extended_Daily & ext_daily = (*ext_daily_iter).second;
      for ( int c = 0; c < num_clusters; ++c ) {
        for ( int d = 0; d < window_size; ++d ) {
          if ( ext_daily.prevalence[ c ][ d ] > 0 ) {
            std::string * values_array = new std::string[ n_values[ S ] ]; 
            std::stringstream ss;
            ss << dates_array[ d ];
            values_array[ 0 ] = ss.str(); ss.str("");
            ss << c;
            values_array[ 1 ] = ss.str(); ss.str("");
            ss << strain;
            values_array[ 2 ] = ss.str(); ss.str("");
            ss << ext_daily.prevalence[ c ][ d ]; 
            values_array[ 3 ] = ss.str(); ss.str("");
            values[ S ].push_back( values_array );
          }
        }
      }
      ++ext_daily_iter;
    }

    // <------------------------------------------------------------------------------------- prepare statement
    S = 4;
    statements[ S ] = std::string( "insert into strain_exposed values ($DAY,$CELL,$STRAIN,$EXPOSED);" );
    n_values[ S ] = 4;
    // store values for first statement ( bound to statement during execute() )
    ext_daily_iter = extended_daily_strain_map.begin();
    while ( ext_daily_iter != extended_daily_strain_map.end() ) {
      int strain = (*ext_daily_iter).first;
      const Extended_Daily & ext_daily = (*ext_daily_iter).second;
      for ( int c = 0; c < num_clusters; ++c ) {
        for ( int d = 0; d < window_size; ++d ) {
          if ( ext_daily.exposed[ c ][ d ] > 0 ) {
            std::string * values_array = new std::string[ n_values[ S ] ]; 
            std::stringstream ss;
            ss << dates_array[ d ];
            values_array[ 0 ] = ss.str(); ss.str("");
            ss << c;
            values_array[ 1 ] = ss.str(); ss.str("");
            ss << strain;
            values_array[ 2 ] = ss.str(); ss.str("");
            ss << ext_daily.exposed[ c ][ d ]; 
            values_array[ 3 ] = ss.str(); ss.str("");
            values[ S ].push_back( values_array );
          }
        }
      }
      ++ext_daily_iter;
    }

    // clear the transaction
    clear();
  }
};



/* ****************************************************************************
 *
 * FergEvolution, derived Evolution class
 *
 * ****************************************************************************
 */

using namespace std;

class FergEvolution : public MSEvolution {

 public:

  virtual ~FergEvolution();

  void initialize_reporting_grid( Regional_Layer * grid );
  void setup( Disease * disease );
  void update( int day );
 
  Infection * transmit( Infection * infection, Transmission & transmission, Person * infectee );
  
  void try_to_mutate( Infection * infection, Transmission & transmission );
  
  double get_prob_taking(Person *infectee, int new_strain, double quantity, int day);
  
  void add_failed_infection( Transmission & transmission, Person * infectee );
  
  void get_infecting_and_infected_patches( Infection * infection,
    int & infector_patch, int & infectee_patch );

 
  double antigenic_distance( int strain1, int strain2 );
  double genetic_distance( int strain1, int strain2 );

  Transmission::Loads * get_primary_loads( int day );
  
  int create_strain(int start_strain, int gen_dist, int day);
  
  void terminate_person(Person *p);

  void init_prior_immunity( Disease * disease );

 private:
  
  bool reignite( Person * person, int strain_id, int day);
  
  fred::Mutex mutex;

  Regional_Layer * grid;
  
  int reignition_threshold;
  void initialize_reignitors( int number_patches );
  void cache_reignitor( int patch, int strain_id );
  void clear_reignitors();
  void reignite_all_patches( int day );
  fred::Mutex reignition_mutex;

  struct Ferg_Evolution_Init_Past_Infections {
    typedef std::multimap<int,std::vector< Past_Infection > *>::iterator Hist_Itr_Type;
    int num_infections_seeded;
    unsigned int bin_width;
    std::map< int, int > strain_lookup;
    std::multimap< int, std::vector< Past_Infection > * > host_infection_histories;
    Disease * disease;
    FergEvolution * evo;
    Ferg_Evolution_Init_Past_Infections( FergEvolution * _evo ) : evo( _evo ) {
      disease = evo->get_disease();
      bin_width = 127;
      num_infections_seeded = 0;
    }
    void operator() ( Person & person );
    void set_bin_width( unsigned int _bin_width ) { bin_width = _bin_width; }
    int get_bin( int val ) { return ( val | bin_width ); }
    void finalize();
  };

 private:

  void merge_reports_by_parallel_reduction( FergEvolution_Report * merged_report );

 protected:
  
  int aminoAcids[ 64 ];
  int num_codons;
  
  double delta;
  std::vector< double > mutation_cdf;
  int base_strain;
  int last_strain;
  int num_ntides;
  
  State< FergEvolution_Report > report;
  FergEvolution_Report * weekly_report;

  // thread-safe state array of Person pointers per patch 
  typedef int * Reignitors_Array;
  State< Reignitors_Array > reignitors;

  int num_clusters;
};

#endif

