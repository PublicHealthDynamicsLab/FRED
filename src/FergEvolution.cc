/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include <map>
#include <cmath>
#include <fstream>
#include <cfloat>

#include "FergEvolution.h"
#include "MSEvolution.h"
#include "Random.h"
#include "Evolution.h"
#include "Infection.h"
#include "Trajectory.h"
#include "Global.h"
#include "IntraHost.h"
#include "Antiviral.h"
#include "Health.h"
#include "Person.h"
#include "Piecewise_Linear.h"
#include "Past_Infection.h"
#include "Transmission.h"
#include "Strain.h"
#include "Params.h"
#include "Utils.h"
#include "Population.h"
#include "Geo.h"
#include "Place.h"
#include "Regional_Layer.h"
#include "Regional_Patch.h"

using namespace std;

void FergEvolution :: setup( Disease * disease ) {

  MSEvolution::setup( disease );
  // codon translation table maps codon number to amino acid
  char numAA_file_name[ MAX_PARAM_SIZE ];
  char numAA_file_param[ MAX_PARAM_SIZE ] = "codon_translation_file";
  Params::get_param_from_string( numAA_file_param, numAA_file_name );
  Utils::get_fred_file_name( numAA_file_name );
  ifstream aafile;
  aafile.open( numAA_file_name );
  if ( !aafile.good() || strcmp( "none", numAA_file_name ) == 0 ) {
    Utils::fred_abort(
        "The file %s specified by parameter %s %s %s",
        "(required by FergEvolution::setup)",
        "is non-existent, corrupted, empty, or otherwise not 'good()'... ",
        numAA_file_name, numAA_file_param );
  }
  while( !aafile.eof() ){
    int n;
    aafile >> n;
    aafile >> aminoAcids[ n ];
  }
  aafile.close();
  // get mutation probability (nucleotide)
  Params::get_param((char *) "mutation_rate", &delta);
  Params::get_param((char *) "num_codons", &num_codons );
  num_ntides = num_codons * 3;
  // setup cdf for mutation probability
  mutation_cdf.clear();
  // pass mutation cdf in by reference
  Random::build_binomial_cdf( delta, num_ntides, mutation_cdf );

  for ( int n = 0; n < mutation_cdf.size(); ++n ) {
    std::cout << "mutation cdf " << mutation_cdf[ n ] << std::endl;
  }

  base_strain = 0;
  last_strain = base_strain;
  disease->add_root_strain( num_codons ); 
  reignition_threshold = 1;
}

void FergEvolution::initialize_reporting_grid( Regional_Layer * _grid ) {
  grid = _grid;
  num_clusters = grid->get_rows() * grid->get_cols();

  report = State< FergEvolution_Report >( fred::omp_get_max_threads() );
  for ( int t = 0; t < report.size(); ++t ) {
    report( t ).init( num_clusters );
  }
  report( 0 ).report_root_strain();
  weekly_report = NULL;

  initialize_reignitors( num_clusters );
}

void FergEvolution::merge_reports_by_parallel_reduction(
    FergEvolution_Report * merged_report ) {

  FRED_VERBOSE( 1, "Merging weekly reports by parallel reduction:\n", "" );

  std::vector< int > space( report.size() );
  
  for ( int i = 0; i < report.size(); ++i ) {
    space[ i ] = i;
  }

  #pragma omp parallel
  while ( space.size() > 1 ) {
    int even_size = space.size() - ( space.size() & 1 );
    #pragma omp single
    FRED_VERBOSE( 2, "%s %d %s\n",
        "There are", (int) space.size(),
        "reports remaining in the reduction space." )
    #pragma omp for ordered
    for ( int i = even_size - 1; i > 0; i -= 2 ) {
      FRED_VERBOSE( 2, "... Merging report %4d   into report %4d ...\n",
          space[ i ], space[ i - 1 ] );
      #pragma omp ordered
      {
        report( space[ i ] ).clear();
        space.erase( space.begin() + i );
      }
    }
  #pragma omp barrier
  }

  assert( space.size() == 1 );
  merged_report->merge( report( 0 ) );
  report( 0 ).clear();
  FRED_VERBOSE( 1, "%s %s\n", "Finished merging weekly reports.",
      "Merged report is ready for submission to database." );
}


