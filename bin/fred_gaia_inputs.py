#! /usr/bin/env python

import os,sys,string
import math
import optparse
from fred import FRED,FRED_RUN,FRED_Household_Set
from gaia import GAIA,ConfInfo,PlotInfo,Constants,computeBoundaries,computePercentBoundariesAndColors,computeColors
import time
import glob
from copy import copy

POP_DENSITY_STYLE_NUMBER = 2
INC_STYLE_NUMBER = 3


###############################################################################################
### Grid Class definititions
                
class Grid:
    def __init__(self):
        self.incX = 0.0
        self.incY = 0.0
        self.bounding_box = None
        self.number_lon = 0
        self.number_lat = 0
        self.data = None
        self.dir = None
        self.polyOffSet = 1
	self.variable = None

    def setupFromFredGridFile(self,gridDir,variable='C'):
        self.dir = gridDir
	self.variable=variable
        with open(self.dir+"/grid.txt") as f:
            self.number_lat = int(f.readline().split("=")[1])
            self.number_lon = int(f.readline().split("=")[1])
            min_lat = float(f.readline().split("=")[1])
            min_lon = float(f.readline().split("=")[1])
            self.incX = float(f.readline().split("=")[1])
            self.incY = float(f.readline().split("=")[1])
            print "%d %d %g %g %g %g"%(self.number_lat,self.number_lon
                                       ,min_lat,min_lon,self.incX,self.incY)
            self.bounding_box = (min_lat,min_lon,
                                  min_lat+(self.incX*self.number_lat),
                                  min_lon+(self.incY*self.number_lon))
            self.data = [0 for x in range(self.number_lon*self.number_lat)]
            
    def setupExplicit(self,bounding_box,incX,incY,variable='C'):
        self.incX = incX
        self.incY = incY
        self.bounding_box = bounding_box
	self.variable=variable
        
        self.number_lon = int(((self.bounding_box[3]-self.bounding_box[1])/self.incY))+1
        self.number_lat = int(((self.bounding_box[2]-self.bounding_box[0])/self.incX))+1
        print str(self.number_lon) + " " + str(self.number_lat)
        self.data = [0 for x in range(self.number_lon*self.number_lat)]

    def fillDataRunAveTimeMax(self,disease_id=0,percents=False):
        Gdir = "%s"%(self.dir)
        runs = glob.glob(Gdir+"/run*")
        print str(runs)
        numRuns = len(runs)
        numDays = 0
        for run in runs:
            dayFiles = glob.glob(run + "/dis" + str(disease_id) + "/" + self.variable + "/day-*.txt")
            numDays = len(dayFiles)
            for dayFile in dayFiles:
                with open(dayFile,"rb") as fDay:
                    for line in fDay:
                        lineSplit = line.split()
                        x = int(lineSplit[0])
                        y = int(lineSplit[1])
                        value = float(lineSplit[2])
                        if percents is True:
                            people = float(lineSplit[3])
                            if(value > people):
                                raise RuntimeError("There is a value that higher than the number of "\
                                                   +"peole in grid %d %d: %g %g"%(x,y,value,people))
                            if int((value/people)*100.0) > self.data[y*self.number_lat+x]:
                                self.data[y*self.number_lat+x] = int((value/people)*100.0)
                        else:
                            if value > self.data[y*self.number_lat+x]:
                                self.data[y*self.number_lat+x] = value
        for i in range(0,len(self.data)):
            self.data[i] /= float(numRuns)
    
    def fillDataRunTimeAve(self,disease_id=0,percents=False):
        ### Find the proper directory
        Gdir = "%s"%(self.dir)
        runs = glob.glob(Gdir+"/run*")
        print str(runs)
        numRuns = len(runs)
        numDays = 0
        for run in runs:
            dayFiles = glob.glob(run + "/dis" + str(disease_id) + "/" + self.variable + "/day-*.txt")
            numDays = len(dayFiles)
            for dayFile in dayFiles:
                with open(dayFile,"rb") as fDay:
                    for line in fDay:
                        lineSplit = line.split()
                        x = int(lineSplit[0])
                        y = int(lineSplit[1])
                        value = float(lineSplit[2])
                        if percents is True:
                            people = float(lineSplit[3])
                            if(value > people):
                                raise RuntimeError("There is a value that higher than the number of "\
                                                   +"peole in grid %d %d: %g %g"%(x,y,value,people))
                            self.data[y*self.number_lat+x] += value/people
                        else:
                            self.data[y*self.number_lat+x] += value
        for i in range(0,len(self.data)):
            self.data[i] = int(float(self.data[i])/float(numRuns*numDays))
    
    def fillDataRunAveTimeSum(self,disease_id=0,percents=False):
        Gdir = "%s"%(self.dir)
        runs = glob.glob(Gdir+"/run*")
        print str(runs)
        numRuns = len(runs)
        numDays = 0
        for run in runs:
            dayFiles = glob.glob(run + "/dis" + str(disease_id) + "/" + self.variable + "/day-*.txt")
            numDays = len(dayFiles)
            for dayFile in dayFiles:
                with open(dayFile,"rb") as fDay:
                    for line in fDay:
                        lineSplit = line.split()
                        x = int(lineSplit[0])
                        y = int(lineSplit[1])
                        value = float(lineSplit[2])
                        if percents is True:
                            people = float(lineSplit[3])
                            if(value > people):
                                raise RuntimeError("There is a value that higher than the number of "\
                                                   +"peole in grid %d %d: %g %g"%(x,y,value,people))
                            self.data[y*self.number_lat+x] += int((value/people)*100.0)
                        else:
			    self.data[y*self.number_lat+x] += value
			    			    
        for i in range(0,len(self.data)):
            self.data[i] /= float(numRuns)
                                   
    def fillDataForDayRunAve(self,disease_id=0,day=0,percents=False):
        Gdir = "%s"%(self.dir)
        runs = glob.glob(Gdir+"/run*")
        print str(runs)
        numRuns = len(runs)
        for run in runs:
            dayFile = run + "/dis" + str(disease_id) + "/" + self.variable + "/day-"+str(day)+".txt"
            with open(dayFile,"rb") as fDay:
                for line in fDay:
                    lineSplit = line.split()
                    x = int(lineSplit[0])
                    y = int(lineSplit[1])
                    value = float(lineSplit[2])
                    if percents is True:
                        people = float(lineSplit[3])
                        if(value > people):
                            raise RuntimeError("There is a value that higher than the number of "\
                                               +"peole in grid %d %d: %g %g"%(x,y,value,people))
                        self.data[y*self.number_lat+x] += int((value/people)*100.0)
                    else:
                        self.data[y*self.number_lat+x] += value
			
        for i in range(0,len(self.data)):
            self.data[i] /= float(numRuns)
                
    def indexes(self,latitude,longitude):
        if self.bounding_box is None:
            raise RuntimeError("Grid has not been initialized before trying to calculate and index")
        lonind = int(math.floor((longitude - self.bounding_box[1])/self.incY))
        latind = int(math.floor((latitude  - self.bounding_box[0])/self.incX))
        return lonind*self.number_lat + latind

    def _zeroData(self):
        for i in range(0,len(self.data)):
            self.data[i] = 0.0
    
    def __div__(self,value):
        return [ float(x/value) for x in self.data ]

    def _sumData(self):
        return sum(self.data)
    
    def writeToGaiaFile(self,fileHandle,day=None,styleID=None):
        polyCount = self.polyOffSet
        for i in range(0,self.number_lon):
            for j in range(0,self.number_lat):
                
                lon1 = self.bounding_box[1] + \
                       float(i)*(self.incX)
                lon2 = self.bounding_box[1] + \
                       float(i+1)*(self.incX)
                lat1 = self.bounding_box[0] + float(j)*(self.incY)
                lat2 = self.bounding_box[0] + float(j+1)*(self.incY)                  
                upperLeft   = (lon1,lat1)
                upperRight  = (lon2,lat1)
                lowerLeft = (lon1,lat2)
                lowerRight = (lon2,lat2)
                grid_coord = i*self.number_lat + j
                if day is None:
                    if(self.data[grid_coord] != 0):
                        if styleID:
                            fileHandle.write("lonlat-poly %d %g %g %10d:%d\n"\
                                       %(polyCount,upperLeft[1],upperLeft[0],
                                        self.data[grid_coord],styleID))
                            fileHandle.write("lonlat-poly %d %g %g %10d:%d\n"\
                                       %(polyCount,upperRight[1],upperRight[0],
                                         self.data[grid_coord],styleID))
                            fileHandle.write("lonlat-poly %d %g %g %10d:%d\n"\
                                       %(polyCount,lowerRight[1],lowerRight[0],
                                         self.data[grid_coord],styleID))
                            fileHandle.write("lonlat-poly %d %g %g %10d:%d\n"\
                                       %(polyCount,lowerLeft[1],lowerLeft[0],
                                         self.data[grid_coord],styleID))
                        else:
                            fileHandle.write("lonlat-poly %d %g %g %10d\n"\
                                       %(polyCount,upperLeft[1],upperLeft[0],
                                         self.data[grid_coord]))
                            fileHandle.write("lonlat-poly %d %g %g %10d\n"\
                                       %(polyCount,upperRight[1],upperRight[0],
                                         self.data[grid_coord]))
                            fileHandle.write("lonlat-poly %d %g %g %10d\n"\
                                       %(polyCount,lowerRight[1],lowerRight[0],
                                         self.data[grid_coord]))
                            fileHandle.write("lonlat-poly %d %g %g %10d\n"\
                                       %(polyCount,lowerLeft[1],lowerLeft[0],
                                         self.data[grid_coord]))
			polyCount +=1
                else:
                    if(self.data[grid_coord] != 0):
                        if styleID:
                            fileHandle.write("lonlat-poly %d %g %g %10d %10d:%d\n"\
                                       %(polyCount,upperLeft[1],upperLeft[0],
                                        self.data[grid_coord],day,styleID))
                            fileHandle.write("lonlat-poly %d %g %g %10d %10d:%d\n"\
                                       %(polyCount,upperRight[1],upperRight[0],
                                         self.data[grid_coord],day,styleID))
                            fileHandle.write("lonlat-poly %d %g %g %10d %10d:%d\n"\
                                       %(polyCount,lowerRight[1],lowerRight[0],
                                         self.data[grid_coord],day,styleID))
                            fileHandle.write("lonlat-poly %d %g %g %10d %10d:%d\n"\
                                       %(polyCount,lowerLeft[1],lowerLeft[0],
                                         self.data[grid_coord],day,styleID))
                        else:
                            fileHandle.write("lonlat-poly %d %g %g %10d %10d\n"\
                                       %(polyCount,upperLeft[1],upperLeft[0],
                                         self.data[grid_coord],day))
                            fileHandle.write("lonlat-poly %d %g %g %10d %10d\n"\
                                       %(polyCount,upperRight[1],upperRight[0],
                                         self.data[grid_coord],day))
                            fileHandle.write("lonlat-poly %d %g %g %10d %10d\n"\
                                       %(polyCount,lowerRight[1],lowerRight[0],
                                         self.data[grid_coord],day))
                            fileHandle.write("lonlat-poly %d %g %g %10d %10d\n"\
                                       %(polyCount,lowerLeft[1],lowerLeft[0],
                                         self.data[grid_coord],day))
			polyCount+=1
        self.polyOffSet = polyCount
    
    def writeStyleFile(self,fileHandle,styleID=None,legend=False,percents=False):
        if percents is True:
            maxBoundary = 25
            colors,boundaries = computePercentBoundariesAndColors(maxBoundary)
        else:
	    maxData = max(self.data)
	    
            colors,boundaries = computePercentBoundariesAndColors(.25*maxData,scalar=max(self.data))
            #colors = computeColors(startColor,endColor,len(boundaries))
	    #if self.variable == "N":
	#	colors = computeColors("255.200.200.200","255.0.0.0",7)
	#    else:
	#	colors = ["255.255.255.255","255.255.0.0","255.255.199.0",
	#		  "255.103.250.0",
	#		  "255.15.249.167","255.17.140.255",
	#		  "255.0.0.255"
	print str(len(colors)) + " " + str(len(boundaries))	
        if styleID:
            fileHandle.write("id=%d\n"%styleID)
            if legend:
                fileHandle.write("legend-support=1\n")
        for boundary in boundaries:
            if boundary[0] == 0 and boundary[1] == 0:
                pass
            else:
                fileHandle.write('%s 0 %g %g\n'%(colors[boundaries.index(boundary)],
						 boundary[0],boundary[1]))

