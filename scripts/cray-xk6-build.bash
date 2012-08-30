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
module load PrgEnv-pgi
module swap PrgEnv-pgi PrgEnv-gnu
module load gsl
module swap PrgEnv-gnu PrgEnv-pgi
module unload xt-mpt
module load xt-mpich2
module load xt-shmem
module load fftw
module load atlas
module load git
module load cudatoolkit
module list

set -x

#export CC=nvcc
export MPICC=cc
export CFLAGS="-m64 -O2"
export CUDA_LIBS=$CRAY_CUDATOOLKIT_DIR/lib64
export LDFLAGS="$CRAY_CUDATOOLKIT_INCLUDE_OPTS $CRAY_CUDATOOLKIT_POST_LINK_OPTS -lcublas -lcuda -lcudart"
export BLAS_LIBS=$ATLAS_LIB
export FFTW3_LIBS="-I$FFTW_INC -L$FFTW_DIR -lfftw3"

#autoconf			|| exit
./configure --disable-openmp	|| exit
make clean
make -j16			|| exit
mv systemburn systemburn-mpi
make clean

export SHMEM_LIB="-L${CRAY_SHMEM_DIR}/lib64 -lsma"
export SHMEM_LIBS="-L${CRAY_SHMEM_DIR}/lib64 -lsma"

#autoconf			|| exit
./configure --with-shmem --disable-openmp	|| exit
make clean
make -j16			|| exit
mv systemburn systemburn-shmem
make clean

exit 