FergEvolution::~FergEvolution() {
  clear_reignitors();
}

double FergEvolution::antigenic_distance( int strain1, int strain2 ) {
  if ( strain1 == strain2 ) {
    return 0;
  }
  double distance = 0.0;
  const Strain_Data & s1d = disease->get_strain_data( strain1 );
  const Strain_Data & s2d = disease->get_strain_data( strain2 );
  // TODO this assert fails some times, not sure how or why...
  //assert( s1d.size() == s2d.size() );
  for ( int i = 0; i < s1d.size() && i < s2d.size(); ++i ){  
    if ( aminoAcids[ s1d[ i ] ] != aminoAcids[ s2d[ i ] ] ) {
      distance += 1;
    }
  }
  return distance;
}

double FergEvolution::genetic_distance( int strain1, int strain2 ) {
  double distance = 0.0;
  int n = disease->get_num_strain_data_elements( strain1 );
  for ( int i = 0; i < n; ++i ) {
    int seq1 = disease->get_strain_data_element( strain1, i );
    int seq2 = disease->get_strain_data_element( strain2, i );
    if ( seq1 % 4 != seq2 % 4 ) distance += 1;
    seq1 /= 4;
    seq2 /= 4;
    if ( seq1 % 4 != seq2 % 4 ) {
      distance += 1;
    }
    seq1 /= 4;
    seq2 /= 4;
    if ( seq1 % 4 != seq2 % 4 ) {
      distance += 1;
    }
  }
  distance /= ( 3 * n );
  return distance;
}

void FergEvolution::get_infecting_and_infected_patches( Infection * infection,
    int & infector_patch, int & infectee_patch ) {

  Place * household = NULL;
  fred::geo lon, lat;
  // get infectee's patch (must exist)
  household = infection->get_host()->get_household();
  assert( household != NULL );
  lon = household->get_longitude();
  lat = household->get_latitude();
  infectee_patch = grid->get_patch( lat, lon )->get_id();
  // get infector's patch (if doesn't exist, use infectee's patch)
  if ( infection->get_infector() != NULL ) {
    household = infection->get_infector()->get_household();
    lon = household->get_longitude();
    lat = household->get_latitude();
    infector_patch = grid->get_patch( lat, lon )->get_id();
  }
  else {
    infector_patch = infectee_patch;
  }
}

