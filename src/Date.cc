/*
 Copyright 2011 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Date.cc
//

#include "Date.h"
#include "Utils.h"
#include "Global.h"
#include <time.h>
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <math.h>
#include <string>
#include <cstdlib>
#include <cmath>
#include <string.h>

using namespace std;

const int Date::day_table[2][13] = {
    {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};

const int Date::doomsday_month_val[2][13] = {
    {0, 31, 28, 7, 4, 9, 6, 11, 8, 5, 10, 7, 12},
    {0, 32, 29, 7, 4, 9, 6, 11, 8, 5, 10, 7, 12}};

const string Date::MMDDYYYY = string("MM/DD/YYYY");
const string Date::MMDDYY = string("MM/DD/YY");
const string Date::DDMMYYYY = string("DD/MM/YYYY");
const string Date::DDMMYY = string("DD/MM/YY");
const string Date::YYYYMMDD = string("YYYY-MM-DD");

bool Date::is_initialized = false;
vector<int> * Date::day_of_month_vec = NULL;
vector<int> * Date::month_vec = NULL;
vector<int> * Date::year_vec = NULL;

Date::Date() {

  this->days_since_jan_1_epoch_year = 0;

  //Create the vectors once
  if(!Date::is_initialized) {
    Date::initialize_vectors();
  }

  //Get the system time
  time_t rawtime;
  time(&rawtime);

  int month, day, year;
  struct tm * curr_time = localtime(&rawtime);

  /*
   * Meaning of values in structure tm:
   * tm_mday day of the month  1-31
   * tm_mon  months since January  0-11
   * tm_year years since 1900
   */
  month = curr_time->tm_mon + 1;
  day = curr_time->tm_mday;
  year = curr_time->tm_year + 1900;

  this->set_date(year, month, day);

}

Date::Date(string date_string, string format_string) {

  this->days_since_jan_1_epoch_year = 0;

  //Create the vectors once
  if(!Date::is_initialized) {
    Date::initialize_vectors();
  }

  int year = Date::parse_year_from_date_string(date_string, format_string);
  int month = Date::parse_month_from_date_string(date_string, format_string);
  int day_of_month = Date::parse_day_of_month_from_date_string(date_string, format_string);

  printf("year = %d, month = %d, day_of_month = %d\n", year, month, day_of_month);

  this->set_date(year, month, day_of_month);
  printf("Date = [%s]\n", this->to_string().c_str());
}

Date::Date(int year, int day_of_year) {

  this->days_since_jan_1_epoch_year = 0;

  //Create the vectors once
  if(!Date::is_initialized) {
    Date::initialize_vectors();
  }

  //Find month and day
  int month = 0;
  int day_of_month = 0;
  bool is_found = false;
  int days_so_far = 0;

  for (int i = Date::JANUARY; i <= Date::DECEMBER && !is_found; i++) {
    days_so_far += Date::day_table[Date::is_leap_year(year)][i];
    if (day_of_year <= days_so_far ) {
      month = i;
      int days_to_subtract = 0;
      for (int j = Date::JANUARY; j < month; j++) {
        //std::cout << "day table=>" << Date::day_table[Date::is_leap_year(year)][j] << std::endl;
        days_to_subtract += Date::day_table[Date::is_leap_year(year)][j];
      }
      day_of_month = day_of_year - days_to_subtract;
      is_found = true;
    }
  }

  if(is_found) {
    this->set_date(year, month, day_of_month);
  } else {
    this->set_date(year, Date::JANUARY, 1);
  }

}

Date::Date(int year, int month, int day_of_month) {

  this->days_since_jan_1_epoch_year = 0;

  //Create the vectors once
  if(!Date::is_initialized) {
    Date::initialize_vectors();
  }

  this->set_date(year, month, day_of_month);
}

/**
 * Sets the month, day, and year to the values.  The values will be set
 * in the order Year, Month, then Day.  So, setting Feb, 29, 2009 will
 * result in an error, since the day is outside of the range for that
 * month in that year.
 */
