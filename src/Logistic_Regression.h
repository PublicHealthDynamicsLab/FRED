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

#ifndef _FRED_LOGISTIC_REGRESSION_H
#define _FRED_LOGISTIC_REGRESSION_H

#include <string>
#include <vector>
using namespace std;

#include "Global.h"


class Logistic_Regression {

public:
  Logistic_Regression(char* model_name, int values, double _z0, std::vector<double> &beta);
  ~Logistic_Regression();
  virtual double get_score(double* x);
  virtual double get_prob(double* x);
  virtual int classify(double* x);
  virtual int get_outcome(double* x);
  
protected:
  char name[FRED_STRING_SIZE];
  int number_of_values;
  int number_of_factors;
  double z0;
  double* coeff;
  
private:
};



class Ordinal_Logistic_Regression : public Logistic_Regression {

public:
  Ordinal_Logistic_Regression(char* model_name, int values, double _z0, std::vector<double> &beta, double *cutoffs);
  ~Ordinal_Logistic_Regression();
  int get_outcome(double* x);
  int classify(double* x);

private:
  double* cutoff;
};

#endif  // #ifndef _FRED_LOGISTIC_REGRESSION_H