void FergEvolution::try_to_mutate( Infection * infection, Transmission & transmission ) {

  int exp = infection->get_exposure_date();
  Trajectory * trajectory = infection->get_trajectory();
  int duration = trajectory->get_duration();
  vector< int > strains;
  infection->get_strains( strains );
  // only support single infections for now
  assert( strains.size() == 1 );
  // get the geographic cluster where this infection occurred
  int infector_patch_id, infectee_patch_id;
  get_infecting_and_infected_patches( infection, infector_patch_id, infectee_patch_id );
  bool mutated = false;

  for ( int d = 0; d < duration; d++ ){
    for ( int k = 0; k < strains.size(); ++k ) {
      int parent_strain_id = strains[ k ];
      int child_strain_id = parent_strain_id;

      mutated = false;

      // find out how many mutations we'll have
      int n_mutations = Random::draw_from_cdf_vector( mutation_cdf );
      // if zero, do nothing, otherwise pick which nucleotides will change
      // and report the mutation to the database
      if ( n_mutations > 0 ) {
        int mutated_nucleotides[ n_mutations ];
        if ( n_mutations == 1 ) {
          mutated_nucleotides[ 0 ] = Random::draw_random_int( 0, num_ntides - 1 );
        }
        else {
          Random::sample_range_without_replacement( num_ntides, n_mutations, mutated_nucleotides );
        }

        const Strain & parent_strain = disease->get_strain( parent_strain_id ); 
        const Strain_Data & parent_strain_data = disease->get_strain_data( parent_strain_id ); 
        // we know that we're going to have a mutation, so create the child strain.  If the mutation already
        // exists in the strain table we'll delete it later on
        Strain * new_strain = new Strain( parent_strain );
        Strain_Data & new_data = new_strain->get_data();

        for ( int i = 0; i < n_mutations; ++i ) {
          int aa_index = mutated_nucleotides[ i ] / 3;

          int nt_index_in_codon = 2 * ( mutated_nucleotides[ i ] % 3 );
          
          int bitmask = int( 3 ) << nt_index_in_codon; 
          new_data[ aa_index ] = 
            ( ( Random::draw_random_int(0,63) ^ new_data[ aa_index ] ) & bitmask ) | ( parent_strain_data[ aa_index ] & (~bitmask) );
        }
     
        bool novel = false;
        { // <--------------------------------------------------- Mutex for add strain
          fred::Scoped_Lock lock( mutex );
          child_strain_id = disease->add_strain( new_strain, disease->get_transmissibility( parent_strain_id ), parent_strain_id );
          if ( child_strain_id > last_strain ) {
            novel = true;
            last_strain = child_strain_id;
          }
          else {
            delete new_strain;
          }
          // update the trajectory
          trajectory->mutate( parent_strain_id, child_strain_id, d ); 
          //trajectory->mutate( parent_strain_id, child_strain_id, 0 ); // TODO wtf is this???
          FRED_VERBOSE( 1, "Evolution created a mutated strain! %5d   --> %5d %s\n",
              parent_strain_id, child_strain_id, novel ? " ...novel!" : "" );
        } // <--------------------------------------------------- Mutex for add strain
        // New strain created
        if ( novel ) {
          int pid = -1;
          int date = -1;
          
          if ( transmission.get_infected_place() ) {
            pid = transmission.get_infected_place()->get_id();
            date = transmission.get_exposure_date();
          }
          int infectee_id = infection->get_host()->get_id();
          int infector_id = -1;
          
          if ( transmission.get_infector()) {
            infector_id = transmission.get_infector()->get_id();
          }
        }
        // <----------------------- report mutation to database
        report().report_mutation( infectee_patch_id, child_strain_id, parent_strain_id, novel );
      }
      // <------------------------ report incidence to database (lockless, thread-safe)
      report().report_incidence( infector_patch_id, infectee_patch_id, child_strain_id ); 
      int exposed_duration = infection->get_infectious_date() - infection->get_exposure_date();
      int infectious_duration = infection->get_recovery_date() - infection->get_infectious_date();
      report().report_exposure( infectee_patch_id, child_strain_id, exposed_duration );
      report().report_prevalence( infectee_patch_id, child_strain_id, exposed_duration, infectious_duration );
      // update reignitor for this patch
      cache_reignitor( infectee_patch_id, child_strain_id );
    }
  }
}

void FergEvolution::add_failed_infection( Transmission & transmission, Person * infectee ) {
  Transmission::Loads * loads = transmission.get_initial_loads();
  Transmission::Loads::const_iterator it;
  for ( it = loads->begin(); it != loads->end(); ++it ) {
    int strain_id = it->first;
    int day = transmission.get_exposure_date();
    int recovery_date = day - 6;
    int age_at_exposure = infectee->get_age();
    infectee->add_past_infection( strain_id, recovery_date, age_at_exposure, disease );
  }
}

double FergEvolution::get_prob_taking( Person * infectee, int new_strain, double quantity, int day ) {
  return MSEvolution::get_prob_taking( infectee, new_strain, quantity, day );
}

Infection * FergEvolution::transmit( Infection * infection, Transmission & transmission, Person * infectee ) {
  infection = MSEvolution::transmit( infection, transmission, infectee );
  if ( infection == NULL ) {
    // if the infection failed, record as past_infection (stored in Health object of infectee)
    add_failed_infection( transmission, infectee );
  }
  else {
    // attempt to mutate this infection; this will also do reporting for this infection
    try_to_mutate( infection, transmission );
  }
  // return the (possibly mutated) infection to Health
  return infection;
}

