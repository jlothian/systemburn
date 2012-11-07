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

