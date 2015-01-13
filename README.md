##FRED 
###A Framework for Reconstructing Epidemiological Dynamics
An open source modeling system developed by the University of Pittsburgh [Public Health Dynamics Laboratory](http://www.phdl.pitt.edu "PHDL website") in collaboration with the [Pittsburgh Supercomputing Center](http://www.psc.edu "PSC website") and the [Carnegie Mellon University School of Computer Science](http://www.cs.cmu.edu "CMU CS website").

####Version information
This is the latest stable version of FRED. There should not be commits to this branch. For versions prior to 2.9.1, you can download from the [FRED Website](http://fred.publichealth.pitt.edu/DownloadFRED/ "FRED Download Site")

####Notes on compiling
FRED was compiled using gcc which is not C++11 by default, so the file src/Place_List.h includes two references to tr1. The following lines

```c++
#include <tr1/unordered_map>
//...
typedef std::tr1::unordered_map<string, int> LabelMapT;

//Should be changed to
#include <unordered_map>
//...
typedef std::unordered_map<string, int> LabelMapT;
```

Also, by default FRED will try to use the openMP libraries. The clang compiler (newer versions of XCode for Mac will have this) does not use openMP so the build will fail. If your compiler does not use openMP, simply comment out the CPPFLAGS line

```make
CPPFLAGS = -g $(M64) -O3 -fopenmp $(LOGGING_PRESET_3) -DNCPU=$(NCPU) -DSQLITE=$(SQLITE) -DSNAPPY=$(SNAPPY) -fno-omit-frame-pointer $(INCLUDE_DIRS)
```

and uncomment this one

```make
CPPFLAGS = -g $(M64) -O3 $(LOGGING_PRESET_3) -DNCPU=1 -DSQLITE=$(SQLITE) -DSNAPPY=$(SNAPPY) -fno-omit-frame-pointer $(INCLUDE_DIRS)
```
