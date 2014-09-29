#! /usr/bin/env python

import sys,os,glob,ntpath
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
        if os.getenv('FRED_RESULTS'):
            self.results_dir = os.getenv('FRED_RESULTS')
        else:
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
        ### It is ambiguous as to how the files are named starting from 0 or one
        ### So we need to do this to get all of the proper file names
        
        self.outputFileList = glob.glob(self.run_data_out_dir+"/out*.txt")
        #print str(self.outputFileList)
        if len(self.outputFileList) != self.number_of_runs:
            raise RuntimeError("\nThe number of output files is inconsitent with the RUNS meta data parameter")
    
        self.outputs = {}
        self.outputsAve = []
        self.outputKeyOrdered = []
        firstRun = True
	outCount = 0
        for outFile in self.outputFileList:
            outFileKey = ntpath.basename(outFile)[3:-4]
            #print outFileKey
            self.outputs[outFileKey] = []
            with open(outFile,"rb") as f:
                dayCount = 0
               
                #self.outputsAve.append({})
                for line in f:
                    #print "i = " + str(i) + " DayCount = " + str(dayCount)
                    self.outputs[outFileKey].append({})
	#	    if firstRun: print str(dayCount) 
                    if firstRun: self.outputsAve.append({})
                    lineSplit = line.split()
                    for j in range(0,len(lineSplit),2):
                        key = lineSplit[j]
                        if key not in self.outputKeyOrdered:
                            self.outputKeyOrdered.append(key)
                        value = lineSplit[j+1].strip()
                        
                        self.outputs[outFileKey][dayCount][key] = value
                        #if value.isdigit():
			try:
		            valueN = float(value)
                            if self.outputsAve[dayCount].has_key(key) is False:
                                self.outputsAve[dayCount][key] = 0
                            self.outputsAve[dayCount][key] += float(valueN)
                        except:
                            #print "except: " + str(lineSplit[j]) + ": " + lineSplit[j+1]
                            self.outputsAve[dayCount][key] = lineSplit[j+1]
                    dayCount += 1
            firstRun=False
	    outCount += 1
        daycount = 0
        for day in self.outputsAve:
            for key in day.keys():
                value =  day[key]
                if isinstance(value,float):
                    self.outputsAve[self.outputsAve.index(day)][key] = int(round(value/float(self.number_of_runs)))
		#print "Day: " + str(dayCount) + " key: " + str(key) + " value: " + str(self.outputsAve[self.outputsAve.index(day)][key])	
        #This is a dictionary so that I can load specific runs and index them.
        self.infections_files = {}
        self.infection_file_names = glob.glob(self.run_data_out_dir+"/infections*.txt")

        ## If grid files exist, make a pointer to those
        self.gaiaDir = self.run_data_out_dir+"/GAIA"
            
#   create load members for large data sets from results so that one has control
#   to do them when only needed.

    def get_infection_set(self,run_number):
        infection_file_name = self.run_data_out_dir + "/infections" + str(run_number) + ".txt"
        #print infection_file_name
        try:
            open(infection_file_name,"rb")
        except IOError:
            self.fred.fred_class_error("Unable to load infection file for run " + str(run_number) + ".")

        infSet = FRED_Infection_Set(infection_file_name)
        return infSet
    
    def load_infection_files(self,run_number=None):
        for i in range(1,self.number_of_runs+1):
            ### Not Specifying a run_number gives you all infections files
            if run_number is not None and int(run_number) != i:
                continue
            infection_file_name = self.run_data_out_dir + "/infections" + str(i) + ".txt"
            #print infection_file_name
            try:
                open(infection_file_name,"rb")
            except IOError:
                self.fred.fred_class_error("Unable to load infection file for run " + str(i) + ".")

            self.infections_files[i] = FRED_Infection_Set(infection_file_name)
            
    def print_meta_summary(self):
        print "----- Summary of Meta Data for FRED Job " + self.key + "------"
        print " "
        print "\tDate of Run:\t" + self.date
        print "\tRun Time:\t" + str(self.run_time)
        print "\tRun on Machine:\t" + self.where
        print "\tRun by User:\t" + self.user

    def print_outputs(self,run_number):
        if str(run_number) not in self.outputs.keys():
            raise RuntimeError("Realization %s does not exist for job %s"%(str(run_number),self.key))
                               
        print "------ Outputs for FRED Job " + self.key + " Realization " + str(run_number) + "--------"
        output = self.outputs[str(run_number)]
        for day in output:
            dayString = ""
            for key in self.outputKeyOrdered:
                dayString += "%s %s "%(key,day[key])
            print dayString
            
    def print_ave_outputs(self):
        print "------ Average Outputs from FRED Job " + self.key + "------"
        print " "
        for day in self.outputsAve:
            dayString = ""
            for key in self.outputKeyOrdered:
                dayString += "%s %s "%(key,day[key])
            print dayString
        
    def get_param(self,param_string):
        if self.params_dict.has_key(param_string):
            return self.params_dict[param_string]
        else:
            self.fred.fred_class_error("In FRED run " + str(self.key) + " cannot find parameter: " + param_string)

    def get_meta_variable(self,variable):
        #iif os.access(self.run_meta_dir + "/" + variable,os.F_OK) == True:
       	returnList = []
        if os.access(self.run_meta_dir + "/" + variable,os.F_OK) == True:
            with open(self.run_meta_dir + "/" + variable,"rb") as f:
                for line in f:
                    returnList.append(line.strip())
            if len(returnList) == 1:
		return returnList[0]
            else:
                return returnList
	    #return open(self.run_meta_dir + "/" + variable,"rb").readline().strip()
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
                 
