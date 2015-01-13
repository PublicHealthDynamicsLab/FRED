/*
 *  MockVaccine.h
 *  FRED
 *
 *  Created by Tom on 7/3/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../Vaccine.h"

class Vaccine_Dose;

class MockVaccine : public Vaccine {
public:
  MOCK_METHOD1(add_dose,
               void(Vaccine_Dose* dose));
  MOCK_CONST_METHOD0(get_strain,
                     int());
  MOCK_CONST_METHOD0(get_ID,
                     int());
  MOCK_CONST_METHOD0(get_number_doses,
                     int());
  MOCK_CONST_METHOD0(get_low_age,
                     int());
  MOCK_CONST_METHOD0(get_high_age,
                     int());
  MOCK_CONST_METHOD1(get_dose,
                     Vaccine_Dose*(int i));
  MOCK_CONST_METHOD0(get_initial_stock,
                     int());
  MOCK_CONST_METHOD0(get_total_avail,
                     int());
  MOCK_CONST_METHOD0(get_current_reserve,
                     int());
  MOCK_CONST_METHOD0(get_current_stock,
                     int());
  MOCK_CONST_METHOD0(get_additional_per_day,
                     int());
  MOCK_METHOD1(add_stock,
               void(int add));
  MOCK_METHOD1(remove_stock,
               void(int remove));
  MOCK_CONST_METHOD0(print,
                     void());
  MOCK_METHOD1(update,
               void(int day));
  MOCK_METHOD0(reset,
               void());
};