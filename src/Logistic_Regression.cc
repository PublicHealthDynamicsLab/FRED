/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#include "Logistic_Regression.h"

#include "Random.h"
#include "Utils.h"

Logistic_Regression::Logistic_Regression(char* model_name, int values, double _z0, std::vector<double> &beta) {

  assert(strcmp(model_name,"") != 0);
  strcpy(this->name, model_name);

  assert(values > 1);
  this->number_of_values = values;

  this->number_of_factors = beta.size();
  this->z0 = _z0;

  // record coefficients
  this->coeff = NULL;
  if (this->number_of_factors > 0) {
    this->coeff = new double[this->number_of_factors];
    for (int i = 0; i < this->number_of_factors; ++i) {
      this->coeff[i] = beta[i];
    }
  }

}

Logistic_Regression::~Logistic_Regression() {
  delete[] coeff;
}


double Logistic_Regression::get_score(double* x) {
  double z = this->z0;
  for(int i = 0; i < this->number_of_factors; ++i) {
    double beta = this->coeff[i];
    if (beta != 0.0) {
      z += beta * x[i]; 
    }
  }
  return z;
}

// simple binary logistic regression model

double Logistic_Regression::get_prob(double* x) {
  double z = get_score(x);
  return 1.0 / (1.0 + exp(-z));
}
    
int Logistic_Regression::classify(double* x) {
  // compute score for this observation
  double z = get_score(x);
  // printf("classify score = %f\n", z);
  
  // return maximum likelihood prediction
  return (0.0 <= z);
}


int Logistic_Regression::get_outcome(double* x) {
  
  // probability of (value == 1)
  double p = get_prob(x);

  // draw random number
  double r = Random::draw_random();

  // compare to target
  return (r <= p);
}


Ordinal_Logistic_Regression::Ordinal_Logistic_Regression(char* model_name, int values, double _z0, std::vector<double> &beta, double *cutoffs) :
  Logistic_Regression(model_name, values, _z0, beta) {

  // record cutoffs
  this->cutoff = new double[values-1];
  for (int i = 0; i < values-1; ++i) {
    this->cutoff[i] = cutoffs[i];
  }

}

Ordinal_Logistic_Regression::~Ordinal_Logistic_Regression() {
  delete[] cutoff;
}


int Ordinal_Logistic_Regression::get_outcome(double* x) {

  // compute score for this observation
  double z = get_score(x);
  // FRED_VERBOSE(0, "z = %f\n",z);

  // assign one of the (ordered) values
  double total = 0.0;
  double p[this->number_of_values];
  p[0] = 1.0 / (1.0 + exp(z - this->cutoff[0]));
  total += p[0];
  for (int i = 1; i < this->number_of_values-1; ++i) {
    p[i] = 1.0 / (1.0 + exp(z - this->cutoff[i])) - 1.0 / (1.0 + exp(z - this->cutoff[i-1]));
    total += p[i];
  }
  p[this->number_of_values-1] = 1.0 - 1.0 / (1.0 + exp(z - this->cutoff[this->number_of_values-2]));
  total += p[this->number_of_values-1];
  /*
  for (int i = 0; i < this->number_of_values; ++i) {
    FRED_VERBOSE(0, "GET_OUTCOME: p[%d] = %f\n", i, p[i]);
  }
  */

  double r = Random::draw_random();
  // FRED_VERBOSE(0, "r = %f\n", r);
  int value = -1;
  for (int i = 0; value == -1 && i < this->number_of_values; i++) {
    if (r < p[i]) {
      value = i;
    } else {
      r -= p[i];
    }
  }
  // FRED_VERBOSE(0, "value = %d\n", value);
  assert(value > -1);
  return value;
}


int Ordinal_Logistic_Regression::classify(double* x) {
  // this is not available for ordinal logistic regression
  assert(0);
  return -1;
}


