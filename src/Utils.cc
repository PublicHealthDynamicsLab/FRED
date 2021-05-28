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
// File: Utils.cc
//

#include "Utils.h"
#include "Expression.h"
#include "Global.h"
#include "Person.h"

static high_resolution_clock::time_point start_timer;
static high_resolution_clock::time_point fred_timer;
static high_resolution_clock::time_point day_timer;
static high_resolution_clock::time_point initialization_timer;
static high_resolution_clock::time_point update_timer;
static high_resolution_clock::time_point epidemic_timer;

static char ErrorFilename[FRED_STRING_SIZE];

// helper method for sorting people in a list
bool Utils::compare_id (Person* p1, Person* p2) {
  return p1->get_id() < p2->get_id();
}

std::string Utils::delete_spaces(std::string &str) {
  str.erase(std::remove(str.begin(), str.end(), ' '), str.end());
  return str;
}

string_vector_t Utils::get_string_vector(std::string str, char delim) {
  string_vector_t result;
  std::stringstream ss(str);
  while(ss.good()) {
    std::string substr;
    getline(ss, substr, delim);
    if(substr != "") {
      result.push_back(Utils::delete_spaces(substr));
    }
  }
  return result;
}

bool Utils::is_number(std::string s) {
  char* p;
  strtod(s.c_str(), &p);
  return *p == 0;
}

string_vector_t Utils::get_top_level_parse(std::string str, char delim) {
  string_vector_t result;
  str = delete_spaces(str);
  char cstr[FRED_STRING_SIZE];
  char tmp[FRED_STRING_SIZE];
  strcpy(cstr, str.c_str());
  // FRED_VERBOSE(0, "input string = |%s|\n", cstr);
  int inside = 0;
  char* s = cstr;
  char* t = tmp;
  *t = '\0';
  while(*s != '\0') {
    // printf("*s = |%c| tmp = |%s|\n", *s, tmp); fflush(stdout);
    if(*s  ==  delim && !inside) {
      // FRED_VERBOSE(0, "push_back |%s|\n", tmp);
      result.push_back(string(tmp));
      t = tmp;
      *t = '\0';
    } else {
      if(*s == '(') {
        inside++;
      }
      if(*s==')') {
        inside--;
      }
      *t++ = *s;
      *t = '\0';
    }
    s++;
  }
  result.push_back(string(tmp));
  return result;
}

void Utils::fred_abort(const char* format, ...) {

  // open ErrorLog file if it doesn't exist
  if(Global::ErrorLogfp == NULL) {
    Global::ErrorLogfp = fopen(ErrorFilename, "w");
    if(Global::ErrorLogfp == NULL) {
      // output to stdout
      printf("\nFRED ERROR: Can't open errorfile %s\n", ErrorFilename);
      // current error message:
      va_list ap;
      va_start(ap,format);
      printf("\nFRED ERROR: ");
      vprintf(format,ap);
      va_end(ap);
      fflush(stdout);
      fred_end();
      abort();
    }
  }

  // output to error file
  va_list ap;
  va_start(ap,format);
  fprintf(Global::ErrorLogfp,"\nFRED ERROR: ");
  vfprintf(Global::ErrorLogfp,format,ap);
  va_end(ap);
  fflush(Global::ErrorLogfp);

  // output to stdout
  va_start(ap,format);
  printf("\nFRED ERROR: ");
  vprintf(format,ap);
  va_end(ap);
  fflush(stdout);

  fred_end();
  abort();
}

void Utils::fred_warning(const char* format, ...){

  // open ErrorLog file if it doesn't exist
  if(Global::ErrorLogfp == NULL) {
    Global::ErrorLogfp = fopen(ErrorFilename, "w");
    if(Global::ErrorLogfp == NULL) {
      // output to stdout
      printf("\nFRED ERROR: Can't open errorfile %s\n", ErrorFilename);
      // current error message:
      va_list ap;
      va_start(ap,format);
      printf("\nFRED WARNING: ");
      vprintf(format,ap);
      va_end(ap);
      fflush(stdout);
      fred_end();
      abort();
    }
  }

  // output to error file
  va_list ap;
  va_start(ap,format);
  fprintf(Global::ErrorLogfp,"\nFRED WARNING: ");
  vfprintf(Global::ErrorLogfp,format,ap);
  va_end(ap);
  fflush(Global::ErrorLogfp);

  // output to stdout
  va_start(ap,format);
  printf("\nFRED WARNING: ");
  vprintf(format,ap);
  va_end(ap);
  fflush(stdout);
}

void Utils::fred_open_output_files(){
  int run = Global::Simulation_run_number;
  char filename[FRED_STRING_SIZE];
  char directory[FRED_STRING_SIZE];
  sprintf(directory, "%s/RUN%d", Global::Simulation_directory, run);
  fred_make_directory(directory);

  // ErrorLog file is created at the first warning or error
  Global::ErrorLogfp = NULL;
  sprintf(ErrorFilename, "%s/err.txt", directory);

  Global::Recordsfp = NULL;
  if(Global::Enable_Records > 0) {
    sprintf(filename, "%s/health_records.txt", directory);
    Global::Recordsfp = fopen(filename, "w");
    if(Global::Recordsfp == NULL) {
      Utils::fred_abort("Can't open %s\n", filename);
    }
  }

  return;
}

