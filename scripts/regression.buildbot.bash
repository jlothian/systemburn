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
#PBS -A STF006
#PBS -N SystemBurn
#PBS -l size=64
# 12288 for jaguar, 384 for chester
####### size=24
####### mppwidth=32
####### mppnppn=1
####### mppdepth=12
#PBS -l walltime=03:00:00
#PBS -j eo
#PBS

#set -x
#job configuration adjust for test platform

# If MAILREPORT is set to "false", email reporting is disabled. If
# MAILREPORT is set to something other than "false", the script will
# attempt to use the value of MAILREPORT as an email address and send a
# copy fo the Pass/Fail report to that address.  NOTE: The email subject
# will include the hostname.
export BUILDDIR=$(pwd)

export SHORTHOSTNAME=$(hostname --short)
case ${SHORTHOSTNAME} in
	stride*)
		export PROCESSORS_PER_NODE=2
		export CORES_PER_PROCESSOR=8
		export CORES_PER_NODE=16
		export GPUS_PER_NODE=0
		export WORKERS=$(( ${CORES_PER_NODE} + ${GPUS_PER_NODE} ))
		export RANKS=6
		# export COMMSIZES="-c1 -c1000 -c5000 -c10000 -c50000"
		# export COMMSIZES="-c1 -c100 -c1000"
		export COMMSIZES=""
		export MASK1="MASK  0  1  2  3  4  5  6  7"
		export MASK2="MASK  8  9 10 11 12 13 14 15"
		export MASK3=""
		export MASK4=""
		export MASK5=""
		export MASK6=""
		export MASK7=""
		export MASK8=""
		export SYSTEMBURN=systemburn-stride
		export WORKDIR=${BUILDDIR}/regression_test
		export MPI_INCANTATION="mpirun --verbose -np 2 --npernode 1 --hostfile ${HOME}/hostfiles/stride -mca btl tcp,sm,self --prefix /usr/lib64/openmpi -x ENVIRONMENT=BATCH"
		export RUNTIMETAGLINE='XXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
		: ;;
	chester*)
		export PROCESSORS_PER_NODE=1
		export CORES_PER_PROCESSOR=16
		export CORES_PER_NODE=16
		export GPUS_PER_NODE=1
		export GPUTHREADS=512
		export GPULOOPS=16
		export WORKERS=$(( ${CORES_PER_NODE} + ${GPUS_PER_NODE} ))
		export COMMSIZES="1 10"
		export MASK1="MASK 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15"
		export MASK2=""
		export MASK3=""
		export MASK4=""
		export MASK5=""
		export MASK6=""
		export MASK7=""
		export MASK8=""
		export GPU_MASK="MASK 0 4 8 12 "
		export RANKS=32
		export RANKS=$(( ${PBS_NNODES} / ${CORES_PER_NODE} ))
		export SYSTEMBURN="systemburn-chester-mpi"
		export SYSTEMBURN="systemburn-chester-mpi systemburn-chester-shmem"
		export WORKDIR=/tmp/work/kuehn
		export MPI_INCANTATION="aprun -n${RANKS} -N1 -d${CORES_PER_NODE}"
		export XT_SYMMETRIC_HEAP_SIZE=16G
		# export SHMEM_ENV_DISPLAY=1
		# export SHMEM_MEMINFO_DISPLAY=1
		export RUNTIMETAGLINE='Application.*resources: utime.*stime.*|Application .* exit codes: .*'
		: ;;
	jaguar*)
		export PROCESSORS_PER_NODE=2
		export CORES_PER_PROCESSOR=6
		export CORES_PER_NODE=12
		export GPUS_PER_NODE=0
		export WORKERS=$(( ${CORES_PER_NODE} + ${GPUS_PER_NODE} ))
		export COMMSIZES="-c1 -c1000 -c60000"
		export MASK1="MASK  0  1  2  3  4  5"
		export MASK2="MASK  6  7  8  9 10 11"
		export MASK3=""
		export MASK4=""
		export MASK5=""
		export MASK6=""
		export MASK7=""
		export MASK8=""
		export RANKS=1024
		export RANKS=$(( ${PBS_NNODES} / ${CORES_PER_NODE} ))
		export SYSTEMBURN=systemburn-jaguar
		export WORKDIR=/tmp/work/kuehn
		export MPI_INCANTATION="aprun -n${RANKS} -N1 -d${CORES_PER_NODE}"
		export RUNTIMETAGLINE='Application.*resources: utime.*stime.*'
		: ;;
	spry*)
		export PROCESSORS_PER_NODE=2
		export CORES_PER_PROCESSOR=24
		export CORES_PER_NODE=48
		export GPUS_PER_NODE=0
		export WORKERS=$(( ${CORES_PER_NODE} + ${GPUS_PER_NODE} ))
		export RANKS=6
		export COMMSIZES="-c1 -c1000 -c5000 -c10000 -c50000"
	  #export COMMSIZES="-c1 -c100 -c1000"
		#export COMMSIZES=""
		export MASK1=""
		export MASK2=""
		export MASK3=""
		export MASK4=""
		export MASK5=""
		export MASK6=""
		export MASK7=""
		export MASK8=""
		export SYSTEMBURN=systemburn
		export WORKDIR=${BUILDDIR}/regression_test
		export MPI_INCANTATION="mpirun --verbose -np 1 --npernode 1 -mca btl tcp,sm,self -x ENVIRONMENT=BATCH"
		export RUNTIMETAGLINE='XXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
		: ;;
	pod*)
		export PROCESSORS_PER_NODE=2
		export CORES_PER_PROCESSOR=4
		export CORES_PER_NODE=8
		export GPUS_PER_NODE=0
		export WORKERS=$(( ${CORES_PER_NODE} + ${GPUS_PER_NODE} ))
		export RANKS=6
		export COMMSIZES="-c1 -c1000"
		# export COMMSIZES="-c1 -c100 -c1000"
		export MASK1="MASK  0  1  2  3"
		export MASK2="MASK  4  5  6  7"
		export MASK3=""
		export MASK4=""
		export MASK5=""
		export MASK6=""
		export MASK7=""
		export MASK8=""
		export SYSTEMBURN=systemburn
		export WORKDIR=${BUILDDIR}
    if [ x"${SHORTHOSTNAME}" == x"pod10" ] ; then
		  export MPI_INCANTATION="mpirun --verbose -np 1 --npernode 1 -mca btl tcp,sm,self --prefix /usr/lib64/mpi/gcc/openmpi -x ENVIRONMENT=BATCH"
    else
		  export MPI_INCANTATION="mpirun --verbose -np 1 --npernode 1 -mca btl tcp,sm,self --prefix /usr/lib64/openmpi -x ENVIRONMENT=BATCH"
    fi
		export RUNTIMETAGLINE='XXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
		: ;;
	*)
		echo HOSTNAME unrecognized. bailing out...
		exit
