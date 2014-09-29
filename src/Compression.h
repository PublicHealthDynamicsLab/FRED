/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Compression.h
//
// This file added to FRED by J. DePasse (jvd10@pitt.edu) in July 2012
//
//
#ifndef _FRED_COMPRESSION_H
#define _FRED_COMPRESSION_H

#include <cstring>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include <inttypes.h>
#include <vector>

#include <snappy.h>

#include "Global.h"

#define FSZ_WARNING(format, ...){\
printf( "WARNING: <%s, LINE:%d> " format, __FILE__, __LINE__, ## __VA_ARGS__ );\
}

#define FSZ_ABORT(format, ...){\
printf( "WARNING: <%s, LINE:%d> " format, __FILE__, __LINE__, ## __VA_ARGS__ );\
exit( 1 );\
}

#define FSZ_STATUS(verbosity, format, ...){\
printf( "STATUS: <%s, LINE:%d> " format, __FILE__, __LINE__, ## __VA_ARGS__ );\
}


class SnappyFileCompression {

  // try to include as close to default_block_size bytes (uncompressed)
  // in each compressed block
  static const size_t default_block_size = 1ul << 25;

  static const char* FSZ_MAGIC() {
    return "FSZ 20100404 v01";
  }

  static const int FSZ_MAGIC_LEN() {
    return std::strlen( FSZ_MAGIC() );
  }

  char * infile_name;
  
  char * begin;
  char * end;

  uint64_t total_uncompressed_bytes;
  uint64_t total_compressed_bytes;
  uint64_t total_header_bytes;

  fred::Mutex mutex;

  public:

  SnappyFileCompression( char * _infile_name ) {
    infile_name = new char[ std::strlen( _infile_name ) ];
    std::strcpy( infile_name, _infile_name );
  }

  /*
   * Compresses file supplied to constructor to stdout
   */
  void compress_file_to_stdout(); 

  /*
   * Uncompresses file supplied to constructor to stdout
   */
  void uncompress_file_to_stdout();

  void init_compressed_block_reader();

  bool check_magic_bytes();

  bool load_next_block_stream( std::stringstream & stream );

};






#endif
