import os,sys,string
import math
import optparse
from fred import FRED,FRED_RUN,FRED_Infection_Set,FRED_Household_Set,FRED_People_Set
import apollo131 as apollo
import time
import glob,hashlib
#from threading import Thread
from multiprocessing import Process,Queue
import random 

def fillInfection(q,j,fred_run):
        InfByBG = []
	numDays = fred_run.get_param("days")
	for i in range(0,int(numDays)):
		InfByBG.append({})

        infection_file = fred_run.infection_file_names[j]
        run_number = infection_file.split("infections")[1].split(".")[0]
        print "Run = " + run_number
        infSet = fred_run.get_infection_set(int(run_number))
        print "Length = %d"%len(infSet.infectionList)
        for i in range(0,len(infSet.infectionList)):
		day = int(infSet.get('day',i))
		infected_id = int(infSet.get('host',i))
            #personRec = fred_population.people[int(infRec.infected_id)]
		personFips = fred_population.get("stcotrbg",infected_id)
            #if personFips == "420034754012":
            #    print "There is someone here on Day %d"%(day)
            #print personFips
	#	if personFips not in fipsList:
	#		fipsList.append(personFips)
            #print "%d %d"%(j,day)
		if InfByBG[day].has_key(personFips) is False:
			InfByBG[day][personFips] = 0.0
		InfByBG[day][personFips] += 1
	q.put(InfByBG)

if __name__ == '__main__':
    parser = optparse.OptionParser(usage="""
    %prog [--help][-k key][-r run_number][-t vis_type]
    -k or --key   Fred Key
    -r or --run   The run number of vizualize
                  ave = produce an average result
                  all = produce an input for all runs
   
    -w or --time  Turn on profiling
    -d or --debug Turn on debug printing
    """)

    parser.add_option("-k","--key",type="string",
                      help="The FRED key for the run you wish to visualize")
    parser.add_option("-r","--run",type="string",
                      help="The number of the run you would like to visualize (number,ave, or all)",
                      default=1)
    parser.add_option("-w","--time",action="store_true",
                      default=False)
    parser.add_option("-d","--debug",action="store_true",
                      default=False)

    opts,args=parser.parse_args()

    DBHosts = ["warhol-fred.psc.edu"]
    ### This only currently works for allegheny county

    #### Initialize the FRED SIMS
    fred = FRED()
    
    key = opts.key

    run = opts.run

    try:
        int(run)
    except:
        if run != "ave" and run != "all":
            print "Invalid parameter for run, needs to be an integer, all or ave"
            sys.exit(1)

    fred_run = FRED_RUN(fred,key)
    numberDays = int(fred_run.get_param("days"))
    outputsAve = fred_run.outputsAve

    apolloDBs = [ apollo.ApolloDB(host_=x) for x in DBHosts ]
    for apolloDB in apolloDBs:
	apolloDB.connect() 
	concString = ""
	m5hash = ""
    ### create a string of the concatenated input files
	exclusionList = ['dbfile.txt','params.default']
	fred_work_dir = fred_run.run_work_dir
	if os.path.exists(fred_work_dir) is True:
	    workingFiles = glob.glob(fred_work_dir + "/*")
	    ### concatenate params file first
	    ### find it and add it to the exclusionList
	    print "Fred PARAMS file is " + fred_run.run_params_file
	    for workingFile in workingFiles:
		print "looking at workingFile " + str(workingFile)
		with open(workingFile,"rb") as f:
		    if f.readline() == "#FRED PARAM FILE\n":
			exclusionList.append(os.path.basename(workingFile))

	    print str(exclusionList)
	    concList = []
	    ### First the params List
	    concList.append("===== FRED Parameter File =======\n")
	    with open(fred_run.run_params_file,"rb") as f:
		for line in f.readlines():
		    concList.append(line)