esac

# test load configuration -- no need to change for regression test
export TIME=30
export DGEMM=512m
export DSTREAM=512m
export DSTRIDE=512m
export FFT1D=512m
export FFT2D=512m
export GUPS=512m
export LSTREAM=512m
export LSTRIDE=512m
export PV1=512m
export PV2=512m
export PV3=512m
export PV3=512m
export PV4=5m
export RDGEMM1=512m
export RDGEMM2=512m
export SLEEP=5
export TILT="128 12"
export CBA=512m

# constants -- not good enough....
export JOBTIME=`date +%Y.%m.%d__%H:%M`
export BOILER='^$|Calibrating performance timers|Initialization complete|Elapsed time for this load|Run Completed|^[ 0-9 ][ 0-9]*$'
export TEMPER='No valid temperature monitoring|temperature across all cores'
export LDTOP='General load information|^   Comm [ED].*abled|^   Load runtime|^   Thread count|^   Load schedule'
export LDBOT='^ *Subload|^   Number of threads|^   Plan|^   cpuset'
export PLANLB='^   DGEMM|^   RDGEMM|^   [DL]STREAM|^   [DL]STRIDE|^   FFT[12]D|^   GUPS|^   PV[123]|^   SLEEP|^   TILT|^   CBA'
export PERF="^PERF:|^PAPI:"