class FRED_Infection_Set:
    def __init__(self,infection_file):
        self.infectionList = []
        self.infectionKey = {}

        with open(infection_file,"rb") as f:
            for line in f:
                ## fill the infectionKey first
                lineSplit = line.split()
                if len(self.infectionKey) == 0:
                    count = 0
                    for i in range(0,len(lineSplit),2):
                        self.infectionKey[str(lineSplit[i])] = count
                        count += 1
                infList = []
                for i in range(1,len(lineSplit),2):
                    infList.append(lineSplit[i])
        	self.infectionList.append(infList)

    def get(self,key,infNum):
        index = self.infectionKey[key]
        return self.infectionList[infNum][index]
    
class FRED_Household_Set:
    def __init__(self,location_file=None):
        self.locations = []
        self.locationsKey = {}
        self.locationsIDDict = {}
        if location_file is not None:
            self.location_file = location_file

            with open(self.location_file,"r") as f:
                header = f.readline()
                headerSplit = header.split(",")
                count = 0
                for column in headerSplit:
                    self.locationsKey[column.strip()] = count
                    count +=1

                lcount = 0
                for line in f:
                    recs = string.split(line,",")
                    self.locations.append(recs)
                    self.locationsIDDict[recs[0]] = lcount
                    lcount += 1

        self.countyList = []
        self.stateList = []
        self.tractList = []
        self.blockList = []
        
        for i in xrange(0,len(self.locations)):
            fipsIndex = self.locationsKey['stcotrbg']
            fips = self.locations[i][fipsIndex]
            stateFips = fips[0:2]
            if stateFips not in self.stateList:
                self.stateList.append(stateFips)
            countyFips = fips[0:5]
            if countyFips not in self.countyList:
                self.countyList.append(countyFips)
            tractFips = fips[0:11]
            if tractFips not in self.tractList:
                self.tractList.append(tractFips)
            if fips not in self.blockList:
                self.blockList.append(fips)

    def get(self,key,locNum):
        index = self.locationsKey[key]
        return self.locations[locNum][index]
    
    def getByID(self,key,locID):
        indexID = self.locationsIDDict[locID]
        index = self.locationsKey[key]
        return self.locations[indexID][index]

    def bounding_box(self):
        maxlat = -999999
        maxlon = -9999999
        minlat = 9999999
        minlon = 9999999
        
        #for location in self.locations:
        for i in range(0,len(self.locations)):
            latIndex = self.locationsKey['latitude']
            lonIndex = self.locationsKey['longitude']
        
            lat = float(self.locations[i][latIndex])
            lon = float(self.locations[i][lonIndex])
            maxlat = max(maxlat,lat)
            maxlon = max(maxlon,lon)
            minlat = min(minlat,lat)
            minlon = min(minlon,lon)

        return (minlat,minlon,maxlat,maxlon)

    
class FRED_People_Set:

    def __init__(self,population_file=None):
        self.people = []
        self.peopleKey = {}
        if population_file is not None:
            self.population_file = population_file

            with open(self.population_file,"r") as f:
                ### header with column headings
                header = f.readline()
		if header[0:2] == "Is":
                    header = f.readline()
                headerSplit = header.split(",")
                count = 0
                for column in headerSplit:
                    self.peopleKey[column.strip()] = count
                    count += 1

                for line in f:
                    recs = string.split(line,",")
                    self.people.append(recs)
                    
    def get(self,key,perNum):
        index = self.peopleKey[key]
        return self.people[int(perNum)][index]

    def __len__(self):
        return len(self.people)

    


