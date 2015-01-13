/*
 Copyright 2011 by the University of Pittsburgh
 Licensed under the Academic Free License version 3.0
 See the file "LICENSE" for more information
 */

//
//
// File: Date.h
//

#ifndef DATE_H_
#define DATE_H_

#include <string>
#include <vector>

using namespace std;

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
  static const string YYYYMMDD;
  static const string MMDDYYYY;
  static const string MMDDYY;
  static const string DDMMYYYY;
  static const string DDMMYY;
  static const int INVALID = -1;

  /**
   * Default constructor
   */
  Date();

  /**
   *
   */
  Date(string date_string, string format_string = Date::YYYYMMDD);
  Date(int year, int day_of_year);
  Date(int year, int month, int day_of_month);
  void set_date(int year, int month, int day_of_month);

  /**
   * Get the instance variable for the year
   *
   * @return the year
   */
  int get_year() { return this->year; }

  /**
   * Get the year that it will be in t days
   *
   * @param t the number of days from this instance's current date
   * @return the year
   */
  int get_year(int t);

  /**
   * Get the instance variable for the month
   *
   * @return the month
   */
  int get_month() { return this->month; }

  /**
   * Get the month that it will be in t days
   *
   * @param t the number of days from this instance's current date
   * @return the month
   */
  int get_month(int t);

  /**
   * Get the month in string format
   *
   * @return the month in string format
   */
  string get_month_string();

  /**
   * Get the month in string format that it will be in t days
   *
   * @param t the number of days from this instance's current date
   * @return the month in string format
   */
  string get_month_string(int t);

  /**
   * Get the instance variable for the day_of_month
   *
   * @return the day_of_month
   */
  int get_day_of_month() { return this->day_of_month; }

  /**
   * Get the day_of_month that it will be in t days
   *
   * @param t the number of days from this instance's current date
   * @return the day_of_month
   */
  int get_day_of_month(int t);

  /**
   * Get the instance variable for the day_of_year
   *
   * @return the day_of_year
   */
  int get_day_of_year() { return this->day_of_year; }

  /**
   * Get the day_of_year that it will be in t days
   *
   * @param t the number of days from this instance's current date
   * @return the day_of_year
   */
  int get_day_of_year(int t);

  /**
   * Get the instance variable for the day_of_week
   *
   * @return the day_of_week
   */
  int get_day_of_week() { return this->day_of_week; }

  /**
   * Get the day_of_week that it will be in t days
   *
   * @param t the number of days from this instance's current date
   * @return the day_of_week
   */
  int get_day_of_week(int t);

  string get_day_of_week_string() { return this->get_day_of_week_string(0); }
  string get_day_of_week_string(int t);
  int get_epi_week() { return this->get_epi_week(0); }
  int get_epi_week(int t);
  int get_epi_week_year()  { return this->get_epi_week_year(0); }
  int get_epi_week_year(int t);
  int get_days_since_jan_1_epoch_year() { return this->days_since_jan_1_epoch_year; }
  void advance();
  void advance(int days);
  Date * clone();
  bool equals(Date * check_date);
  int compare_to(Date * check_date);
  string to_string();
  virtual ~Date();
  string get_YYYYMMDD();
  string get_YYYYMMDD(int day);
  string get_YYYYMM();
  string get_YYYYMM(int day);
  string get_MMDD();
  string get_MMDD(int day);
  bool is_weekend();
  bool is_weekday();
  
  bool operator< (const Date &other) const;
  bool operator> (const Date &other) const;
  bool operator== (const Date &other) const;
  bool operator<= (const Date &other) const;
  bool operator>= (const Date &other) const;
  bool operator!= (const Date &other) const;

  static int days_between(const Date * date_1, const Date * date_2);
  static int days_between(int sim_day, Date * date_2);
  static bool is_leap_year(int year);
  static int get_day_of_year(int year, int month, int day);
  static int parse_month_from_date_string(string date_string, string format_string);
  static int parse_day_of_month_from_date_string(string date_string, string format_string);
  static int parse_year_from_date_string(string date_string, string format_string);
  static bool day_is_between_MMDD(char * current, char * start, char * end);
  static bool match_pattern(Date * check_date, string pattern);
  static bool day_in_range_MMDD(Date * check_date, char * start_day, char * end_day);

  static int get_epoch_start_year() { return Date::EPOCH_START_YEAR; }
private:
  static const int EPOCH_START_YEAR = 1800;
  static const int day_table[2][13];
  static const int doomsday_month_val[2][13];
  static vector<int> *day_of_month_vec;
  static vector<int> *month_vec;
  static vector<int> *year_vec;
  static bool is_initialized;
  int days_since_jan_1_epoch_year;

  int day_of_month;
  int month;
  int year;
  int day_of_week;
  int day_of_year;

  static void initialize_vectors();
  static void add_to_vectors(int _days_since_jan_1_epoch_year);
  static int get_doomsday_month(int month, int year)
  { return Date::doomsday_month_val[(Date::is_leap_year(year) ? 1 : 0)][month]; }
  static int get_doomsday_century(int year);
  static int get_day_of_week(int year, int month, int day_of_month);
  //char date_string[32];
};

#endif /* DATE_H_ */