void Utils::fred_make_directory(char* directory) {
  struct stat info;
  if(stat(directory, &info) == 0) {
    // file already exists. verify that it is a directory
    if(info.st_mode & S_IFDIR) {
      // printf( "fred_make_directory: %s already exists\n", directory );
      return;
    } else {
      Utils::fred_abort("fred_make_directory: %s exists but is not a directory\n", directory);
      return;
    }
  }
  // try to create the directory:
  mode_t mask;        // the user's current umask
  mode_t mode = 0777; // as a start
  mask = umask(0);    // get the current mask, which reads and sets...
  umask(mask);        // so now we have to put it back
  mode ^= mask;       // apply the user's existing umask
  if(0 != mkdir(directory, mode) && EEXIST != errno) { // make it
    Utils::fred_abort("mkdir(%s) failed with %d\n", directory, errno); // or die
  }
}

void Utils::fred_end(void) {

  // This is a function that cleans up FRED and exits
  if(Global::Statusfp != NULL) {
    fclose(Global::Statusfp);
  }

  if(Global::Birthfp != NULL) {
    fclose(Global::Birthfp);
  }

  if(Global::Deathfp != NULL) {
    fclose(Global::Deathfp);
  }

  if(Global::ErrorLogfp != NULL) {
    fclose(Global::ErrorLogfp);
  }

  if(Global::Recordsfp != NULL) {
    fclose(Global::Recordsfp);
  }
}

void Utils::fred_print_wall_time(const char* format, ...) {
  time_t clock;
  time(&clock);
  va_list ap;
  va_start(ap,format);
  vfprintf(Global::Statusfp, format, ap);
  va_end(ap);
  fprintf(Global::Statusfp, " %s", ctime(&clock));
  fflush(Global::Statusfp);
}

void Utils::fred_start_timer() {
  fred_timer = high_resolution_clock::now();
  start_timer = fred_timer;
}

void Utils::fred_start_timer(high_resolution_clock::time_point* lap_start_time) {
  *lap_start_time = high_resolution_clock::now();
}

void Utils::fred_start_epidemic_timer() {
  epidemic_timer = high_resolution_clock::now();
}

void Utils::fred_print_epidemic_timer(string msg) {
  high_resolution_clock::time_point stop_timer = high_resolution_clock::now();
  double duration = 0.000001 * std::chrono::duration_cast<std::chrono::microseconds>( stop_timer - epidemic_timer ).count();
  fprintf(Global::Statusfp, "%s took %f seconds\n", msg.c_str(), duration);
  fflush(Global::Statusfp);
  epidemic_timer = stop_timer;
}

void Utils::fred_start_initialization_timer() {
  initialization_timer = high_resolution_clock::now();
}

void Utils::fred_print_initialization_timer() {
  high_resolution_clock::time_point stop_timer = high_resolution_clock::now();
  double duration = 0.000001 * std::chrono::duration_cast<std::chrono::microseconds>( stop_timer - initialization_timer ).count();
  fprintf(Global::Statusfp, "FRED initialization took %f seconds\n\n", duration);
  fflush(Global::Statusfp);
}

void Utils::fred_start_day_timer() {
  day_timer = high_resolution_clock::now();
}

void Utils::fred_print_day_timer(int day) {
  high_resolution_clock::time_point stop_timer = high_resolution_clock::now();
  double duration = 0.000001 * std::chrono::duration_cast<std::chrono::microseconds>( stop_timer - day_timer ).count();
  fprintf(Global::Statusfp, "DAY_TIMER day %d took %f seconds\n\n", day, duration);
  fflush(Global::Statusfp);
}

void Utils::fred_print_finish_timer() {
  high_resolution_clock::time_point stop_timer = high_resolution_clock::now();
  double duration = 0.000001 * std::chrono::duration_cast<std::chrono::microseconds>( stop_timer - fred_timer ).count();
  fprintf(Global::Statusfp, "FRED took %f seconds\n", duration);
  fflush(Global::Statusfp);
}

void Utils::fred_print_lap_time(const char* format, ...) {
  high_resolution_clock::time_point stop_timer = high_resolution_clock::now();
  double duration = 0.000001 * std::chrono::duration_cast<std::chrono::microseconds>( stop_timer - start_timer ).count();
  va_list ap;
  va_start(ap,format);
  vfprintf(Global::Statusfp,format,ap);
  va_end(ap);
  fprintf(Global::Statusfp, " took %f seconds\n", duration);
  fflush(Global::Statusfp);
  start_timer = stop_timer;
}

void Utils::fred_print_update_time(const char* format, ...) {
  high_resolution_clock::time_point stop_timer = high_resolution_clock::now();
  double duration = 0.000001 * std::chrono::duration_cast<std::chrono::microseconds>( stop_timer - update_timer ).count();
  va_list ap;
  va_start(ap,format);
  vfprintf(Global::Statusfp,format,ap);
  va_end(ap);
  fprintf(Global::Statusfp, " took %f seconds\n", duration);
  fflush(Global::Statusfp);
  update_timer = stop_timer;
}

