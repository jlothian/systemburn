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
 * \param [in] line Input line for the plan.
 * \param [out] output Holds the data for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makeSCUBLASPlan 
 * \sa initSCUBLASPlan
 * \sa execSCUBLASPlan
 * \sa perfSCUBLASPlan
 * \sa killSCUBLASPlan
*/
int parseSCUBLASPlan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);
        output->name = SCUBLAS;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan Holds the data and the memory for the plan.
 * \return int Error flag value
 * \sa parseSCUBLASPlan 
 * \sa makeSCUBLASPlan
 * \sa execSCUBLASPlan
 * \sa perfSCUBLASPlan
 * \sa killSCUBLASPlan
*/
int   initSCUBLASPlan(void *plan) {
	size_t avail, total, arraybytes;
	size_t M,i;
	int ret = make_error(ALLOC,generic_err);
	double gputhreads;
	cudaError_t cudaStat;
	struct cudaDeviceProp prop;
	Plan *p;
	SCUBLASdata *d = NULL;
	p = (Plan *)plan;
	if (p) {
		d = (SCUBLASdata*)p->vptr;
		p->exec_count = 0;
		perftimer_init(&p->timers, NUM_TIMERS);
	}
	if (d) {
		CUDA_CALL( cudaSetDevice(d->device) );
		CUDA_CALL( cudaMemGetInfo(&avail, &total) );
		CUDA_CALL( cudaGetDeviceProperties(&prop, d->device) );
                if (d->nGpuThreads != 0) {      // use the user spec'd number of threads or default to warp*cores
                        gputhreads = (double)(d->nGpuThreads);
                } else {
                        gputhreads = d->nGpuThreads = prop.warpSize * prop.multiProcessorCount;
                }
                if (prop.major < 2) {   // check results on older devices
                        d->resultCheck = 1;
                } else {
                        d->resultCheck = 0;
                }
                // calculate M for 3 M*M arrays to fill 100%/75%/50% of GPU free memory 
                // M = (d->nGpuThreads) * (int)(sqrt(0.75*avail/(3.0*sizeof(double)*gputhreads*gputhreads)));
                // M = (d->nGpuThreads) * (int)(sqrt(0.50*avail/(3.0*sizeof(double)*gputhreads*gputhreads)));
                M = (d->nGpuThreads) * (int)(sqrt(1.00*avail/(3.0*sizeof(float)*gputhreads*gputhreads)));
		// assume one will fit in host memory
		d->M = M;
		arraybytes = M*M*sizeof(float);
		d->arraybytes = arraybytes;
		// host array and device arrays
		CUDA_CALL( cudaMallocHost((void **)(&(d->HA)), arraybytes) );
		CUDA_CALL( cudaMalloc    ((void **)(&(d->DA)), arraybytes) );
		CUDA_CALL( cudaMalloc    ((void **)(&(d->DB)), arraybytes) );
		CUDA_CALL( cudaMalloc    ((void **)(&(d->DC)), arraybytes) );
		// initialize so that results are M*PI**2/100
		for(i=0; i<M*M; i++) d->HA[i] = (float)0.31415926535;
		CUDA_CALL( cudaMemcpy( (d->DA), (d->HA), arraybytes, cudaMemcpyHostToDevice) );
		CUDA_CALL( cudaMemcpy( (d->DB), (d->DA), arraybytes, cudaMemcpyDeviceToDevice) );
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Frees the memory used in the plan
 * \param [in] plan Points to the memory to be free'd
 * \sa parseSCUBLASPlan
 * \sa makeSCUBLASPlan
 * \sa initSCUBLASPlan
 * \sa execSCUBLASPlan
 * \sa perfSCUBLASPlan
 */
void * killSCUBLASPlan(void *plan) {
	Plan *p;
	SCUBLASdata *d;
	p = (Plan *)plan;
	d = (SCUBLASdata*)p->vptr;
	CUDA_CALL( cudaThreadSynchronize() );
	if(d->DC) CUDA_CALL( cudaFree((void*)(d->DC)) );
	if(d->DB) CUDA_CALL( cudaFree((void*)(d->DB)) );
	if(d->DA) CUDA_CALL( cudaFree((void*)(d->DA)) );
	if(d->HA) CUDA_CALL( cudaFreeHost((void*)(d->HA)) );
	CUDA_CALL( cudaThreadExit() );
	free((void*)(d));
	free((void*)(p));
	return (void*)NULL;
}

/**
 * \brief A CUDA single precision load for GPUs. The load is sized automatically to memory available on the GPU. The GPU "device" number, the "count" of iterations per pass, and the number of GPU "threads" to be used, may optionally be specified.  The defaults are device 0, count 8, and a thread count appropriate to the device hardware.
 * \param [in] plan Holds the data for the plan.
 * \return int Error flag value
 * \sa parseSCUBLASPlan
 * \sa makeSCUBLASPlan
 * \sa initSCUBLASPlan
 * \sa perfSCUBLASPlan
 * \sa killSCUBLASPlan
 */
int execSCUBLASPlan(void *plan) {
	int M, K, N, lda, ldb, ldc,i;
	float *DA, *DB, *DC;
	float alpha, beta;
	int ret = ERR_CLEAN;
	ORB_t t1, t2;
	Plan *p;
	SCUBLASdata *d;
	p = (Plan *)plan;
	d = (SCUBLASdata*)p->vptr;
	alpha = 1.0;
	beta = 0.0;
	DA = d->DA;
	DB = d->DB;
	DC = d->DC;
	M = K = N = lda = ldb = ldc = d->M;
	/* update execution count */
	p->exec_count++;
	
	ORB_read(t1);
	for (i=0; i<(d->nLoopCount); i++)
		cublasSgemm('N', 'T', M, N, K, alpha, DA, lda, DB, ldb, beta, DC, ldc);
	ORB_read(t2);
	perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));
	if (CHECK_CALC && d->resultCheck == 1) {
		CUDA_CALL( cudaThreadSynchronize() );
		CUDA_CALL( cudaMemcpy( (d->HA), (d->DC), (d->arraybytes), cudaMemcpyDeviceToHost) );
		float err; 
		float maxerr=0.0;
		ORB_read(t1);
		for (i=0; i<((d->M)*(d->M)); i++) {
			err = (d->HA[i]) - (d->M)*(0.31415926535*0.31415926535);
			if (err < 0.0) err = -err;
			if (err > maxerr) maxerr = err;
		}
		ORB_read(t2);
		perftimer_accumulate(&p->timers, TIMER1, ORB_cycles_a(t2, t1));
		if (maxerr > 1.0e-6) {
			EmitLogfs(MyRank, 9999, "SCUBLAS GPU check: MaxError:", maxerr, "", 0);
			ret = make_error(CALC, generic_err);
		}
	}
	return ret;
}

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parseSCUBLASPlan
 * \sa makeSCUBLASPlan
 * \sa initSCUBLASPlan
 * \sa execSCUBLASPlan
 * \sa killSCUBLASPlan
 */
