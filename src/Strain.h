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
// File: Strain.h
//

#ifndef _FRED_STRAIN_H
#define _FRED_STRAIN_H

#include <map>
#include <string>

#include "Global.h"
#include "Epidemic.h"

using namespace std;

class Person;
class Disease;
class Age_Map;


class Strain_Data {
  short int num_elements;
  unsigned short int * data;

  public:

  Strain_Data( int N ) : num_elements( N ) {
    data = new unsigned short int[ num_elements ];
    for ( int n = 0; n < N; ++n ) {
      data[ n ] = 0;
    }
  }

  Strain_Data( const Strain_Data & other ) : num_elements( other.num_elements ) {
    data = new unsigned short int[ num_elements ];
    for ( int n = 0; n < num_elements; ++n ) {
      data[ n ] = other.data[ n ];
    }
  }

  ~Strain_Data() {
    delete[] data;
  }


  int get_num_elements() { return num_elements; }
  int size() const { return num_elements; }

  int get_data_element( int i ) {
    return data[ i ];
  }

  bool operator== ( const Strain_Data & other ) {
    if ( num_elements != other.num_elements ) { 
      return false;
    }
    for ( int i = 0; i < num_elements; ++i ) {
      if ( data[ i ] != other.data[ i ] ) {
        return false;
      }
    }
    return true;
  }

  short unsigned int & operator[] ( int i ) { return data[ i ]; }  
  const short unsigned int & operator[] ( int i ) const { return data[ i ]; }  

};


class Strain {
  public:

    Strain( int num_elements );
    Strain( const Strain & other );

    ~Strain();

    void reset();
    void setup( int strain, Disease * disease, double trans, Strain * parent );
    void print();
    void print_alternate( stringstream & out );

    int get_id() { return id; }
    double get_transmissibility() { return transmissibility; }

    int get_num_data_elements() { return strain_data.get_num_elements(); };
    int get_data_element( int i ) { return strain_data.get_data_element( i ); };

    Strain_Data & get_data() { return strain_data; }
    const Strain_Data & get_strain_data() { return strain_data; }

    std::string to_string();

  private:
    Strain * parent;
    int id;
    double transmissibility;
    Disease * disease;
    Strain_Data strain_data;

  };

#endif // _FRED_STRAIN_H
