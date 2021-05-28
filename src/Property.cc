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
// File Property.cc
//

#include <algorithm>
#include <stdio.h>
#include <math.h>
#include <string>
#include <sstream>

#include "Property.h"
#include "Condition.h"
#include "Global.h"
#include "Natural_History.h"
#include "Network_Type.h"
#include "Person.h"
#include "Place_Type.h"
#include "Rule.h"
#include "Utils.h"

using namespace std;

// static variables
int Property::UseFRED = 0;
int Property::abort_on_failure = 1;
std::vector<std::string> Property::property_names;
std::vector<std::string> Property::property_values;
std::vector<std::string> Property::property_not_found;
std::vector<std::string> Property::property_file;
std::vector<int> Property::property_lineno;
std::vector<int> Property::property_is_duplicate;
std::vector<int> Property::property_is_used;
std::vector<int> Property::property_is_default;
std::unordered_map<std::string, int> Property::property_map;
int Property::number_of_properties = 0;
char Property::PSA_Method[FRED_STRING_SIZE];
char Property::PSA_List_File[FRED_STRING_SIZE];
int Property::PSA_Sample_Size = 0;
int Property::PSA_Sample = 0;
int Property::check_properties = 0;
int Property::default_properties = 1;
std::string Property::error_string = "";

string remove_leading_whitespace(string str) {
  char copy[FRED_STRING_SIZE];
  char tmp[FRED_STRING_SIZE];
  strcpy(copy, str.c_str());
  char* s = copy;
  if (copy[0]==' ') {
    while (*s!='\0' && *s==' ') {
      s++;
    } 
  }
  strcpy(tmp,s);
  string result = string(tmp);
  return result;
}


string remove_spaces(string current) {
  char tmp [FRED_STRING_SIZE];
  char clean [FRED_STRING_SIZE];

  strcpy(tmp,current.c_str());
  int inside = 0;
  char* s = tmp;
  // printf("remove_spaces: |%s|\n", tmp);
  // printf("remove_spaces: |%s|\n", s);
  char* c = clean;
  while (*s!='\0') {

    // *c = '\0';
    // printf("1 s=|%c| clean|%s|\n",*s, clean);

    if (*s=='(') {
      if (inside==0) {
	// delete trailing space in clean
	while (c!=clean && *(c-1)==' ') {
	  c--;
	}
      }      
      inside++;
      // printf("inside = %d\n",inside);
    }
    if (*s==')') {
      inside--;
      // printf("inside = %d\n",inside);
    }
    if (inside) {
      if ((*s)!=' ' && (*s)!='\n') {
	*c++ = (*s);
      }
    }
    else {
      if ((*s)=='\n') {
	*c++ = ' ';
      }
      else {
	*c++ = (*s);
      }
    }
    *c = '\0';
    // printf("2 s=|%c| clean|%s|\n",(*s), clean);
    s++;
    // printf("3 s=|%c| clean|%s|\n",(*s), clean);
  }
  *c++ = '\n';
  *c = '\0';

  current = string(clean);
  return current;
}


std::string delete_whitespace(std::string str) {

  // printf("delete_ws: |%s|\n", str.c_str());

  // remove comments
  int comment = str.find("#");
  if (comment!=string::npos) {
    str = str.substr(0,comment);
  }
  // printf("delete_ws: |%s|\n", str.c_str());

  // replace tabs with spaces
  std::replace(str.begin(), str.end(), '\t', ' ');
  // printf("delete_ws: |%s|\n", str.c_str());

  // replace newline with spaces
  std::replace(str.begin(), str.end(), '\n', ' ');
  // printf("delete_ws: |%s|\n", str.c_str());

  // replace multiple spaces with one space
  for(int i=str.size()-1; i >= 0; i--) {
    if(str[i]==' '&&str[i]==str[i-1]) {
      str.erase( str.begin() + i );
    }
  }
  // printf("|%s|\n", str.c_str());

  // replace initial space if present
  if (str.size() && str[0]==' ') {
      str.erase( str.begin() );
  }
  // printf("|%s|\n", str.c_str());

  // replace final space if present
  if (str.size() && str[str.size()-1]==' ') {
    str.erase( str.begin() + str.size()-1);
  }
  // printf("|%s|\n", str.c_str());

  // erase space before left paren
  for(int i=str.size()-2; i >= 0; i--) {
    if(str[i]==' '&&str[i+1]=='(') {
      str.erase( str.begin() + i );
    }
  }
  // printf("|%s|\n", str.c_str());

  // erase any spaces within paren
  int inner = 0;
  for(int i=str.size()-1; i >= 0; i--) {
    if(str[i]==')') {
      inner++;
    }
    if(str[i]=='(') {
      inner--;
    }
    if(str[i]==' '&& inner>0) {
      str.erase( str.begin() + i );
    }
  }

  return str;
}

