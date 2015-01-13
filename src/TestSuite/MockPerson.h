/*
 *  MockPerson.h
 *  FRED
 *
 *  Created by Tom on 6/6/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../Person.h"

class MockPerson : public Person {
public:
  MOCK_METHOD9(setup,
               void(int index, int age, char sex, int marital, int occ, int profession, Place **favorite_places, int profile, Population* pop));
  MOCK_METHOD0(reset,
               void());
  MOCK_METHOD1(update,
               void(int day));
  MOCK_CONST_METHOD1(print,
                     void(int strain));
  MOCK_CONST_METHOD1(print_out,
                     void(int strain));
  MOCK_METHOD1(update_schedule,
               void(int day));
  MOCK_CONST_METHOD2(get_schedule,
                     void(int *n, Place **sched));
  MOCK_CONST_METHOD2(is_on_schedule,
                     int(int day, int loc));
  MOCK_CONST_METHOD0(print_schedule,
                     void());
  MOCK_METHOD1(become_susceptible,
               void(int strain));
  MOCK_METHOD1(become_exposed,
               void(Infection *infection));
  MOCK_METHOD1(become_infectious,
               void(Strain *strain));
  MOCK_METHOD1(become_symptomatic,
               void(Strain *strain));
  MOCK_METHOD1(become_immune,
               void(Strain *strain));
  MOCK_METHOD1(recover,
               void(Strain * strain));
  MOCK_METHOD1(behave,
               void(int day));
  MOCK_CONST_METHOD0(is_symptomatic,
                     int());
  MOCK_CONST_METHOD0(get_id,
                     int());
  MOCK_CONST_METHOD0(get_age,
                     int());
  MOCK_CONST_METHOD0(get_sex,
                     char());
  MOCK_CONST_METHOD0(get_occupation,
                     char());
  MOCK_CONST_METHOD0(get_marital_status,
                     char());
  MOCK_CONST_METHOD0(get_profession,
                     int());
  MOCK_CONST_METHOD0(get_places,
                     int());
  MOCK_CONST_METHOD1(get_strain_status,
                     char(int strain));
  MOCK_CONST_METHOD1(get_susceptibility,
                     double(int strain));
  MOCK_CONST_METHOD1(get_infectivity,
                     double(int strain));
  MOCK_CONST_METHOD1(get_exposure_date,
                     int(int strain));
  MOCK_CONST_METHOD1(get_infectious_date,
                     int(int strain));
  MOCK_CONST_METHOD1(get_recovered_date,
                     int(int strain));
  MOCK_CONST_METHOD1(get_infector,
                     int(int strain));
  MOCK_CONST_METHOD1(get_infected_place,
                     int(int strain));
  MOCK_CONST_METHOD1(get_infected_place_type,
                     char(int strain));
  MOCK_CONST_METHOD1(get_infectees,
                     int(int strain));
  MOCK_METHOD1(add_infectee,
               int(int strain));
  MOCK_CONST_METHOD2(is_new_case,
                     int(int day, int strain));
  MOCK_CONST_METHOD0(get_health,
                     Health*());
  MOCK_CONST_METHOD0(get_behavior,
                     Behavior*());
  MOCK_CONST_METHOD0(get_demographics,
                     Demographics*());
  MOCK_CONST_METHOD0(get_population,
                     Population*());
  MOCK_METHOD0(set_changed,
               void());
};