void FergEvolution::update( int day ) {

  // report patch stats (naive/total population per patch) every 30th day
  if ( day % 30 == 0 ) {
    Global::Simulation_Region->report_grid_stats( day );
  }

  if ( report( 0 ).get_day() == FergEvolution_Report::window_size - 1 ) {
    weekly_report = new FergEvolution_Report( num_clusters );
    weekly_report->set_disease( disease );
    // all thread-local reports in the report State will be merged into
    // the weekly_report pointer given as argument; reports are automatically
    // cleared following following merge
    merge_reports_by_parallel_reduction( weekly_report );
    Global::db.enqueue_transaction( weekly_report );
  }
  else {
    for ( int t = 0; t < report.size(); ++t ) {
      report( t ).advance_day();
    }
  }

  // get total activity (prevalence from weekly_report).  If less than reignition threshold,
  // then reignite all patches.  THIS IS A PARALLEL METHOD
  int total_incidence = disease->get_epidemic()->get_incident_infections();
  if ( total_incidence < reignition_threshold ) {
    FRED_STATUS( 0, "%s Total incidence = %d, reignition threshold = %d\n",
      "Reignition required!", total_incidence, reignition_threshold );
    reignite_all_patches( day );
  }
}

bool FergEvolution::reignite( Person * pers, int strain_id, int day ) {

  fred::Scoped_Lock lock( reignition_mutex );

  int disease_id = disease->get_id();
  Infection * curr_inf = pers->get_health()->get_infection( disease_id );
  
  // If the person is currently infected, don't reignite
  if ( curr_inf != NULL ) {
    return false;
  }
  // otherwise, make sure that they're susceptible
  else {
    pers->become_susceptible( disease );
  }

  Transmission transmission = Transmission( NULL, NULL, day );
  // force the transmission to succeed
  transmission.set_forcing( true );
  transmission.set_initial_loads( Evolution::get_primary_loads( day, strain_id ) );
  pers->become_exposed( this->disease, transmission );
  return true;
}

Transmission::Loads * FergEvolution::get_primary_loads(int day) {
  int strain = 0;
  return Evolution::get_primary_loads(day, strain);
}

void FergEvolution::terminate_person( Person * p ) { }

void FergEvolution::initialize_reignitors( int num_patches ) {
  reignitors = State< Reignitors_Array >( fred::omp_get_max_threads() );
  for ( int t = 0; t < fred::omp_get_max_threads(); ++t ) {
    reignitors( t ) = new int [ num_patches ];
    for ( int c = 0; c < num_patches; ++c ) {
      reignitors( t )[ c ] = -1;
    }
  }
}

void FergEvolution::cache_reignitor( int patch, int strain_id ) {
  FRED_STATUS( 4, "%s Thread = %d, patch = %d, strain_id = %d\n"
      "Caching strain for later reignition.",
      fred::omp_get_thread_num(), patch, strain_id );
  reignitors()[ patch ] = strain_id;
}

void FergEvolution::clear_reignitors() {
  for ( int t = 0; t < fred::omp_get_max_threads(); ++t ) {
    delete[] reignitors( t );
  }
}

void FergEvolution::reignite_all_patches( int day ) {
  FRED_STATUS( 0, "Reiginiting all patches on day %d\n", day );
  double selection_probability = 1.0 / double( fred::omp_get_max_threads() );
  #pragma omp parallel for
  for ( int patch_id = 0; patch_id < num_clusters; ++patch_id ) {
    int strain = reignitors()[ patch_id ];
    if ( strain >= 0 && Random::draw_random() < selection_probability ) { 
      Regional_Patch * patch = Global::Simulation_Region->get_patch_from_id( patch_id );
      FRED_STATUS( 0, "Reigniting person in patch %d with strain %d\n",
          patch_id, reignitors()[ patch_id ] );
      Person * person = patch->select_random_person();
      reignite( person, strain, day );
    }
  }
}


