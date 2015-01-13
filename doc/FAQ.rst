FAQ
===

How can I increase FRED's performance?
--------------------------------------

...use an alternative memory allocator...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Substantial reductions in runtime can result from the use of an alternative malloc implementation (as much as 50% faster!).  These libraries can either be linked during compilation, or by using LD_PRELOAD:
For example, to use **TCmalloc** (http://goog-perftools.sourceforge.net/doc/tcmalloc.html) just add the ``-ltcmalloc`` linker flag in the makefile or set the environment variable ``$ LD_PRELOAD="/usr/lib/libtcmalloc.so"``.
Other alternatives are **libhoard** (http://www.hoard.org/) and **Intel's TBB scalable allocator** (http://software.intel.com/en-us/blogs/2009/08/21/is-your-memory-management-multi-core-ready/).

...reduce the compile-time constant MAX_NUM_DISEASES...
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you're not running with multiple **diseases**, set ``Global::MAX_NUM_DISEASES = 1``.
If you are running with multiple **diseases** consider using **strains** instead (see the USER GUIDE for details on the distinction between these two terms), or set the ``Global::MAX_NUM_DISEASES`` to the minumum value needed for your experiment.
This constant is defined in the file ``Global.h``.


