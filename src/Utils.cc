/*
  Copyright 2009 by the University of Pittsburgh
  Licensed under the Academic Free License version 3.0
  See the file "LICENSE" for more information
*/
//
//
// File: Utils.cc
//

#include "Utils.h"
#include "Global.h"
#include <stdlib.h>
#include <string.h>

using namespace std;
static time_t start_timer, stop_timer, fred_timer, day_timer;
static char ErrorFilename[256];

void Utils::fred_abort(const char* format, ...){

  // open ErrorLog file if it doesn't exist
  if(Global::ErrorLogfp == NULL){
    Global::ErrorLogfp = fopen(ErrorFilename, "w");
    if (Global::ErrorLogfp == NULL) {
      // output to stdout
      printf("FRED ERROR: Can't open errorfile %s\n", ErrorFilename);
      // current error message:
      va_list ap;
      va_start(ap,format);
      printf("FRED ERROR: ");
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
  fprintf(Global::ErrorLogfp,"FRED ERROR: ");
  vfprintf(Global::ErrorLogfp,format,ap);
  va_end(ap);
  fflush(Global::ErrorLogfp);

  // output to stdout
  va_start(ap,format);
  printf("FRED ERROR: ");
  vprintf(format,ap);
  va_end(ap);
  fflush(stdout);

  fred_end();
  abort();
}

void Utils::fred_warning(const char* format, ...){

  // open ErrorLog file if it doesn't exist
  if(Global::ErrorLogfp == NULL){
    Global::ErrorLogfp = fopen(ErrorFilename, "w");
    if (Global::ErrorLogfp == NULL) {
      // output to stdout
      printf("FRED ERROR: Can't open errorfile %s\n", ErrorFilename);
      // current error message:
      va_list ap;
      va_start(ap,format);
      printf("FRED WARNING: ");
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
  fprintf(Global::ErrorLogfp,"FRED WARNING: ");
  vfprintf(Global::ErrorLogfp,format,ap);
  va_end(ap);
  fflush(Global::ErrorLogfp);

  // output to stdout
  va_start(ap,format);
  printf("FRED WARNING: ");
  vprintf(format,ap);
  va_end(ap);
  fflush(stdout);
}

void Utils::fred_open_output_files(char * directory, int run){
  char filename[256];

  // ErrorLog file is created at the first warning or error
  Global::ErrorLogfp = NULL;
  sprintf(ErrorFilename, "%s/err%d.txt", directory, run);

  sprintf(filename, "%s/out%d.txt", directory, run);
  Global::Outfp = fopen(filename, "w");
  if (Global::Outfp == NULL) {
    Utils::fred_abort("Can't open %s\n", filename);
  }
  Global::Tracefp = NULL;
  if (strcmp(Global::Tracefilebase, "none") != 0) {
    sprintf(filename, "%s/trace%d.txt", directory, run);
    Global::Tracefp = fopen(filename, "w");
    if (Global::Tracefp == NULL) {
      Utils::fred_abort("Can't open %s\n", filename);
    }
  }
  Global::Infectionfp = NULL;
  if (Global::Track_infection_events) {
    sprintf(filename, "%s/infections%d.txt", directory, run);
    Global::Infectionfp = fopen(filename, "w");
    if (Global::Infectionfp == NULL) {
      Utils::fred_abort("Can't open %s\n", filename);
    }
  }
  Global::VaccineTracefp = NULL;
  if (strcmp(Global::VaccineTracefilebase, "none") != 0) {
    sprintf(filename, "%s/vacctr%d.txt", directory, run);
    Global::VaccineTracefp = fopen(filename, "w");
    if (Global::VaccineTracefp == NULL) {
      Utils::fred_abort("Can't open %s\n", filename);
    }
  }
  Global::Birthfp = NULL;
  if (Global::Enable_Births) {
    sprintf(filename, "%s/births%d.txt", directory, run);
    Global::Birthfp = fopen(filename, "w");
    if (Global::Birthfp == NULL) {
      Utils::fred_abort("Can't open %s\n", filename);
    }
  }
  Global::Deathfp = NULL;
  if (Global::Enable_Deaths) {
    sprintf(filename, "%s/deaths%d.txt", directory, run);
    Global::Deathfp = fopen(filename, "w");
    if (Global::Deathfp == NULL) {
      Utils::fred_abort("Can't open %s\n", filename);
    }
  }
  Global::Prevfp = NULL;
  if (strcmp(Global::Prevfilebase, "none") != 0) {
    sprintf(filename, "%s/prev%d.txt", directory, run);
    Global::Prevfp = fopen(filename, "w");
    if (Global::Prevfp == NULL) {
      Utils::fred_abort("Can't open %s\n", filename);
    }
    Global::Report_Prevalence = true;
  }
  Global::Incfp = NULL;
  if (strcmp(Global::Incfilebase, "none") != 0) {
    sprintf(filename, "%s/inc%d.txt", directory, run);
    Global::Incfp = fopen(filename, "w");
    if (Global::Incfp == NULL) {
      Utils::fred_abort("Help! Can't open %s\n", filename);
    }
    Global::Report_Incidence = true;
  }
  Global::Immunityfp = NULL;
  if (strcmp(Global::Immunityfilebase, "none") != 0) {
    sprintf(filename, "%s/immunity%d.txt", directory, run);
    Global::Immunityfp = fopen(filename, "w");
    if (Global::Immunityfp == NULL) {
      Utils::fred_abort("Help! Can't open %s\n", filename);
    }
    Global::Report_Immunity = true;
  }
  Global::Transmissionsfp = NULL;
  if (strcmp(Global::Transmissionsfilebase, "none") != 0) {
    sprintf(filename, "%s/transmissions%d.txt", directory, run);
    Global::Transmissionsfp = fopen(filename, "w");
    if (Global::Transmissionsfp == NULL) {
      Utils::fred_abort("Help! Can't open %s\n", filename);
    }
    Global::Report_Transmissions = true;
  }
  Global::Strainsfp = NULL;
  if (strcmp(Global::Strainsfilebase, "none") != 0) {
    sprintf(filename, "%s/strains%d.txt", directory, run);
    Global::Strainsfp = fopen(filename, "w");
    if (Global::Strainsfp == NULL) {
      Utils::fred_abort("Help! Can't open %s\n", filename);
    }
    Global::Report_Strains = true;
  }
  Global::Householdfp = NULL;
  if (Global::Print_Household_Locations) {
    sprintf(filename, "%s/households.txt", directory);
    Global::Householdfp = fopen(filename, "w");
    if (Global::Householdfp == NULL) {
      Utils::fred_abort("Can't open %s\n", filename);
    }
  }
  return;
}

void Utils::fred_end(void){
  // This is a function that cleans up FRED and exits
  if (Global::Outfp != NULL) fclose(Global::Outfp);
  if (Global::Tracefp != NULL) fclose(Global::Tracefp);
  if (Global::Infectionfp != NULL) fclose(Global::Infectionfp);
  if (Global::VaccineTracefp != NULL) fclose(Global::VaccineTracefp);
  if (Global::Prevfp != NULL) fclose(Global::Prevfp);
  if (Global::Incfp != NULL) fclose(Global::Incfp);
  if (Global::Immunityfp != NULL) fclose(Global::Immunityfp);
  if (Global::Householdfp != NULL) fclose(Global::Householdfp);
}


void Utils::fred_print_wall_time(const char* format, ...) {
  time_t clock;
  time(&clock);
  va_list ap;
  va_start(ap,format);
  vfprintf(Global::Statusfp,format,ap);
  va_end(ap);
  fprintf(Global::Statusfp," %s",ctime(&clock));
  fflush(Global::Statusfp);
}

void Utils::fred_start_timer() {
  time(&fred_timer);
  start_timer = fred_timer;
}

void Utils::fred_start_timer( time_t * lap_start_time ) {
  time( lap_start_time );
}

void Utils::fred_start_day_timer() {
  time(&day_timer);
  start_timer = day_timer;
}

void Utils::fred_print_day_timer(int day) {
  time(&stop_timer);
  fprintf(Global::Statusfp, "day %d took %d seconds\n",
    day, (int) (stop_timer-day_timer));
  fflush(Global::Statusfp);
  start_timer = stop_timer;
}

void Utils::fred_print_finish_timer() {
  time(&stop_timer);
  fprintf(Global::Statusfp, "FRED took %d seconds\n",
    (int)(stop_timer-fred_timer));
  fflush(Global::Statusfp);
}

void Utils::fred_print_lap_time(const char* format, ...) {
  time(&stop_timer);
  va_list ap;
  va_start(ap,format);
  vfprintf(Global::Statusfp,format,ap);
  va_end(ap);
  fprintf(Global::Statusfp, " took %d seconds\n",
    (int) (stop_timer - start_timer));
  fflush(Global::Statusfp);
  start_timer = stop_timer;
}

void Utils::fred_print_lap_time( time_t * start_lap_time, const char* format, ...) {
  time_t stop_lap_time;
  time( &stop_lap_time );
  va_list ap;
  va_start(ap,format);
  vfprintf(Global::Statusfp,format,ap);
  va_end(ap);
  fprintf(Global::Statusfp, " took %d seconds\n",
    (int) ( stop_lap_time - ( *start_lap_time ) ) );
  fflush(Global::Statusfp);
}

void Utils::fred_verbose(int verbosity, const char* format, ...){
  if (Global::Verbose > verbosity) {
    va_list ap;
    va_start(ap,format);
    vprintf(format,ap);
    va_end(ap);
    fflush(stdout);
  }
}

void Utils::fred_verbose_statusfp(int verbosity, const char* format, ...) {
  if (Global::Verbose > verbosity) {
    va_list ap;
    va_start(ap,format);
    vfprintf(Global::Statusfp,format,ap);
    va_end(ap);
    fflush(Global::Statusfp);
  }
}

void Utils::fred_log(const char* format, ...){
  va_list ap;
  va_start(ap,format);
  vfprintf(Global::Statusfp,format,ap);
  va_end(ap);
  fflush(Global::Statusfp);
}

void Utils::fred_report(const char* format, ...){
  va_list ap;
  va_start(ap,format);
  vfprintf(Global::Outfp,format,ap);
  fflush(Global::Outfp);
  va_start(ap,format);
  vfprintf(Global::Statusfp,format,ap);
  fflush(Global::Statusfp);
  va_end(ap);
}

FILE *Utils::fred_open_file(char * filename) {
  FILE *fp;
  get_fred_file_name(filename);
  printf("fred_open_file: opening file %s for reading\n", filename);
  fp = fopen(filename, "r");
  return fp;
}

void Utils::get_fred_file_name(char * filename) {
  string str;
  str.assign(filename);
  if (str.compare(0,10,"$FRED_HOME") == 0) {
    char * fred_home = getenv("FRED_HOME");
    if (fred_home != NULL) {
      str.erase(0,10);
      str.insert(0,fred_home);
      strcpy(filename, str.c_str());
    }
    else {
      fred_abort("get_fred_file_name: the FRED_HOME environmental variable cannot be found\n");
    }
  }
}

#include <sys/resource.h>
#define   RUSAGE_SELF     0
#define   RUSAGE_CHILDREN     -1
/*
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
  printf("day %d maxrss %ld\n",
   day, r_usage.ru_maxrss);
  fflush(stdout);
}

/********************************************************
 * Input: in_str is a csv string, possiblly ending with \n
 * Output out_str is a csv string with empty fields replaced
 * by replacement string.
 *
 * Notes: It is assumed that empty strings do not have white space
 * between commas. Newlines in the input string are ignored, so the
 * output string contains no newlines.
 *        
 */
void Utils::replace_csv_missing_data(char* out_str, char* in_str, const char* replacement) {
  // printf("in = |%s|replacement = %s\n",in_str,replacement);
  int i = 0;
  int j = 0;
  int new_field = 1;
  while (in_str[i] != '\0') {
    if (in_str[i] == '\n') {
      i++;
    }
    else if (new_field && in_str[i] == ',') {
      // field is missing, so insert replacement
      int k = 0;
      while (replacement[k] != '\0') { out_str[j++] = replacement[k++]; }
      out_str[j++] = ',';
      i++;
      new_field = 1;
    }
    else if (in_str[i] == ',') {
      out_str[j++] = in_str[i++];
      new_field = 1;
    }
    else {
      out_str[j++] = in_str[i++];
      new_field = 0;
    }
  }
  // printf("new_field = %d\n", new_field); fflush(stdout);
  if (new_field) {
    // last field is missing
    int k = 0;
    while (replacement[k] != '\0') { out_str[j++] = replacement[k++]; }
  }
  out_str[j] = '\0';
  // printf("out = |%s|\n",out_str); fflush(stdout);
}


