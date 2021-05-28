/*
 * This file is part of the FRED system.
 *
 * Copyright (c) 2010-2012, University of Pittsburgh, John Grefenstette, Shawn Brown, 
 * Roni Rosenfield, Alona Fyshe, David Galloway, Nathan Stone, Jay DePasse, 
 * Anuroop Sriram, and Donald Burke
 * All rights reserved.
 *
 * Copyright (c) 2013-2019, University of Pittsburgh, John Grefenstette, Robert Frankeny,
 * David Galloway, Mary Krauland, Michael Lann, David Sinclair, and Donald Burke
 * All rights reserved.
 *
 * FRED is distributed on the condition that users fully understand and agree to all terms of the 
 * End User License Agreement.
 *
 * FRED is intended FOR NON-COMMERCIAL, EDUCATIONAL OR RESEARCH PURPOSES ONLY.
 *
 * See the file "LICENSE" for more information.
 */

//
//
// File: Date.h
//

#ifndef DATE_H_
#define DATE_H_

#include <string>
#include <cstdio>
#include <unordered_map>
#include <map>
#include "Global.h"

using namespace std;

#define MAX_DATES (250*366)


struct date_t {
  int year;
  int month;
  int day_of_month;
  int day_of_week;
  int day_of_year;
  int epi_week;
  int epi_year;
};

class Date {
public:
  static const int SUNDAY = 0;
  static const int MONDAY = 1;
  static const int TUESDAY = 2;
  static const int WEDNESDAY = 3;
  static const int THURSDAY = 4;
  static const int FRIDAY = 5;
  static const int SATURDAY = 6;
  static const int JANUARY = 1;
  static const int FEBRUARY = 2;
  static const int MARCH = 3;
  static const int APRIL = 4;
  static const int MAY = 5;
  static const int JUNE = 6;
  static const int JULY = 7;
  static const int AUGUST = 8;
  static const int SEPTEMBER = 9;
  static const int OCTOBER = 10;
  static const int NOVEMBER = 11;
  static const int DECEMBER = 12;
  static const int INVALID = -1;

  static char Start_date[];
  static char End_date[];

  /**
   * Default constructor
   */
  // Date(){};
  Date();
  ~Date(){};

