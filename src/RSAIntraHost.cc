/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.
  RSA IntraHost class designed by Sarah Lukens
  
  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include <vector>
#include <map>

#include "RSAIntraHost.h"
#include "Disease.h"
// #include "Infection.h"
// #include "Trajectory.h"
#include "Params.h"
#include "Random.h"
#include "Person.h"


RSAIntraHost::RSAIntraHost() {
  days_sick = 0; // 11;
  prob_symptomatic = -1.0;
  symptoms_scaling = 0;
  lower_age_bound = 0.0;
  upper_age_bound = 0.0;
  
}

void RSAIntraHost::setup(Disease *disease) {
  IntraHost::setup(disease);

  int id = disease->get_id();
  //Params::get_indexed_param("symp",id,&prob_symptomatic);

}

Trajectory * RSAIntraHost::get_trajectory(double real_age) {
  double Symp_threshold = disease->get_symptomaticity_threshold();
  Params::get_param_from_string("symptoms_scaling", &symptoms_scaling );
  Params::get_param_from_string("viral_infectivity_scaling", &viral_infectivity_scaling );
  Params::get_param_from_string("days_sick",  &days_sick );
  Params::get_param_from_string("lower_age_bound", &lower_age_bound );
  Params::get_param_from_string("upper_age_bound", &upper_age_bound );

  double Vscale = viral_infectivity_scaling;
  int days_latent = 1; // Start vectors at 0.  
  
  double d1 = get_recovery_phenotype_value(real_age, lower_age_bound, upper_age_bound);
  //double d1 = random_phenotypic_value(0.5);
  double d2 = random_phenotypic_value(0.5);

  Loads* loads;
  loads = new Loads;
  loads->clear();
  loads->insert( pair<int, double> (1, 1.0) );

  Trajectory * trajectory = new Trajectory();

  map<int, double> :: iterator it;
  double Sympt, Inft;

  // Loop over different strains.  
  for(it = loads->begin(); it != loads->end(); it++) {

 // Now evaluate infectivity and symptoms score for each day. 
/* -----------------------------------------------------------------------------------*/
    // Everything is 0 on day 0:
    
    vector<double> infectivity_trajectory(days_latent, 0.0);
    vector<double> symptomaticity_trajectory(days_latent, 0.0);
    //vector<double> infectivity_AStrajectory(days_latent, 0.0);

// Symptomatic:
    	for (int kk = 0; kk<days_sick; kk++){
      		Sympt = symptoms_scaling*Get_symptoms_score(d1, d2, kk+1);
      		//Sympt = Get_symptoms_score(d1, d2, kk+1);
      		Inft = Get_Infectivity(d1, d2, kk+1, Vscale);
      		Inft = std::max(Inft, 0.0);

      		// Break from loop if individual clears before day Ndays - should be while loop
      		if ( (kk > 3) && (Inft < 1e-4) ){
      			break;
      			}

      		infectivity_trajectory.push_back( Inft );
      		symptomaticity_trajectory.push_back( Sympt );
      		//infectivity_AStrajectory.push_back( 0.5534*Inft );
    		}	
    		//Check if asymptomatic:
    		//double max_symp = *std::max_element(symptomaticity_trajectory.begin(), symptomaticity_trajectory.end());
    		//if (max_symp < Symp_threshold){
    		//	trajectory->set_infectivity_trajectory(it->first, infectivity_AStrajectory);
    		//}
    		//else{
    		trajectory->set_infectivity_trajectory(it->first, infectivity_trajectory);
    		//}
    		trajectory->set_symptomaticity_trajectory(symptomaticity_trajectory);
    
  }	// end loop over strains
  
    return trajectory;
}


/* -----------------------------------------------------------------------------------*/
/*           Sub-functions used to get trajectory:                                     */
/* -----------------------------------------------------------------------------------*/
double RSAIntraHost::get_recovery_phenotype_value(double age, double Ay, double Ao){
// d1 is value from recovery phenotype direction.  In Baseline, this is a random number.  
// In the age-severity study, this value related to age;

  double d1;
  //bool age_severity_study_enabled = true;
  Params::get_param_from_string("age_severity_study_enabled",  &age_severity_study_enabled );

  // Get age: for infectee's age: // d2 recovery direction. d2=1,longest recovery
   //double d1 = piecewiselinear_map_age_value(age, 18.0, 65.0, 0.5);
  //double d1 = piecewiselinear_map_age_value(age, 0.0, 83.0, 0.5);
  
    
  if (age_severity_study_enabled)
  	//d1 = piecewiselinear_map_age_value(age, 18.0, 65.0, 0.5);	
  	d1 = piecewiselinear_map_age_value(age, Ay, Ao, 0.5);
  else
    d1 = random_phenotypic_value(0.5);

	return d1;
}

