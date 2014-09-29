FRED RELEASE
------------

#. Documentation; reSt conversion to HTML (need CSS)
#. Update release docs to include new params

BUGS
----

#. Lack of reproducibility when running with births enabled (new people not initialized completely) <---- [fix committed, need to verify]
#. Antiviral.{cc,h}: Antiviral::quality_control/initialization problem: 'reduce_infectious_period' never initialized!!! Apparently never used throughout FRED.  Removed from quality_control for now.
#. Why is "int get_next_id()" in Population static?  Should be accessed as method of singleton Global::Pop.  Same applies to other statics in Population.

MISCELLANEOUS QUESTIONS
-----------------------

#. In Place::attempt_transmission the total_infections count is incremented for disease_id == 0 only... why?

TESTING
-------

#. Need demographics regression test
#. Finalize Vaccine/Antiviral regression test
#. Need seasonality regression test
#. Need School closure regression test
#. Add option to rt that runs all available regression tests
#. Statistical equivalence tests

NEW FEATURES
------------

#. Continue to adapt FRED to use bloque parallel data structure for Population and Place loops
#. Default settings of params 'enable_vaccination', 'enable_antivirals'
#. External libraries? (Boost, zlib, tbb, tcmalloc, libhorde)
#. PRNG streams using 'rolling circle' approach (allow reproducibility for optional features/parallelization) 
#. Mortality due to infection
#. Autotools

OPTIMIZATION
------------

#. Use OpenMP >= 3.0 features when available?
#. SSE2 optimization of bloque bit mask manipulations
#. Demographics update
#. Date class
#. Speed initialization by reading compressed data directly (Boost? zlib?) instead of system call to gunzip.
#. Replace current PRNG with SIMD version of mersenne twister; use specialization for doubles (http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/)
#. Blacklight performance?
#. Cache performance, size of allocations

DOCUMENTATION
-------------

#. Update to cover SMP code
#. Update to cover new evolution features
#. Checked this TODO list into cvs
#. Doxygen commenting conventions <---- [put everything in headers]
#. reSt FAQ document