void Date::set_date(int year, int month, int day_of_month) {
  if (year < Date::EPOCH_START_YEAR) {
    Utils::fred_abort("Help! \"%d\" is an invalid year! Years prior to the EPOCH_START_YEAR, %d, are not recognized!\n", year, Date::EPOCH_START_YEAR);
  } else {
    this->year = year;
  }

  if (month < Date::JANUARY || month > Date::DECEMBER) {
    Utils::fred_abort("Help!  Month must be between 1 and 12 inclusive.\n");
  } else {
    this->month = month;
  }

  if (day_of_month < 1 || day_of_month > Date::day_table[(Date::is_leap_year(year) ? 1 : 0)][month]) {
    Utils::fred_abort("Help!  Day of month is out of range for the given month.\n");
  } else {
    this->day_of_month = day_of_month;
  }

  //total years since Date::EPOCH_START_YEAR
  int temp_days = (year - Date::EPOCH_START_YEAR) * 365;

  //add in the leap days
  for (int i = Date::EPOCH_START_YEAR; i < year; i += 4) {
    if (i % 4 == 0)
      temp_days++;
  }

  //subtract the leap days of centuries
  for (int i = Date::EPOCH_START_YEAR; i < year; i += 100) {
    if (i % 100 == 0)
      temp_days--;
  }

  //add in the leap days for centuries divisible by 400
  for (int i = Date::EPOCH_START_YEAR; i < year; i += 100) {
    if (i % 400 == 0)
      temp_days++;
  }

  this->day_of_year = 0;

  //add in the days of the current year (up to the current month)
  for (int i = Date::JANUARY; i < month; i++) {
    temp_days += Date::day_table[(Date::is_leap_year(year) ? 1 : 0)][i];
    this->day_of_year += Date::day_table[(Date::is_leap_year(year) ? 1 : 0)][i];
  }

  //add in the days of the current month
  temp_days += (day_of_month - 1);
  this->day_of_year += (day_of_month);

  this->day_of_week = Date::get_day_of_week(this->year, this->month, this->day_of_month);

  this->days_since_jan_1_epoch_year = temp_days;

}

void Date::advance() {
  this->days_since_jan_1_epoch_year++;

  this->year = Date::year_vec->at(this->days_since_jan_1_epoch_year);
  this->month = Date::month_vec->at(this->days_since_jan_1_epoch_year);
  this->day_of_month = Date::day_of_month_vec->at(this->days_since_jan_1_epoch_year);

  this->day_of_year = Date::get_day_of_year(this->year, this->month, this->day_of_month);
  if(this->day_of_week == Date::SATURDAY) {
  this->day_of_week = Date::SUNDAY;
  } else {
  this->day_of_week++;
  }
}


void Date::advance(int days){
  for(int i = 0; i < days; ++i) {
  this->advance();
  }
}

int Date::get_year(int t) {

  if(t == 0) {
  return this->year;
  } else {
  if(Date::year_vec != NULL && (this->days_since_jan_1_epoch_year + t) < (int)Date::year_vec->size()) {
      int * temp = & Date::year_vec->at(this->days_since_jan_1_epoch_year + t);
      return * temp;
    } else {
      Date::add_to_vectors(this->days_since_jan_1_epoch_year + t);

      int * temp = & Date::year_vec->at(this->days_since_jan_1_epoch_year + t);
      return * temp;
    }
  }

}

int Date::get_month(int t) {

  if (t == 0) {
  return this->month;
  } else{
  if(Date::month_vec != NULL && (this->days_since_jan_1_epoch_year + t) < (int)Date::month_vec->size()) {
    int * temp = & Date::month_vec->at(this->days_since_jan_1_epoch_year + t);
    return * temp;
  } else {
    Date::add_to_vectors(this->days_since_jan_1_epoch_year + t);

    int * temp = & Date::month_vec->at(this->days_since_jan_1_epoch_year + t);
    return * temp;
  }
  }
}

string Date::get_month_string() {
  int temp_month = this->month;

  switch(temp_month) {
    case Date::JANUARY:
      return "Jan";
    case Date::FEBRUARY:
      return "Feb";
    case Date::MARCH:
      return "Mar";
    case Date::APRIL:
      return "Apr";
    case Date::MAY:
      return "May";
    case Date::JUNE:
      return "Jun";
    case Date::JULY:
      return "Jul";
    case Date::AUGUST:
      return "Aug";
    case Date::SEPTEMBER:
      return "Sep";
    case Date::OCTOBER:
      return "Oct";
    case Date::NOVEMBER:
      return "Nov";
    case Date::DECEMBER:
      return "Dec";
  }

  return "INVALID MONTH";
}

