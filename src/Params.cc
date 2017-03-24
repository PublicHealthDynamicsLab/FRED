/*
  This file is part of the FRED system.

  Copyright (c) 2013-2016, University of Pittsburgh, John Grefenstette,
  David Galloway, Mary Krauland, Michael Lann, and Donald Burke.

  Based in part on FRED Version 2.9, created in 2010-2013 by
  John Grefenstette, Shawn Brown, Roni Rosenfield, Alona Fyshe, David
  Galloway, Nathan Stone, Jay DePasse, Anuroop Sriram, and Donald Burke.

  Licensed under the BSD 3-Clause license.  See the file "LICENSE" for
  more information.
*/

//
//
// File Params.cc
//

#include "Params.h"
#include "Global.h"
#include "Utils.h"
#include <math.h>
#include <string>
#include <sstream>
#include <stdio.h>

using namespace std;

int Params::abort_on_failure = 1;
std::vector<std::string> Params::param_names;
std::vector<std::string> Params::param_values;

void Params::read_psa_parameter(char* paramfile, int line_number) {
  /*
  FILE* fp;
  char name[FRED_STRING_SIZE];

  fp = Utils::fred_open_file(paramfile);
  if(fp != NULL) {
    fprintf(stdout, "FRED reading PSA parameter file %s\n", paramfile); fflush(stdout);
    int current_line = 1;
    while (current_line < line_number) {
      int ch = 1;
      while(ch != EOF && ch != '\n') {
	ch = fgetc(fp);
      }
      if(ch == EOF) {
	Utils::fred_abort("Unexpected EOF in params file %s on line %d\n",
			  paramfile, current_line);
      }
      current_line++;
    }
    if(fscanf(fp, "%s", name) == 1) {
      if(fscanf(fp, " = %[^\n]", Params::param_value[Params::param_count]) == 1) {
	//Remove end of line comments if they are there
	string temp_str(Params::param_value[Params::param_count]);
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
	strcpy(Params::param_value[Params::param_count], temp_str.c_str());
          
	strcpy(Params::param_name[Params::param_count], name);
	printf("READ_PSA_PARAMETER: %s = %s\n", Params::param_name[Params::param_count],
	       Params::param_value[Params::param_count]);
	Params::param_count++;
      } else {
	Utils::fred_abort("Bad format in params file %s on line starting with %s\n",
			  paramfile, name);
      }
    } else {
      Utils::fred_abort("Bad format in params file %s on line %d\n",
			paramfile, current_line);
    }
    fclose(fp);
  } else {
    fprintf(stdout, "FRED failed reading parameter file %s\n", paramfile); fflush(stdout);
    abort();
    // Utils::fred_abort("Help!  Can't read paramfile %s\n", paramfile);
  }
  */
}


void Params::read_parameter_file(char* paramfile) {
  FILE *fp;
  char name[FRED_STRING_SIZE];

  fp = Utils::fred_open_file(paramfile);
  if(fp != NULL) {
    fprintf(stdout, "FRED reading parameter file %s\n", paramfile); fflush(stdout);
    while(fscanf(fp, "%s", name) == 1) {
      if(name[0] == '#') {
        int ch = 1;
        while (ch != '\n')
          ch = fgetc(fp);
        continue;
      } else if (strcmp(name, "include") == 0) {
	char includefilename[FRED_STRING_SIZE];
	if(fscanf(fp, "%s", includefilename) == 1) {
	  read_parameter_file(includefilename);
	}
      } else {
	// printf("PARAM NAME = |%s|\n",name);fflush(stdout);
	string name_str = string(name);
	char value[1024];
        if(fscanf(fp, " = %[^\n]", value) == 1) {
          
          //Remove end of line comments if they are there
          string value_str(value);
          size_t pos;
          string whitespaces(" \t\f\v\n\r");
          
          pos = value_str.find("#");
          if(pos != string::npos) {
            value_str = value_str.substr(0, pos);
          }
          
          //trim trailing whitespace
          pos = value_str.find_last_not_of(whitespaces);
          if (pos != string::npos) {
            if(pos != (value_str.length() - 1))
              value_str.erase(pos + 1);
          } else {
            value_str.clear(); //str is all whitespace
          }
          if(Global::Debug > 2) {
            printf("READ_PARAMS: %s = %s\n", name_str.c_str(), value_str.c_str());
          }
	  if (!name_str.empty() && !value_str.empty()) {
	    param_names.push_back(name_str);
	    param_values.push_back(value_str);
	    printf("READ_PARAMS STRING: %s = |%s|\n", param_names.back().c_str(), param_values.back().c_str());
	  }
        } else {
          Utils::fred_abort("Bad format in params file %s on line starting with %s\n",
			    paramfile, name);
        }
      }
    }
  } else {
    fprintf(stdout, "FRED failed reading parameter file %s\n", paramfile); fflush(stdout);
    abort();
    // Utils::fred_abort("Help!  Can't read paramfile %s\n", paramfile);
  }
  fprintf(stdout, "FRED finished reading parameter file %s\n", paramfile); fflush(stdout);
  fclose(fp);
}


