/*
 *  MockBehavior.h
 *  FRED
 *
 *  Created by Tom on 7/25/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../Health.h"

class Strain;
class Infection;
class Vaccine_Health;
class AV_Health;
class Vaccine;
class Antiviral;

class MockHealth : public Health {
public:
  MOCK_METHOD0(reset,
      void());
  MOCK_METHOD1(update,
      void(int day));
  MOCK_METHOD1(become_susceptible,
      void(int strain));
  MOCK_METHOD1(become_exposed,
      void(Infection *inf));
  MOCK_METHOD1(become_infectious,
      void(Strain * strain));
  MOCK_METHOD1(become_symptomatic,
      void(Strain *strain));
  MOCK_METHOD1(become_immune,
      void(Strain* strain));
  MOCK_METHOD1(declare_at_risk,
      void(Strain* strain));
  MOCK_METHOD1(recover,
      void(Strain * strain));
  MOCK_CONST_METHOD0(is_symptomatic,
      int());
  MOCK_CONST_METHOD1(is_immune,
      bool(Strain* strain));
  MOCK_CONST_METHOD1(is_at_risk,
      bool(Strain* strain));
  MOCK_CONST_METHOD1(get_strain_status,
      char(int strain));
  MOCK_CONST_METHOD0(get_self,
      Person*());
  MOCK_CONST_METHOD0(get_num_strains,
      int());
  MOCK_METHOD1(add_infectee,
      int(int strain));
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
  MOCK_CONST_METHOD1(get_susceptibility,
      double(int strain));
  MOCK_CONST_METHOD1(get_infectivity,
      double(int strain));
  MOCK_CONST_METHOD1(get_infection,
      Infection*(int strain));
  MOCK_CONST_METHOD2(is_on_av_for_strain,
      bool(int day, int strain));
  MOCK_METHOD2(take,
      void(Vaccine *vacc, int day));
  MOCK_METHOD2(take,
      void(Antiviral *av, int day));
  MOCK_CONST_METHOD0(get_number_av_taken,
      int());
  MOCK_CONST_METHOD1(get_checked_for_av,
      int(int s));
  MOCK_METHOD1(flip_checked_for_av,
      void(int s));
  MOCK_CONST_METHOD0(is_vaccinated,
      bool());
  MOCK_CONST_METHOD0(get_number_vaccines_taken,
      int());
  MOCK_CONST_METHOD1(get_av_health,
      AV_Health*(int i));
  MOCK_CONST_METHOD1(get_vaccine_health,
      Vaccine_Health*(int i));
  MOCK_METHOD2(modify_susceptibility,
      void(int strain, double multp));
  MOCK_METHOD2(modify_infectivity,
      void(int strain, double multp));
  MOCK_METHOD3(modify_infectious_period,
      void(int strain, double multp, int cur_day));
  MOCK_METHOD3(modify_symptomatic_period,
      void(int strain, double multp, int cur_day));
  MOCK_METHOD3(modify_asymptomatic_period,
      void(int strain, double multp, int cur_day));
  MOCK_METHOD3(modify_develops_symptoms,
      void(int strain, bool symptoms, int cur_day));
};
