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
// File: Date.cc
//

#include "Date.h"
#include "Global.h"
#include "Property.h"
#include "Utils.h"
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

std::map<std::string,int> Date::month_map = {
  {"Jan", 1}, {"Feb", 2}, {"Mar", 3}, {"Apr", 4},
  {"May", 5}, {"Jun", 6}, {"Jul", 7}, {"Aug", 8},
  {"Sep", 9}, {"Oct", 10}, {"Nov", 11}, {"Dec", 12}
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
int Date::max_days = 0;
char Date::Start_date[FRED_STRING_SIZE];
char Date::End_date[FRED_STRING_SIZE];
date_t * Date::date = NULL;

Date::Date() {
}

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

void Date::setup_dates() {

  char msg[FRED_STRING_SIZE];

  Property::disable_abort_on_failure();
  strcpy(Date::Start_date,"");
  Property::get_property("start_date", Date::Start_date);
  strcpy(Date::End_date,"");
  Property::get_property("end_date", Date::End_date);
  int set_days = 0;
  Property::get_property("days", &set_days);
  Property::set_abort_on_failure();


  // extract date from date strings
  int start_year = 0;
  int start_month = 0;
  int start_day_of_month = 0;
  if (sscanf(Date::Start_date,"%d-%d-%d", &start_year, &start_month, &start_day_of_month) != 3) {
    char tmp[8];
    char mon_str[8];
    if (sscanf(Date::Start_date,"%d-%s", &start_year, tmp)==2) {
      strncpy(mon_str,tmp,3);
      mon_str[3] = '\0';
      start_month = Date::month_map[string(mon_str)];
      sscanf(&tmp[4], "%d", &start_day_of_month);
      // FRED_VERBOSE(0,"START_DATE = |%d| |%s| |%02d|\n", start_year, mon_str, start_day_of_month);
    }
    else {
      sprintf(msg, "Bad start_date = '%s'", Date::Start_date);
      Utils::print_error(msg);
      return;
    }
  }
  if (start_year < 1900 || start_year > 2200
      || start_month < 1 || start_month > 12
      || start_day_of_month < 1 || start_day_of_month > get_days_in_month(start_month, start_year)) {
    sprintf(msg, "Bad start_date = '%s'", Date::Start_date);
    Utils::print_error(msg);
    return;
  }
  // FRED_VERBOSE(0,"START_DATE = |%d| |%02d| |%02d|\n", start_year, start_month, start_day_of_month);

  int end_year = 0;
  int end_month = 0;
  int end_day_of_month = 0;
  if (set_days == 0) {
    if (sscanf(Date::End_date,"%d-%d-%d", &end_year, &end_month, &end_day_of_month) != 3) {
      char tmp[8];
      char mon_str[8];
      if (sscanf(Date::End_date,"%d-%s", &end_year, tmp)==2) {
	strncpy(mon_str,tmp,3);
	mon_str[3] = '\0';
	end_month = Date::month_map[string(mon_str)];
	sscanf(&tmp[4], "%d", &end_day_of_month);
      }
      else {
	sprintf(msg, "Bad end_date = '%s'", Date::End_date);
	Utils::print_error(msg);
	return;
      }
    }
    if (end_year < 1900 || end_year > 2200
	|| end_month < 1 || end_month > 12
	|| end_day_of_month < 1 || end_day_of_month > get_days_in_month(end_month, end_year)) {
      sprintf(msg, "Bad end_date = '%s'", Date::End_date);
      Utils::print_error(msg);
      return;
    }
    // printf("END_DATE = |%d| |%02d| |%02d|\n", end_year, end_month, end_day_of_month);

    int start_date_int = 10000 * start_year + 100 * start_month + start_day_of_month;
    int end_date_int = 10000 * end_year + 100 * end_month + end_day_of_month;
    if (end_date_int < start_date_int) {
      sprintf(msg, "end_date %s is before start date %s", Date::End_date, Date::Start_date);
      Utils::print_error(msg);
      return;
    }
  }
  else {
    end_year = start_year + 1 + (set_days / 365);
  }

  int epoch_year = start_year - 120;
  int max_years = end_year - epoch_year + 1;
  Date::max_days = 366*max_years;
  // FRED_VERBOSE(0,"MAX YEARS %d MAX DAYS %d\n", max_years, Date::max_days);

  Date::date = new date_t [ Date::max_days ];

  Date::date[0].year = epoch_year;
  Date::date[0].month = 1;
  Date::date[0].day_of_month = 1;
  Date::date[0].day_of_year = 1;

  /*
  printf("START_DATE = |%d| |%02d| |%02d|\n", start_year, start_month, start_day_of_month);
  printf("END_DATE = |%d| |%02d| |%02d|\n", end_year, end_month, end_day_of_month);
  printf("DATE:date[0] = |%d| |%02d| |%02d|\n", Date::date[0].year, Date::date[0].month, Date::date[0].day_of_month);
  exit(0);
  */

  // assign the right epi week number:
  int jan_1_day_of_week = Date::get_day_of_week(epoch_year,1,1);
  Date::date[0].day_of_week = jan_1_day_of_week;
  int dec_31_day_of_week = (jan_1_day_of_week + (Date::is_leap_year(epoch_year)? 365 : 364)) % 7;
  bool short_week;
  if (jan_1_day_of_week < 3) {
    Date::date[0].epi_week = 1;
    Date::date[0].epi_year = epoch_year;
    short_week = false;
  }
  else {
    Date::date[0].epi_week = 52;
    Date::date[0].epi_year = epoch_year-1;
    short_week = true;
  }

  for (int i = 0; i < Date::max_days; i++) {
    // printf("setup Date::date[%d]\n", i); fflush(stdout);
    int new_year = Date::date[i].year;
    int new_month = Date::date[i].month;
    int new_day_of_month = Date::date[i].day_of_month + 1;
    int new_day_of_year = Date::date[i].day_of_year + 1;
    int new_day_of_week = (Date::date[i].day_of_week + 1) % 7;
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
    Date::date[i+1].year = new_year;
    Date::date[i+1].month = new_month;
    Date::date[i+1].day_of_month = new_day_of_month;
    Date::date[i+1].day_of_year = new_day_of_year;
    Date::date[i+1].day_of_week = new_day_of_week;

    // set epi_week and epi_year
    if (new_month == 1 && new_day_of_month == 1) {
      jan_1_day_of_week = new_day_of_week;
      dec_31_day_of_week = (jan_1_day_of_week + (Date::is_leap_year(new_year)? 365 : 364)) % 7;
      if (jan_1_day_of_week <= 3) {
	Date::date[i+1].epi_week = 1;
	Date::date[i+1].epi_year = new_year;
	short_week = false;
      }
      else {
	Date::date[i+1].epi_week = Date::date[i].epi_week;
	Date::date[i+1].epi_year = Date::date[i].epi_year;
	short_week = true;
      }
    }
    else {
      if ((new_month == 1) && short_week && (new_day_of_month <= 7 - jan_1_day_of_week)) {
	Date::date[i+1].epi_week = Date::date[i].epi_week;
	Date::date[i+1].epi_year = Date::date[i].epi_year;
      }
      else {
	if ((new_month == 12) &&
	    (dec_31_day_of_week < 3) && 
	    (31 - dec_31_day_of_week) <= new_day_of_month) {
	  Date::date[i+1].epi_week = 1;
	  Date::date[i+1].epi_year = new_year+1;
	}
	else {
	  Date::date[i+1].epi_week = (short_week ? 0 : 1) + (jan_1_day_of_week + new_day_of_year - 1) / 7;
	  Date::date[i+1].epi_year = new_year;
	}
      }
    }

    // set offset 
    if (Date::date[i].year == start_year && Date::date[i].month == start_month && Date::date[i].day_of_month == start_day_of_month) {
      Date::today = i;
      // printf("today = %d\n", Date::today); fflush(stdout);
    }

    if (set_days==0 && Date::date[i].year == end_year && Date::date[i].month == end_month && Date::date[i].day_of_month == end_day_of_month) {
      Global::Simulation_Days = i - Date::today +1;
      // printf("Days = %d\n", Global::Simulation_Days); fflush(stdout);
    }
  }
  if (set_days > 0) {
    Global::Simulation_Days = set_days;
    // printf("Days = %d\n", Global::Simulation_Days); fflush(stdout);
  }

  Date::year = Date::date[Date::today].year;
  Date::month = Date::date[Date::today].month;
  Date::day_of_month = Date::date[Date::today].day_of_month;
  Date::day_of_week = Date::date[Date::today].day_of_week;
  Date::day_of_year = Date::date[Date::today].day_of_year;
  Date::epi_week = Date::date[Date::today].epi_week;
  Date::epi_year = Date::date[Date::today].epi_year;
  Date::sim_start_index = Date::today;

}

int Date::get_sim_day(int y, int m, int d) {
  int result = 999999;
  int maxy = Date::date[Date::max_days-1].year;
  int maxm = Date::date[Date::max_days-1].month;
  int maxd = Date::date[Date::max_days-1].day_of_month;

  if ((Date::is_leap_year(y)==false) && m == 2 && d == 29) {
    d = 28;
  }

  if (maxy < y) {
    return result;
  }

  if (maxy == y && maxm < m) {
    return result;
  }

  if (maxy == y && maxm == m && maxd < d) {
    return result;
  }

  // binary search
  int low = 0;
  int high = Date::max_days-1;
  int pos = (high + low) / 2;
  while (Date::date[pos].year != y || Date::date[pos].month != m || Date::date[pos].day_of_month != d) {
    if (Date::date[pos].year < y || (Date::date[pos].year == y && (Date::date[pos].month < m || (Date::date[pos].month == m && Date::date[pos].day_of_month < d)))) {
      // FRED_VERBOSE(0, "y %d m %d d %d date[%d] = %d %d %d\n", y,m,d,pos,Date::date[pos].year,Date::date[pos].month,Date::date[pos].day_of_month);
      low = pos;
      pos = (high+low) / 2;
    }
    else {
      // FRED_VERBOSE(0, "y %d m %d d %d date[%d] = %d %d %d\n", y,m,d,pos,Date::date[pos].year,Date::date[pos].month,Date::date[pos].day_of_month);
      high = pos;
      pos = (high+low) / 2;
    }
  }
  result = pos - Date::sim_start_index;
  // FRED_VERBOSE(0, "SUCCESS y %d m %d d %d date[%d] = %d %d %d result %d\n", y,m,d,pos,Date::date[pos].year,Date::date[pos].month,Date::date[pos].day_of_month, result);
  return result;
}


int Date::get_sim_day(string date_str) {
  int y, m, d;
  if (date_str.substr(4,1)=="-") {
    y = strtod(date_str.substr(0,4).c_str(), NULL);
    m = Date::get_month_from_name(date_str.substr(5,3));
    d = strtod(date_str.substr(9).c_str(), NULL);
    printf("1: y %d m %d d %d\n",y,m,d);fflush(stdout);
    return Date::get_sim_day(y,m,d);
  }
  if (date_str.substr(3,1)=="-") {
    m = Date::get_month_from_name(date_str.substr(0,3));
    d = strtod(date_str.substr(4).c_str(), NULL);
    y = Date::get_year();
    int today = Date::get_date_code(Date::get_month(),Date::get_day_of_month());
    if (Date::get_date_code(m,d) < today) {
      y++;
    }
    // printf("2: |%s| y %d m %d d %d today %d date_code %d\n",date_str.c_str(),y,m,d,today, Date::get_date_code(m,d));fflush(stdout);
    return Date::get_sim_day(y,m,d);
  }
  return -1;
}
