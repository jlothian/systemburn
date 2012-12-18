/*
  This file is part of SystemBurn.

  Copyright (C) 2012, UT-Battelle, LLC.

  This product includes software produced by UT-Battelle, LLC under Contract No. 
  DE-AC05-00OR22725 with the Department of Energy. 

  This program is free software; you can redistribute it and/or modify
  it under the terms of the New BSD 3-clause software license (LICENSE). 
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
  LICENSE for more details.

  For more information please contact the SystemBurn developers at: 
  systemburn-info@googlegroups.com

*/
#include <systemheaders.h>
#include <systemburn.h>
#include <planheaders.h>
#include <math.h>

#ifdef HAVE_PAPI
#define NUM_PAPI_EVENTS 1 
#define PAPI_COUNTERS { PAPI_FP_OPS } 
#define PAPI_UNITS { "FLOPS" } 
#endif //HAVE_PAPI

/* GNU's GCC provides this. PGI's PGCC can digest them too, but needs the #defines */
/* they are required for the CUDA includes.  */
#define __align__(n)    __attribute__((aligned(n)))
#define __annotate__(a) __attribute__((a))
#define __location__(a) __annotate__(a)
#define CUDARTAPI

#include <cuda_runtime.h>
#include <cublas.h>

#define CUDA_CALL(x)                                                            \
	do {                                                                    \
		cudaError_t cudaTemporaryErrorCheck = (x);                      \
		if(cudaTemporaryErrorCheck != cudaSuccess) {                    \
			fprintf(stderr, "%s: %d, %s\n", #x,                     \
				cudaTemporaryErrorCheck,                        \
				cudaGetErrorString(cudaTemporaryErrorCheck));   \
			exit(1);                                                \
		}                                                               \
	} while(0)

/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line The line of input data.
 * \param [out] output The data and storage for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makeDCUBLASPlan 
 * \sa initDCUBLASPlan
 * \sa execDCUBLASPlan
 * \sa perfDCUBLASPlan
 * \sa killDCUBLASPlan
*/
int parseDCUBLASPlan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);
	output->name = DCUBLAS;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan The data and memory location for the plan.
 * \return int Error flag value
 * \sa parseDCUBLASPlan 
 * \sa makeDCUBLASPlan
 * \sa execDCUBLASPlan
 * \sa perfDCUBLASPlan
 * \sa killDCUBLASPlan
