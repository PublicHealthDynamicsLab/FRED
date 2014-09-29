import os,sys,string
import math
import optparse
from fred import FRED,FRED_RUN,FRED_Infection_Set,FRED_Household_Set,FRED_People_Set
import apollo
import time
import glob,hashlib
import datetime
import random
#from threading import Thread
from multiprocessing import Process,Queue

def statusUpdate(status,message):
	statusFile = './starttime'
	with open(statusFile,"wb") as f:
		f.write("%s %s"%(status,message))

def error_exit(message):
	statusUpdate("LOGERROR",message)
	print message
	sys.exit(1)
	
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
    parser.add_option("-i","--runId",type="string",
		      help="The Apollo RunId for this call")
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
    simulationRunId = opts.runId

    statusFile = "./starttime"

    if simulationRunId is None:
	    error_exit("Need to specify an Apollo SimulationRunId to use this program")

    try:
	    fred_run = FRED_RUN(fred,key)
	    numberDays = int(fred_run.get_param("days"))
	    outputsAve = fred_run.outputsAve
    except Exception as e:
	    error_exit(str(e))

    try:
	    apolloDBs = [ apollo.ApolloDB(host_=x) for x in DBHosts ]
	    
	    for apolloDB in apolloDBs:
		    print "working on DB: " + str(apolloDB._host)
		    apolloDB.connect() 
		    
		    ### Get the runId from the database 
		   
		    locationList = [fred_run.get_param('fips')]
		    for state,fileName in apolloDB.stateToDataFileDict.items():
			    stateStringList = []
			    if state == "C": continue
			    for location in locationList:	    
				    for day in outputsAve:
					    state_dis = "%s_0"%state
					    if state_dis in day.keys():
						    stateStringList.append("US%s %d %d:1\n"%(str(location),day[state_dis],day['Day']))
			    
			    
			    
			    stateString = "".join(stateStringList)
			    if stateString == "":continue
			    m = hashlib.md5()
			    m.update(stateString)
			    m5hash = m.hexdigest()
			    SQLString = 'INSERT INTO run_data_content (text_content, md5_hash_of_content) values ("%s","%s")'%(stateString,m5hash)
			    #print SQLString
			    apolloDB.query(SQLString)
			    runDataContentId = apolloDB.insertID()
			    #print "stateString = " + stateString
			    runDataDescriptionId = apolloDB.getRunDataDescriptionId(fileName)

			    SQLString = 'INSERT INTO run_data (run_id, description_id, content_id) values '\
				'("%s","%s","%s")'%(simulationRunId,runDataDescriptionId,runDataContentId)
		            #print SQLString
			    apolloDB.query(SQLString)
			    
			    with open(fileName,"wb") as f:
				    f.write("".join(stateStringList))
    except Exception as e:
	    error_exit(str(e))

    try:
	    numDays = int(fred_run.get_param("days"))
            bgoutStr = []
	    aveInfByBG = []
	    fipsSet = set()
	    time1 = time.time()
	    for i in range(0,numDays):
		    aveInfByBG.append({})

	    
	    AllInfs = []
	    time1 = time.time()
	    for j in range(0,len(fred_run.block_infection_file_names)):
		    #print "J = " + str(j)
		    InfSet = []
		    for i in range(0,int(numDays)):
			    InfSet.append({})

		    bgInfSet = fred_run.get_block_infection_set(j+1)
		    #print "Length = " + str(len(bgInfSet.infectionList))
		    for i in range(0,len(bgInfSet.infectionList)):
			    #print str(bgInfSet.infectionList[i])
			    day = bgInfSet.get("Day",i)
			    bg = bgInfSet.get("BlockGroup",i)
			    inf = bgInfSet.get("C",i)
			    #print "BG: %s Day: %d inf: %d"%(str(bg),int(day),int(inf))
			    InfSet[int(day)][bg] = float(inf)
		    AllInfs.append(InfSet)
	    time2 = time.time()
	    print "Time to get data = %10.2lf"%(time2-time1)
	    time1 = time.time()
	    for Inf in AllInfs:
		    for day in range(0,len(Inf)):
			    for fips,value in Inf[day].items():
				    fipsSet.add(fips)
	    for day in xrange(0,numDays):
		    for fips in fipsSet:
			    aveInfByBG[day][fips] = 0.0
	
	    for Inf in AllInfs:
		    for day in xrange(0,len(Inf)):
			    for fips,value in Inf[day].items():
				    aveInfByBG[day][fips]+= value

	    time2 = time.time()
	    print "time to sum data = %10.2f"%(time2-time1)
	    time1 = time.time()
	    for day in range(0,len(aveInfByBG)):
		    for fips in aveInfByBG[day].keys():
			    aveInfByBG[day][fips]/= float(len(fred_run.block_infection_file_names))

	    time2 = time.time()

	    print "Time to divide data %10.2f"%(time2-time1)
	    ### Create a textfile that can be used by visualizer service to create a GAIA file
	    time1 = time.time()
	    for day in range(0,len(aveInfByBG)):
			    for fips in aveInfByBG[day].keys():
				    bgoutStr.append("US%s %d %d:1\n"%(str(fips),int(math.ceil(aveInfByBG[day][fips])),day))
	    time2 = time.time()
	    print "Time to create string = %10.2f"%(time2-time1)

    except Exception as e:
	    print str(e)
	    #error_exit(str(e))
    
    try:
	    time1 = time.time()
	    bgStr = "".join(bgoutStr)
            #print "BgStr = " + bgStr
	    m = hashlib.md5()
	    m.update(bgStr)
	    m5hash = m.hexdigest()
	    with open("test.txt","wb") as f:
		    f.write("%s"%bgStr)
	    bufferSize = len(bgStr)
	    count = 0
	    runDataContentId = -1
#	    while count*bufferSize < len(bgStr):
#		    start = count*bufferSize
#		    if count*bufferSize + bufferSize > len(bgStr):
#			    end = len(bgStr)
#		    else:
#			    end = start+bufferSize
#		    if count == 0:
	    SQLString = 'INSERT INTO run_data_content (text_content, md5_hash_of_content) values ("%s","%s")'%(bgStr,m5hash)
	    apolloDB.query(SQLString)
	    runDataContentId = apolloDB.insertID()
	    print "ID 1 = " + str(runDataContentId)
#		    else:
#			    SQLString = 'UPDATE run_data_content SET text_content=concat(text_content,"%s") where id = %s'%(bgStr[start:end],str(runDataContentId))
#			    apolloDB.query(SQLString)
	    #print SQLString
		    
		    #count+=1
	    runDataDescriptionId = apolloDB.getRunDataDescriptionId("newly_exposed.txt")

	    print "ID = " +str(runDataContentId)
	    SQLString = "INSERT INTO run_data(run_Id,description_id,content_id) values ('%s','%d','%d')"%(simulationRunId,runDataDescriptionId,runDataContentId)
	    apolloDB.query(SQLString)
	    
	    time2 = time.time()
	    print "Time to insert in database = %10.2f"%(time2-time1)
	    apolloDB.close()
	    statusUpdate("LOG_FILES_WRITTEN","%s"%datetime.datetime.now().strftime("%a %b %d %H:%M:%S EDT %Y"))
    except Exception as e:
	    error_exit(str(e))
