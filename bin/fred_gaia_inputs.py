import os,sys,string
import optparse
from fred import FRED,FRED_RUN,FRED_Infection_Record,FRED_Locations_Set,FRED_People_Set

class Grid:
    def __init__(self,bounding_box,increment):
        self.increment = increment
        self.bounding_box = bounding_box

        self.number_lon = int((self.bounding_box[3]-self.bounding_box[1])/self.increment) + 1
        self.number_lat = int(((self.bounding_box[2]-self.bounding_box[0])/self.increment) + 1)

        self.data = [None]*(self.number_lon)
        for i in range(0,(self.number_lon)):
            self.data[i] = [None]*(self.number_lat)
            for j in range(0,self.number_lat):
                self.data[i][j] = 0

    def indexes(self,latitude,longitude):
        lonind = int((longitude - self.bounding_box[1])/self.increment)
        latind = int((latitude  - self.bounding_box[0])/self.increment)
        return (latind,lonind)

if __name__ == '__main__':
    parser = optparse.OptionParser(usage="""
    %prog [--help][-k key][-r run_number][-t vis_type]
    -k or --key   Fred Key
    -r or --run   The run number of vizualize
                  ave = produce an average result
                  all = produce an input for all runs
    -t or --type  Type of GAIA visualization
		  hous_den   = Household Density Plot
                  inc_anim   = incidence animation
                  inc_static = incidence map
    -p or --pop   FRED Population file to use (if blank will try to use the one from input)
    -l or --loc   FRED Location file to use (if blank will try to use the one from input)
    -g or --grid  Size of grid points to plot
    """)

    parser.add_option("-k","--key",type="string",
                      help="The FRED key for the run you wish to visualize")
    parser.add_option("-r","--run",type="string",
                      help="The number of the run you would like to visualize (number,ave, or all)",
                      default="ave")
    parser.add_option("-t","--type",type="string",
                      help="The type of GAIA Visualization you would like (inc_anim,inc_static)",
                      default="inc_static")
    parser.add_option("-p","--pop",type="string",
                      help="The FRED Population File to use")
    parser.add_option("-l","--loc",type="string",
                      help="The FRED Location File to use")

    parser.add_option("-g","--grid",type="float",
                      help="The size of grid points",
			default=0.002)
    

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

	valid_types = ["inc_static","inc_anim","hous_den"]
    if opts.type not in valid_types:
        print "Invalid or unsupported type parameter, needs to be inc_static or inc_anim"
        sys.exit(1)
    type = opts.type

   ### Find the FRED run in this SIMS
    fred_run = FRED_RUN(fred,key)
    print fred_run.number_of_runs
    pop_file = opts.pop
    if pop_file == None:
        pop_file = fred_run.get_param("popfile")
        
    try:
        open(pop_file,"r")
    except IOError:
        print "Problem opening FRED population file " + pop_file
        sys.exit(1)

    loc_file = opts.loc
    if loc_file == None:
        loc_file = fred_run.get_param("locfile")

    try:
        open(loc_file,"r")
    except IOError:
        print "Problem opening FRED location file " + loc_file

    fred_locations_households = FRED_Locations_Set(loc_file,'H')
    fred_population = FRED_People_Set(pop_file)