bool unbalanced_parens(std::string str) {
  int inner = 0;
  for(int i=str.size()-1; i >= 0; i--) {
    if(str[i]==')') {
      inner++;
    }
    if(str[i]=='(') {
      inner--;
    }
  }
  // printf("UNBAL line |%s| inner %d\n", str.c_str(),inner);
  return (inner != 0);
}

int Property::read_properties(char* program_file) {
  char filename[FRED_STRING_SIZE];
  
  property_names.clear();
  property_values.clear();
  strcpy(filename, "$FRED_HOME/data/config.fred");
  Property::default_properties = 1;
  read_program_file(filename);
  Property::default_properties = 0;
  read_program_file(program_file);
  // exit(0);
  Property::get_property("check_properties", &Property::check_properties);

  /*
    Property::get_property("psa_method", Property::PSA_Method);
    Property::get_property("psa_list_file", Property::PSA_List_File);
    Property::get_property("psa_sample_size", &Property::PSA_Sample_Size);
    Property::get_property("psa_sample", &Property::PSA_Sample);
    if(Property::PSA_Sample > 0) {
    FILE* fp;
    fp = Utils::fred_open_file(Property::PSA_List_File);
    if(fp != NULL) {
    char psa_property[FRED_STRING_SIZE];
    while(fscanf(fp, "%s", psa_property) == 1) {
    sprintf(filename, "$FRED_HOME/data/PSA/%s/%d/%s",
    Property::PSA_Method, Property::PSA_Sample_Size, psa_property);
    read_psa_program(filename, Property::PSA_Sample);
    }
    fclose(fp);
    } else {
    printf("read_properties: failed to open PSA list file %s\n", Property::PSA_List_File);
    abort();
    }
    }
  */

  return Property::property_names.size();
}


void Property::read_psa_program(char* program_file, int line_number) {
  /*
    FILE* fp;
    char name[FRED_STRING_SIZE];

    fp = Utils::fred_open_file(program_file);
    if(fp != NULL) {
    fprintf(stdout, "FRED reading PSA program file %s\n", program_file); fflush(stdout);
    int current_line = 1;
    while (current_line < line_number) {
    int ch = 1;
    while(ch != EOF && ch != '\n') {
    ch = fgetc(fp);
    }
    if(ch == EOF) {
    Utils::fred_abort("Unexpected EOF in FRED program %s on line %d\n",
    program_file, current_line);
    }
    current_line++;
    }
    if(fscanf(fp, "%s", name) == 1) {
    if(fscanf(fp, " = %[^\n]", Property::property_value[Property::property_count]) == 1) {
    //Remove end of line comments if they are there
    string temp_str(Property::property_value[Property::property_count]);
    size_t pos;
    string whitespaces(" \t\f\v\n\r");

    pos = temp_str.find("#");
    if(pos != string::npos) {
    temp_str = temp_str.substr(0, pos);
    }
    //trim trailing whitespace
    pos = temp_str.find_last_not_of(whitespaces);
    if(pos != string::npos) {
    if(pos != (temp_str.length() - 1)) {
    temp_str.erase(pos + 1);
    }
    } else {
    temp_str.clear(); //str is all whitespace
    }
    strcpy(Property::property_value[Property::property_count], temp_str.c_str());
          
    strcpy(Property::property_name[Property::property_count], name);
    printf("READ_PSA_PARAMETER: %s = %s\n", Property::property_name[Property::property_count],
    Property::property_value[Property::property_count]);
    Property::property_count++;
    } else {
    Utils::fred_abort("Bad format in FRED program %s on line starting with %s\n",
    program_file, name);
    }
    } else {
    Utils::fred_abort("Bad format in FRED program %s on line %d\n",
    program_file, current_line);
    }
    fclose(fp);
    } else {
    fprintf(stdout, "FRED failed reading program file %s\n", program_file); fflush(stdout);
    abort();
    // Utils::fred_abort("Help!  Can't read program_file %s\n", program_file);
    }
  */
}

