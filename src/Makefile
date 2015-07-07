##################### FRED Makefile ###########################

################   OS DEPENDENCIES   ###############################

################   Multithreading   ###############################
# For multithreading, NCPU should be set here or as an argument to make.
# NCPU must be greater than or equal to the maximum number of threads that will
# be used by OpenMP.  The number of threads used by OpenMP can be set at runtime
# with the environment variable OMP_NUM_THREADS.  The value used for OMP_NUM_THREADS
# must be less than or equal the value of NCPU set during compilation.
# If NCPU > omp_get_max_threads, some relatively small amount of memory will be
# wasted, but it's harmless otherwise.
#
# NCPU is defined to be 1 in Global.h if value not set here 
#
# Autodetect all available processors for linux or mac
# Fall back to single-threaded operation
OS := $(shell uname)
ifeq ($(OS),Linux)
NCPU ?= $(shell grep -c ^processor /proc/cpuinfo)
M64 = -m64
else ifeq ($(shell echo $(OS) | cut -f1 -d_),CYGWIN)
NCPU ?=  $(shell grep -c ^processor /proc/cpuinfo)
else ifeq ($(OS),Darwin)
NCPU ?= $(shell sysctl -n hw.ncpu)
M64 = -m64
OSX_VERSION = $(shell sw_vers -productVersion | sed 's/10.//' | sed 's/\..*//')
ifeq ($(OSX_VERSION),10)
# USE_STDLIB = -stdlib=libstdc++
endif
endif
# NCPU = 1

################   Snappy Compression  ###############################
## Snappy can significantly improve FRED startup time by reading
## compressed population files.  If you have snappy installed,
## set the following flag to 1, and make sure the library locations
## below are coreect.
SNAPPY = 0 
ifeq ($(SNAPPY),1)
SNAPPY_DIR = /usr/local
SNAPPY_INC_DIR = $(SNAPPY_DIR)/include
SNAPPY_HDR = $(SNAPPY_INC_DIR)/snappy.h 
INCLUDE_DIRS = -I$(SNAPPY_INC_DIR)
SNAPPY_LIB_DIR = $(SNAPPY_DIR)/lib

# location of library differs by OS
OS := $(shell uname)
ifeq ($(OS),Linux)
SNAPPY_LIB = $(SNAPPY_LIB_DIR)/libsnappy.so
else ifeq ($(shell echo $(OS) | cut -f1 -d_),CYGWIN)
SNAPPY_LIB = $(SNAPPY_LIB_DIR)/libsnappy.la 
else ifeq ($(OS),Darwin)
SNAPPY_LIB = $(SNAPPY_LIB_DIR)/libsnappy.dylib
endif

SNAPPY_LFLAGS = $(SNAPPY_LIB_DIR)/libsnappy.a
SNAPPY_LDFLAGS = -L$(SNAPPY_LIB_DIR)
SNAPPY_OBJ = Compression.o
FSZ = fsz
endif

################   SQLite  ###############################
## Off by default
SQLITE = 0
ifeq ($(SQLITE),1)
SQLITE_DIR = $(SRC_DIR)/sqlite
INCLUDE_DIRS += -I$(SQLITE_DIR)
SQLITE_OBJ = sqlite3.o FergEvolution.o
endif

#################  MD5 Program ##############################

UNIX	:= $(shell uname)
ifeq ($(UNIX), Linux)
MD5SUM	:= md5sum
else ifeq ($(shell echo $(UNIX) | cut -f1 -d_),CYGWIN)
MD5SUM := md5sum
else
MD5SUM	:= md5 -q
endif

################   END OS DEPENDENCIES   #########################

################   Compiler Flags   ##############################
FRED_EXECUTABLE_NAME = FRED
FRED_memcheck: FRED_EXECUTABLE_NAME = FRED_memcheck
SRC_DIR = $(shell pwd)

CPP = g++ 
CXX = $(CPP)
LDFLAGS =
LFLAGS =

LOGGING_PRESET_0 = 
LOGGING_PRESET_1 = -DFREDWARNING
LOGGING_PRESET_2 = -DFREDSTATUS -DFREDWARNING
LOGGING_PRESET_3 = -DFREDVERBOSE -DFREDSTATUS -DFREDWARNING -DFREDDEBUG

# use this for development:
# CPPFLAGS = -g -O2 $(LOGGING_PRESET_3) -Wall -DSQLITE=$(SQLITE) -DSNAPPY=$(SNAPPY)

# use one of these for profiling
# CPPFLAGS = -g $(M64) -O3 $(LOGGING_PRESET_3) # -DNDEBUG -DSQLITE=$(SQLITE) -DSNAPPY=$(SNAPPY)
# CPPFLAGS = -pg $(M64) -O2 -DNDEBUG  -DSQLITE=$(SQLITE) -DSNAPPY=$(SNAPPY) $(LOGGING_PRESET_3) -fno-inline-functions -fno-inline-functions-called-once -fno-optimize-sibling-calls -fno-omit-frame-pointer

## Use this to run with multiple threads
# CPPFLAGS = -g $(M64) -O3 -fopenmp $(LOGGING_PRESET_3) -DNCPU=$(NCPU) -DSQLITE=$(SQLITE) -DSNAPPY=$(SNAPPY) $(INCLUDE_DIRS)

FRED_memcheck: 	CPPFLAGS = -g $(M64) -O0 -fopenmp $(LOGGING_PRESET_3) -DNCPU=$(NCPU) -DSQLITE=$(SQLITE) -DSNAPPY=$(SNAPPY) -fno-omit-frame-pointer $(INCLUDE_DIRS)

