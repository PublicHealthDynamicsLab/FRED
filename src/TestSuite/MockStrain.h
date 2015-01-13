/*
 *  MockStrain.h
 *  FRED
 *
 *  Created by Tom on 6/7/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../Strain.h"
#include "../Population.h"
#include "../Age_Map.h"

class MockStrain : public Strain {
public:
  MOCK_METHOD0(reset,
               void());
  MOCK_METHOD3(setup,
               void(int s, Population *pop, double *mut_prob));
  MOCK_METHOD0(print,
               void());
  MOCK_METHOD1(update,
               void(int day));
  MOCK_METHOD4(attempt_infection,
               bool(Person* infector, Person* infectee, Place* place, int exposure_date));
  MOCK_METHOD0(get_days_latent,
               int());
  MOCK_METHOD0(get_days_incubating,
               int());
  MOCK_METHOD0(get_days_asymp,
               int());
  MOCK_METHOD0(get_days_symp,
               int());
  MOCK_METHOD0(get_days_recovered,
               int());
  MOCK_METHOD0(get_symptoms,
               int());
  MOCK_METHOD0(get_asymp_infectivity,
               double());
  MOCK_METHOD0(get_symp_infectivity,
               double());
  MOCK_METHOD0(get_id,
               int());
  MOCK_METHOD0(get_transmissibility,
               double());
  MOCK_METHOD0(get_prob_symptomatic,
               double());
  MOCK_METHOD0(get_attack_rate,
               double());
  MOCK_CONST_METHOD0(get_residual_immunity,
                     Age_Map*());
  MOCK_CONST_METHOD0(get_at_risk,
                     Age_Map*());
  MOCK_METHOD1(insert_into_exposed_list,
               void(Person *person));
  MOCK_METHOD1(insert_into_infectious_list,
               void(Person *person));
  MOCK_METHOD1(remove_from_exposed_list,
               void(Person *person));
  MOCK_METHOD1(remove_from_infectious_list,
               void(Person *person));
  MOCK_METHOD1(update_stats,
               void(int day));
  MOCK_METHOD1(print_stats,
               void(int day));
  MOCK_METHOD0(get_population,
               Population*());
};