void Utils::fred_print_lap_time(high_resolution_clock::time_point* start_lap_time, const char* format, ...) {
  high_resolution_clock::time_point stop_timer = high_resolution_clock::now();
  double duration = 0.000001 * std::chrono::duration_cast<std::chrono::microseconds>( stop_timer - (*start_lap_time) ).count();
  va_list ap;
  va_start(ap, format);
  vfprintf(Global::Statusfp,format,ap);
  va_end(ap);
  fprintf(Global::Statusfp, " took %f seconds\n", duration);
  fflush(Global::Statusfp);
}

void Utils::fred_verbose(int verbosity, const char* format, ...) {
  if(Global::Verbose > verbosity) {
    va_list ap;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
    fflush(stdout);
  }
}

void Utils::fred_status(int verbosity, const char* format, ...) {
  if(Global::Verbose > verbosity) {
    va_list ap;
    va_start(ap,format);
    vfprintf(Global::Statusfp, format, ap);
    va_end(ap);
    fflush(Global::Statusfp);
  }
}

void Utils::fred_log(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  vfprintf(Global::Statusfp, format, ap);
  va_end(ap);
  fflush(Global::Statusfp);
}

FILE* Utils::fred_open_file(char* filename) {
  FILE* fp;
  get_fred_file_name(filename);
  printf("fred_open_file: opening file %s for reading\n", filename);
  fp = fopen(filename, "r");
  return fp;
}

FILE* Utils::fred_write_file(char* filename) {
  FILE* fp;
  get_fred_file_name(filename);
  printf("fred_write_file: opening file %s for writing\n", filename);
  fp = fopen(filename, "w");
  return fp;
}

void Utils::get_fred_file_name(char* filename) {
  string str;
  str.assign(filename);
  if(str.compare(0,10,"$FRED_HOME") == 0) {
    char* fred_home = getenv("FRED_HOME");
    if(fred_home != NULL) {
      str.erase(0, 10);
      str.insert(0, fred_home);
      strcpy(filename, str.c_str());
    } else {
      fred_abort("get_fred_file_name: the FRED_HOME environmental variable cannot be found\n");
    }
  }
}

#include <sys/resource.h>
/*
  NOTE: FROM sys/resource.h ...

  #define   RUSAGE_SELF     0
  #define   RUSAGE_CHILDREN     -1

  struct rusage {
  struct timeval ru_utime; // user time used
  struct timeval ru_stime; // system time used
  long ru_maxrss;          // integral max resident set size
  long ru_ixrss;           // integral shared text memory size
  long ru_idrss;           // integral unshared data size
  long ru_isrss;           // integral unshared stack size
  long ru_minflt;          // page reclaims
  long ru_majflt;          // page faults
  long ru_nswap;           // swaps
  long ru_inblock;         // block input operations
  long ru_oublock;         // block output operations
  long ru_msgsnd;          // messages sent
  long ru_msgrcv;          // messages received
  long ru_nsignals;        // signals received
  long ru_nvcsw;           // voluntary context switches
  long ru_nivcsw;          // involuntary context switches
  };
*/

void Utils::fred_print_resource_usage(int day) {
  rusage r_usage;
  getrusage(RUSAGE_SELF, &r_usage);
  printf("day %d maxrss %ld\n", day, r_usage.ru_maxrss);

  printf("day %d cur_phys_mem_usage_gbs %0.4f\n", day, get_fred_phys_mem_usg_in_gb());

  fflush(stdout);
}

double Utils::get_daily_probability(double prob, int days) {
  // p = total prob; d = daily prob; n = days
  // prob of survival = 1-p = (1-d)^n
  // log(1-p) = n*log(1-d)
  // (1/n)*log(1-p) = log(1-d)
  // (1-p)^(1/n) = 1-d
  // d = 1-(1-p)^(1/n)
  double daily = 1.0 - pow((1.0 - prob), (1.0 / days));
  return daily;
}

std::string Utils::str_tolower(std::string s) {
  std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return s;
}

bool Utils::does_path_exist(const std::string &s) {
  char filename[FRED_STRING_SIZE];
  sprintf(filename, "%s", s.c_str());
  Utils::get_fred_file_name(filename);
  struct stat buffer;
  return (stat (filename, &buffer) == 0);
}

void Utils::print_error(const std::string &msg) {
  char error_file[FRED_STRING_SIZE];
  sprintf(error_file, "%s/errors.txt", Global::Simulation_directory);
  FILE* fp = fopen(error_file, "a");
  fprintf(fp, "\nFRED Error (file %s) %s\n", Global::Program_file, msg.c_str());
  fclose(fp);
  Global::Error_found = true;
}

void Utils::print_warning(const std::string &msg) {
  char warning_file[FRED_STRING_SIZE];
  sprintf(warning_file, "%s/warnings.txt", Global::Simulation_directory);
  FILE* fp = fopen(warning_file, "a");
  fprintf(fp, "\nFRED Warning (file %s) %s\n", Global::Program_file, msg.c_str());
  fclose(fp);
}
