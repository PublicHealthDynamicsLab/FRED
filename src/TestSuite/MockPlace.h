/*
 *  MockPlace.h
 *  FRED
 *
 *  Created by Tom on 6/6/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include <gmock/gmock.h>

#include "../Place.h"

class MockPlace : public Place {
public:
	MOCK_METHOD6(setup, void(int loc_id, const char *lab, double lon, double lat, Place *cont, Population *));
	MOCK_METHOD0(reset, void());
	MOCK_METHOD1(print, void(int strain));
	MOCK_METHOD2(add_susceptible, void(int strain, Person * per));
	MOCK_METHOD2(delete_susceptible, void(int strain, Person * per));
	MOCK_METHOD1(print_susceptibles, void(int strain));
	MOCK_METHOD2(add_infectious, void(int strain, Person * per));
	MOCK_METHOD2(delete_infectious, void(int strain, Person * per));
	MOCK_METHOD1(print_infectious, void(int strain));
	MOCK_METHOD2(spread_infection, void(int day, int strain));
	MOCK_METHOD1(is_open, int(int day));
	MOCK_METHOD0(get_population, Population *());
	MOCK_METHOD1(get_parameters, void(int strain));
	MOCK_METHOD2(get_group_type, int(int strain, Person * per));
	MOCK_METHOD3(get_transmission_prob, double(int strain, Person * i, Person * s));
	MOCK_METHOD2(should_be_open, int(int day, int strain));
	MOCK_METHOD1(get_contacts_per_day, double(int strain));
	MOCK_METHOD0(get_id, int());
	MOCK_METHOD0(get_label, char *());
	MOCK_METHOD0(get_type, int());
	MOCK_METHOD0(get_latitude, double());
	MOCK_METHOD0(get_longitude, double());
	MOCK_METHOD1(get_S, int(int strain));
	MOCK_METHOD1(get_I, int(int strain));
	MOCK_METHOD1(get_symptomatic, int(int strain));
	MOCK_METHOD0(get_size, int());
	MOCK_METHOD0(get_close_date, int());
	MOCK_METHOD0(get_open_date, int());
	MOCK_METHOD0(get_adults, int());
	MOCK_METHOD0(get_children, int());
	MOCK_METHOD0(get_HoH, Person*());
	MOCK_METHOD1(set_HoH, void(Person * per));
	MOCK_METHOD0(get_num_schools, int());
	MOCK_METHOD1(get_school, Place*(int i));
	MOCK_METHOD1(set_id, void(int n));
	MOCK_METHOD1(set_type, void(char t));
	MOCK_METHOD1(set_latitude, void(double x));
	MOCK_METHOD1(set_longitude, void(double x));
	MOCK_METHOD1(set_close_date, void(int day));
	MOCK_METHOD1(set_open_date, void(int day));
	MOCK_METHOD1(set_population, void (Population *));
	MOCK_METHOD1(set_container, void (Place *));
};