# usage: genload TIME PROCESSORS_PER_NODE CORES_PER_PROCESSOR LOAD SIZE
#           where:
#                TIME    : Runtime for this load
#                PROCESSORS_PER_NODE   : Number of processors per node
#                CORES_PER_PROCESSOR   : Number of cores per processor
#                LOAD    : Load name
#                SIZE    : Size parameter for this load
#                SIZE2   : SECOND Size parameter for this load
#		 MOD     : tag to customize load name
genGPUload() {
	if [ "${GPUS_PER_NODE}" -gt "0" ]
	then
		echo "	SUBLOAD 1"
		echo "	["
		for (( COUNT=${GPUS_PER_NODE} ; ${COUNT} ; COUNT=${COUNT} - 1 ))
		do
			echo "		PLAN 1 SCUBLAS $(( ${COUNT} - 1 )) ${GPULOOPS} ${GPUTHREADS}"
		done
		echo "		${GPU_MASK}"
		echo "	]"
	fi
	return
}

genload() {
	ITIME=$1
	IPROCESSORS_PER_NODE=$2
	ICORES_PER_PROCESSOR=$3
	ILOAD=$4
	ISIZE=$5
	ISIZE2=$6
	MOD=$7
	OUTFILE=$(echo testload ${ITIME}sec ${IPROCESSORS_PER_NODE}p ${ICORES_PER_PROCESSOR}c ${ILOAD} ${ISIZE} ${ISIZE2} ${MOD}|tr ' ' '_')
	echo "LOAD_START"								>  ${OUTFILE}
	echo "	RUNTIME  ${ITIME}"							>> ${OUTFILE}
	echo "	SCHEDULE SUBLOAD_SPECIFIC"						>> ${OUTFILE}
	echo "	SUBLOAD  ${IPROCESSORS_PER_NODE}"					>> ${OUTFILE}
	echo "	["									>> ${OUTFILE}
	echo "		PLAN ${ICORES_PER_PROCESSOR} ${ILOAD} ${ISIZE} ${ISIZE2}"	>> ${OUTFILE}
	echo "		${MASK1}"							>> ${OUTFILE}
	echo "		${MASK2}"							>> ${OUTFILE}
	echo "		${MASK3}"							>> ${OUTFILE}
	echo "		${MASK4}"							>> ${OUTFILE}
	echo "		${MASK5}"							>> ${OUTFILE}
	echo "		${MASK6}"							>> ${OUTFILE}
	echo "		${MASK7}"							>> ${OUTFILE}
	echo "		${MASK8}"							>> ${OUTFILE}
	echo "	]"									>> ${OUTFILE}
	genGPUload									>> ${OUTFILE}
	echo "LOAD_END"									>> ${OUTFILE}
	return
}

