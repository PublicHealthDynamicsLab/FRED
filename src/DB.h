/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
// File: DB.h
//

#ifndef _FRED_DB_H
#define _FRED_DB_H

/*
 * Thread-safe database logging classes.
 *
 * Allows asynchronous logging of both individual events as well as periodic summaries.
 * 
 * Submissions to the logging system queue may be made asynchronously.  A background thread will
 * automatically commit queued transactions to an sqlite3 database.
 *
 * The logging system must be flushed to release memory from the internal buffers.
 *
 * J. DePasse <jvd10@pitt.edu> 
 *
 * 2012 08 15
 *
 */

#include <string>
#include <vector>
#include <deque>
#include <stdio.h>

#include "Global.h"

#include <sqlite3.h>

struct Transaction {

  // <------------------------------------------------------------------------- Constructor
  Transaction() { }

  // <------------------------------------------------------------------------- Destructor
  virtual ~Transaction() {
    /*
    if ( statements ) { delete[] statements; }
    for ( int s = 0; s < n_stmts; ++s ) {
      std::vector< std::string * >::iterator iter = values[ s ].begin();
      for ( ; iter != values[ s ].end(); ++iter ) {
        delete[] (*iter);
        (*iter) = NULL;
      }
    }
    if ( values ) { delete[] values; }
    if ( n_values ) { delete[] n_values; }
    */
  }
   
  // <------------------------------------------------------------------------- Member variables 
  int n_stmts;
  std::string * statements;
  // n_values is an array listing the number of elements in each values array
  int * n_values;
  // vector of arrays of strings (each array of n_values[ statemen_number ] length)
  std::vector< std::string * > * values;

  // <------------------------------------------------------------------------- Pure virtual interface method
  //                                                                            must be implemented in derived class
  virtual const char * initialize( int statement_index ) = 0;                                                                            
  virtual void prepare() = 0;
  
  // <------------------------------------------------------------------------- Methods to get statement and value strings 
  //                                                                            
  int get_n_stmts() { return n_stmts; }

  std::string * get_statements() { return statements; }

  int * get_n_values() { return n_values; }

  std::vector< std::string * > * get_values() { return values; }

};

class DB {

  fred::Mutex mutex;

  sqlite3 * db;
  typedef std::deque< Transaction * > TransactionQueue;
  TransactionQueue transactions;
  static const char * delimiter() { return ", "; }
  bool run_asynchronously;
  bool wait;

  public:

  DB() {
    db = NULL;
    run_asynchronously = false;
    wait = false;
  }

  void open_database( const char * dbfilename ) {
    char * sErrMsg = 0;
    remove( dbfilename );
    sqlite3_open( dbfilename, &db );
    sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, &sErrMsg);
    sqlite3_exec(db, "PRAGMA journal_mode = MEMORY", NULL, NULL, &sErrMsg);
  }

  void close_database() {
    sqlite3_close( db );
  }

  void enqueue_transaction( Transaction * t ) {
    fred::Scoped_Lock lock( mutex );
    transactions.push_back( t );
  }

  void process_transactions() {
    fred::Scoped_Lock lock( mutex );
    TransactionQueue::iterator iter = transactions.begin();
    for ( ; iter != transactions.end(); ++iter ) {
      (*iter)->prepare();
      execute( *iter );
      delete *iter;
    }
    transactions.clear();
  }

  void finalize_transactions() {
    #pragma omp atomic
    run_asynchronously &= false;
    #pragma omp flush (run_asynchronously)
  
    #pragma omp atomic
    wait &= false;
    #pragma omp flush (wait)
  }

  void pause_background_processing() {
    #pragma omp atomic
    wait |= true;
    #pragma omp flush (wait)
  }

  void resume_background_processing() {
    #pragma omp atomic
    wait &= false;
    #pragma omp flush (wait)
  }

  void process_transactions_asynchronously() {

    #pragma omp atomic
    run_asynchronously |= true;
    #pragma omp flush(run_asynchronously)

    int completed = 0;
    int batch_size = 10;

    while ( run_asynchronously ) {
      if ( transactions.empty() ) {
        usleep( 1 );
      }
      else {
        #pragma omp flush (wait)
        if ( wait ) {
          continue;
        }
        else {
          while ( run_asynchronously && completed < ( transactions.size() - 2 ) && completed < batch_size ) {
            transactions[ completed ]->prepare();
            execute( transactions[ completed ] );
            ++completed;
            #pragma omp flush (run_asynchronously)
          }
          if ( completed > 0 ) {
            for ( int i = 0; i < completed; ++i ) {
              delete transactions[ i ];
            }
            transactions.erase( transactions.begin(), transactions.begin() + completed );
            completed = 0;
          }
        }
      }
      #pragma omp flush (run_asynchronously)
    }
    process_transactions();
  }


  void execute( Transaction * trans ) {
  
    for ( int n = 0; n < trans->get_n_stmts(); ++n ) {
      sqlite3_stmt * init_stmt;
      sqlite3_prepare_v2( db, trans->initialize( n ), -1, &init_stmt, NULL );
      sqlite3_step( init_stmt );
      sqlite3_finalize( init_stmt );
    }

    int n_stmts = trans->get_n_stmts();
    std::string * statements = trans->get_statements();
    int * n_values = trans->get_n_values();
    std::vector< std::string * > * values = trans->get_values();

    for ( int n = 0; n < n_stmts; ++n ) {
      sqlite3_stmt * stmt;
      sqlite3_prepare_v2( db, statements[ n ].c_str(), -1, &stmt, NULL );
      //std::cerr << statements[ n ] << std::endl;

      std::vector< std::string * >::iterator iter = values[ n ].begin();
      for ( ; iter != values[ n ].end(); ++iter ) {
        sqlite3_reset( stmt );
        for ( int v = 0; v < n_values[ n ]; ++v ) {
          //std::cerr << (*iter)[ v ];
          if ( v != n_values[ n ] - 1 ) {
            //std::cerr << delimiter();
          }
          sqlite3_bind_text( stmt, v + 1, (*iter)[ v ].c_str(), (*iter)[ v ].size(), SQLITE_STATIC );
        }
        //std::cerr << std::endl;
        sqlite3_step( stmt );
      }
      sqlite3_finalize( stmt );
    }
  }

  void print( Transaction * trans ) {
  }

};

#endif



