#	    for keyV,value in fred_run.params_dict.items():
#		concList.append("%s = %s\n"%(keyV,value))

	    ### Next add all of the files in the working directory that should be
	    for workingFile in workingFiles:
		if os.path.basename(workingFile) not in exclusionList:
		    concList.append("====== %s =======\n"%os.path.basename(workingFile))
		    with open(workingFile,"rb") as f:
			for line in f:
			    concList.append(line)
	    concString = "".join(concList)
	    m = hashlib.md5()
	    m.update(concString)
	    m5hash = m.hexdigest()
	    print "M5Hash = " + m5hash
	else:
	    print "!!!WARNING!!! No FRED working directory exists at %s"%fred_work_dir
	    print "              Not doing adding configuration or MD5 hash to database"
	
	## Fill in the run table
    	SQLString = 'INSERT INTO run set label = "' + key + '",'\
		    +' configurationFile = "' + concString + '",'\
		    +' md5HashOfConfigurationFile = "' + m5hash + '"'
	apolloDB.query(SQLString)
    	runInsertID = apolloDB.insertID()

    	stateList = {'S':'susceptible','E':'exposed','I':'infectious','R':'recovered','C':'newly exposed',
		     'V':'received vaccine control measure','Av':'received antiviral control measure',
		     'ScCl':'school that is closed'}
    	#locationList = ['42003'] # need to replace with an automatic way of getting this info
	locationList = [fred_run.get_param('fips')]
    	stateInsertIDDict = {}

    ### setup simulated_population_axis
    
    	axisInsertIDDict = apolloDB._populationAxis()
	
    ### setup simulated_populations table
    	for state in stateList.keys():
	    for location in locationList:
		stateInsertIDDict[(state,location)] =\
		  apolloDB.checkAndAddSimulatedPopulation(stateList[state],location)
			
    	for state in stateList.keys():
	    for location in locationList:
		for day in outputsAve:
		    stateFU = "%s_0"%state
		    stateUse = None
                    if stateFU in day.keys(): 
			    stateUse = stateFU
		    if state in day.keys():
			    stateUse = state
		    if stateUse is not None:
			    populationID = stateInsertIDDict[(state,location)]
			    SQLString = 'INSERT INTO time_series set '\
				+'run_id = "' + str(runInsertID) + '", '\
				+'population_id = "' + str(populationID) + '", '\
				+'time_step = "' + str(day['Day']) +'", '\
				+'pop_count = "' + str(day[stateUse]) + '"'
			    #if stateUse == "V":
			   # 	print SQLString
			    apolloDB.query(SQLString)

	

    synth_pops_dir = fred_run.get_param("synthetic_population_directory")
    synth_pop_dir = os.path.expandvars(synth_pops_dir + "/2005_2009_ver2_"+location)
    #synth_pop_id = fred_run.get_param("synthetic_population_id")
    synth_pop_prefix = os.path.expandvars(synth_pop_dir +"/2005_2009_ver2_"+location)
    synth_pop_filename = synth_pop_prefix + "_synth_people.txt"
    synth_pop_filename_fsz = synth_pop_filename + ".fsz"
    synth_household_filename = synth_pop_prefix + "_synth_households.txt"
    
    #try:
    #    open(synth_pop_filename,"r")
    #except IOError:
    #    print "Problem opening FRED population file " + synth_pop_filename
    #    sys.exit(1)
	
    fred_population = None
    rand = str(random.random()*100000)
    if os.path.exists(synth_pop_filename_fsz):
    	tmpFilename = synth_pop_dir + "/tmp_"+str(rand) + ".txt"
    	os.system("fsz -u " + synth_pop_filename_fsz + " > " + tmpFilename) 
	fred_population = FRED_People_Set(tmpFilename)
        os.remove(tmpFilename)
    else:
	fred_population = FRED_People_Set(synth_pop_filename)
 
    numDays = fred_run.get_param("days")
    aveInfByBG = []
    fipsList = []
    time1 = time.time()
    for i in range(0,int(numDays)):
	aveInfByBG.append({})
    InfByBG = []

    #for infection_file in fred_run.infection_file_names:
    queues = [] 
    threads = []
    for j in range(0,len(fred_run.infection_file_names)):
	queues.append(Queue())
	threads.append(Process(target=fillInfection,args=(queues[j],j,fred_run)))

    for t in threads:
	t.start()
        	
    for q in queues:
	InfByBG.append(q.get())

    for t in threads:
	t.join()	

    ### finish off averages
    numRuns = fred_run.number_of_runs
    for infFile in InfByBG:
	sum = 0
        for day in range(0,len(infFile)):
	    for fips in infFile[day].keys():
		if fips not in fipsList:
		   fipsList.append(fips)
		if fips not in aveInfByBG[day].keys():
		   aveInfByBG[day][fips]=0.0
	        aveInfByBG[day][fips] += infFile[day][fips] 
		sum += infFile[day][fips]
	print "Sum = " + str(sum)

    for day in range(0,len(aveInfByBG)):
            for fips in aveInfByBG[day].keys():
                aveInfByBG[day][fips] /= float(numRuns) 

    time2 = time.time()
    print "Time to calculate ave = %10.5f seconds"%(time2-time1) 
    for apolloDB in apolloDBs:
    ### update simulated_population
	fipsInsertIDDict = {}
	for fips in fipsList:
	    insertID = apolloDB.checkAndAddSimulatedPopulation('newly exposed',fips)
	    fipsInsertIDDict[fips] = insertID
       	
	time1 = time.time() 
	#    for day in range(0,len(aveInfByBG)):
	#    SQLString = 'INSERT INTO time_series (run_id,population_id,time_step,pop_count)'\
	# 
	#+' VALUES'

	for day in range(0,numberDays):
		'''
	    if False: #day > len(aveInfByBG):
		for fips in fipsList:
		    SQLString = 'INSERT INTO time_series set '\
				+'run_id = "' + str(runInsertID) + '", '\
				+'population_id = "' + str(fipsInsertIDDict[fips]) + '", '\
				+'time_step = "' + str(day) + '", '\
				+'pop_count = "0"'
		    apolloDB.query(SQLString)
		   
	    else:
	    '''
		if(len(aveInfByBG[day].keys())>0):
			SQLStringList = []
			SQLStringList.append('INSERT INTO time_series (run_id,population_id,time_step,pop_count)'\
						     +' VALUES')
			for fips in aveInfByBG[day].keys():
				SQLStringList.append('('\
						     +'"'+str(runInsertID) + '", '\
						     +'"' + str(fipsInsertIDDict[fips]) + '",'\
						     +'"' + str(day) + '", '\
						     +'"' + str(int(aveInfByBG[day][fips])) + '"),')
			SQLStringList[len(SQLStringList)-1] = SQLStringList[len(SQLStringList)-1][:-1]
		#print "SQLSTring = " + SQLString		    
	
			apolloDB.query(''.join(SQLStringList))
                
	time2 = time.time()
	print "Time To populate TimeSeries = %10.5f"%(time2-time1)
	apolloDB.close()