int Params::read_parameters(char* paramfile) {
  char filename[FRED_STRING_SIZE];
  
  param_names.clear();
  param_values.clear();
  strcpy(filename, "$FRED_HOME/data/defaults");
  read_parameter_file(filename);
  read_parameter_file(paramfile);
  Params::get_param("psa_method", Global::PSA_Method);
  Params::get_param("psa_list_file", Global::PSA_List_File);
  Params::get_param("psa_sample_size", &Global::PSA_Sample_Size);
  Params::get_param("psa_sample", &Global::PSA_Sample);
  /*
  if(Global::PSA_Sample > 0) {
    FILE* fp;
    fp = Utils::fred_open_file(Global::PSA_List_File);
    if(fp != NULL) {
      char psa_param[FRED_STRING_SIZE];
      while(fscanf(fp, "%s", psa_param) == 1) {
	sprintf(filename, "$FRED_HOME/data/PSA/%s/%d/%s",
		Global::PSA_Method, Global::PSA_Sample_Size, psa_param);
	read_psa_parameter(filename, Global::PSA_Sample);
      }
      fclose(fp);
    } else {
      printf("read_params: failed to open PSA list file %s\n", Global::PSA_List_File);
      abort();
    }
  }
  */
  if(Global::Debug > 1) {
    for(int i = 0; i < Params::param_names.size(); ++i) {
      printf("READ_PARAMS: %s = %s\n", Params::param_names[i].c_str(), Params::param_values[i].c_str());
    }
  }
  return Params::param_names.size();
}

int Params::get_param(string name, int* p) {
  int found = 0;
  int n = Params::param_names.size();
  for(int i = 0; i < n; ++i) {
    if(Params::param_names[i] == name) {
      if(sscanf(Params::param_values[i].c_str(), "%d", p)) {
        found = 1;
      }
    }
  }
  if(found) {
    if (Global::Debug > 0) {
      printf("PARAMS: %s = %d\n", name.c_str(), *p);
      fflush( stdout);
    }
    return 1;
  } else {
    if(Global::Debug > 0) {
      printf("PARAMS: %s not found\n", name.c_str());
      fflush(stdout);
    }
    if (Params::abort_on_failure) {
      Utils::fred_abort("PARAMS: %s not found\n", name.c_str()); 
    }
  }
  return 0;
}

int Params::get_param(string name, unsigned long* p) {
  int found = 0;
  int n = Params::param_names.size();
  for(int i = 0; i < n; ++i) {
    if(Params::param_names[i] == name) {
      if(sscanf(Params::param_values[i].c_str(), "%lu", p)) {
        found = 1;
      }
    }
  }
  if(found) {
    if(Global::Debug > 0) {
      printf("PARAMS: %s = %lu\n", name.c_str(), *p);
      fflush( stdout);
    }
    return 1;
  } else {
    if(Global::Debug > 0) {
      printf("PARAMS: %s not found\n", name.c_str());
      fflush( stdout);
    }
    if (Params::abort_on_failure) {
      Utils::fred_abort("PARAMS: %s not found\n", name.c_str()); 
    }
  }
  return 0;
}

int Params::get_param(string name, double* p) {
  int found = 0;
  int n = Params::param_names.size();
  for(int i = 0; i < n; ++i) {
    if(Params::param_names[i] == name) {
      if(sscanf(Params::param_values[i].c_str(), "%lf", p)) {
        found = 1;
      }
    }
  }
  if(found) {
    if(Global::Debug > 0) {
      printf("PARAMS: %s = %f\n", name.c_str(), *p);
      fflush( stdout);
    }
    return 1;
  } else {
    if(Global::Debug > 0) {
      printf("PARAMS: %s not found\n", name.c_str());
      fflush( stdout);
    }
    if (Params::abort_on_failure) {
      Utils::fred_abort("PARAMS: %s not found\n", name.c_str()); 
    }
  }
  return 0;
}

int Params::get_param(string name, float* p) {
  int found = 0;
  int n = Params::param_names.size();
  for(int i = 0; i < n; ++i) {
    if(Params::param_names[i] == name) {
      if(sscanf(Params::param_values[i].c_str(), "%f", p)) {
        found = 1;
      }
    }
  }
  if(found) {
    if(Global::Debug > 0) {
      printf("PARAMS: %s = %f\n", name.c_str(), *p);
      fflush( stdout);
    }
    return 1;
  } else {
    if(Global::Debug > 0) {
      printf("PARAMS: %s not found\n", name.c_str());
      fflush( stdout);
    }
    if (Params::abort_on_failure) {
      Utils::fred_abort("PARAMS: %s not found\n", name.c_str());
    }
  }
  return 0;
}

