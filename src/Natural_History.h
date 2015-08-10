/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

#ifndef _FRED_NATURAL_HISTORY_H
#define _FRED_NATURAL_HISTORY_H

class Disease;
class Person;

class Natural_History {
public:
  
  virtual ~Natural_History() {};

  /**
   * This static factory method is used to get an instance of a specific Natural_History Model.
   * Depending on the model parameter, it will create a specific Natural_History Model and return
   * a pointer to it.
   *
   * @param a string containing the requested Natural_History model type
   * @return a pointer to a specific Natural_History model
   */
  static Natural_History * get_natural_history(char* natural_history_model);

  /**
   * Set the attributes for the Natural_History
   *
   * @param dis the disease to which this Natural_History model is associated
   */
  virtual void setup(Disease *dis);

  // called from Infection

  virtual int get_latent_period(Person* host);
  virtual int get_duration_of_infectiousness(Person* host);
  virtual int get_incubation_period(Person* host);
  virtual int get_duration_of_symptoms(Person* host);


protected:
  Disease *disease;
};

#endif
