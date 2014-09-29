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
// File: Antiviral.h
//
#ifndef _FRED_ANTIVIRALS_H
#define _FRED_ANTIVIRALS_H

#include <stdio.h>
#include <iomanip>
#include <vector>
#define MAX_ANTIVIRALS 4

using namespace std;

class Antiviral;

/**
 * Antivirals is a class to contain a group of Antiviral classes
 */
class Antivirals {

public:
  // Creation Operations
  /**
   * Default constructor<br />
   * Reads all important values from parameter file
   */
  Antivirals();          // This is created from input
  
  //Paramter Access Members
  /**
   * @return <code>true</code> if there are Antivirals, <code>false</code> if not
   */
  bool do_av()                               const { return AVs.size() > 0; } 

  /**
   * @return the count of antivirals
   */
  int get_number_antivirals()                const { return AVs.size(); }

  /**
   * @return the total current stock of all Antivirals in this group
   */
  int get_total_current_stock()              const;

  /**
   * @return a pointer to this groups Antiviral vector
   */
  vector <Antiviral *> const get_AV_vector() const { return AVs; }

  /**
   * Return a pointer to a specific Antiviral in this group's vector
   */
  Antiviral* const get_AV(int nav)           const { return AVs[nav]; }
  
  // Utility Functions
  /**
   * Print out information about this object
   */
  void print() const;

  /**
   * Print out current stock information for each Anitviral in this group's vector
   */
  void print_stocks() const;

  /**
   * Put this object back to its original state
   */
  void reset();

  /**
   * Perform the daily update for this object
   *
   * @param day the simulation day
   */
  void update(int day);

  /**
   * Print out a daily report
   *
   * @param day the simulation day
   */
  void report(int day) const;

  /**
   * Used during debugging to verify that code is functioning properly. <br />
   * Checks the quality_control of each Antiviral in this group's vector of AVs
   *
   * @param ndiseases the bumber of diseases
   * @return 1 if there is a problem, 0 otherwise
   */
  void quality_control(int ndiseases) const;
  
  // Polling the collection 
  /**
   * This method looks through the vector of Antiviral objects and checks to see if each one is
   * effective against the particular disease and if it also has some in stock.  If so, then that
   * particular AV is added to the return vector.
   *
   * @param the disease to poll for
   * @return a vector of pointers to Antiviral objects
   */
  vector < Antiviral*> find_applicable_AVs(int disease) const;

  /**
   * @return a vector of pointers to all Antiviral objects in this group that are prophylaxis
   */
  vector < Antiviral*> prophylaxis_AVs() const;
  
private:
  vector < Antiviral* > AVs;  // A Vector to hold the AVs in the collection
  FILE* reportFile;           // Create an AV report file, to be deprecated
};
#endif // _FRED_ANTIVIRALS_H