int Params::get_param(string name, string &p){
  int found = 0;
  int n = Params::param_names.size();
  for(int i = 0; i < n; ++i) {
    if(Params::param_names[i] == name) {
      p = Params::param_values[i];
      found = 1;
    }
  }
  if(found) {
    if(Global::Debug > 0) {
      printf("PARAMS: %s = %s\n", name.c_str(), p.c_str());
      fflush( stdout);
    }
    return 1;
  } else {
    if(Global::Debug > 0) {
      printf("PARAMS: %s not found\n", name.c_str());
      fflush(stdout);
    }
    if (Params::abort_on_failure) {
      Utils::fred_abort("PARAMS: %s not found\n", name.c_str()); 
    }
  }
  return 0;
}

int Params::get_param(string name, char* p) {
  int found = 0;
  int n = Params::param_names.size();
  for(int i = 0; i < n; ++i) {
    if(Params::param_names[i] == name) {
      if(strcpy(p, Params::param_values[i].c_str())) {
        found = 1;
      }
    }
  }
  if(found) {
    if(Global::Debug > 0) {
      printf("PARAMS: %s = %s\n", name.c_str(), p);
      fflush( stdout);
    }
    return 1;
  } else {
    if(Global::Debug > 0) {
      printf("PARAMS: %s not found\n", name.c_str());
      fflush( stdout);
    }
    if (Params::abort_on_failure) {
      Utils::fred_abort("PARAMS: %s not found\n", name.c_str()); 
    }
  }
  return 0;
}

int Params::get_param_vector(char* name, vector<int> &p){
  char str[FRED_STRING_SIZE];
  int n;
  char* pch;
  int v;
  Params::get_param(name, str);
  pch = strtok(str," ");
  if(sscanf(pch,"%d",&n) == 1){
    for(int i = 0; i < n; ++i){
      pch = strtok(NULL," ");
      if(pch == NULL) {
        Utils::fred_abort("Help! bad param vector: %s\n", name); 
      }
      sscanf(pch,"%d",&v);
      p.push_back(v);
    }
  } else {
    Utils::fred_abort("Incorrect format for vector %s\n", name); 
  }
  return n;
}

int Params::get_param_vector(char* s, vector<double> &p){
  char str[FRED_STRING_SIZE];
  int n = Params::get_param(s, str);
  if (n == 0) {
    return n;
  }
  char* pch = strtok(str," ");
  if (sscanf(pch, "%d", &n) == 1) {
    for (int i = 0; i < n; i++) {
      double v;
      pch = strtok (NULL, " ");
      if(pch == NULL) {
        Utils::fred_abort("Help! bad param vector: %s\n", s); 
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

int Params::get_param_vector(char* s, double* p) {
  char str[FRED_STRING_SIZE];
  int n;
  char* pch;
  Params::get_param(s, str);
  pch = strtok(str, " ");
  if(sscanf(pch, "%d", &n) == 1) {
    for(int i = 0; i < n; ++i) {
      pch = strtok(NULL, " ");
      if(pch == NULL) {
        Utils::fred_abort("Help! bad param vector: %s\n", s);
      }
      sscanf(pch, "%lf", &p[i]);
    }
  } else {
    Utils::fred_abort("PARAMS: %s not found\n", s); 
  }
  return n;
}

int Params::get_param_vector(char* s, int* p) {
  char str[FRED_STRING_SIZE];
  int n;
  char* pch;
  Params::get_param(s, str);
  pch = strtok(str, " ");
  if(sscanf(pch, "%d", &n) == 1) {
    for(int i = 0; i < n; ++i) {
      pch = strtok(NULL, " ");
      if(pch == NULL) {
        Utils::fred_abort("Help! bad param vector: %s\n", s);
      }
      sscanf(pch, "%d", &p[i]);
    }
  } else {
    Utils::fred_abort(""); 
  }
  return n;
}


int Params::get_param_vector(char* s, string* p) {
  char str[FRED_STRING_SIZE];
  int n;
  char* pch;
  Params::get_param(s, str);
  pch = strtok(str, " ");
  if(sscanf(pch, "%d", &n) == 1) {
    for(int i = 0; i < n; ++i) {
      pch = strtok(NULL, " ");
      if(pch == NULL) {
        Utils::fred_abort("Help! bad param vector: %s\n", s);
      }
      p[i] = pch;
    }
  } else {
    Utils::fred_abort(""); 
  }
  return n;
}

//added
int Params::get_param_vector_from_string(char *s, vector < double > &p){
  int n;
  char *pch;
  double v;
  pch = strtok(s," ");
  if (sscanf(pch, "%d", &n) == 1) {
    for (int i = 0; i < n; i++) {
      pch = strtok (NULL, " ");
      if (pch == NULL) {
        Utils::fred_abort("Help! bad param vector: %s\n", s); 
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

int Params::get_param_matrix(char* s, double*** p) {
  int n = 0;
  Params::get_param((char*)s, &n);
  if(n) {
    double* tmp;
    tmp = new double[n];
    Params::get_param_vector((char*)s, tmp);
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


bool Params::does_param_exist(string name) {
  bool found = false;
  int n = param_names.size();
  for(int i = 0; i < n && !found; ++i) {
    found = (name == param_names[i]);
  }
  return found;
}
