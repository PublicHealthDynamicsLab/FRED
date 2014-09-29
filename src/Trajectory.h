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
// File: Trajectory.h
//

#ifndef _FRED_TRAJECTORY_H
#define _FRED_TRAJECTORY_H


#include <vector>
#include <iostream>
#include <algorithm>
#include <map>
#include <string>
#include <fstream>

#include "Transmission.h"

typedef std::vector<double> trajectory_t;

class Trajectory {
  public:
    Trajectory();
    Trajectory(std::map< int, trajectory_t > infectivity_copy, trajectory_t symptomaticity_copy);

    /**
     * Create a copy of this Trajectory and return a pointer to it
     * @return a pointer to the new Trajectory
     */
    Trajectory * clone();

    /**
     * @param strain the strain to check for
     * @return <code>true</code> if the Trajectory contains the strain, <code>false</code> if not
     */
    bool contains(int strain);


    trajectory_t get_infectivity_trajectory(int strain);
    trajectory_t get_symptomaticity_trajectory();
    void get_all_strains(std::vector<int> &);

    void set_symptomaticity_trajectory(trajectory_t symt);
    void set_infectivity_trajectory(int strain, trajectory_t vlt);
    void set_infectivities(std::map<int, trajectory_t > inf);

    Transmission::Loads * get_current_loads( int day );

    int get_duration() {
      return duration;
    }

    struct point {
      double infectivity;
      double symptomaticity;
      point( double infectivity_value, double symptomaticity_value) {
        infectivity = infectivity_value;
        symptomaticity = symptomaticity_value;
      };
    };

    point get_data_point(int t);

    /**
     * The class to allow iteration over a set of Trajectory objects
     */
    class iterator {
      private:
        Trajectory * trajectory;
        int current;
        bool next_exists;
      public:
        iterator(Trajectory * trj) {
          trajectory = trj;
          next_exists = false;
          current = -1;
        }

        bool has_next() {
          current++;
          next_exists = ( current < trajectory->duration );
          return next_exists;
        }

        int get_current() {
          return current;
        }

        Trajectory::point next() {
          return trajectory->get_data_point(current);
        }
    };


    //void calculate_aggregate_infectivity();
    std::map<int, double> * getInoculum(int day);
    void modify_symp_period(int startDate, int days_left);
    void modify_asymp_period(int startDate, int days_left, int sympDate);
    void modify_develops_symp(int sympDate, int sympPeriod);
    void mutate(int from_strain, int to_strain, unsigned int day);

    std::string to_string();
    void print();
    void print_alternate(std::stringstream &out);

  private:
    int duration;

    std::map< int, trajectory_t > infectivity;
    trajectory_t symptomaticity;

    //trajectory_t aggregate_infectivity;
  };

#endif

