#!/bin/bash
#  This file is part of SystemBurn.
#
#  Copyright (C) 2012, UT-Battelle, LLC.
#
#  This product includes software produced by UT-Battelle, LLC under Contract No. 
#  DE-AC05-00OR22725 with the Department of Energy. 
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the New BSD 3-clause software license (LICENSE). 
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
#  LICENSE for more details.
#
#  For more information please contact the SystemBurn developers at: 
#  systemburn-info@googlegroups.com

. /opt/modules/default/etc/modules.sh
module purge
module load modules
module load Base-opts
module load DefApps
module load PrgEnv-gnu
module unload xt-mpt
module load xt-mpich2
module load xt-shmem
module load fftw
module load gsl
module load git
module list

set -x

export CC=cc
export MPICC=cc
export CFLAGS="-m64 -O2"
export BLAS_LIBS=-lgslcblas
export FFTW3_LIBS="${FFTW_INCLIDE_OPTS} -l${FFTW_DIR}/libfftw3.a"

#autoconf			|| exit
./configure			|| exit
make -j12			|| exit
mv systemburn systemburn-jaguar-mpi
make clean

export SHMEM_LIBS=-lsma

#autoconf			|| exit
./configure --with-shmem	|| exit
make -j12			|| exit
mv systemburn systemburn-jaguar-shmem
make clean

exit 