###############################################################################################

if __name__ == '__main__':
    parser = optparse.OptionParser(usage="""
    %prog [--help][-k key][-r run_number][-t vis_type]
    -k or --key        Fred Key
    -r or --run        (Not Working)
                       The run number of vizualize
                       ave = produce an average result
                       all = produce an input for all runs
    -v or --variable   The variable from FRED you would like to plot.
                       Available variables are actually determined by FRED but here are some
		       examples:
		       N - Populaton Density
		       C - Newly exposed persons
		       P - Prevalence
		       I - Number of people infectious
    -t or --type       Type of GAIA visualization
                       static = a static image
		       animated = a ogg movie of the visualization
    -o or --op         Operation to perform on the data (only for static)
                       sum = will sum up all of the data over days of simulation
		       max = will only take the maximum value for each grid over days of simulation
		       (Note if -v is set to N and -t is set to static, this is always max).
    -m or --movformat  Format you would like the movie in (only for -t animated)
                       mov - produces quicktime movie
		       mp4 - produces mp4 movie
		       ogg - procudes ogg movie (default)
    -T or --title      Title for the visualization
    -p or --percents   If set, will plot the percentage of the value in each grid cell.
    -l or --loc        FRED Location file to use (if blank will try to use the one from input
    -w or --time       (Not Working) Turn on profiling
    -d or --debug      (Not Working) Turn on debug printing
    """)

    parser.add_option("-k","--key",type="string",
                      help="The FRED key for the run you wish to visualize")
    ### HACK FROM SHAWN
    parser.add_option("-W","--taiwan",action="store_true",
                      default=False)
    parser.add_option("-r","--run",type="string",
                      help="(Not Working) The number of the run you would like to visualize"\
		      +"(number,ave, or all)",
                      default=1)
    parser.add_option("-v","--variable",type='choice',action='store',
                      choices=['N','I','Is','C','Cs','P','Vec'],
                      default='C', help='Variable that you would like GAIA to plot' )
    parser.add_option("-t","--type",type="choice",action="store",
                      choices=["static","animated"],
                      help="The type of GAIA Visualization you would like (inc_anim,inc_static)",
                      default="static")
    parser.add_option("-o","--op",type="choice",action="store",
		      choices=["max","sum"],
		      help="Operation to perform on the data (only for -t = static)",
		      default="sum")
    parser.add_option("-p","--percents",action="store_true",
		      default=False,
                      help="If true, plot each grid point as a percentage of the number of people there")
    parser.add_option("-m","--movformat",type="choice",action="store",
		      choices=["mp4","mov","ogg"],
		      help="Format GAIA movie produces (only for -t animated",
		      default="ogg")
    parser.add_option("-l","--loc",type="string",
                      help="The FRED Location File to use")
    parser.add_option("-w","--time",action="store_true",
                      default=False)
    parser.add_option("-d","--debug",action="store_true",
                      default=False)

    parser.add_option("-T","--title",type="string",
                      help="Title to put on the GAIA vizualization",default="")
    

    opts,args=parser.parse_args()

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

    vizType = opts.type
    vizVariable = opts.variable
    vizOp = opts.op
    movFormat = opts.movformat


    imgFormat = "png"
    if vizType == "animated":
	imgFormat = "gif"

    percentsString = ""
    if opts.percents is True:
	percentsString = "_percents"

    ### Find the FRED run in this SIMS
    fred_run = FRED_RUN(fred,key)

    ### Find the Population File

    time1 = time.time()
    if opts.taiwan is True:
        countyList = None
    else:
        countyList = []
        #if fred_run.params_dict['city'] is not None:
    #	print fred_run.params_dict['city']
    #	countyList = [fred.cityDict[fred_run.params_dict['city']]]
    #    elif fred_run.params_dict['county'] is not None:
    #	countyList = [fred.countyDict[fred_run.params_dict['county']]]
    #    else:
            #synth_pop_dir = fred_run.get_param("synthetic_population_directory")
            #synth_pop_id = fred_run.get_param("synthetic_population_id")

            #synth_pop_prefix = os.path.expandvars(synth_pop_dir + "/" + synth_pop_id + "/" + synth_pop_i
        synth_pop_dir = fred_run.get_meta_variable("POPULATION")
	if not isinstance(synth_pop_dir,list):
            synth_tmp = copy(synth_pop_dir)
            synth_pop_dir = [synth_tmp] 
        for synth_dir in synth_pop_dir:
            synthSplit = synth_dir.split("/")
            synth_pop_id = synthSplit[len(synthSplit)-1]
            print synth_dir + ": " + synth_pop_id
            synth_pop_prefix = os.path.expandvars(synth_dir + "/" + synth_pop_id)
            synth_household_filename = synth_pop_prefix + "_synth_households.txt"

            try:
                open(synth_household_filename,"r")
            except IOError:
                print "Problem opening FRED location file " + synth_household_filename

            fredHouseholds = FRED_Household_Set(synth_household_filename)
            countyList += fredHouseholds.countyList
	countyList = set(countyList)
    
    grid = Grid()
    grid.setupFromFredGridFile(fred_run.gaiaDir,variable=vizVariable)    
    gaia_input_file = "gaia_input_%s_%s_%s_%s%s.txt"%(vizType,key,vizVariable,vizOp,percentsString)
    gaia_style_file = "gaia_style_%s_%s_%s_%s%s.txt"%(vizType,key,vizVariable,vizOp,percentsString)

    if vizType == "static":
	if vizVariable == "N":
	    grid.fillDataRunAveTimeMax(0, percents=opts.percents)
	elif vizOp == "sum":
	    grid.fillDataRunAveTimeSum(0, percents=opts.percents)
	elif vizOp == "max":
	    grid.fillDataRunAveTimeMax(0, percents=opts.percents)
	    
        with open(gaia_input_file,"wb") as f:
            if countyList is None:
                f.write("HASC TW* -1\n")
            else:
                for countyFips in countyList:
                    f.write("USFIPS st%2s.ct%3s.tr*.bl* -1\n"%(countyFips[0:2],countyFips[2:5]))
            grid.writeToGaiaFile(f,styleID=1)
        with open(gaia_style_file,"wb") as f:
            grid.writeStyleFile(f,styleID=1,
                                legend=True,percents=opts.percents)
	    
    elif vizType == "animated":
        nDays = int(fred_run.params_dict["days"])
        with open(gaia_input_file,"wb") as f:        
            maxDay = -11111
            maxSum = -999999
            for day in range(0,nDays):
                grid._zeroData()
                grid.fillDataForDayRunAve(0,day,percents=opts.percents)
                sumG = grid._sumData()
                if countyList is None:
                    f.write("HASC TW* -1 %d\n"%(day))
                else:
                    for countyFips in countyList:
                        f.write("USFIPS st%2s.ct%3s.tr*.bl* -1 %d\n"%(countyFips[0:2],countyFips[2:5],day))
		if sumG > 0.0:
		    grid.writeToGaiaFile(f,day=day,styleID=1)
                if sumG > maxSum:
                    maxSum = sumG
                    maxDay = day
            
        ### Fill the maximum one again so I can compute boundaries.
        grid._zeroData()
        grid.fillDataForDayRunAve(0,maxDay,percents=False)
        with open(gaia_style_file,"wb")as f:
            grid.writeStyleFile(f, styleID=1, legend=1, percents=opts.percents)

    legendTitle = "Number of Persons"
    if opts.percents:
	legendTitle = "Percentage"
                                 
    plotInfo = PlotInfo(input_filename_= gaia_input_file,
                        styles_filenames_=[gaia_style_file],
                        output_format_=imgFormat,
                        bundle_format_= movFormat,
                        stroke_width_=0.5,
                        max_resolution_=500,
                        font_size_=18.0,
                        background_color_="255.255.255.255",
                        title_= opts.title,
                        legend_=legendTitle,
                        legend_font_size_=12.0)
    gaia = GAIA(plotInfo)
    print "Calling GAIA"
    gaia.call()

