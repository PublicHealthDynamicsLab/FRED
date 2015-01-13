/*
 *  MockBehavior.h
 *  FRED
 *
 *  Created by Tom on 6/6/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../Behavior.h"
#include "../Person.h"
#include "../Place.h"

class MockBehavior : public Behavior {
public:
  MOCK_METHOD0(reset,
               void());
  MOCK_METHOD1(update,
               void(int day));
  MOCK_METHOD2(get_schedule,
               void(int *n, Place **sched));
  MOCK_METHOD2(is_on_schedule,
               int(int day, int loc));
  MOCK_METHOD1(update_schedule,
               void(int day));
  MOCK_METHOD0(print_schedule,
               void());
  MOCK_METHOD1(become_susceptible,
               void(int strain));
  MOCK_METHOD1(become_infectious,
               void(int strain));
  MOCK_METHOD1(become_exposed,
               void(int strain));
  MOCK_METHOD1(become_immune,
               void(int strain));
  MOCK_METHOD1(recover,
               void(int strain));
  MOCK_METHOD0(get_profile,
               int());
  MOCK_METHOD0(get_favorite_places,
               int());
  MOCK_METHOD0(compliance_to_vaccination,
               int());
  MOCK_METHOD0(get_HoH,
               Person*());
  MOCK_METHOD0(get_household,
               Place*());
  MOCK_METHOD0(get_school,
               Place*());
};