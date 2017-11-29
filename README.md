## FRED 
### A Framework for Reconstructing Epidemiological Dynamics
An open source modeling system developed by the University of Pittsburgh [Public Health Dynamics Laboratory](http://www.phdl.pitt.edu "PHDL website") in collaboration with the [Pittsburgh Supercomputing Center](http://www.psc.edu "PSC website") and the [Carnegie Mellon University School of Computer Science](http://www.cs.cmu.edu "CMU CS website").

#### Version information
This is the Master branch of FRED. It is should only be updated after a Development branch is ready to be declared Stable

#### Notes on compiling
By default FRED will try to use the clang compiler (newer versions of XCode for Mac will have this). If your compiler is not clang, you should find this section of FRED/src/Makefile and comment it out.

```
# comment out if not using clang
CLANG_FLAGS = -mllvm -inline-threshold=1000
```

so that it looks like this:

```
# comment out if not using clang
# CLANG_FLAGS = -mllvm -inline-threshold=1000
```
