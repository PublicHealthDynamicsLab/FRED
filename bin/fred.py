import sys,os
import string
from datetime import datetime,timedelta

## Translation Dictionaries
FRED_Location_Types = {"H":"Household","W":"Workplace","S":"School","O":"Office","C":"Classroom","X":"Neighborhood"}

class FRED:
    def __init__(self):
        #get FRED_HOME
        self.home_dir = os.getenv('FRED_HOME')
        if self.home_dir == None:
            self.fred_class_error("FRED_HOME environment variable is undefined")

        #set up key variables
        self.results_dir = self.assign_directory(self.home_dir + "/RESULTS")
        self.results_key_file = self.assign_directory(self.results_dir + "/KEY")
        self.results_job_dir = self.assign_directory(self.results_dir + "/JOB")
        
        self.key_dict = dict()
        with open(self.results_key_file,"r") as f:
            for key_pair in f:
                self.key_dict[string.split(key_pair)[0]] = string.split(key_pair)[1]

        #print self.key_dict

    def key_lookup(self,key):
        if self.key_dict.has_key(str(key)):
            return self.key_dict[str(key)]
        else:
            return None

    def print_keys(self):
        print "Currently available Keys in the FRED results directory"
        print "--------------------------------------------------------"
        for key in self.key_dict.keys():
            print key
        print "--------------------------------------------------------"

    @staticmethod
    def fred_class_error(err):
        print "FRED_CLASS_ERROR: " + err
        sys.stdout.flush()
        sys.exit(1)

    def assign_directory(self,dirname):
        if os.access(dirname,os.F_OK) == False:
            self.fred_class_error(dirname +" does not exist")
        else:
            return dirname
        
class FRED_RUN:
    def __init__(self,fred,key):
        assert(isinstance(fred,FRED))
        self.fred = fred
        self.key_value = fred.key_lookup(key)
        if self.key_value == None:
            print "Key Value " + str(key) + " was not found"
            self.fred.print_keys()
            self.fred.fred_class_error("Unable to locate Key Value in Results")
        
        self.key = key
        
        self.run_dir = fred.assign_directory(fred.results_job_dir + "/" + str(self.key_value))
        self.run_data_dir = fred.assign_directory(self.run_dir + "/DATA")
        self.run_meta_dir = fred.assign_directory(self.run_dir + "/META")
        self.run_work_dir = fred.assign_directory(self.run_dir + "/WORK")
        
####### Get Job's Meta data
        self.date = self.get_meta_variable("DATE")
        #print "Date = " + str(self.date)
        self.number_of_runs = int(self.get_meta_variable("RUNS"))
        self.start_date = datetime.strptime(self.get_meta_variable("START"),'%a %b %d %H:%M:%S %Y')
        self.finish_date = datetime.strptime(self.get_meta_variable("FINISHED"),'%a %b %d %H:%M:%S %Y')
        self.run_time = self.finish_date - self.start_date
        self.where = self.get_meta_variable("WHERE")
        self.user = self.get_meta_variable("USER")

        # Store the contents of the log file by line
        self.meta_log_file = list()
        with open(self.run_meta_dir + "/LOG","r") as f:
            for line in f:
                self.meta_log_file.append(line)
        
        # Create a dict of the fred parameters (may have to make more robust if needed)
        self.run_params_file = self.run_meta_dir + "/PARAMS"
        self.run_default_params_file = self.run_meta_dir + "/DEFAULT"
        self.params_dict = dict()

        ## first need to run through params.def
        with open(self.run_default_params_file,"r") as f:
            for lines in f:
                line = string.split(lines,"\n")[0]
                #print "|" + line + "|" + str(len(lines))
                #sys.stdout.flush()
                if len(line) > 1:
                    if line[0] != "#" and line[0] != " ":
                        self.params_dict[string.split(line,"=")[0].strip()] = string.split(line,"=")[1].strip()

        with open(self.run_params_file,"r") as f:
            for lines in f:
                line = string.split(lines,"\n")[0]
                if len(line) > 1:
                    if line[0] != "#" and line[0] != " ":
                        self.params_dict[string.split(line,"=")[0].strip()] = string.split(line,"=")[1].strip()

######### Now populate the DATA Directory
        self.run_data_out_dir = fred.assign_directory(self.run_data_dir + "/OUT")
        self.run_data_reports_dir = fred.assign_directory(self.run_data_dir + "/REPORTS")

        #self.command = open(self.run_data_out_dir + "/COMMAND_LINE","r").readline().strip()

