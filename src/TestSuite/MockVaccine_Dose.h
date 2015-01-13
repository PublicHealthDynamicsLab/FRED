/*
 *  MockVaccine_Dose.h
 *  FRED
 *
 *  Created by Tom on 7/3/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../Vaccine_Dose.h"

class Age_Map;

class MockVaccine_Dose : public Vaccine_Dose {
public:
  MOCK_CONST_METHOD0(get_efficacy_map,
                     Age_Map*());
  MOCK_CONST_METHOD0(get_efficacy_delay_map,
                     Age_Map*());
  MOCK_CONST_METHOD1(get_efficacy,
                     double(int age));
  MOCK_CONST_METHOD1(get_efficacy_delay,
                     double(int age));
  MOCK_CONST_METHOD0(get_days_between_doses,
                     int());
  MOCK_CONST_METHOD1(is_within_age,
                     bool(int age));
  MOCK_CONST_METHOD0(print,
                     void());
};