  // static void setup_dates(char * date_string);
  static void setup_dates();
  static void update() {
    Date::today++; 
    Date::year = Date::date[today].year;
    Date::month = Date::date[today].month;
    Date::day_of_month = Date::date[today].day_of_month;
    Date::day_of_week = Date::date[today].day_of_week;
    Date::day_of_year = Date::date[today].day_of_year;
    Date::epi_week = Date::date[today].epi_week;
    Date::epi_year = Date::date[today].epi_year;
  }
  static int get_year() { return Date::year; }
  static int get_year(int sim_day) { return sim_start_index+sim_day < Date::max_days? Date::date[sim_start_index+sim_day].year : -1; }
  static int get_month() { return Date::month; }
  static int get_month(int sim_day) { return sim_start_index+sim_day < Date::max_days ? Date::date[sim_start_index+sim_day].month : -1; }
  static int get_day_of_month() { return Date::day_of_month; }
  static int get_day_of_month(int sim_day) { return sim_start_index+sim_day < Date::max_days ? Date::date[sim_start_index+sim_day].day_of_month : -1; }
  static int get_day_of_week() { return Date::day_of_week; }
  static int get_day_of_week(int sim_day) { return sim_start_index+sim_day < Date::max_days ? Date::date[sim_start_index+sim_day].day_of_week : -1; }
  static int get_day_of_year() { return Date::day_of_year; }
  static int get_day_of_year(int sim_day) { return sim_start_index+sim_day < Date::max_days ? Date::date[sim_start_index+sim_day].day_of_year : -1; }
  static int get_date_code() { return 100*get_month() + get_day_of_month(); }
  static int get_date_code(int month, int day_of_month) { return 100*month + day_of_month; }
  static int get_date_code(string month_str, int day_of_month) { return 100*get_month_from_name(month_str) + day_of_month; }
  static int get_date_code(string date_str) { 
    string month_str = date_str.substr(0,3);
    string day_string = date_str.substr(4);
    int day = -1;
    day = (int) strtod(day_string.c_str(), NULL);
    // printf("GET_DATE_STR: |%s| mon |%s|, day %d\n", date_str.c_str(), month_str.c_str(),day);
    if (day < 0) {
      return -1;
    }
    else {
      return 100*get_month_from_name(month_str) + day;
    }
  }
  static int get_epi_week() { return Date::epi_week; }
  static int get_epi_week(int sim_day) { return sim_start_index+sim_day < Date::max_days ? Date::date[sim_start_index+sim_day].epi_week : -1; }
  static int get_epi_year() { return Date::epi_year; }
  static int get_epi_year(int sim_day) { return sim_start_index+sim_day < Date::max_days ? Date::date[sim_start_index+sim_day].epi_year : -1; }
  static bool is_weekend() { 
    int day = get_day_of_week(); 
    return (day == Date::SATURDAY || day == Date::SUNDAY);
  }
  static bool is_weekend(int sim_day) { 
    int day = get_day_of_week(sim_day); 
    return (day == Date::SATURDAY || day == Date::SUNDAY);
  }
  static bool is_weekday() { return (is_weekend() == false); }
  static bool is_weekday(int sim_day) { return (is_weekend(sim_day) == false); }
  static bool is_leap_year() { return is_leap_year(year); }
  static bool is_leap_year(int year);
  static string get_date_string() {
    char str[12];
    sprintf(str,"%04d-%02d-%02d", Date::year, Date::month, Date::day_of_month);
    std::string cppstr(str);
    return cppstr;
  }
  static string get_date_string(int sim_day) {
    char str[12];
    sprintf(str,"%04d-%02d-%02d", get_year(sim_day), get_month(sim_day), get_day_of_month(sim_day));
    std::string cppstr(str);
    return cppstr;
  }
  static string get_day_of_week_string() {
    return day_of_week_string[get_day_of_week()];
  }
  static int get_sim_day(int y, int m, int d);
  static int get_sim_day(string date_str);

  static int get_month_from_name(string name) {
    auto search = Date::month_map.find(name);
    if (search != Date::month_map.end()) {
      return search->second;
    }
    return 0;
  }
  static string get_12hr_clock() {
    return get_12hr_clock(Global::Simulation_Hour);
  }
  static string get_12hr_clock(int hour) {
    char oclock [5];
    hour = hour % 24;
    if (hour == 0) {
      sprintf(oclock, "12am");
    }
    else if (hour == 12) {
      sprintf(oclock, "12pm");
    }
    else if (hour < 12) {
      sprintf(oclock, "%dam", hour);
    }
    else {
      sprintf(oclock, "%dpm", hour-12);
    }
    return string(oclock);
  }

private:
  static int year;
  static int month;
  static int day_of_month;
  static int day_of_week;
  static int day_of_year;
  static int epi_week;
  static int epi_year;
  static int today;	// index of Global::Simulation_Day in date array
  static int sim_start_index; // index of Global::Simulation_Day=0 in date array
  static int max_days;
  static date_t * date;
  static std::map<std::string,int> month_map;

  // names of days of week
  static const string day_of_week_string[7];

  // static const int EPOCH_START_YEAR = 1700;
  static const int day_table[2][13];
  static const int doomsday_month_val[2][13];

  static int get_doomsday_month(int month, int year) {
    return Date::doomsday_month_val[(Date::is_leap_year(year) ? 1 : 0)][month];
  }
  static int get_doomsday_century(int year);
  static int get_day_of_week(int year, int month, int day_of_month);
  static int get_days_in_month(int month, int year);
};

#endif /* DATE_H_ */