void FergEvolution::init_prior_immunity( Disease * disease ) {
  int num_strains_added = 0;
  int num_hosts_added = 0;
  // Functor for to apply population immune history intialization in parallel
  Ferg_Evolution_Init_Past_Infections init_past_infection_functor( this );
  std::map< int, int > & strain_lookup = init_past_infection_functor.strain_lookup;
  typedef std::multimap< int, std::vector< Past_Infection > * > History_Type;
  History_Type & host_history = init_past_infection_functor.host_infection_histories;
  // <------------------------------------------------------------------------- read past strains
  char past_strains_file[ MAX_PARAM_SIZE ];
  char past_strains_param[ MAX_PARAM_SIZE ] = "past_infections_strains_file";
  Params::get_param_from_string( past_strains_param, past_strains_file );
  if ( strcmp( "none", past_strains_file ) ) {
    Utils::get_fred_file_name( past_strains_file );
    FRED_STATUS( 0, "Initializing prior exposure StrainTable using file: %s\n",
        past_strains_file ); 
    ifstream past_strains_stream;
    past_strains_stream.open( past_strains_file );
    if ( !( past_strains_stream.good() ) ) {
      Utils::fred_abort(
          "The file %s specified by parameter %s %s %s",
          "(required by FergEvolution::init_prior_immunity) is",
          "non-existent, corrupted, empty, or otherwise not 'good()'... ",
          past_strains_file, past_strains_param );
    }
    // first line of file gives the number of strains
    int num_past_strains;
    past_strains_stream >> num_past_strains;
    // subsequent lines give strain id, followed by the (integer) codons making up
    // the genome
    while ( !past_strains_stream.eof() && num_strains_added < num_past_strains ) {
      int past_strain_id, new_strain_id;
      past_strains_stream >> past_strain_id;
      // make new strain
      Strain * new_strain = new Strain( num_codons );
      Strain_Data & new_data = new_strain->get_data();
      for ( int c = 0; c < num_codons; ++c ) {
        int codon;
        past_strains_stream >> codon;
        new_data[ c ] = codon;
      }
      new_strain_id = disease->add_strain( new_strain, disease->get_transmissibility( 0 ) );
      // the past_strain_id is unique, but multiple past_strains may have the same
      // genotype and map to the same entry in the FRED StrainTable
      strain_lookup[ past_strain_id ] = new_strain_id;
      ++num_strains_added;
      FRED_STATUS( 1,  "Assigned id %d to past infection strain %d : %s\n",
          new_strain_id, past_strain_id, new_strain->to_string().c_str() );
    }
    past_strains_stream.close();
    if ( num_strains_added > 0 ) {
      FRED_STATUS( 0, "Added %d unique genotypes from file %s to the StrainTable\n",
        num_strains_added, past_strains_file );
    }
    else {
      FRED_WARNING( "Help! No past strains found in file %s\n",
        past_strains_file );
    }
  }
  // <------------------------------------------------------------------------- read host exposure history
  char past_hosts_file[ MAX_PARAM_SIZE ];
  char past_hosts_param[ MAX_PARAM_SIZE ] = "past_infections_hosts_file";
  Params::get_param_from_string( past_hosts_param, past_hosts_file );
  if ( strcmp( "none", past_hosts_file ) ) {
    Utils::get_fred_file_name( past_hosts_file );
    FRED_STATUS( 0,
        "Initializing population exposure and immune history using file: %s\n",
        past_hosts_file ); 
    ifstream past_hosts_stream;
    past_hosts_stream.open( past_hosts_file );
    if ( !( past_hosts_stream.good() ) ) {
      Utils::fred_abort(
          "The file %s specified by parameter %s %s %s",
          "(required by FergEvolution::init_prior_immunity) is",
          "non-existent, corrupted, empty, or otherwise not 'good()'... ",
          past_hosts_file, past_hosts_param );
    }
    // first line of file gives the number of hosts
    int num_past_hosts;
    past_hosts_stream >> num_past_hosts;
    //  for each host:
    //    line N: two numbers, host age in days and number of times the host has been infected
    //    line N+1+: one line for each time this host has been infected.
    //      two numbers:  ID of the strain that the host was infected with and
    //                    the age of the host in days when they were infected with that strain.
    //
    //  note: the host is still infected if the host's age is <= 6 days after the infection age.
    //  (The infection lasts 6 days, of which the first 2 are considered latent and the next 4
    //  are considered infectious. Anything after 6 days is recovered.)
    while ( !past_hosts_stream.eof() && num_hosts_added < num_past_hosts ) {
      unsigned int current_age, num_infections;
      past_hosts_stream >> current_age;
      past_hosts_stream >> num_infections;
      std::vector< Past_Infection > * past_infections = NULL;
      if ( num_infections > 0 ) {
        past_infections = new std::vector< Past_Infection >(); 
        for ( int i = 0; i < num_infections; ++i ) {
          int past_strain_id, strain_id, age_at_exposure, recovery_day;
          past_hosts_stream >> past_strain_id;
          past_hosts_stream >> age_at_exposure;
          recovery_day = 6 - ( current_age - age_at_exposure );
          if ( strain_lookup.find( past_strain_id ) != strain_lookup.end() ) {
            strain_id = strain_lookup[ past_strain_id ];
            past_infections->push_back( Past_Infection( strain_id, recovery_day, age_at_exposure ) );
          }
        }
        // bin current age when storing in host multimap (round up to next 2^6 days)
        init_past_infection_functor.set_bin_width( 127 );
        current_age = init_past_infection_functor.get_bin( current_age ); 
      }
      host_history.insert( std::make_pair( current_age, past_infections ) );  
      ++num_hosts_added;
    }
    past_hosts_stream.close();
    if ( num_hosts_added > 0 ) {
      FRED_STATUS( 0, "Successfully read %d past infection histories from file: %s\n",
        num_hosts_added, past_hosts_file );
    }
    else {
      FRED_WARNING( "Help! No past infection histories found in file: %s\n",
        past_hosts_file );
    }
  } 
  // <------------------------------------------------------------------------- apply immune histories to population 
  if ( num_hosts_added > 0 && num_strains_added > 0 ) {
    FRED_STATUS( 0, "%s using %d strains and %d host histories.\n",
        "Beginning immune history initialization",
        num_strains_added, num_hosts_added );
    Global::Pop.parallel_apply( init_past_infection_functor ); 
    init_past_infection_functor.finalize();
  }
  else {
    FRED_WARNING( "Help!%s %s strains: %d hosts: %d\n",
        "Failed to read both past strains and host histories;",
        "skipping population immune history initialization.",
        num_strains_added, num_hosts_added );
  }
}