passfail() {
	OUTPUT=$1
	PASS=true
	STATUS=$(cat r.${OUTPUT}.status)
	if [ ! -s r.${OUTPUT}.stdout ]
	then
		PASS=false
	fi
	ERRORCOUNT=$(cat r.${OUTPUT}.stdout | egrep -v "$BOILER" | egrep -v "$TEMPER" | egrep -v "$LDTOP" | egrep -v "$LDBOT" | egrep -v "$PLANLB" | egrep -v "$RUNTIMETAGLINE" | egrep -v "$PERF" | wc -l)
	if [ "${ERRORCOUNT}" -ne "0" ]
	then
		PASS=false
	fi
	if [ "${PASS}" = "true" ]
	then
		echo $(date +"%Y/%m/%d %H:%M:%S %Z (%s)") PASSED: ${OUTPUT}
	else
		echo $(date +"%Y/%m/%d %H:%M:%S %Z (%s)") FAILED: ${OUTPUT}
		echo $(date +"%Y/%m/%d %H:%M:%S %Z (%s)") Errors Identified:
		cat r.${OUTPUT}.stdout | egrep -v "$BOILER" | egrep -v "$TEMPER" | egrep -v "$LDTOP" | egrep -v "$LDBOT" | egrep -v "$PLANLB" | egrep -v "$RUNTIMETAGLINE"
		echo $(date +"%Y/%m/%d %H:%M:%S %Z (%s)") r.${OUTPUT}.stderr:
		cat r.${OUTPUT}.stderr
	fi
}	

cd ${BUILDDIR}
mkdir -p            ${WORKDIR}/SystemBurn.${JOBTIME}
cp    ${SYSTEMBURN} ${WORKDIR}/SystemBurn.${JOBTIME}

cd $WORKDIR/SystemBurn.${JOBTIME}

genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR DGEMM	"$DGEMM"
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR DSTREAM	"$DSTREAM"
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR DSTRIDE	"$DSTRIDE"
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR FFT1D	"$FFT1D"
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR FFT2D	"$FFT2D"
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR GUPS	"$GUPS"
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR LSTREAM	"$LSTREAM"
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR LSTRIDE	"$LSTRIDE"
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR PV1	"$PV1"
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR PV2	"$PV2"
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR PV3	"$PV3"
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR RDGEMM	"$RDGEMM1"	a
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR RDGEMM	"$RDGEMM2"	b
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR TILT	"${TILT}"
genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR CBA "${CBA}"
# genload $TIME $PROCESSORS_PER_NODE $CORES_PER_PROCESSOR SLEEP	"$SLEEP"

cat > systemburn.config <<EOF1
WORKERS ${WORKERS}
REST_TIME 30
MAX_TEMP 85
MONITOR_FREQ 120
MONITOR_OUT 120
EOF1

echo > report

for SYSTEMBURNEXE in ${SYSTEMBURN}
do
	echo $(date +"%Y/%m/%d %H:%M:%S %Z (%s)") "TEST BEGINS for $SYSTEMBURNEXE" | tee -a report
	for COMMTEST in ${COMMSIZES}
	do
		echo $(date +"%Y/%m/%d %H:%M:%S %Z (%s)") Testing with the following COMM load options:  -c "${COMMTEST}"
		for LOAD in testload*
		do
		  ${MPI_INCANTATION} ./${SYSTEMBURNEXE} -c "${COMMTEST}" ${LOAD}   > r.${SYSTEMBURNEXE}_${LOAD}_c${COMMTEST}.stdout       2> r.${SYSTEMBURNEXE}_${LOAD}_c${COMMTEST}.stderr
		  echo "$?"                                                > r.${SYSTEMBURNEXE}_${LOAD}_c${COMMTEST}.status
		  passfail ${SYSTEMBURNEXE}_${LOAD}_c${COMMTEST}
		done
		#${MPI_INCANTATION} ./${SYSTEMBURNEXE} -c ${COMMTEST} testload* > r.${SYSTEMBURNEXE}_testload-all_c${COMMTEST}.stdout 2> r.${SYSTEMBURNEXE}_testload-all_c${COMMTEST}.stderr
		#echo "$?"                                                > r.${SYSTEMBURNEXE}_testload-all_c${COMMTEST}.status
		#passfail ${SYSTEMBURNEXE}_testload-all_c${COMMTEST}
	done 2>&1 | tee -a report
	echo $(date +"%Y/%m/%d %H:%M:%S %Z (%s)") "TEST ENDS for $SYSTEMBURNEXE" | tee -a report
done

if grep FAILED report
	then
		exit 1
	else
    exit 0
fi