void Property::print_errors(char* filename) {
  if (Property::error_string!="") {
    FILE* fp = fopen(filename, "a");
    fprintf(fp, "%s", Property::error_string.c_str());
    fclose(fp);
  }
}

void Property::print_warnings(char* filename) {
  int size = Property::property_names.size();
  int found = 0;
  for (int i = 0; i < size && found == 0; i++) {
    found = (Property::property_is_default[i] == 0 && Property::property_is_duplicate[i] == 0 && Property::property_is_used[i] == 0)
      || (Property::property_is_default[i] == 0 && Property::property_is_duplicate[i] == 1);
  }
  if (found) {
    FILE* fp = fopen(filename, "a");
    for (int i = 0; i < size; i++) {
      if (Property::property_is_default[i] == 0 && Property::property_is_duplicate[i] == 0 && Property::property_is_used[i] == 0) {
	fprintf(fp, "FRED Warning (file %s, line %d) Unrecognized property statement: %s = %s\n",
		Property::property_file[i].c_str(),
		Property::property_lineno[i],
		Property::property_names[i].c_str(),
		Property::property_values[i].c_str());
	int pos = Property::property_names[i].find(".states");
	if (pos!=string::npos) {
	  string cond = Property::property_names[i].substr(0,pos);
	  fprintf(fp, "  Is %s a missing condition?\n", cond.c_str());
	}
      }
      else if (Property::property_is_default[i] == 0 && Property::property_is_duplicate[i] == 1) {
	fprintf(fp, "FRED Warning (file %s, line %d) Ignored duplicate property statement: %d %s = %s\n",
		Property::property_file[i].c_str(),
		Property::property_lineno[i],
		i, Property::property_names[i].c_str(),
		Property::property_values[i].c_str());
      }
    }
    fclose(fp);
  }
}