string Date::get_month_string(int t) {
  int temp_month = 0;
  if(t == 0)
  temp_month = this->month;
  else
  temp_month = this->get_month(t);

  switch(temp_month) {
    case Date::JANUARY:
      return "Jan";
    case Date::FEBRUARY:
      return "Feb";
    case Date::MARCH:
      return "Mar";
    case Date::APRIL:
      return "Apr";
    case Date::MAY:
      return "May";
    case Date::JUNE:
      return "Jun";
    case Date::JULY:
      return "Jul";
    case Date::AUGUST:
      return "Aug";
    case Date::SEPTEMBER:
      return "Sep";
    case Date::OCTOBER:
      return "Oct";
    case Date::NOVEMBER:
      return "Nov";
    case Date::DECEMBER:
      return "Dec";
  }

  return "INVALID MONTH";
}

int Date::get_day_of_month(int t) {

  if(t == 0) {
  return this->day_of_month;
  } else {
    if(Date::day_of_month_vec != NULL && (this->days_since_jan_1_epoch_year + t) < (int)Date::day_of_month_vec->size()) {
      int * temp = & Date::day_of_month_vec->at(this->days_since_jan_1_epoch_year + t);
      return * temp;
    } else {
      Date::add_to_vectors(this->days_since_jan_1_epoch_year + t);

      int * temp = & Date::day_of_month_vec->at(this->days_since_jan_1_epoch_year + t);
      return * temp;
    }
  }
}

int Date::get_day_of_year(int t) {

  if(t == 0) {
  return Date::get_day_of_year(this->year, this->month, this->day_of_month);
  } else {
  return Date::get_day_of_year(this->get_year(t), this->get_month(t), this->get_day_of_month(t));
  }
}

int Date::get_day_of_week(int t) {
  int year, month, day_of_month;

  if(t == 0) {
    year = this->year;
  month = this->month;
    day_of_month = this->day_of_month;
  } else {
    year = this->get_year(t);
    month = this->get_month(t);
    day_of_month = this->get_day_of_month(t);
  }

  return Date::get_day_of_week(year, month, day_of_month);

}

string Date::get_day_of_week_string(int t) {
  int temp_day_of_week = 0;
  if(t == 0)
  temp_day_of_week = this->day_of_week;
  else
  temp_day_of_week = this->get_day_of_week(t);

  switch(temp_day_of_week) {
    case Date::SUNDAY:
      return "Sun";
    case Date::MONDAY:
      return "Mon";
    case Date::TUESDAY:
      return "Tue";
    case Date::WEDNESDAY:
      return "Wed";
    case Date::THURSDAY:
      return "Thu";
    case Date::FRIDAY:
      return "Fri";
    case Date::SATURDAY:
      return "Sat";
  }

  return "INVALID DAY OF WEEK";
}

/**
 *  For week and week year, we are using the CDC's definition:
 *
 *  The first day of any MMWR week is Sunday.  MMWR week numbering is
 *  sequential beginning with 1 and incrementing with each week to a
 *  maximum of 52 or 53.  MMWR week #1 of an MMWR year is the first week
 *  of the year that has at least four days in the calendar year.
 *  For example, if January 1 occurs on a Sunday, Monday, Tuesday or
 *  Wednesday, the calendar week that includes January 1 would be MMWR
 *  week #1.  If January 1 occurs on a Thursday, Friday, or Saturday,
 *  the calendar week that includes January 1 would be the last MMWR
 *  week of the previous year (#52 or #53).  Because of this rule,
 *  December 29, 30, and 31 could potentially fall into MMWR week #1
 *  of the following MMWR year.
 */
