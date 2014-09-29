#include <stdlib.h>
#include <stdio.h>
#include <string>


#include "Tracker.h"
using namespace std;
int main(void){
  Tracker<int> dayTrack;
  //string title = dayTrack.title;
  //cout << "Tracker Title: "
  printf("Tracker Title: %s\n",dayTrack.get_title().c_str());
  printf("Tracker Variable Name: %s\n",dayTrack.get_index_name().c_str());
  dayTrack.add_index(1);
  dayTrack.add_index(34);
  dayTrack.add_index(2);
  int pos = dayTrack.get_index_position(34);
  printf("Tracker 34 at dayTrack %d\n",dayTrack.get_index_value(pos));
  dayTrack.add_key("Int Key","int");
  dayTrack.add_key("Double Key","double");
  dayTrack.add_key("String Key","string");
  printf("Is Double %d\n",int(dayTrack.is_allowed_type("double")));
  printf("Is Double %d\n",int(dayTrack.is_allowed_type("mytype")));
  printf("Has Key (Double Key): %s\n",dayTrack.has_key("Double Key").c_str());
  printf("Has Key (Shorts): %s\n",dayTrack.has_key("Shorts").c_str());
  printf("%s\n",dayTrack.print_key_table().c_str());
  
  // Int
  dayTrack.set_index_key_pair(1,"Int Key",3);
  dayTrack.set_index_key_pair(34,"Int Key",34);
  dayTrack.increment_index_key_pair(2,"Int Key",2);
  
  // String
  dayTrack.set_index_key_pair(1,"String Key","testString1");
  dayTrack.set_index_key_pair(34,"String Key","testString2");
  dayTrack.set_index_key_pair(2,"String Key","testString3");
  dayTrack.increment_index_key_pair(34,"String Key");
  
  // Double
  dayTrack.set_index_key_pair(1,"Double Key",3.43);
  dayTrack.set_index_key_pair(34,"Double Key",34.2509809853);
  dayTrack.increment_index_key_pair(2,"Double Key");

  dayTrack.add_index(45);
  dayTrack.add_key("New Key","int");
  
  dayTrack.set_index_key_pair(20,"New Key 2",30.0);
  dayTrack.set_index_key_pair(21,"New Key 3","New String");
  dayTrack.set_index_key_pair(22,"New Key 4",30);

  printf("%s",dayTrack.print_key_index_list("Test KEY").c_str());
  printf("%s",dayTrack.print_key_index_list("String Key").c_str());
  printf("%s",dayTrack.print_key_index_list("Double Key").c_str());

  printf("Inline Report Format\n\n");
  printf("%s",dayTrack.print_inline_report_format().c_str());
  
  FILE* outfile = fopen("outfile.test","w");
  dayTrack.output_inline_report_format(outfile);
  
  fclose(outfile);
  
  outfile = fopen("outfile.csv","w");
  dayTrack.output_csv_report_format(outfile);
  fclose(outfile);
  
  //Test collectives
  dayTrack.reset_all_index_key_pairs_to_zero();
  dayTrack.set_all_index_for_key("Int Key",55);
  
  outfile = fopen("outColl.csv","w");
  dayTrack.output_csv_report_format(outfile);
  fclose(outfile);
  
  
  //vector < string > intKeys = dayTrack._get_keys("int");
  //for(int i=0; i<intKeys.size();++i)
  //  printf("Key: %s\n",intKeys[i].c_str());
  //vector < pair < string, string > > testString;
  //testString.push_back(pair <string, string> ("Test","String"));
  return 0;
}