void Property::report_parameter_check() {
  int size = Property::property_names.size();

  FILE* fp = fopen("CHECK_PARAMETERS.txt", "w");

  fprintf(fp, "### BEGIN CHECK PARAMETERS\n\n");

  fprintf(fp, "### These user entries were not used to set values in this run of FRED.\n"); 
  fprintf(fp, "### WARNING: CHECK THESE ENTRIES CAREFULLY. THEY MAY BE MISSPELLED.\n"); 
  fprintf(fp, "### BEGIN UNUSED USER-DEFINED PROPERTIES\n");
  for (int i = 0; i < size; i++) {
    if (Property::property_is_default[i] == 0 && Property::property_is_duplicate[i] == 0 && Property::property_is_used[i] == 0) {
      fprintf(fp, "%s = %s\n",
	      Property::property_names[i].c_str(),
	      Property::property_values[i].c_str());
    }
  }
  fprintf(fp, "### END   UNUSED USER-DEFINED PROPERTIES\n\n");

  fprintf(fp, "### These user entries appear in the FRED program but are overridden by later entries.\n"); 
  fprintf(fp, "### BEGIN DUPLICATE USER-DEFINED PROPERTIES\n");
  for (int i = 0; i < size; i++) {
    if (Property::property_is_default[i] == 0 && Property::property_is_duplicate[i] == 1) {
      fprintf(fp, "%s = %s\n",
	      Property::property_names[i].c_str(),
	      Property::property_values[i].c_str());
    }
  }
  fprintf(fp, "### END   DUPLICATE USER-DEFINED PROPERTIES\n\n");

  fprintf(fp, "### These user entries appear in the FRED program and are not overridden by later entries.\n"); 
  fprintf(fp, "### BEGIN AVAILABLE USER-DEFINED PROPERTIES\n");
  for (int i = 0; i < size; i++) {
    if (Property::property_is_default[i] == 0 && Property::property_is_duplicate[i] == 0) {
      fprintf(fp, "%s = %s\n",
	      Property::property_names[i].c_str(),
	      Property::property_values[i].c_str());
    }
  }
  fprintf(fp, "### END   AVAILABLE USER-DEFINED PROPERTIES\n\n");

  fprintf(fp, "### These user entries were used to set values in this run of FRED.\n"); 
  fprintf(fp, "### BEGIN USED USER-DEFINED PROPERTIES\n");
  for (int i = 0; i < size; i++) {
    if (Property::property_is_default[i] == 0 && Property::property_is_used[i] == 1) {
      fprintf(fp, "%s = %s\n",
	      Property::property_names[i].c_str(),
	      Property::property_values[i].c_str());
    }
  }
  fprintf(fp, "### END   USED USER-DEFINED PROPERTIES\n\n");

  fprintf(fp, "### These entries appear in the config file but are overridden by later entries.\n"); 
  fprintf(fp, "### BEGIN OVERRIDDEN DEFAULT PROPERTIES\n");
  for (int i = 0; i < size; i++) {
    if (Property::property_is_default[i] && Property::property_is_duplicate[i] == 1) {
      fprintf(fp, "%s = %s\n",
	      Property::property_names[i].c_str(),
	      Property::property_values[i].c_str());
    }
  }
  fprintf(fp, "### END   OVERRIDDEN DEFAULT PROPERTIES\n\n");

  fprintf(fp, "### These entries appear in the config file and are not overridden by later entries.\n"); 
  fprintf(fp, "### BEGIN AVAILABLE DEFAULT PROPERTIES\n");
  for (int i = 0; i < size; i++) {
    if (Property::property_is_default[i] && Property::property_is_duplicate[i] == 0) {
      fprintf(fp, "%s = %s\n",
	      Property::property_names[i].c_str(),
	      Property::property_values[i].c_str());
    }
  }
  fprintf(fp, "### END   AVAILABLE DEFAULT PROPERTIES\n\n");

  fprintf(fp, "### These entries from the config file were used to set values in this run of FRED.\n"); 
  fprintf(fp, "### BEGIN USED DEFAULT PROPERTIES\n");
  for (int i = 0; i < size; i++) {
    if (Property::property_is_default[i] && Property::property_is_used[i] == 1) {
      fprintf(fp, "%s = %s\n",
	      Property::property_names[i].c_str(),
	      Property::property_values[i].c_str());
    }
  }
  fprintf(fp, "### END   USED DEFAULT PROPERTIES\n\n");

  fprintf(fp, "### The following entries in the config file were ignored in this run of FRED.\n"); 
  fprintf(fp, "### BEGIN UNUSED DEFAULT PROPERTIES\n");
  for (int i = 0; i < size; i++) {
    if (Property::property_is_default[i] && Property::property_is_duplicate[i] == 0 && Property::property_is_used[i] == 0) {
      fprintf(fp, "%s = %s\n",
	      Property::property_names[i].c_str(),
	      Property::property_values[i].c_str());
    }
  }
  fprintf(fp, "### END   UNUSED DEFAULT PROPERTIES\n\n");

  fprintf(fp, "### The following properties were not found in the property files, but\n");
  fprintf(fp, "### default values are specified in the source code.\n"); 
  fprintf(fp, "### \n");
  fprintf(fp, "### BEGIN PROPERTIES NOT FOUND\n");
  for (int i = 0; i < Property::property_not_found.size(); i++) {
    fprintf(fp, "%s\n",
	    Property::property_not_found[i].c_str());
  }
  fprintf(fp, "### END   PROPERTIES NOT FOUND\n\n");

  fprintf(fp, "### END CHECK PARAMETERS\n\n");
  fclose(fp);
}

