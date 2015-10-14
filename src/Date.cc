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
// File: Date.cc
//

#include "Date.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


const int Date::day_table[2][13] = {
  {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
  {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

const int Date::doomsday_month_val[2][13] = {
  {0, 31, 28, 7, 4, 9, 6, 11, 8, 5, 10, 7, 12},
  {0, 32, 29, 7, 4, 9, 6, 11, 8, 5, 10, 7, 12}};

const string Date::day_of_week_string[7] = {
  "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};

int Date::year = 0;
int Date::month = 0;
int Date::day_of_month = 0;
int Date::day_of_week = 0;
int Date::day_of_year = 0;
int Date::epi_week = 0;
int Date::epi_year = 0;
int Date::today = 0;
int Date::sim_start_index = 0;
date_t * Date::date = NULL;

bool Date::is_leap_year(int year) {
  if (year%400 == 0)
    return true;
  if (year%100 == 0)
    return false;
  if (year%4 == 0)
    return true;
  return false;
}

int Date::get_doomsday_century(int year) {
  int century = year - (year % 100);
  int r = -1;
  switch (century % 400) {
  case 0:
    r = 2;
    break;
  case 100:
    r = 0;
    break;
  case 200:
    r = 5;
    break;
  case 300:
    r = 3;
    break;
  }
  return r;
}

int Date::get_days_in_month(int month, int year) {
  return Date::day_table[(Date::is_leap_year(year) ? 1 : 0)][month];
}

int Date::get_day_of_week(int year, int month, int day_of_month) {
  int x = 0, y = 0;
  int weekday = -1;
  int ddcentury = -1;
  int ddmonth = Date::get_doomsday_month(month, year);
  int century = year - (year % 100);

  ddcentury = Date::get_doomsday_century(year);

  if (ddcentury < 0) return -1;
  if (ddmonth < 0) return -1;
  if (ddmonth > day_of_month) {
    weekday = (7 - ((ddmonth - day_of_month) % 7 ) + ddmonth);
  } else {
    weekday = day_of_month;
  }

  x = (weekday - ddmonth);
  x %= 7;
  y = ddcentury + (year - century) + (floor((year - century)/4));
  y %= 7;
  weekday = (x + y) % 7;

  return weekday;
}

void Date::setup_dates(char * date_string) {
  // extract date from date string
  int _year;
  int _month;
  int _day_of_month;
  if (sscanf(date_string,"%d-%d-%d", &_year, &_month, &_day_of_month) != 3) {
    printf("setup_dates cannot parse date string %s\n", date_string);
    abort();
  }
  
  date = new date_t [ MAX_DATES ];
  int epoch_year = _year - 125;
  date[0].year = epoch_year;
  date[0].month = 1;
  date[0].day_of_month = 1;
  date[0].day_of_year = 1;

  // assign the right epi week number:
  int jan_1_day_of_week = Date::get_day_of_week(epoch_year,1,1);
  int dec_31_day_of_week = (jan_1_day_of_week + (Date::is_leap_year(epoch_year)? 365 : 364)) % 7;
  bool short_week;
  if (jan_1_day_of_week < 3) {
    date[0].epi_week = 1;
    date[0].epi_year = epoch_year;
    short_week = false;
  }
  else {
    date[0].epi_week = 52;
    date[0].epi_year = epoch_year-1;
    short_week = true;
  }

  for (int i = 0; i < MAX_DATES-1; i++) {
    int new_year = date[i].year;
    int new_month = date[i].month;
    int new_day_of_month = date[i].day_of_month + 1;
    int new_day_of_year = date[i].day_of_year + 1;
    int new_day_of_week = (date[i].day_of_week + 1) % 7;
    if (new_day_of_month > Date::get_days_in_month(new_month,new_year)) {
      new_day_of_month = 1;
      if (new_month < 12) {
	new_month++;
      }
      else {
	new_year++;
	new_month = 1;
	new_day_of_year = 1;
      }
    }
    date[i+1].year = new_year;
    date[i+1].month = new_month;
    date[i+1].day_of_month = new_day_of_month;
    date[i+1].day_of_year = new_day_of_year;
    date[i+1].day_of_week = new_day_of_week;

    // set epi_week and epi_year
    if (new_month == 1 && new_day_of_month == 1) {
      jan_1_day_of_week = new_day_of_week;
      dec_31_day_of_week = (jan_1_day_of_week + (Date::is_leap_year(new_year)? 365 : 364)) % 7;
      if (jan_1_day_of_week <= 3) {
	date[i+1].epi_week = 1;
	date[i+1].epi_year = new_year;
	short_week = false;
      }
      else {
	date[i+1].epi_week = date[i].epi_week;
	date[i+1].epi_year = date[i].epi_year;
	short_week = true;
      }
    }
    else {
      if ((new_month == 1) && short_week && (new_day_of_month <= 7 - jan_1_day_of_week)) {
	date[i+1].epi_week = date[i].epi_week;
	date[i+1].epi_year = date[i].epi_year;
      }
      else {
	if ((new_month == 12) &&
	    (dec_31_day_of_week < 3) && 
	    (31 - dec_31_day_of_week) <= new_day_of_month) {
	  date[i+1].epi_week = 1;
	  date[i+1].epi_year = new_year+1;
	}
	else {
	  date[i+1].epi_week = (short_week ? 0 : 1) + (jan_1_day_of_week + new_day_of_year - 1) / 7;
	  date[i+1].epi_year = new_year;
	}
      }
    }


    // set offset 
    if (date[i].year == _year && date[i].month == _month && date[i].day_of_month == _day_of_month) {
      today = i;
    }
  }
  year = date[today].year;
  month = date[today].month;
  day_of_month = date[today].day_of_month;
  day_of_week = date[today].day_of_week;
  day_of_year = date[today].day_of_year;
  epi_week = date[today].epi_week;
  epi_year = date[today].epi_year;
  sim_start_index = today;
}

int Date::get_sim_day(int y, int m, int d) {
  if ((Date::is_leap_year(y)==false) && m == 2 && d == 29) {
    d = 28;
  }
  int yr = Date::get_year(0);
  int day = (y-yr)*365;
  while (Date::get_year(day) < y) { 
    day += 365 ;
  }
  while (Date::get_month(day) < m) {
    day -= Date::get_days_in_month(m,y);
  }
  while (Date::get_month(day) > m) {
    day -= Date::get_days_in_month(m,y);
  }
  day += d-Date::get_day_of_month(day);
  printf("%d-%02d-%02d %d=%02d-%02d\n",y,m,d,Date::get_year(day),Date::get_month(day),Date::get_day_of_month(day)); fflush(stdout);
  return day;
}