#
        self.infections_files = []
        
#   create load members for large data sets from results so that one has control
#   to do them when only needed.

    def load_infection_files(self):
        for i in range(1,self.number_of_runs+1):
            print i
            inf_list = []
            infection_file_name = self.run_data_out_dir + "/infections" + str(i) + ".txt"
            print infection_file_name
            try:
                open(infection_file_name,"r")
            except IOError:
                self.fred.fred_class_error("Unable to load infection file for run " + str(i) + ".")
                                       
            with open(infection_file_name,"r") as f:
                for line in f:
                    inf_line = string.split(line)
                    inf_list.append(FRED_Infection_Record(inf_line[1],
                                                          inf_line[3],
                                                          inf_line[5],
                                                          inf_line[7],
                                                          inf_line[9],
                                                          inf_line[11],
                                                          inf_line[13]))
            self.infections_files.append(inf_list)
            
    def print_meta_summary(self):
        print "----- Summary of Meta Data for FRED Job " + self.key + "------"
        print " "
        print "\tDate of Run:\t" + self.date
        print "\tRun Time:\t" + str(self.run_time)
        print "\tRun on Machine:\t" + self.where
        print "\tRun by User:\t" + self.user
        
        
    def get_param(self,param_string):
        if self.params_dict.has_key(param_string):
            return self.params_dict[param_string]
        else:
            self.fred.fred_class_error("In FRED run " + str(self.key) + " cannot find parameter: " + param_string)

    def get_meta_variable(self,variable):
        if os.access(self.run_meta_dir + "/" + variable,os.F_OK) == True:
            return open(self.run_meta_dir + "/" + variable,"r").readline().strip()
        else:
            fred.fred_class_error("cannot obtain "+ variable +" from meta data for key = " + str(self.key))

    def print_avail_params(self):
        print "The available FRED parameters defined for run key " + self.key
        param_string = ""
        count = 0
        # find maximum parameter length
        max_len = 0
        for param in self.params_dict.keys():
            if len(param) > max_len:
                max_len = len(param)
        column_width = max_len + 5
        for param in self.params_dict.keys():
            if count == 2:
                print param_string
                param_string = ""
                count = 0
            param_string += param + " "* (column_width-len(param))
            count = count + 1
                 
class FRED_Infection_Record:
    def __init__(self,day,disease_id,infected_id,infected_age,infector_id,infector_age,infection_place):
        self.day = int(day)
        self.infected_id = infected_id
        self.disease_id = int(disease_id)
        self.infected_age = float(infected_age)
        self.infector_id = infector_id
        self.infector_age = float(infector_age)
        self.infection_place = infection_place
        
 #for i in range(1,self.number_of_runs):
        #    self.log_file.append(list())
        #    with open(self.run_meta_dir + "/LOG" + str(i),"r") as f:
        #        for line in f:
        #            self.log_file[i].append(line)
        

class FRED_Location:
    def __init__(self,_id,loc_type,lat,lon):
        self.id = _id
        self.loc_type = loc_type
        self.lat = float(lat)
        self.lon = float(lon)

    def __str__(self):
        return "<FRED_Location> ID: " + str(self.id)\
               + " type: " + FRED_Location_Types[self.loc_type]\
               + " lat,lon = " + str(self.lat) + "," + str(self.lon)


class FRED_Person:
    def __init__(self,_id,age,sex,marital_status,occupation,household,school,workplace):
        self.id = _id
        self.age = float(age)
        self.sex = sex
        self.marital_status = marital_status
        self.occupation = int(occupation)
        self.household = household
        self.school = school
        self.workplace = workplace

    def __str__(self):
        return "<FRED_Person> ID: " + str(self.id)\
               + " age: " + str(self.age) + " sex: " + self.sex\
               + " marital_status: " + self.marital_status\
               + " occupation: " + str(self.occupation)\
               + " household: " + str(self.household)\
               + " school: " + str(self.school)\
               + " workplace: " + str(self.workplace)
                
