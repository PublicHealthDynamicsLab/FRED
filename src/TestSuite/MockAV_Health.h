/*
 *  MockAV_Health.h
 *  FRED
 *
 *  Created by Tom on 7/3/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../AV_Health.h"

class Health;

class MockAV_Health : public AV_Health {
public:
  MOCK_CONST_METHOD0(get_av_start_day,
                     int());
  MOCK_CONST_METHOD0(get_av_end_day,
                     int());
  MOCK_CONST_METHOD0(get_health,
                     Health*());
  MOCK_CONST_METHOD0(get_strain,
                     int());
  MOCK_CONST_METHOD0(get_antiviral,
                     Antiviral*());
  MOCK_CONST_METHOD1(is_on_av,
                     int(int day));
  MOCK_CONST_METHOD0(is_effective,
                     int());
  MOCK_METHOD1(update,
               void(int day));
  MOCK_CONST_METHOD0(print,
                     void());
  MOCK_CONST_METHOD0(printTrace,
                     void());
};