void Property::read_program_file(char* program_file) {
  FILE *fp;
  char name[FRED_STRING_SIZE];
  char* line = NULL;
  size_t linecap = 0;
  size_t linelen;
  int useFRED = 0;

  fp = Utils::fred_open_file(program_file);
  int first_line = 1;
  if(fp != NULL) {
    fprintf(stdout, "FRED reading program file %s\n", program_file); fflush(stdout);

    int linenum = 0;
    string single_line = "";
    while((linelen = getline(&line, &linecap, fp)) > 0) {

      if (linelen == -1) {
	break;
      }

      linenum++;

      if (unbalanced_parens(line)) {
	Property::error_string += "Missing parenthesis:\n  " + string(line);
	continue;
      }

      // printf("line  before = |%s|\n", line);
      string current = delete_whitespace(line);
      // printf("current after = |%s|\n", current.c_str());
      
      if (current.length()==0) {
	continue;
      }
      
      if (first_line && current.find("use FRED;")==0) {
	first_line = 0;
	int pos1 = current.find("use ");
	int pos2 = current.find("FRED;", pos1);
	if (pos1!=string::npos && pos2!=string::npos) {
	  useFRED = 1;
	  printf("READ_params use FRED;\n");
	  continue;
	}
	else {
	  // printf("READ_params DON'T use FRED;\n");
	}
      }

      if (current.find("include ") == 0) {
	char includefilename[FRED_STRING_SIZE];
	sscanf(current.c_str(), "include %s", includefilename);
	// printf("INCLUDE found: include |%s|\n", includefilename);
	read_program_file(includefilename);
	// exit(0);
	continue;
      }

      if (current.find("use FRED::") == 0) {
	char includefilename[FRED_STRING_SIZE];
	string library_name = current.substr(strlen("use FRED::"));
	sprintf(includefilename, "$FRED_HOME/library/%s/%s.fred", library_name.c_str(),library_name.c_str());
	read_program_file(includefilename);
	continue;
      }

      if (useFRED) {
	int semi = current.find(";");
	if (semi!=string::npos) {
	  string complete = current.substr(0,semi);
	  single_line += complete;
	  single_line = remove_spaces(single_line);
	  // cout << "useFRED READ_params line " << single_line << "\n";
	  process_single_line(single_line, linenum, program_file);
	  if (semi+1 < current.length()) {
	    single_line = current.substr(semi+1,current.length()-semi-1);
	  }
	  else {
	    single_line = "";
	  }
	}
	else {
	  single_line += current + " ";
	}
	continue;
      }
      else {
	process_single_line(current, linenum, program_file);
      }
    }
    process_single_line(single_line, linenum, program_file);
    fclose(fp);
  }
  else {
    fprintf(stdout, "FRED failed reading program file %s\n", program_file); fflush(stdout);
    abort();
    // Utils::fred_abort("Help!  Can't read program_file %s\n", program_file);
  }
  fprintf(stdout, "FRED finished reading program file %s\n", program_file); fflush(stdout);
}