void FergEvolution::Ferg_Evolution_Init_Past_Infections::operator() ( Person & person ) {
  int age = person.get_age_in_days();
  int binned_age = get_bin( age );
  Hist_Itr_Type hist_itr; 
  int num_in_bin = host_infection_histories.count( binned_age );
  if ( num_in_bin > 0 ) {
    hist_itr = host_infection_histories.lower_bound( binned_age );
    // randomly select one of the histories in the bin:
    int adv = Random::draw_random_int( 0, num_in_bin - 1 );
    //printf( "num_in_bin %d advance by %d\n", num_in_bin, adv );
    std::advance( hist_itr, adv );
    std::vector< Past_Infection > * history = (*hist_itr).second;
    if ( history != NULL ) {
      std::vector< Past_Infection >::iterator inf_itr = history->begin();
      for ( ; inf_itr != history->end(); ++inf_itr ) {
        int strain_id = (*inf_itr).get_strain();
        int recovery_date = (*inf_itr).get_recovery_date();
        int age_at_exp = (*inf_itr).get_age_at_exposure();
   
        if ( recovery_date > 0 ) {
          // re-use reignition mechanism to seed from past infection history 
          evo->reignite( &person, strain_id, 0 );
          #pragma omp atomic
          ++num_infections_seeded;
          FRED_VERBOSE( 1,
            "%s person_id %d age %d bin %d strain %d recovery %d age_at_exp %d\n",
            "Seeded infection from past history:", person.get_id(), age,
            binned_age, strain_id, recovery_date, age_at_exp );
        }
        else {
          if ( recovery_date > 0 ) {
            recovery_date = 0;
            age_at_exp = age;
          }
          // infection not active; store as past infection
          person.add_past_infection( strain_id, recovery_date, age_at_exp, disease );
          // if running in parallel, avoid printing or performace suffers
          FRED_VERBOSE( 4,
            "%s person_id %d age %d bin %d strain %d recovery %d age_at_exp %d\n",
            "Added past infection to:", person.get_id(), age, binned_age,
            strain_id, recovery_date, age_at_exp );
        }
      }
    }
  }
}

void FergEvolution::Ferg_Evolution_Init_Past_Infections::finalize() {
  FRED_STATUS( 0,
      "Initialization of population prior exposure history complete. %s %d %s\n",
      "Seeded", num_infections_seeded, "infections from past infection history" );
  // free memory
  Hist_Itr_Type hist_itr = host_infection_histories.begin();
  for ( ; hist_itr != host_infection_histories.end(); ++hist_itr ) {
    delete (*hist_itr).second;
  }
}