## CREATE the GAIA Input
    gaia_bounding_box = fred_locations_households.bounding_box()
    print str(gaia_bounding_box) 
    sys.stdout.flush()
    if opts.type == "inc_static":
        count = 0
        fred_run.load_infection_files()
        for infections_file in fred_run.infections_files:
            gaia_grid = Grid(gaia_bounding_box,opts.grid)
            gaia_input_file = "fred_gaia_" + str(fred_run.key) + "." + str(count) + ".txt"
            print gaia_input_file
            count = count + 1
            for inf_rec in infections_file:
                inf_id = int(inf_rec.infected_id)
                household = fred_population.people[inf_id].household
                place = fred_locations_households.locations[household]
                lon = place.lon
                lat = place.lat
                grid_coords = gaia_grid.indexes(lat,lon)
                gaia_grid.data[grid_coords[1]][grid_coords[0]] = gaia_grid.data[grid_coords[1]][grid_coords[0]] + 1
                #print "inf_id " + str(inf_id) + " household = " + str(household) + " lon = " + str(lon) + " lat: " + str(lat)

            with open(gaia_input_file,"w") as f:
                for i in range(0,gaia_grid.number_lon):
                    for j in range(0,gaia_grid.number_lat):
                        lonpoint = gaia_grid.bounding_box[1] + float(i)*gaia_grid.increment + gaia_grid.increment/2.0
                        latpoint = gaia_grid.bounding_box[0] + float(j)*gaia_grid.increment + gaia_grid.increment/2.0
                        if(gaia_grid.data[i][j] != 0):
                            f.write("lonlat %10.10f %10.10f %10d\n"%(latpoint,lonpoint,gaia_grid.data[i][j]))

    if opts.type == "hous_den":
	gaia_grid = Grid(gaia_bounding_box,opts.grid)
	gaia_input_file = "fred_gaia_house_den_"+str(fred_run.key) + ".txt"
	for location in fred_locations_households.locations.keys():
		place = fred_locations_households.locations[location]
		lon = place.lon
		lat = place.lat
		grid_coords = gaia_grid.indexes(lat,lon)
		gaia_grid.data[grid_coords[1]][grid_coords[0]] = gaia_grid.data[grid_coords[1]][grid_coords[0]] + 1
		print str(location) + ": " + str(lon) + "," + str(lat)
		with open(gaia_input_file,"w") as f:
    	            for i in range(0,gaia_grid.number_lon):
        	            for j in range(0,gaia_grid.number_lat):
                	        lonpoint = gaia_grid.bounding_box[1] + float(i)*gaia_grid.increment + gaia_grid.increment/2.0
                        	latpoint = gaia_grid.bounding_box[0] + float(j)*gaia_grid.increment + gaia_grid.increment/2.0
                      		if(gaia_grid.data[i][j] != 0):
                            		f.write("lonlat %10.10f %10.10f %10d\n"%(latpoint,lonpoint,gaia_grid.data[i][j]))

    if opts.type == "inc_anim":
        print "Opts Type == inc_anim"
        count = 0
	fred_run.load_infection_files()
        for infections_file in fred_run.infections_files:
			## First find out how many days one needs to show
            max_day = -99999
            for inf_rec in infections_file:
                day = inf_rec.day
                max_day = max(day,max_day)
                gaia_grids = []
            for i in range(0,max_day+1):
                gaia_grids.append(Grid(gaia_bounding_box,opts.grid))

                gaia_input_file = "fred_gaia_ts_" + str(fred_run.key) + "." + str(count) + ".txt"
            for inf_rec in infections_file:
                inf_id = int(inf_rec.infected_id)
                household = fred_population.people[inf_id].household
                place = fred_locations_households.locations[household]
                lon = place.lon
                lat = place.lat
                day = int(inf_rec.day)
                print "day = " + str(day) + ": " + str(max_day)
                sys.stdout.flush()
                grid_coords = gaia_grids[day].indexes(lat,lon)
                gaia_grids[day].data[grid_coords[1]][grid_coords[0]] = gaia_grids[day].data[grid_coords[1]][grid_coords[0]] + 1

            with open(gaia_input_file,"w") as f:
                for k in range(0,max_day+1):
                    gaia_grid = gaia_grids[k]
               # for gaia_grid in gaia_grids:
                    for i in range(0,gaia_grid.number_lon):
                        for j in range(0,gaia_grid.number_lat):
                            lonpoint = gaia_grid.bounding_box[1] + float(i)*gaia_grid.increment + gaia_grid.increment/2.0
                            latpoint = gaia_grid.bounding_box[0] + float(j)*gaia_grid.increment + gaia_grid.increment/2.0
                            if(gaia_grid.data[i][j] != 0):
                                print str(latpoint) + "|" + str(lonpoint) +"|" + str(gaia_grid.data[i][j]) + "|"+ str(k)
                                sys.stdout.flush()
                                f.write("lonlat %10.5f %10.5f %10d %10d\n"%(latpoint,lonpoint,int(gaia_grid.data[i][j]),k))	
