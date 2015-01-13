/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/

//
//
// File: Utils.h
//

#ifndef UTILS_H_
#define UTILS_H_

#include <string>
#include <stdarg.h>
#include <stdio.h>

////// LOGGING MACROS
////// gcc recognizes a signature without variadic args: (verbosity, format) as well as
////// with: (vebosity, format, arg_1, arg_2, ... arg_n).  Other preprocessors may not.
////// To ensure compatibility, always provide at least one varg (which may be an empty string,
////// eg: (vebosity, format, "")

// FRED_VERBOSE and FRED_CONDITIONAL_VERBOSE print to the stout using Utils::fred_verbose
#ifdef FREDVERBOSE
#define FRED_VERBOSE(verbosity, format, ...){\
  if ( Global::Verbose > verbosity ) {\
    Utils::fred_verbose(verbosity, "FRED_VERBOSE: <%s, LINE:%d> " format, __FILE__, __LINE__, ## __VA_ARGS__);\
  }\
}

#else
#define FRED_VERBOSE(verbosity, format, ...){}\

#endif
// FRED_CONDITIONAL_VERBOSE prints to the stout if the verbose level is exceeded and the supplied conditional is true
#ifdef FREDVERBOSE
#define FRED_CONDITIONAL_VERBOSE(verbosity, condition, format, ...){\
  if ( Global::Verbose > verbosity && condition ) {\
    Utils::fred_verbose(verbosity, "FRED_CONDITIONAL_VERBOSE: <%s, LINE:%d> " format, __FILE__, __LINE__, ## __VA_ARGS__);\
  }\
}

#else
#define FRED_CONDITIONAL_VERBOSE(verbosity, condition, format, ...){}\

#endif

// FRED_STATUS and FRED_CONDITIONAL_STATUS print to Global::Statusfp using Utils::fred_verbose_statusfp
// If Global::Verbose == 0, then abbreviated output is produced
#ifdef FREDSTATUS
#define FRED_STATUS(verbosity, format, ...){\
  if ( verbosity == 0 && Global::Verbose <= 1 ) {\
    Utils::fred_verbose_statusfp(verbosity, format, ## __VA_ARGS__);\
  }\
  else if ( Global::Verbose > verbosity ) {\
    Utils::fred_verbose_statusfp(verbosity, "FRED_STATUS: <%s, LINE:%d> " format, __FILE__, __LINE__, ## __VA_ARGS__);\
  }\
}

#else
#define FRED_STATUS(verbosity, format, ...){}\

#endif
// FRED_CONDITIONAL_STATUS prints to Global::Statusfp if the verbose level is exceeded and the supplied conditional is true
#ifdef FREDSTATUS
#define FRED_CONDITIONAL_STATUS(verbosity, condition, format, ...){\
  if ( verbosity == 0 && Global::Verbose <= 1 && condition ) {\
    Utils::fred_verbose_statusfp(verbosity, format, ## __VA_ARGS__);\
  }\
  else if ( Global::Verbose > verbosity && condition ) {\
    Utils::fred_verbose_statusfp(verbosity, "FRED_CONDITIONAL_STATUS: <%s, LINE:%d> " format, __FILE__, __LINE__, ## __VA_ARGS__);\
  }\
}

#else
#define FRED_CONDITIONAL_STATUS(verbosity, condition, format, ...){}\

#endif

// FRED_WARNING and FRED_CONDITIONAL_WARNING print to both stdout and the Global::ErrorLogfp using Utils::fred_warning
#ifdef FREDWARNING
#define FRED_WARNING(format, ...){\
  Utils::fred_warning("FRED_WARNING: <%s, LINE:%d> " format, __FILE__, __LINE__, ## __VA_ARGS__);\
}

#else
#define FRED_WARNING(format, ...){}\

#endif
// FRED_CONDITIONAL_WARNING prints only if supplied conditional is true
#ifdef FREDWARNING
#define FRED_CONDITIONAL_WARNING(condition, format, ...){\
  if (condition) {\
    Utils::fred_warning("FRED_CONDITIONAL_WARNING: <%s, LINE:%d> " format, __FILE__, __LINE__, ## __VA_ARGS__);\
  }\
}

#else
#define FRED_CONDITIONAL_WARNING(condition, format, ...){}\

#endif


using namespace std;

namespace Utils{
  void fred_abort(const char* format,...);
  void fred_warning(const char* format,...);
  void fred_open_output_files(char * directory, int run);
  void fred_end();
  void fred_print_wall_time(const char* format, ...);
  void fred_start_timer();
  void fred_start_timer( time_t * lap_start_time );
  void fred_start_day_timer();
  void fred_print_day_timer(int day);
  void fred_print_finish_timer();
  void fred_print_lap_time(const char* format, ...);
  void fred_print_lap_time( time_t * start_lap_time, const char* format, ...);
  void fred_verbose(int verbosity, const char* format, ...);
  void fred_verbose_statusfp(int verbosity, const char* format, ...);
  void fred_log(const char* format, ...);
  void fred_report(const char* format, ...);
  FILE *fred_open_file(char * filename);
  void get_fred_file_name(char * filename);
  void fred_print_resource_usage(int day);
  void replace_csv_missing_data(char *out_str, char* in_str, const char * replacement);
}

#endif /* UTILS_H_ */