*/
int   initDCUBLASPlan(void *plan) {
	size_t avail, total, arraybytes;
	int M,i;
	int ret = make_error(ALLOC,generic_err);
	double gputhreads;
	cudaError_t cudaStat;
	struct cudaDeviceProp prop;
	Plan *p;
	DCUBLASdata *d = NULL;
	p = (Plan *)plan;

#ifdef HAVE_PAPI
        int temp_event, i;
        int PAPI_Events [NUM_PAPI_EVENTS] = PAPI_COUNTERS;
        char* PAPI_units [NUM_PAPI_EVENTS] = PAPI_UNITS;
#endif //HAVE_PAPI

	if (p) {
		d = (DCUBLASdata*)p->vptr;
		p->exec_count = 0;
                if (DO_PERF){
                        perftimer_init(&p->timers, NUM_TIMERS);

#ifdef HAVE_PAPI
                        /* Initialize plan's PAPI data */
                        p->PAPI_EventSet = PAPI_NULL;
                        p->PAPI_Num_Events = 0;

                        TEST_PAPI(PAPI_create_eventset(&p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
                        
                        //Add the desired events to the Event Set; ensure the dsired counters
                        //  are on the system then add, ignore otherwise
                        for(i=0; i<TOTAL_PAPI_EVENTS && i<NUM_PAPI_EVENTS; i++){
                            temp_event = PAPI_Events[i];
                            if(PAPI_query_event(temp_event) == PAPI_OK){
                                p->PAPI_Num_Events++;
                                TEST_PAPI(PAPI_add_event(p->PAPI_EventSet, temp_event), PAPI_OK, MyRank, 9999, PRINT_SOME);
                            }
                        }

                        PAPIRes_init(p->PAPI_Results, p->PAPI_Times);
                        PAPI_set_units(p->name, PAPI_units, NUM_PAPI_EVENTS);
                
                        TEST_PAPI(PAPI_start(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
#endif //HAVE_PAPI
                } //DO_PERF
	}
	if(d) {
		CUDA_CALL( cudaSetDevice(d->device) );
		CUDA_CALL( cudaMemGetInfo(&avail, &total) );
		CUDA_CALL( cudaGetDeviceProperties(&prop, d->device) );
		if (d->nGpuThreads != 0) {	// use the user spec'd number of threads or default to warp*cores
			gputhreads = (double)(d->nGpuThreads);
		} else {
			gputhreads = d->nGpuThreads = prop.warpSize * prop.multiProcessorCount;
		}
		if (prop.major < 2) {	// check results on older devices
			d->resultCheck = 1;
		} else {
			d->resultCheck = 0;
		}
		// calculate M for 6 M*M arrays to fill 100%/75%/50% of GPU free memory 
		// M = (d->nGpuThreads) * (int)(sqrt(0.75*avail/(6.0*sizeof(double)*gputhreads*gputhreads)));
		// M = (d->nGpuThreads) * (int)(sqrt(0.50*avail/(6.0*sizeof(double)*gputhreads*gputhreads)));
		M = (d->nGpuThreads) * (int)(sqrt(1.00*avail/(6.0*sizeof(double)*gputhreads*gputhreads)));
		// assume one will fit in host memory
		d->M = M;
		arraybytes = 3*M*M*sizeof(double);
		d->arraybytes = arraybytes;
		// host array and device arrays
		CUDA_CALL( cudaMallocHost((void **)(&(d->HA)), arraybytes) );
		CUDA_CALL( cudaMalloc    ((void **)(&(d->DA)), arraybytes) );
		CUDA_CALL( cudaMalloc    ((void **)(&(d->DB)), arraybytes) );
		// CUDA_CALL( cudaMalloc    ((void **)(&(d->DC)), arraybytes) );
		// initialize so that results are M*PI**2/100
		for(i=0; i<3*M*M; i++) d->HA[i] = (double)0.31415926535;
		CUDA_CALL( cudaMemcpy( (d->DA), (d->HA), arraybytes, cudaMemcpyHostToDevice) );
		CUDA_CALL( cudaMemcpy( (d->DB), (d->DA), arraybytes, cudaMemcpyDeviceToDevice) );
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Frees the memory used in the plan
 * \param [in] plan The data and memory locations for the plan.
 * \sa parseDCUBLASPlan
 * \sa makeDCUBLASPlan
 * \sa initDCUBLASPlan
 * \sa execDCUBLASPlan
 * \sa perfDCUBLASPlan
 */
void * killDCUBLASPlan(void *plan) {
	Plan *p;
	DCUBLASdata *d;
	p = (Plan *)plan;
	d = (DCUBLASdata*)p->vptr;
        int retval;

#ifdef HAVE_PAPI
        TEST_PAPI(PAPI_stop(p->PAPI_EventSet, NULL), PAPI_OK, MyRank, 9999, PRINT_SOME);
#endif //HAVE_PAPI

	CUDA_CALL( cudaThreadSynchronize() );
	// if(d->DC) CUDA_CALL( cudaFree((void*)(d->DC)) );
	if(d->DB) CUDA_CALL( cudaFree((void*)(d->DB)) );
	if(d->DA) CUDA_CALL( cudaFree((void*)(d->DA)) );
	if(d->HA) CUDA_CALL( cudaFreeHost((void*)(d->HA)) );
	CUDA_CALL( cudaThreadExit() );

	free((void*)(d));
	free((void*)(p));
	return (void*)NULL;
}

/**
 * \brief A CUDA double precision load for GPUs. The load is sized automatically to memory available on the GPU. The GPU "device" number, the "count" of iterations per pass, and the number of GPU "threads" to be used, may optionally be specified. The defaults are device 0, count 8, and a thread count appropriate to the device hardware.
 * \param [in] plan The data and memory location for the plan.
 * \return int Error flag value
 * \sa parseDCUBLASPlan
 * \sa makeDCUBLASPlan
 * \sa initDCUBLASPlan
 * \sa perfDCUBLASPlan
 * \sa killDCUBLASPlan
 */
int execDCUBLASPlan(void *plan) {
#ifdef HAVE_PAPI
        int k;
        long long start, end;
#endif //HAVE_PAPI

        /* local vars */
	int M, K, N, lda, ldb, ldc,i;
	double *DA, *DB, *DC;
	double alpha, beta;
	ORB_t t1, t2;
	Plan *p;
	DCUBLASdata *d;
	p = (Plan *)plan;
	d = (DCUBLASdata*)p->vptr;
	alpha = 1.0;
	beta = 0.0;
	M = K = N = lda = ldb = ldc = d->M;
	DA = d->DA;
	DB = DA + M*M;
	DC = DB + M*M;
	/* update execution count */
	p->exec_count++;
	// DeviceToDevice is not async wrt the device kernels
	// try uncommenting:
	// CUDA_CALL( cudaMemcpyAsync( (d->DA), (d->DB), (d->arraybytes), cudaMemcpyDeviceToDevice, 0) );

        if (DO_PERF){
#ifdef HAVE_PAPI
                /* Start PAPI counters and time */
                TEST_PAPI(PAPI_reset(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
                start = PAPI_get_real_usec();
#endif //HAVE_PAPI

                ORB_read(t1);
        } //DO_PERF
	for (i=0; i<(d->nLoopCount); i++)
		// original:
		// cublasDgemm('N', 'T', M, N, K, alpha, DA, lda, DB, ldb, beta, DC, ldc);
		// alternate:
		cublasDgemm('N', 'N', M, N, K, alpha, DA, lda, DB, ldb, beta, DC, ldc);
        }

        if (DO_PERF){
                ORB_read(t2);
#ifdef HAVE_PAPI
                end = PAPI_get_real_usec(); //PAPI time

                /* Collect PAPI counters and store time elapsed */
                TEST_PAPI(PAPI_accum(p->PAPI_EventSet, p->PAPI_Results), PAPI_OK, MyRank, 9999, PRINT_SOME);
                for(k=0; k<p->PAPI_Num_Events && k<TOTAL_PAPI_EVENTS; k++){
                    p->PAPI_Times[k] += (end - start);
                }
#endif //HAVE_PAPI

                perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));
        } //DO_PERF

	// DeviceToHost is async wrt the device kernels
	// try uncommenting:
	// CUDA_CALL( cudaMemcpyAsync( (d->HA), (d->DA), (d->arraybytes), cudaMemcpyDeviceToHost, 0) );
//	if (d->resultCheck == 1) {
//		CUDA_CALL( cudaThreadSynchronize() );
//		CUDA_CALL( cudaMemcpyAsync( (d->HA), (DC), (M*M*sizeof(double)), cudaMemcpyDeviceToHost, 0) );
//		double err; 
//		double maxerr=0.0;
//		for (i=0; i<(M*M); i++) {
//			err = (d->HA[i]) - (M)*(0.31415926535*0.31415926535);
//			if (err < 0.0) err = -err;
//			if (err > maxerr) maxerr = err;
//		}
//		if (maxerr > 1.0e-6) printf("GPU CHECK: MaxError = %f\n", maxerr);
//	}
	return ERR_CLEAN;
}

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parseDCUBLASPlan
 * \sa makeDCUBLASPlan
 * \sa initDCUBLASPlan
 * \sa execDCUBLASPlan
 * \sa killDCUBLASPlan
 */
int perfDCUBLASPlan (void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	DCUBLASdata *d;
	p = (Plan *)plan;
	d = (DCUBLASdata *)p->vptr;
	if (p->exec_count > 0) {
		uint64_t M2 = (uint64_t)d->M * (uint64_t)d->M;
		opcounts[TIMER0] = (M2 * (2 * (uint64_t)d->M + 1)) * (uint64_t)d->nLoopCount * p->exec_count; // Count # of floating point operations
		opcounts[TIMER1] = 0;
		opcounts[TIMER2] = 0;
		
		perf_table_update(&p->timers, opcounts, p->name);
#ifdef HAVE_PAPI
		PAPI_table_update(p->name, p->PAPI_Results, p->PAPI_Times, p->PAPI_Num_Events);
#endif //HAVE_PAPI
		
		double flops  = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		EmitLogfs(MyRank, 9999, "DCUBLAS plan performance:", flops, "MFLOPS", PRINT_SOME);
		EmitLog  (MyRank, 9999, "DCUBLAS execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info DCUBLAS_info = {
	"DCUBLAS",
	NULL,
	0,
	makeDCUBLASPlan,
	parseDCUBLASPlan,
	execDCUBLASPlan,
	initDCUBLASPlan,
	killDCUBLASPlan,
	perfDCUBLASPlan,
	{ "FLOPS", NULL, NULL }
};

/* PLAN 1 DCUBLAS GPUDEV#  LOOPCOUNT BLOCKFACTOR */
/* GPUDEV#      indicates to which device the load should be bound.                          Default: 0 */
/* LOOPCOUNT    is the number of DGEMM iterations called for a single execution of the load. Default: 8 */
/* BLOCKFACTOR  array sizes will be a multiple of this. If zero or unspecified it will default to  warps*cores   */
/* currently we run only with square matrices sized based on GPU memory  */
/* this can change if there's a good reason                              */

/**
 * \brief Allocates and returns the data struct for the plan
 * \param [in] m The input data for the plan.
 * \return void* Data struct 
 * \sa parseDCUBLASPlan
 * \sa initDCUBLASPlan
 * \sa execDCUBLASPlan
 * \sa perfDCUBLASPlan
 * \sa killDCUBLASPlan
*/
void * makeDCUBLASPlan(data *m) {
	Plan *p;
	DCUBLASdata *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initDCUBLASPlan;
		p->fptr_execplan = &execDCUBLASPlan;
		p->fptr_killplan = &killDCUBLASPlan;
		p->fptr_perfplan = &perfDCUBLASPlan;
		p->name = DCUBLAS;
		d = (DCUBLASdata *)malloc(sizeof(DCUBLASdata));
		assert(d);
		// defaults
		d->device      = 0;
		d->nLoopCount  = 8;
		d->nGpuThreads = 0;
		// inputs
		if (m->isize>0) d->device      = m->i[0];
		if (m->isize>1) d->nLoopCount  = m->i[1];
		if (m->isize>2) d->nGpuThreads = m->i[2];
		(p->vptr)=(void*)d;
	}
	/* delay allocating any further structures... for CPU-binding of NUMA & GPU control thread */
	return p;
}
