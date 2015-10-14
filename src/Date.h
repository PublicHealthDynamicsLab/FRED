/*
  This file is part of the FRED system.

  Copyright (c) 2010-2015, University of Pittsburgh, John Grefenstette,
  Shawn Brown, Roni Rosenfield, Alona Fyshe, David Galloway, Nathan
  Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File: Date.h
//

#ifndef DATE_H_
#define DATE_H_

#include <string>
#include <cstdio>

#define MAX_DATES (250*366)

using namespace std;

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

  /**
   * Default constructor
   */
  Date(){};
  ~Date(){};

  static void setup_dates(char * date_string);
  static void update() {
    today++; 
    year = date[today].year;
    month = date[today].month;
    day_of_month = date[today].day_of_month;
    day_of_week = date[today].day_of_week;
    day_of_year = date[today].day_of_year;
    epi_week = date[today].epi_week;
    epi_year = date[today].epi_year;
  }
  static int get_year() { return year; }
  static int get_year(int sim_day) { return date[sim_start_index+sim_day].year; }
  static int get_month() { return month; }
  static int get_month(int sim_day) { return date[sim_start_index+sim_day].month; }
  static int get_day_of_month() { return day_of_month; }
  static int get_day_of_month(int sim_day) { return date[sim_start_index+sim_day].day_of_month; }
  static int get_day_of_week() { return day_of_week; }
  static int get_day_of_week(int sim_day) { return date[sim_start_index+sim_day].day_of_week; }
  static int get_day_of_year() { return day_of_year; }
  static int get_day_of_year(int sim_day) { return date[sim_start_index+sim_day].day_of_year; }
  static int get_epi_week() { return epi_week; }
  static int get_epi_week(int sim_day) { return date[sim_start_index+sim_day].epi_week; }
  static int get_epi_year() { return epi_year; }
  static int get_epi_year(int sim_day) { return date[sim_start_index+sim_day].epi_year; }
  static bool is_weekend() { 
    int day = get_day_of_week(); 
    return (day == SATURDAY || day == SUNDAY);
  }
  static bool is_weekend(int sim_day) { 
    int day = get_day_of_week(sim_day); 
    return (day == SATURDAY || day == SUNDAY);
  }
  static bool is_weekday() { return (is_weekend() == false); }
  static bool is_weekday(int sim_day) { return (is_weekend(sim_day) == false); }
  static bool is_leap_year() { return is_leap_year(year); }
  static bool is_leap_year(int year);
  static string get_date_string() {
    char str[12];
    sprintf(str,"%04d-%02d-%02d", year, month, day_of_month);
    std::string cppstr(str);
    return cppstr;
  }
  static string get_day_of_week_string() {
    return day_of_week_string[day_of_week];
  }
  static int get_sim_day(int y, int m, int d);

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
  static date_t * date;

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
