/*
  This file is part of the FRED system.

  Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include <string>

#include "Compression.h"


void SnappyFileCompression::compress_file_to_stdout() {
  using namespace std;
  // open the uncompressed input file
  FILE * fp = fopen( infile_name, "r" );
  // seek to end and find input file size
  fseek( fp, 0, SEEK_END );
  uint64_t infile_size = ftell( fp );
  // re-set to 0 and close input file
  fseek( fp, 0, SEEK_SET );
  fclose( fp );
  // memory map the file as a char array
  int fd = open( infile_name, O_RDONLY );
  char * map = (char *) mmap( NULL, infile_size,
      PROT_READ, MAP_PRIVATE, fd, 0 );
  // layout will be:
  // [ size compressed_size_0 ][ char * compressed_data_0 ]...
  // target uncompressed size will be default_block_size bytes
  // but to ease parsing, ensure that blocks end with newline...
  size_t target_block_size =
    default_block_size > infile_size ? infile_size : default_block_size;
  // pointers into the mmapped file
  begin = map;
  end = begin;
  end += target_block_size - 1;
  total_uncompressed_bytes = 0;
  total_compressed_bytes = 0;
  total_header_bytes = 0;
  // write magic bytes:
  printf( "%s", FSZ_MAGIC() ); 
  // run through the mapped file and compress approximately default_block_size
  // bytes into blocks that are preceeded by their byte-encoded size.  Ensure
  // that the blocks end on newlines to facillitate parallel processing when reading
  // compressed file
  std::cerr << std::endl;
  while ( end < map + infile_size ) {
    for ( char * current = end; current >= begin; --current ) {
      if ( *current == '\n' ) {
        // string to contain compressed output
        std::string compressed_output;
        // find input size, compress, get compressed size
        size_t input_size = current - begin + 1;
        snappy::Compress( begin, input_size, &compressed_output );
        // get size of compressed data from string
        size_t compressed_size = compressed_output.size(); 
        // write header (the compressed size in byte form) to stdout
        fwrite( ( char * ) &compressed_size, sizeof( char ),
            sizeof( size_t ), stdout );
        // write compressed data to stdout
        fwrite( compressed_output.data(), sizeof( char ), compressed_size, stdout ); 
        // report stats for this block to stderr
        fprintf( stderr, "...compressed %zu bytes down to %zu bytes, plus %zu additional bytes for header\n",
            input_size, compressed_size, sizeof( size_t ) );
        // running totals for final reporting
        total_compressed_bytes += compressed_size;
        total_uncompressed_bytes += input_size;
        total_header_bytes += sizeof( size_t );
        // re-set the begin and end pointers
        begin = ++current;
        // set target end of block (will be adjusted later to end on a newline)
        end = begin + target_block_size - 1;
        // make sure that we don't run off the end of the mapped file
        if ( end >= map + infile_size && begin < map + infile_size ) {
          end = map + infile_size - 1;
        }
        // break out to work on next block
        break;
      }
    }
  }
  fprintf( stderr, "\nCompressed %llu bytes down to %llu bytes ( plus %llu additional bytes for block and magic headers )\n\n",
      total_uncompressed_bytes, total_compressed_bytes, total_header_bytes + FSZ_MAGIC_LEN() );
}

void SnappyFileCompression::uncompress_file_to_stdout() {
  using namespace std;
  // open the compressed input file
  FILE* fp = fopen( infile_name, "r" );
  // seek to end and find input file size
  fseek( fp, 0, SEEK_END );
  uint64_t infile_size = ftell( fp );
  // re-set to 0 and close input file
  fseek( fp, 0, SEEK_SET );
  fclose( fp );
  // memory map the file
  int fd = open( infile_name, O_RDONLY );
  char * map = (char *) mmap( NULL, infile_size, PROT_READ, MAP_PRIVATE, fd, 0 );
  // layout will be:
  // [ size compressed_size_0 ][ char * compressed_data_0 ]...
  begin = map;
  end = begin;
  end += infile_size;
  total_uncompressed_bytes = 0;
  total_compressed_bytes = 0;
  total_header_bytes = 0;
  // Check magic bytes to make sure that this is fsz compressed
  if ( !( check_magic_bytes() ) ) {
    return;
  }
  std::cerr << std::endl;
  // loop through the mapped file reading, uncompressing and writing to
  // stdout for each block
  while ( begin < end ) {
    char current_block_size_byte_array[ sizeof( size_t ) ];
    // grab the block size as byte array from mmapped compressed file
    memcpy( current_block_size_byte_array, begin, sizeof( size_t ) );
    // Cast the block size into integer
    size_t * current_block_size = reinterpret_cast< size_t * > ( &current_block_size_byte_array );
    // string to contain uncompressed output
    string uncompressed_output;
    // advance reading pointer past the header bytes
    begin += sizeof( size_t );
    // uncompress into a string
    snappy::Uncompress( begin, *current_block_size, &uncompressed_output );
    // write output to stdout
    cout << uncompressed_output;
    // advance read pointer to next block
    begin += (*current_block_size);
    // report some stuff to stderr
    fprintf( stderr, "...uncompressed %zu bytes (plus %zu bytes for the header) to %zu bytes\n",
        *current_block_size, sizeof( size_t ), uncompressed_output.size() );
    // running totals for final reporting
    total_compressed_bytes += *current_block_size;
    total_uncompressed_bytes += uncompressed_output.size();
    total_header_bytes += sizeof( size_t );
  }
  // final summary of work done
  fprintf( stderr, "\nUncompressed %llu bytes (plus %llu additional bytes for block and magic headers) to %llu bytes of output.\n\n",
      total_compressed_bytes, total_header_bytes + FSZ_MAGIC_LEN(), total_uncompressed_bytes);

}


void SnappyFileCompression::init_compressed_block_reader() {
  using namespace std;
  // open the compressed input file
  FILE* fp = fopen( infile_name, "r" );
  // seek to end and find input file size
  fseek( fp, 0, SEEK_END );
  uint64_t infile_size = ftell( fp );
  // re-set to 0 and close input file
  fseek( fp, 0, SEEK_SET );
  fclose( fp );
  // memory map the file
  int fd = open( infile_name, O_RDONLY );
  char * map = (char *) mmap( NULL, infile_size, PROT_READ, MAP_PRIVATE, fd, 0 );
  // layout will be:
  // [ size compressed_size_0 ][ char * compressed_data_0 ]...
  begin = map;
  end = begin;
  end += infile_size;
  total_uncompressed_bytes = 0;
  total_compressed_bytes = 0;
  total_header_bytes = 0;
}

bool SnappyFileCompression::check_magic_bytes() {
  char magic[ FSZ_MAGIC_LEN() ];
  memcpy( magic, begin, FSZ_MAGIC_LEN() );
  if ( std::strncmp( magic, FSZ_MAGIC(), FSZ_MAGIC_LEN() ) == 0 ) {
    begin += FSZ_MAGIC_LEN();
    fprintf( stdout, "Is this an fsz-compressed file?  Magic bytes match.  Treat this file as fsz-compressed.\n" );
    return true;
  }
  else {
    return false;
  }
}

bool SnappyFileCompression::load_next_block_stream( std::stringstream & stream ) {
  
  fred::Scoped_Lock lock( mutex );

  if ( begin < end ) {
    char current_block_size_byte_array[ sizeof( size_t ) ];
    // grab the block size as byte array from mmapped compressed file
    memcpy( current_block_size_byte_array, begin, sizeof( size_t ) );
    // Cast the block size into integer
    size_t * current_block_size = reinterpret_cast< size_t * > ( &current_block_size_byte_array );
    // string to contain uncompressed output
    std::string uncompressed_output;
    // advance reading pointer past the header bytes
    begin += sizeof( size_t );
    // uncompress into a string
    snappy::Uncompress( begin, *current_block_size, &uncompressed_output );
    // add to stream
    if ( uncompressed_output.empty() ) {
      return false;
    }
    else {
      stream.str( uncompressed_output );
      stream.clear();
    }
    // advance read pointer to next block
    begin += (*current_block_size);
    // running totals for final reporting
    total_compressed_bytes += *current_block_size;
    total_uncompressed_bytes += uncompressed_output.size();
    total_header_bytes += sizeof( size_t );
    return true;
  }
  else {
    return false;
  }
}








