void Property::process_single_line(string linestr, int linenum, char* filename) {

  linestr = delete_whitespace(linestr);
  if (linestr.length()==0) {
    return;
  }
      
  // the line is a proprty statement of it contains = but not any comparision operator
  int pos = linestr.find("=");
  if (pos!=string::npos &&
      linestr.find("==")==string::npos &&
      linestr.find("!=")==string::npos &&
      linestr.find("<=")==string::npos &&
      linestr.find(">=")==string::npos) {

    // remove whitespace from property name
    string property = linestr.substr(0,pos);
    string value = linestr.substr(pos+1);
    value = remove_leading_whitespace(value);
    // printf("property = |%s| value = |%s|\n", property.c_str(), value.c_str());
    std::string::iterator end_pos = std::remove(property.begin(), property.end(), ' ');
    property.erase(end_pos, property.end());    
    // now reformulate the linestr:
    linestr = property + " = " + value;

    // check for missing semi-colon in property statement
    pos = linestr.find("=");
    int pos2 = linestr.find("=", pos+1);
    if (pos2!=string::npos) {
      char point[FRED_STRING_SIZE];
      int pos3 = linestr.find_last_not_of(" ", pos2-1);
      if (pos3!=string::npos) {
	int pos4 = linestr.find_last_of(" ", pos3-1);
	if (pos4!=string::npos) {
	  for (int i = 0; i < pos4; i++) {
	    point[i] = ' ';
	  }
	  point[pos4] = '^';
	  point[pos4+1] = '\0';
	}
	printf("FRED Error: multiple properties in statement: |%s|\n", linestr.c_str());
	printf("FRED Error: missing semi-colon?                %s\n", point);
	exit(0);
      }
    }

  }

  char line[FRED_STRING_SIZE];
  strcpy(line, linestr.c_str());
  // printf("PROCESS_SINGLE_LINE = |%s|\n", line);

  if (linestr.find("if ")==0) {
    Rule::add_rule_line(linestr);
    Global::Use_rules = 1;
    return;
  }

  char name[FRED_STRING_SIZE];
  strcpy(name, "");
  sscanf(line, "%s = ", name);
  if (strcmp(name,"")==0) {
    // no property statement found
    return;
  }

  // printf("PARAM NAME = |%s|\n",name);fflush(stdout);
  string name_str = string(name);
  char value[FRED_STRING_SIZE];
  strcpy(value, "");
  strcpy(value, linestr.substr(linestr.find(" = ")+3).c_str());
  printf("PARAM NAME = |%s| VAL len %d VAL = |%s|\n",name,(int) strlen(value),value);fflush(stdout);
  if(strlen(value) > 0) {
    string value_str(value);
      
    printf("READ_params: %s = |%s|\n", name_str.c_str(), value_str.c_str());
    Property::property_file.push_back(string(filename));
    Property::property_lineno.push_back(linenum);
    Property::property_names.push_back(name_str);
    Property::property_values.push_back(value_str);
    Property::property_is_duplicate.push_back(0);
    Property::property_is_used.push_back(0);
    Property::property_is_default.push_back(Property::default_properties);
    if (name_str.find("include_")!=0 && name_str.find("exclude_")!= 0 && name_str.find(".add")==string::npos) {
      std::unordered_map<std::string,int>::const_iterator got = Property::property_map.find(name_str);
      if ( got != Property::property_map.end() ) {
	Property::property_is_duplicate[got->second] = 1;
      }
    }
    int n = Property::number_of_properties;
    Property::property_map[name_str] = Property::number_of_properties++;
    if (name_str == "include_condition") {
      Condition::include_condition(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "exclude_condition") {
      Condition::exclude_condition(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "include_variable") {
      Person::include_variable(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "exclude_variable") {
      Person::exclude_variable(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "include_list_variable") {
      Person::include_list_variable(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "exclude_list_variable") {
      Person::exclude_list_variable(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "include_global_variable") {
      Person::include_global_variable(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "exclude_global_variable") {
      Person::exclude_global_variable(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "include_global_list_variable") {
      Person::include_global_list_variable(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "exclude_global_list_variable") {
      Person::exclude_global_list_variable(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "include_place") {
      Place_Type::include_place_type(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "exclude_place") {
      Place_Type::exclude_place_type(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "include_network") {
      Network_Type::include_network_type(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str == "exclude_network") {
      Network_Type::exclude_network_type(value_str);
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
    }

    if (name_str.find(".add")!=string::npos) {
      Property::property_is_used[n] = 1;
      Property::property_is_duplicate[n] = 0;
      // printf("Property add: %s %s %s %d\n", name_str.c_str(), Property::property_names[n].c_str(), Property::property_values[n].c_str(), n);
    }

  }
  else {
    Utils::fred_abort("Bad format in on line starting with |%s|\n", name);
  }
  return;
}

bool Property::does_property_exist(string name) {
  std::unordered_map<std::string,int>::const_iterator found = Property::property_map.find(name);
  return (found != Property::property_map.end());
}


std::string Property::find_property(string name) {
  std::unordered_map<std::string,int>::const_iterator found = Property::property_map.find(name);
  if ( found != Property::property_map.end() ) {
    int i = found->second;
    property_is_used[i] = 1;
    return Property::property_values[Property::property_map[name]];
  }
  else {
    Property::property_not_found.push_back(name);
    if (Property::abort_on_failure) {
      Utils::fred_abort("params: %s not found\n", name.c_str()); 
    }
    return "";
  }
}


int Property::get_next_property(string name, string* value, int start) {
  // FRED_VERBOSE(0, "get_next_property %s start %d\n", name.c_str(), start);
  *value = "";
  for (int i = start; i < Property::property_names.size(); i++) {
    if (name == Property::property_names[i]) {
      *value = Property::property_values[i];
      return i;
    }
  }
  return -1;
}

int Property::get_property(string name, bool* p) {
  int temp;
  int n = get_property(name, &temp);
  *p = temp;
  return n;
}


int Property::get_property(string name, int* p) {
  // printf("GET_PARAM: look for %s\n", name.c_str()); fflush(stdout);
  string value = find_property(name);
  if (value == "") {
    return 0;
  }
  sscanf(value.c_str(), "%d", p);
  if (Global::Debug > 1) {
    printf("GET_PARAM: %s = %d\n", name.c_str(), *p);
    fflush( stdout);
  }
  return 1;
}

int Property::get_property(string name, unsigned long* p) {
  // printf("GET_PARAM: look for %s\n", name.c_str()); fflush(stdout);
  string value = find_property(name);
  if (value == "") {
    return 0;
  }
  sscanf(value.c_str(), "%lu", p);
  if (Global::Debug > 1) {
    printf("GET_PARAM: %s = %lu\n", name.c_str(), *p);
    fflush( stdout);
  }
  return 1;
}


int Property::get_property(string name, long long int * p) {
  // printf("GET_PARAM: look for %s\n", name.c_str()); fflush(stdout);
  string value = find_property(name);
  if (value == "") {
    return 0;
  }
  sscanf(value.c_str(), "%lld", p);
  if (Global::Debug > 1) {
    printf("GET_PARAM: %s = %lld\n", name.c_str(), *p);
    fflush( stdout);
  }
  return 1;
}


int Property::get_property(string name, double* p) {
  // printf("GET_PARAM: look for %s\n", name.c_str()); fflush(stdout);
  string value = find_property(name);
  if (value == "") {
    return 0;
  }
  sscanf(value.c_str(), "%lf", p);
  if (Global::Debug > 1) {
    printf("GET_PARAM: %s = %f\n", name.c_str(), *p);
    fflush( stdout);
  }
  return 1;
}

int Property::get_property(string name, float* p) {
  // printf("GET_PARAM: look for %s\n", name.c_str()); fflush(stdout);
  string value = find_property(name);
  if (value == "") {
    return 0;
  }
  sscanf(value.c_str(), "%f", p);
  if (Global::Debug > 1) {
    printf("GET_PARAM: %s = %f\n", name.c_str(), *p);
    fflush( stdout);
  }
  return 1;
}

int Property::get_property(string name, string* p) {
  // printf("GET_PARAM: look for %s\n", name.c_str()); fflush(stdout);
  string value = find_property(name);
  if (value == "") {
    return 0;
  }
  *p = value;
  if (Global::Debug > 1) {
    printf("GET_PARAM: %s = |%s|\n", name.c_str(), p->c_str());
    fflush( stdout);
  }
  return 1;
}

int Property::get_property(string name, string &p) {
  // printf("GET_PARAM: look for %s\n", name.c_str()); fflush(stdout);
  string value = find_property(name);
  if (value == "") {
    return 0;
  }
  p = value;
  if (Global::Debug > 1) {
    printf("GET_PARAM: %s = |%s|\n", name.c_str(), p.c_str());
    fflush( stdout);
  }
  return 1;
}

int Property::get_property(string name, char* p) {
  // printf("GET_PARAM: look for %s\n", name.c_str()); fflush(stdout);
  string value = find_property(name);
  if (value == "") {
    return 0;
  }
  strcpy(p, value.c_str());
  if (Global::Debug > 1) {
    printf("GET_PARAM: %s = |%s|\n", name.c_str(), p);
    fflush( stdout);
  }
  return 1;
}


int Property::get_property_vector(char* name, vector<int> &p){
  char str[FRED_STRING_SIZE];
  int n;
  char* pch;
  int v;
  Property::get_property(name, str);
  pch = strtok(str," ");
  if(sscanf(pch,"%d",&n) == 1){
    for(int i = 0; i < n; ++i){
      pch = strtok(NULL," ");
      if(pch == NULL) {
	Utils::fred_abort("Help! bad property vector: %s\n", name); 
      }
      sscanf(pch,"%d",&v);
      p.push_back(v);
    }
  } else {
    Utils::fred_abort("Incorrect format for vector %s\n", name); 
  }
  return n;
}

int Property::get_property_vector(char* s, vector<double> &p){
  char str[FRED_STRING_SIZE];
  int n = Property::get_property(s, str);
  if (n == 0) {
    return n;
  }
  char* pch = strtok(str," ");
  if (sscanf(pch, "%d", &n) == 1) {
    for (int i = 0; i < n; i++) {
      double v;
      pch = strtok (NULL, " ");
      if(pch == NULL) {
	Utils::fred_abort("Help! bad property vector: %s\n", s); 
      }
      sscanf(pch, "%lf", &v);
      p.push_back(v);
    }
  }
  else {
    Utils::fred_abort("Incorrect format for vector %s\n", s); 
  }
  return n;
}

int Property::get_property_vector(char* s, double* p) {
  char str[FRED_STRING_SIZE];
  int n;
  char* pch;
  Property::get_property(s, str);
  pch = strtok(str, " ");
  if(sscanf(pch, "%d", &n) == 1) {
    for(int i = 0; i < n; ++i) {
      pch = strtok(NULL, " ");
      if(pch == NULL) {
	Utils::fred_abort("Help! bad property vector: %s\n", s);
      }
      sscanf(pch, "%lf", &p[i]);
    }
  } else {
    Utils::fred_abort("params: %s not found\n", s); 
  }
  return n;
}

int Property::get_property_vector(char* s, int* p) {
  char str[FRED_STRING_SIZE];
  int n;
  char* pch;
  Property::get_property(s, str);
  pch = strtok(str, " ");
  if(sscanf(pch, "%d", &n) == 1) {
    for(int i = 0; i < n; ++i) {
      pch = strtok(NULL, " ");
      if(pch == NULL) {
	Utils::fred_abort("Help! bad property vector: %s\n", s);
      }
      sscanf(pch, "%d", &p[i]);
    }
  } else {
    Utils::fred_abort(""); 
  }
  return n;
}


int Property::get_property_vector(char* s, string* p) {
  char str[FRED_STRING_SIZE];
  int n;
  char* pch;
  Property::get_property(s, str);
  pch = strtok(str, " ");
  if(sscanf(pch, "%d", &n) == 1) {
    for(int i = 0; i < n; ++i) {
      pch = strtok(NULL, " ");
      if(pch == NULL) {
	Utils::fred_abort("Help! bad property vector: %s\n", s);
      }
      p[i] = pch;
    }
  } else {
    Utils::fred_abort(""); 
  }
  return n;
}

//added
int Property::get_property_vector_from_string(char *s, vector < double > &p){
  int n;
  char *pch;
  double v;
  pch = strtok(s," ");
  if (sscanf(pch, "%d", &n) == 1) {
    for (int i = 0; i < n; i++) {
      pch = strtok (NULL, " ");
      if (pch == NULL) {
	Utils::fred_abort("Help! bad property vector: %s\n", s); 
      }
      sscanf(pch, "%lf", &v);
      p.push_back(v);
    }
    for( std::vector<double>::const_iterator i = p.begin(); i != p.end(); ++i)
      printf("age!! %f \n", *i); //std::cout << *i << ' ';
    fflush(stdout);
  }
  else {
    Utils::fred_abort("Incorrect format for vector %s\n", s); 
  }
  return n;
}
// end added

int Property::get_property_matrix(char* s, double*** p) {
  int n = 0;
  Property::get_property((char*)s, &n);
  if(n) {
    double* tmp;
    tmp = new double[n];
    Property::get_property_vector((char*)s, tmp);
    int temp_n = (int)sqrt((double)n);
    if(n != temp_n * temp_n) {
      Utils::fred_abort("Improper matrix dimensions: matricies must be square found dimension %i\n", n); 
    }
    n = temp_n;
    (*p) = new double*[n];
    for(int i = 0; i < n; ++i) {
      (*p)[i] = new double[n];
    }
    for(int i = 0; i < n; ++i) {
      for(int j = 0; j < n; ++j) {
	(*p)[i][j] = tmp[i * n + j];
      }
    }
    delete[] tmp;
    return n;
  }
  return -1;
}


pair_vector_t Property::get_edges(string network_name) {
  char values[FRED_STRING_SIZE];
  pair_vector_t result;
  result.clear();
  int size = Property::property_names.size();
  for (int i = 0; i < size; i++) {
    if (Property::property_names[i] == network_name + ".add_edge") {
      strcpy(values, Property::property_values[i].c_str());
      int p1, p2;
      if (sscanf(values, "%d %d", &p1,&p2)==2) {
	if (Global::Compile_FRED==0) {
	  result.push_back(std::make_pair(p1,p2));
	}
	Property::property_is_used[i] = 1;
	Property::property_is_duplicate[i] = 0;
      }
    }
  }
  return result;
}
