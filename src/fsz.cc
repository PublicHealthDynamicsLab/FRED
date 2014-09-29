/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Compression.h"


int main( int argc, char *argv[] ) {

  int compress_flag = 0;
  int uncompress_flag = 0;

  opterr = 0;
  extern char * optarg;

  char f;

  char * filename;

  while ((f = getopt (argc, argv, "c:u:")) != -1) {
    switch (f) {
      case 'c':
        compress_flag = 1;
        filename = optarg;
        break;
      case 'u':
        uncompress_flag = 1;
        filename = optarg;
        break;
      case '?':
        std::cerr << "\nfsz, FRED's snappy compression utility.  Usage:\n\n";
        std::cerr << "  fsz -c <file> => file will be compressed using snappy and written to stdout\n";
        std::cerr << "  fsz -u <file> => file assumed to have been compressed with snappy; will be uncompressed to stdout\n\n";
        break;
      default:
        abort();
    }
  }

  if ( compress_flag && uncompress_flag ) {
    std::cerr << "Specify either [-c] or [-u], not both\n\n";
    exit(1);
  }
  else if ( !compress_flag && !uncompress_flag ) {
    std::cerr << "Specify either [-c] or [-u]\n\n";
    exit(1);
  }

  SnappyFileCompression compressor = SnappyFileCompression( filename );

  if ( compress_flag ) {
    compressor.compress_file_to_stdout();
  }
  else if ( uncompress_flag ) {
    compressor.uncompress_file_to_stdout();
  }

}
