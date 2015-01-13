/*
 *  MockPopulation.h
 *  FRED
 *
 *  Created by Tom on 5/25/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../Population.h"

class MockPopulation : public Population {
public:
  MOCK_METHOD0(get_parameters,
               void());
  MOCK_METHOD0(setup,
               void());
  MOCK_METHOD0(population_quality_control,
               void());
  MOCK_METHOD2(print,
               void(int incremental, int day));
  MOCK_METHOD0(end_of_run,
               void());
  MOCK_METHOD1(reset,
               void(int run));
  MOCK_METHOD1(update,
               void(int day));
  MOCK_METHOD1(report,
               void(int day));
  MOCK_METHOD1(get_strain,
               Strain*(int s));
  MOCK_METHOD0(get_strains,
               int());
  MOCK_METHOD0(get_pop,
               Person**());
  MOCK_METHOD0(get_pop_size,
               int());
  MOCK_METHOD0(get_pregnancy_prob,
               Age_Map*());
  MOCK_METHOD0(get_av_manager,
               AV_Manager*());
  MOCK_METHOD0(get_vaccine_manager,
               Vaccine_Manager*());
  MOCK_METHOD1(set_changed,
               void(Person *p));
};