int perfSCUBLASPlan (void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	SCUBLASdata *d;
	p = (Plan *)plan;
	d = (SCUBLASdata *)p->vptr;
	if (p->exec_count > 0) {
		uint64_t M2 = (uint64_t)d->M * (uint64_t)d->M;
		opcounts[TIMER0] = (M2 * (2 * (uint64_t)d->M + 1)) * (uint64_t)d->nLoopCount * p->exec_count; // Count # of floating point operations
		opcounts[TIMER1] = M2 * 2;
		opcounts[TIMER2] = 0;
		
		perf_table_update(&p->timers, opcounts, p->name);
		
		double flops  = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		EmitLogfs(MyRank, 9999, "SCUBLAS plan performance:", flops, "MFLOPS", PRINT_SOME);
		EmitLog  (MyRank, 9999, "SCUBLAS execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info SCUBLAS_info = {
	"SCUBLAS",
	NULL,
	0,
	makeSCUBLASPlan,
	parseSCUBLASPlan,
	execSCUBLASPlan,
	initSCUBLASPlan,
	killSCUBLASPlan,
	perfSCUBLASPlan,
	{ "FLOPS", "FLOPS", NULL }
};

/* PLAN 1 SCUBLAS GPUDEV#  LOOPCOUNT BLOCKFACTOR */
/* GPUDEV#      indicates to which device the load should be bound.                          Default: 0 */
/* LOOPCOUNT    is the number of SGEMM iterations called for a single execution of the load. Default: 8 */
/* BLOCKFACTOR  array sizes will be a multiple of this. If zero or unspecified it will default to  warps*cores   */
/* currently we run only with square matrices sized based on GPU memory  */
/* this can change if there's a good reason                              */
/**
 * \brief Allocates and returns the data struct for the plan
 * \param [in] m Holds the input data for the plan.
 * \return void* Data struct 
 * \sa parseSCUBLASPlan
 * \sa initSCUBLASPlan
 * \sa execSCUBLASPlan
 * \sa perfSCUBLASPlan
 * \sa killSCUBLASPlan
*/
void * makeSCUBLASPlan(data *m) {
	Plan *p;
	SCUBLASdata *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initSCUBLASPlan;
		p->fptr_execplan = &execSCUBLASPlan;
		p->fptr_killplan = &killSCUBLASPlan;
		p->fptr_perfplan = &perfSCUBLASPlan;
		p->name = SCUBLAS;
		d = (SCUBLASdata *)malloc(sizeof(SCUBLASdata));
		assert(d);
		// defaults
		d->device =      0;
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