int Date::get_epi_week(int t) {

  int ret_value = -1;

  //Get the future date info
  int future_year = 0;
  int future_month = 0;
  int future_day_of_month = 0;
  int future_day_of_year = 0;

  if(t == 0) {
  future_year = this->year;
  future_month = this->month;
  future_day_of_month = this->day_of_month;
  } else {
  future_year = this->get_year(t);
  future_month = this->get_month(t);
  future_day_of_month = this->get_day_of_month(t);
  }

  future_day_of_year = Date::get_day_of_year(future_year, future_month, future_day_of_month);

  //Figure out on which day of the week Jan 1 occurs for the future date
  int jan_1_day_of_week = this->get_day_of_week((t - (future_day_of_year - 1)));
  int dec_31_day_of_week = this->get_day_of_week(t - (future_day_of_year - 1) + (Date::is_leap_year(future_year) ? 365 : 364));

  if (future_month == Date::DECEMBER && dec_31_day_of_week < Date::WEDNESDAY &&
      future_day_of_month >= (31 - dec_31_day_of_week)) {
    return 1;
  } else {

    int epi_week = (future_day_of_year + jan_1_day_of_week) / 7;
    if (((future_day_of_year + jan_1_day_of_week) % 7) > 0) epi_week++;

    if (jan_1_day_of_week > Date::WEDNESDAY) {
      epi_week--;
      if (epi_week < 1) {
        //Create a new date that represents the last day of the previous year
        Date * tmp_date = new Date(future_year - 1, Date::DECEMBER, 31);
        epi_week = tmp_date->get_epi_week();
        delete tmp_date;

        return epi_week;
      }
    }

    return epi_week;

  }

  return ret_value;
}

int Date::get_epi_week_year(int t) {

  //Get the future date info
  int future_year = 0;
  int future_month = 0;
  int future_day_of_month = 0;
  int future_day_of_year = 0;

  if(t == 0) {
    future_year = this->year;
    future_month = this->month;
    future_day_of_month = this->day_of_month;
  } else {
    future_year = this->get_year(t);
    future_month = this->get_month(t);
    future_day_of_month = this->get_day_of_month(t);
  }

  future_day_of_year = Date::get_day_of_year(future_year, future_month, future_day_of_month);

  //Figure out on which day of the week Jan 1 occurs for the future date
  int jan_1_day_of_week = this->get_day_of_week((t - (future_day_of_year - 1)));

  //Figure out on which day of the week Dec 31 occurs for the future date
  int dec_31_day_of_week = this->get_day_of_week(t - (future_day_of_year - 1) + (Date::is_leap_year(future_year) ? 365 : 364));

  if (future_month == Date::DECEMBER && dec_31_day_of_week < 3 &&
      future_day_of_month >= 31 - dec_31_day_of_week) {
    return future_year + 1;
  } else {

    int epi_week = (future_day_of_year + jan_1_day_of_week) / 7;
    if (((future_day_of_year + jan_1_day_of_week) % 7) > 0) epi_week++;

    if(jan_1_day_of_week > Date::WEDNESDAY) {
      epi_week--;
      if(epi_week < 1) {
        return future_year - 1;
      }
    }
  }

  return future_year;
}

Date * Date::clone() {
  return new Date(this->year, this->month, this->day_of_month);
}

bool Date::equals(Date * check_date) {
  return (this->compare_to(check_date) == 0);
}

int Date::compare_to(Date * check_date) {

  return (this->days_since_jan_1_epoch_year - check_date->days_since_jan_1_epoch_year);

}

string Date::to_string() {
  stringstream oss;
  oss << setw(4) << setfill('0') << this->year << "-";
  oss << setw(2) << setfill('0') << this->month << "-";
  oss << setw(2) << setfill('0') << this->day_of_month;
  return oss.str();
}

// member function operators

bool Date::operator< (const Date &other) const {
  return (this->days_since_jan_1_epoch_year < other.days_since_jan_1_epoch_year);
}

bool Date::operator== (const Date &other) const {
  return (this->days_since_jan_1_epoch_year == other.days_since_jan_1_epoch_year);
}

bool Date::operator> (const Date &other) const {
  return (this->days_since_jan_1_epoch_year > other.days_since_jan_1_epoch_year);
}

bool Date::operator>= (const Date &other) const {
  return ( ! (this->operator< (other) ) );
}