class FRED_People_Set:

    def __init__(self,population_file=None):
        self.people = list()
        if population_file is not None:
            self.population_file = population_file

            with open(self.population_file,"r") as f:
                number_of_people = int(string.split(f.readline(),"=")[1].strip())
                header = f.readline()
                
                for line in f:
                    recs = string.split(line)
                    self.people.append(FRED_Person(recs[0],recs[1],recs[2],recs[3],recs[4],recs[5],recs[6],recs[7]))


    def __len__(self):
        return len(self.people)

    
    def reduce(self,filter_age=None,filter_marital_status=None,
               filter_occupation=None,filter_household=None,filter_school=None,filter_workplace=None):
        reduced_people_set = FRED_People_Set()
        count = 0
        for person in self.people:
            reduced_people_set.people.append(person)
            ## count = count + 1
            ## if count == 400:
            ##     break
        
        ## Filter age
        if filter_age is not None:
            if isinstance(filter_age,list) == False:
                print("FRED_People_Set ERROR: Filter age must be a list")
                sys.exit(1)
            else:
                for filter in filter_age:
                    if isinstance(filter,int) == False:
                        print("FRED_People_Set ERROR: Filter age must only contain integers")
                        sys.exit(1)
            person_new = [elem for elem in reduced_people_set.people if elem.age in filter_age]
            reduced_people_set.people = person_new
            
        
        allowed_marital_status = {'M':'1','S':'0'}
        if filter_marital_status is not None:
            if filter_marital_status in allowed_marital_status.keys():
                person_new = [elem for elem in reduced_people_set.people
                              if elem.marital_status == allowed_marital_status[filter_marital_status]]
                reduced_people_set.people = person_new

        if filter_occupation is not None:
            if isinstance(filter_occupation,list) == False:
                print("FRED_People_Set ERROR: Filter occupation must be a list")
                sys.exit(1)
            else:
                for filter in filter_occupation:
                    if isinstance(filter,int) == False:
                        print("FRED_People_Set ERROR: Filter occupation must only contain integers")
                        sys.exit(1)
            person_new = [elem for elem in reduced_people_set.people if elem.occupation in filter_occupation]
            reduced_people_set.people = person_new
            
        if filter_school is not None:
            if isinstance(filter_school,list) == False:
                print("FRED_People_Set ERROR: Filter school must be a list")
                sys.exit(1)
            person_new = []
            for person in reduced_people_set.people:
                for filter in filter_school:
                    flength = len(filter)
                    if person.school[0:flength] == filter:
                        person_new.append(person)
            reduced_people_set.people = person_new

        if filter_household is not None:
            if isinstance(filter_household,list) == False:
                print("FRED_People_Set ERROR: Filter household must be a list")
                sys.exit(1)
            person_new = []
            for person in reduced_people_set.people:
                for filter in filter_household:
                    flength = len(filter)
                    if person.household[0:flength] == filter:
                        person_new.append(person)
            reduced_people_set.people = person_new

            if filter_workplace is not None:
                if isinstance(filter_workplace,list) == False:
                    print("FRED_People_Set ERROR: Filter workplace must be a list")
                    sys.exit(1)
                person_new = []
                for person in reduced_people_set.people:
                    for filter in filter_workplace:
                        flength = len(filter)
                        if person.workplace[0:flength] == filter:
                            person_new.append(person)
                reduced_people_set.people = person_new
                    
        return reduced_people_set

class FRED_Locations_Set:
    
    def __init__(self,location_file,loc_type=None):
        self.location_file = location_file
        self.locations = dict()
        
        with open(location_file,"r") as f:
            number_of_locations = int(string.split(f.readline(),"=")[1].strip())
            header = f.readline()
            
            #print "|" + str(number_of_locations) + "|"

            for line in f:
                recs = string.split(line)
                if self.locations.has_key(recs[0]):
                    print "FRED_Locations_Set ERROR: Detected a duplicate location number in file "\
                          + location_file + " for the key " + str(recs[0])
                #tuple = (type, lat, lon)
                if loc_type == None or loc_type == recs[1]:
                    self.locations[recs[0]] = FRED_Location(recs[0],recs[1],recs[2],recs[3])
        
    def print_location_statistics(self):
        counts = dict()
        for location in self.locations.keys():
            types = self.locations[location].loc_type
            if counts.has_key(types) == False:
                counts[types] = 0
                
            counts[types] = counts[types] + 1

        total = 0
        for types in counts.keys():
            print FRED_Location_Types[types] + ": \t" + str(counts[types])
            total = total + counts[types]
        print "Total: \t\t" + str(total)

    def bounding_box(self):
        maxlat = -9999999
        maxlon = -9999999
        minlat = 9999999
        minlon = 9999999
        
        for location in self.locations.keys():
            lat = self.locations[location].lat
            lon = self.locations[location].lon
            maxlat = max(maxlat,lat)
            maxlon = max(maxlon,lon)
            minlat = min(minlat,lat)
            minlon = min(minlon,lon)

        return (minlat,minlon,maxlat,maxlon)