double RSAIntraHost::random_phenotypic_value(double scale) {
  // Picks a uniform random number in [-scale, scale] 
  double r = Random::draw_random() ;
  double phenotype = (2.0*r - 1.0)*scale;	
  return phenotype;
}

double RSAIntraHost::piecewiselinear_map_age_value(double age, double Ay, double Ao, double scale) {
  // Takes age and maps to (0,1), using piecewise linear assumption
  // Here, age = 0 if < Ay (younger), age = 1 if > Ao (older), and a line with slope 1/(Ao-Ay) 
  if (age > Ao){ age = Ao ;}
  if (age < Ay){ age = Ay ;}
  //double agescale = (1.0/47.0)*(age - 18.0);  
  double agescale = (age - Ay)/(Ao - Ay);
  // Then map to [-scale,scale]:
  double phenotype = (2.0*agescale - 1.0)*scale;	
  return phenotype;
}
/* -----------------------------------------------------------------------------------*/
double RSAIntraHost::Get_symptoms_score(double x1, double x2, int day){

    double S[][6] = {{0.0028 ,  -0.0219  , -0.1371  ,  0.0805  ,  0.0065  ,  0.3258},
    {0.6238  ,  0.2043 ,  -1.1329 ,  -0.6542  ,  0.0141  , -0.4606},
    {0.8332  ,  0.6492 ,  -0.5557 ,  -0.7593  ,  0.3423  , -0.3726},
    {0.6610  ,  0.8267 ,  -0.2723 ,  -0.5517  ,  0.6364 ,   0.2271},
    {0.4482   , 0.7849 ,  -0.1936  , -0.2763  ,  0.7802 ,   0.2816},
    {0.2708  ,  0.6001 ,  -0.1848 ,  -0.2685  ,  0.7596  ,  0.2400},
    {0.1590  ,  0.4117 ,  -0.1449 ,  -0.2353  ,  0.5750  ,  0.2031},
    {0.0922  ,  0.2702 ,  -0.1086  , -0.1761  ,  0.4037  ,  0.1516},
    {0.0533  ,  0.1730  , -0.0806 ,  -0.1327  ,  0.2722  ,  0.1053},
    {0.0311  ,  0.1088  , -0.0582 ,  -0.1003  ,  0.1767  ,  0.0716},
    {0.0181  ,  0.0680 ,  -0.0410  , -0.0736  ,  0.1127  ,  0.0488}};

	
	double SymScore = S[day][0] + S[day][1]*x1 + S[day][2]*x2 + S[day][3]*x1*x2 + S[day][4]*x1*x1 + S[day][5]*x2*x2;

	SymScore = std::max(SymScore, 0.0);
	
	return SymScore;
}	
/* ------------------------------------------------------------------------------*/
double RSAIntraHost::Get_Infectivity(double x1, double x2, int day, double Vscale){

double V[][6] = {{ 1.7957 ,  -0.3485  , -4.4816 ,  -0.1895 ,  -0.1568 ,  -1.6468},
    {3.2574  ,  0.3007,   -1.1052  , -0.6959 ,  -0.0630 ,  -2.0984},
    {2.7126  ,  0.8607 ,  -0.2395  , -0.4676  , -0.0365 ,   0.5058},
    {2.1846  ,  1.4181 ,  -0.2884 ,  -0.2562  , -0.1603 ,   0.5260},
    {1.5961  ,  1.9964 ,  -0.3433 ,  -0.2195  , -0.0835 ,   0.5198},
    {0.9935   , 2.5375 ,  -0.4165 ,  -0.2218  ,  0.0490  ,  0.6078},
    {0.4070  ,  3.0739 ,  -0.4965 ,  -0.2629  ,  0.0129  ,  0.6567},
   {-0.1908  ,  3.6330 ,  -0.5486 ,  -0.3169 ,  -0.0335  ,  0.8026},
   {-0.8006  ,  4.2066 ,  -0.6178  , -0.3140 ,  -0.0128  ,  0.9302},
   {-1.4123  ,  4.7840  , -0.7109  , -0.2540  ,  0.0305  ,  0.9870},
   {-2.0198  ,  5.3592  , -0.8238  , -0.1551  ,  0.0694  ,  0.9632}};

	double Vload = V[day][0] + V[day][1]*x1 + V[day][2]*x2 + V[day][3]*x1*x2 + V[day][4]*x1*x1 + V[day][5]*x2*x2;
	//double Vscale = 3.71;  
	double Infectivity = Vload / Vscale;
	//double Infectivity = Vload / 3.71;
	
	return Infectivity;
}
