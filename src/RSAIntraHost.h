/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.
  RSA IntraHost class designed by Sarah Lukens.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
  
  To use RSAIntraHost class, here are the params settings for calibrated version assuming 
  that x1, x2 are both sampled from a uniform distribution:
  intra_host_model[0] = 2
  symptomaticity_threshold[0] = 0.717
  infectivity_threshold[0] = 0.3649 
  ###  CALIBRATION PHASE II STEP 20 at Thu Mar 28 16:32:55 2013
  household_contacts[0] = 0.212139
  neighborhood_contacts[0] = 39.884618
  school_contacts[0] = 13.974653
  workplace_contacts[0] = 1.508962
*/

#ifndef _FRED_RSA_IntraHost_H
#define _FRED_RSA_IntraHost_H

#include <vector>
#include <map>

#include "IntraHost.h"
#include "Infection.h"
#include "Trajectory.h"

class Infection;
class Trajectory;

class RSAIntraHost : public IntraHost {

  public:
  Trajectory * get_trajectory(double real_age);
  RSAIntraHost();

    /**
     * Set the attributes for the IntraHost
     *
     * @param dis the disease to which this IntraHost model is associated
     */
    void setup(Disease *disease);
	/**
     * @return the prob_symptomatic
     */
    double get_prob_symptomatic() {
      return prob_symptomatic;
    }
    /* Sarah functions: */
    double get_recovery_phenotype_value(double age, double Ay, double Ao);
	/**
     * Get random number in [-scale, scale]
     *
     * @param scale
     * @return random number in [-scale, scale]
     */
    double random_phenotypic_value(double scale) ;
    /**
     * Get symptom score
     *
     * @param x1
     * @param x2
     * @param day
     * @return Symptom score at day day as a function of x1,x2
     */
    double Get_symptoms_score(double x1, double x2, int day);
    /**
     * Get symptom score
     *
     * @param x1
     * @param x2
     * @param day
     * @return Infectivity at day day as a function of x1,x2
     */
    double Get_Infectivity(double x1, double x2, int day, double Vscale);
     /**
     * Map agent's age to an input value for RSA evaluation for general age range
     *
     * @param age
     * @param Ao
     * @param Ay
     * @param scale
     * @return age mapped from [Ay, Ao] to [-scale, scale]
     */
    double piecewiselinear_map_age_value(double age, double Ay, double Ao, double scale);

    
        private:
        static const int MAX_LENGTH = 10;
        double prob_symptomatic;
        int id;
        double symptoms_scaling;
        double viral_infectivity_scaling;
        int days_sick;
        double lower_age_bound;
  		double upper_age_bound;
  		int age_severity_study_enabled;

  };

#endif
