/*
 *  MockAV_Health.h
 *  FRED
 *
 *  Created by Tom on 7/3/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../Antiviral.h"

class Health;
class AV_Health;
class Policy;

class MockAntiviral : public Antiviral {
public:
  MOCK_CONST_METHOD0(get_strain,
                     int());
  MOCK_CONST_METHOD0(get_reduce_infectivity,
                     double());
  MOCK_CONST_METHOD0(get_reduce_susceptibility,
                     double());
  MOCK_CONST_METHOD0(get_reduce_asymp_period,
                     double());
  MOCK_CONST_METHOD0(get_reduce_symp_period,
                     double());
  MOCK_CONST_METHOD0(get_prob_symptoms,
                     double());
  MOCK_CONST_METHOD0(get_course_length,
                     int());
  MOCK_CONST_METHOD0(get_percent_symptomatics,
                     double());
  MOCK_CONST_METHOD0(get_efficacy,
                     double());
  MOCK_CONST_METHOD0(get_start_day,
                     int());
  MOCK_CONST_METHOD0(is_prophylaxis,
                     bool());
  MOCK_CONST_METHOD0(roll_will_have_symp,
                     int());
  MOCK_CONST_METHOD0(roll_efficacy,
                     int());
  MOCK_CONST_METHOD0(roll_course_start_day,
                     int());
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
               void(int amount));
  MOCK_METHOD1(remove_stock,
               void(int remove));
  MOCK_METHOD1(update,
               void(int day));
  MOCK_CONST_METHOD0(print,
                     void());
  MOCK_METHOD0(reset,
               void());
  MOCK_CONST_METHOD1(report,
                     void(int day));
  MOCK_CONST_METHOD1(quality_control,
                     int(int nstrains));
  MOCK_CONST_METHOD0(print_stocks,
                     void());
  MOCK_METHOD3(effect,
               void(Health *h, int cur_day, AV_Health* av_health));
  MOCK_METHOD1(set_policy,
               void(Policy* p));
  MOCK_CONST_METHOD0(get_policy,
                     Policy*());
  MOCK_METHOD1(add_given_out,
               void(int amount));
  MOCK_METHOD1(add_ineff_given_out,
               void(int amount));
};
