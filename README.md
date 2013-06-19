Systemburn
==========

SystemBurn is a software package designed to allow users to methodically
create maximal loads on systems ranging from desktops to supercomputers.

For more information, please see docs/SYSTEMBURN_wp.pdf.

Installing SystemBurn
=====================
To compile SystemBurn, you will need either an MPI implementation or a
SHMEM implementation.  SystemBurn has been tested with OpenMPI, Cray
and SGI MPI, OpenSHMEM, and SGI SHMEM.

Other optional dependencies are outlined in docs/FAQ.

Installation
------------
    ./configure
    make
    make install

Running SystemBurn
==================

For detailed information, see docs/SYSTEMBURN_wp.pdf

If you wish to run various scenarios and compare their power utilization,
you can use scripts/harness.py.  harness.py must be called with the
-c flag.  The argument to -c should be an executable (a script of
compiled binary) that returns a number value representing some metric,
generally power or temperature, to compare between the different loads.