bool Date::operator<= (const Date &other) const {
  return ( ! (this->operator> (other) ) );
}

bool Date::operator!= (const Date &other) const {
  return ( ! (this->operator== (other) ) );
}

//Static Methods
int Date::days_between(const Date * date_1, const Date * date_2) {
  return abs(date_1->days_since_jan_1_epoch_year - date_2->days_since_jan_1_epoch_year);
}

int Date::days_between(int sim_day, Date * date_2) {
  return abs(Global::Sim_Current_Date->days_since_jan_1_epoch_year - date_2->days_since_jan_1_epoch_year);
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

int Date::get_day_of_year(int year, int month, int day_of_month) {
  int day_of_year = 0;

  for (int i = 1; i < month; i++) {
    day_of_year += Date::day_table[(Date::is_leap_year(year) ? 1 : 0)][i];
  }

  day_of_year += day_of_month;

  return day_of_year;
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

int Date::parse_month_from_date_string(string date_string, string format_string) {

  string temp_str;

  if (format_string.compare(Date::YYYYMMDD) == 0) {
    size_t pos_1, pos_2;
    pos_1 = date_string.find('-');
    if (pos_1 != string::npos) {
      pos_2 = date_string.find('-', pos_1 + 1);
      if (pos_2 != string::npos) {
        temp_str = date_string.substr(pos_1 + 1, pos_2 - pos_1 - 1);
        int i;
        istringstream my_stream(temp_str);
        if(my_stream >> i)
          return i;
      }
    }
  }
  else if (format_string.compare(Date::MMDDYYYY) == 0 ||
     format_string.compare(Date::MMDDYY) == 0) {
    size_t pos;
    pos = date_string.find('/');
    if (pos != string::npos) {
      temp_str = date_string.substr(0, pos);
      int i;
      istringstream my_stream(temp_str);

      if(my_stream >> i)
        return i;
    }

  } else if (format_string.compare(Date::DDMMYYYY) == 0 ||
            format_string.compare(Date::DDMMYY) == 0) {

    size_t pos_1, pos_2;
    pos_1 = date_string.find('/');
    if (pos_1 != string::npos) {
      pos_2 = date_string.find('/', pos_1 + 1);
      if (pos_2 != string::npos) {
        temp_str = date_string.substr(pos_1 + 1, pos_2 - pos_1 - 1);
        int i;
        istringstream my_stream(temp_str);

        if(my_stream >> i)
          return i;
      }
    }

  } else {
    Utils::fred_abort("Help! Unrecognized date format string [\" %s \"]\n", format_string.c_str()); 
  }

  return -1;
}

int Date::parse_day_of_month_from_date_string(string date_string, string format_string) {

  string temp_str;

  if (format_string.compare(Date::YYYYMMDD) == 0) {
    size_t pos;
    pos = date_string.find('-', date_string.find('-') + 1);
    if (pos != string::npos) {
      temp_str = date_string.substr(pos + 1);
      int i;
      istringstream my_stream(temp_str);
      if (my_stream >> i)
        return i;
    }
  }
  else if (format_string.compare(Date::MMDDYYYY) == 0 ||
      format_string.compare(Date::MMDDYY) == 0) {

    size_t pos_1, pos_2;
    pos_1 = date_string.find('/');

    if (pos_1 != string::npos) {
      pos_2 = date_string.find('/', pos_1 + 1);
      if (pos_2 != string::npos) {
        temp_str = date_string.substr(pos_1 + 1, pos_2 - pos_1 - 1);
        int i;
        istringstream my_stream(temp_str);

        if (my_stream >> i)
          return i;
      }
    }

  } else if (format_string.compare(Date::DDMMYYYY) == 0 ||
             format_string.compare(Date::DDMMYY) == 0) {

    size_t pos;
    pos = date_string.find('/');
    if (pos != string::npos) {
      temp_str = date_string.substr(0, pos);
      int i;

      istringstream my_stream(temp_str);

      if (my_stream >> i)
        return i;
    }

  } else {
    Utils::fred_abort("Help!  Unrecognized date format string [\" %s \"]\n", format_string.c_str()); 
  }

  return -1;

}

int Date::parse_year_from_date_string(string date_string, string format_string) {

  string temp_str;

  if (format_string.compare(Date::YYYYMMDD) == 0) {
    size_t pos;
    pos = date_string.find('-');
    if (pos != string::npos) {
      temp_str = date_string.substr(0, pos);
      int i;
      istringstream my_stream(temp_str);
      if (my_stream >> i)
        return i;
    }
  }
  else if (format_string.compare(Date::MMDDYYYY) == 0 ||
     format_string.compare(Date::DDMMYYYY) == 0 ) {
    size_t pos;
    pos = date_string.find('/', date_string.find('/') + 1);
    if (pos != string::npos) {
      temp_str = date_string.substr(pos + 1);
      int i;
      istringstream my_stream(temp_str);
      if (my_stream >> i)
        return i;
    }
  } else if (format_string.compare(Date::MMDDYY) == 0 ||
             format_string.compare(Date::DDMMYY) == 0) {

    size_t pos;
    pos = date_string.find('/', date_string.find('/') + 1);
    if (pos != string::npos) {
      temp_str = date_string.substr(pos + 1, 2);
      int i;

      istringstream my_stream(temp_str);

      if (my_stream >> i) {
        //70 is the cutoff (i.e any two-digit year greater than or equal to 70 is
        // assumed to be 1900s otherwise, it is 2000s
        i += (i >= 70 ? 1900 : 2000);
        return i;
      }
     }

  } else {
    Utils::fred_abort("Help!  Unrecognized date format string [\" %s \"]\n", format_string.c_str()); 
  }

  return -1;
}

void Date::initialize_vectors() {
  //Get the system time
  time_t rawtime;
  time(&rawtime);

  int cur_year;
  int min_days_to_reserve;
  struct tm * curr_time = localtime(&rawtime);

  /*
   * Meaning of values in structure tm:
   * tm_year years since 1900
   */
  cur_year = curr_time->tm_year + 1900;
  min_days_to_reserve = abs(cur_year - Date::EPOCH_START_YEAR + 10) * 366;


  Date::add_to_vectors(min_days_to_reserve);
  Date::is_initialized = true;

}

void Date::add_to_vectors(int _days_since_jan_1_epoch_year) {

  size_t vec_size = (size_t) (_days_since_jan_1_epoch_year + 1);

  //If the vectors do not exist, create now
  if (Date::year_vec == NULL) {
    Date::year_vec = new vector<int>;
    Date::year_vec->reserve(vec_size);
  }

  if (Date::month_vec == NULL) {
    Date::month_vec = new vector<int>;
    Date::month_vec->reserve(vec_size);
  }

  if (Date::day_of_month_vec == NULL) {
    Date::day_of_month_vec = new vector<int>;
    Date::day_of_month_vec->reserve(vec_size);
  }

  //Check to see if the vector is empty
  size_t current_vec_size = Date::day_of_month_vec->size();
  if (current_vec_size == 0) {
    int current_year = Date::EPOCH_START_YEAR;
    int current_month = Date::JANUARY;
    int current_day_of_month = 1;

    for (int i = 0; i <= _days_since_jan_1_epoch_year; i++) {

      Date::year_vec->push_back(current_year);
      Date::month_vec->push_back(current_month);
      Date::day_of_month_vec->push_back(current_day_of_month);

      if (++current_day_of_month >
        Date::day_table[(Date::is_leap_year(current_year) ? 1 : 0)][current_month]) {
        current_day_of_month = 1;
        if(++current_month > Date::DECEMBER) {
          current_month = Date::JANUARY;
          current_year++;
        }
      }
    }
  } else if (current_vec_size <= (size_t)_days_since_jan_1_epoch_year) {

    int current_year = Date::year_vec->back();
    int current_month = Date::month_vec->back();
    int current_day_of_month = Date::day_of_month_vec->back();

    for (size_t i = 0; i <= ((size_t)_days_since_jan_1_epoch_year - current_vec_size); i++) {

      if (++current_day_of_month >
        Date::day_table[(Date::is_leap_year(current_year) ? 1 : 0)][current_month]) {
        current_day_of_month = 1;
        if(++current_month > Date::DECEMBER) {
          current_month = Date::JANUARY;
          current_year++;
        }
      }

      Date::year_vec->push_back(current_year);
      Date::month_vec->push_back(current_month);
      Date::day_of_month_vec->push_back(current_day_of_month);
    }
  }

}

Date::~Date() {

}

string Date::get_YYYYMMDD() {
  stringstream oss;
  oss << setw(4) << setfill('0') << this->year << "-";
  oss << setw(2) << setfill('0') << this->month << "-";
  oss << setw(2) << setfill('0') << this->day_of_month;
  return oss.str();
}

string Date::get_YYYYMMDD(int day) {
  stringstream oss;
  if(day < 0 || day > 0) {
    oss << setw(4) << setfill('0') << this->get_year(day) << "-";
    oss << setw(2) << setfill('0') << this->get_month(day) << "-";
    oss << setw(2) << setfill('0') << this->get_day_of_month(day);
  } else {
    oss << setw(4) << setfill('0') << this->year << "-";
    oss << setw(2) << setfill('0') << this->month << "-";
    oss << setw(2) << setfill('0') << this->day_of_month;
  }
  return oss.str();
}

string Date::get_YYYYMM() {
  stringstream oss;
  oss << setw(4) << setfill('0') << this->year << "-";
  oss << setw(2) << setfill('0') << this->month;
  return oss.str();
}

string Date::get_YYYYMM(int day) {
  stringstream oss;
  if(day < 0 || day > 0) {
    oss << setw(4) << setfill('0') << this->get_year(day) << "-";
    oss << setw(2) << setfill('0') << this->get_month(day);
  } else {
  oss << setw(4) << setfill('0') << this->year << "-";
  oss << setw(2) << setfill('0') << this->month;
  }
  return oss.str();
}

string Date::get_MMDD() {
  stringstream oss;
  oss << setw(2) << setfill('0') << this->month << "-";
  oss << setw(2) << setfill('0') << this->day_of_month;
  return oss.str();
}

string Date::get_MMDD(int day) {
  stringstream oss;
  if(day < 0 || day > 0) {
    oss << setw(2) << setfill('0') << this->get_month(day) << "-";
    oss << setw(2) << setfill('0') << this->get_day_of_month(day);
  } else {
    oss << setw(2) << setfill('0') << this->month << "-";
    oss << setw(2) << setfill('0') << this->day_of_month;
  }
  return oss.str();
}

bool Date::day_is_between_MMDD(char * current, char * start, char * end) {
  int current_mon, current_day, start_mon, start_day, end_mon, end_day;
  sscanf(current, "%d-%d", &current_mon, &current_day);
  sscanf(start, "%d-%d", &start_mon, &start_day);
  sscanf(end, "%d-%d", &end_mon, &end_day);
  if (current_mon < start_mon ||
      (current_mon == start_mon && current_day < start_day))
    return false;
  if (end_mon < current_mon ||
      (current_mon == end_mon && end_day < current_day))
    return false;
  return true;
}

bool Date::match_pattern(Date * check_date, string pattern) {
  size_t pos1 = pattern.find_first_of("-");
  size_t pos2 = pattern.find_last_of("-");
  assert(pos1 != string::npos && pos2 != string::npos && pos1 < pos2);
  string mon = pattern.substr(0,pos1);
  string day = pattern.substr(pos1+1,pos2-pos1-1);
  string year = pattern.substr(pos2+1);

  string tmp  = check_date->get_YYYYMMDD();
  string sim_year = tmp.substr(0,4);
  string sim_mon = tmp.substr(5,2);
  string sim_day = tmp.substr(8,2);

  if (year != "*" && year != sim_year) return false;
  if (mon != "*" && mon != sim_mon) return false;
  if (day != "*" && day != sim_day) return false;
  return true;
}

bool Date::day_in_range_MMDD(Date * check_date, char * start_day, char * end_day) {
  char current_day[10];
  strcpy(current_day, check_date->get_MMDD().c_str());
  return Date::day_is_between_MMDD(current_day, start_day, end_day);
}

bool Date::is_weekend() {
  return (this->day_of_week == 0 || this->day_of_week == 6);
}

bool Date::is_weekday() {
  return !this->is_weekend();
}