## Use this to make reproducible serial runs
# CPPFLAGS = -g $(M64) -O3 $(LOGGING_PRESET_3) -DNCPU=1 -DSQLITE=$(SQLITE) -DSNAPPY=$(SNAPPY) $(INCLUDE_DIRS) #-fast #-Wall

## Use this to make reproducible serial runs using clang/LLVM on MacOS, since clang does not currently support openMP.
CPPFLAGS = $(USE_STDLIB) -g $(M64) -O3  -mllvm -inline-threshold=5000 $(LOGGING_PRESET_0) -DNCPU=1 -DSQLITE=$(SQLITE) -DSNAPPY=$(SNAPPY) $(INCLUDE_DIRS) # -Wall



###############################################

%.o:%.c %.h
	$(CPP) $(CPPFLAG) -c $< $(INCLUDES)

OBJ =   Fred.o Global.o Age_Map.o Timestep_Map.o Utils.o Params.o Date.o Events.o Random.o RNG.o \
	Geo_Utils.o Neighborhood_Layer.o Neighborhood_Patch.o Regional_Layer.o Regional_Patch.o \
	Visualization_Layer.o Visualization_Patch.o Travel.o \
	Decision.o Policy.o Manager.o \
	Antiviral.o Antivirals.o AV_Decisions.o AV_Policies.o AV_Manager.o AV_Health.o \
	Vaccine_Health.o Vaccine_Dose.o Vaccine.o Vaccines.o \
	Vaccine_Priority_Decisions.o Vaccine_Priority_Policies.o Vaccine_Manager.o \
	Person.o Place.o Place_List.o Population.o \
	Activities.o Behavior.o Demographics.o Health.o Intention.o Perceptions.o \
	Classroom.o Hospital.o Household.o Neighborhood.o Office.o School.o Workplace.o \
	Disease.o Infection.o Epidemic.o \
	EvolutionFactory.o Evolution.o Strain.o Trajectory.o StrainTable.o \
	IntraHost.o DefaultIntraHost.o FixedIntraHost.o RSAIntraHost.o HIV_Intrahost.o \
	Abstract_Grid.o Abstract_Patch.o \
	Seasonality_Timestep_Map.o Seasonality.o \
	Past_Infection.o MSEvolution.o Piecewise_Linear.o \
	Vector_Layer.o Vector_Patch.o $(SNAPPY_OBJ) \
	County.o
	# ODEIntraHost.o ODE.o

SRC = $(OBJ:.o=.cc)

HDR = $(OBJ:.o=.h)

MD5 := FRED.md5

all: FRED FRED.tar.gz $(FSZ) $(MD5)

FRED: $(SQLITE_OBJ)  $(OBJ) # dSFMT.o 
	$(CPP) -o $(FRED_EXECUTABLE_NAME) $(CPPFLAGS) $(INCLUDE_DIRS) $(LDFLAGS) $(OBJ) $(SQLITE_OBJ) $(SNAPPY_LFLAGS) -ldl
	cp $(FRED_EXECUTABLE_NAME) ../bin

Compression.o: $(SNAPPY_LIB)
	$(CPP) -o Compression.o $(CPPFLAGS) $(SNAPPY_INCLUDE_DIRS) -c Compression.cc

fsz: $(SNAPPY_LIB) $(SNAPPY_OBJ) Global.h fsz.cc
	$(CPP) -o fsz $(CPPFLAGS) $(INCLUDE_FLAGS) $(SNAPPY_LDFLAGS) $(SNAPPY_OBJ) $(SNAPPY_LFLAGS) fsz.cc
	cp fsz ../bin

# The following will use the snappy library provided with the distribution (deprecated):
# $(SNAPPY_LIB):
#	@echo Extracting and Compiling Compression Library
#	(cd $(SNAPPY_DIR); tar --strip-components=1 -xzvf snappy-1.0.5.tar.gz; \
#	./configure --prefix=$(SNAPPY_DIR); make && make install)

FRED_memcheck: FRED

FRED_Unit_Tracker: 
	cd TestSuite/Tracker; $(CPP) -g -O0 -fopenmp -DUNIT_TEST=1 -I../../ Tracker_Unit_Test.cc -c -o Tracker_Unit_Test.o
	cd TestSuite/Tracker; $(CPP) -g -O0 -fopenmp -o FRED_Unit_Tracker -DUNIT_TEST=1 -I../../ Tracker_Unit_Test.o

dSFMT.o:
	@echo Compiling Random Number Generator
	$(CPP) $(DSFMT_CPPFLAGS) $(DSFMT_SRC) -c $(DSFMT_HDR) 

sqlite3.o:
	@echo Extracting and compiling sqlite
	cd $(SQLITE_DIR); tar --strip-components=1 -xzvf sqlite-autoconf-3071502.tar.gz; \
	./configure; make; \
	cp sqlite3.o $(SRC_DIR)

DEPENDS: $(SRC) $(HDR)
	$(CPP) $(USE_STDLIB) -MM $(SRC) $(INCLUDE_DIRS) > DEPENDS

include DEPENDS

FRED.tar.gz:
	tar -czvf FRED.tar.gz $(HDR) $(SRC) Makefile;

FRED.md5: FRED.tar.gz
	$(MD5SUM) $< > $@

##############################################

print:
	enscript $(SRC) $(HDR)

clean:
	rm -f *.o FRED FRED_Unit_Tracker ../bin/FRED fsz ../bin/fsz *~
	(cd ../populations; make clean)
	(cd ../tests; make clean)

tags:
	find . -name \*.[ch]* | xargs etags

##############################################
