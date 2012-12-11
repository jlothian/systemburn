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

int bitpatterns[] = {
        0x0,
        0xffff,
        0xaaaa,
        0xf0f0,
        0x0f0f,
        0xa0a0,
        0x0a0a};
int numpatterns = 7;

/* GNU's GCC provides this. PGI's PGCC can digest them too, but needs the #defines */
/* they are required for the CUDA includes.  */
#define __align__(n)    __attribute__((aligned(n)))
#define __annotate__(a) __attribute__((a))
#define __location__(a) __annotate__(a)
#define CUDARTAPI

#include <cuda_runtime.h>

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
 * \sa makeCUDAMEMPlan 
 * \sa initCUDAMEMPlan
 * \sa execCUDAMEMPlan
 * \sa perfCUDAMEMPlan
 * \sa killCUDAMEMPlan
*/
int parseCUDAMEMPlan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);
	output->name = CUDAMEM;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan The data and memory location for the plan.
 * \return int Error flag value
 * \sa parseCUDAMEMPlan 
 * \sa makeCUDAMEMPlan
 * \sa execCUDAMEMPlan
 * \sa perfCUDAMEMPlan
 * \sa killCUDAMEMPlan
*/
int   initCUDAMEMPlan(void *plan) {
	size_t avail, total, arraybytes;
	int M,i;
	int ret = make_error(ALLOC,generic_err);
	double gputhreads;
	cudaError_t cudaStat;
	struct cudaDeviceProp prop;
	Plan *p;
	CUDAMEMdata *d = NULL;
	p = (Plan *)plan;
	if (p) {
		d = (CUDAMEMdata*)p->vptr;
		p->exec_count = 0;
		perftimer_init(&p->timers, NUM_TIMERS);
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
		arraybytes = (size_t)(0.99*avail);
		d->arraybytes = arraybytes;
                d->arrayelems = arraybytes / sizeof(int);
		// host array and device arrays
		CUDA_CALL( cudaMallocHost((void **)(&(d->hostarray)), arraybytes) );
		CUDA_CALL( cudaMalloc    ((void **)(&(d->devicearray)), arraybytes) );
		// initialize so that results are M*PI**2/100
		//for(i=0; i<3*M*M; i++) d->HA[i] = (double)0.31415926535;
		//CUDA_CALL( cudaMemcpy( (d->DA), (d->HA), arraybytes, cudaMemcpyHostToDevice) );
		//CUDA_CALL( cudaMemcpy( (d->DB), (d->DA), arraybytes, cudaMemcpyDeviceToDevice) );
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Frees the memory used in the plan
 * \param [in] plan The data and memory locations for the plan.
 * \sa parseCUDAMEMPlan
 * \sa makeCUDAMEMPlan
 * \sa initCUDAMEMPlan
 * \sa execCUDAMEMPlan
 * \sa perfCUDAMEMPlan
 */
void * killCUDAMEMPlan(void *plan) {
	Plan *p;
	CUDAMEMdata *d;
	p = (Plan *)plan;
	d = (CUDAMEMdata*)p->vptr;
	
	CUDA_CALL( cudaThreadSynchronize() );
	// if(d->DC) CUDA_CALL( cudaFree((void*)(d->DC)) );
	if(d->devicearray) CUDA_CALL( cudaFree((void*)(d->devicearray)) );
	if(d->hostarray) CUDA_CALL( cudaFreeHost((void*)(d->hostarray)) );
	CUDA_CALL( cudaThreadExit() );
	free((void*)(d));
	free((void*)(p));
	return (void*)NULL;
}

/**
 * \brief A CUDA double precision load for GPUs. The load is sized automatically to memory available on the GPU. The GPU "device" number, the "count" of iterations per pass, and the number of GPU "threads" to be used, may optionally be specified. The defaults are device 0, count 8, and a thread count appropriate to the device hardware.
 * \param [in] plan The data and memory location for the plan.
 * \return int Error flag value
 * \sa parseCUDAMEMPlan
 * \sa makeCUDAMEMPlan
 * \sa initCUDAMEMPlan
 * \sa perfCUDAMEMPlan
 * \sa killCUDAMEMPlan
 */
int execCUDAMEMPlan(void *plan) {
	int M, K, N, lda, ldb, ldc, i;
        size_t k;
        int ret = ERR_CLEAN;
        int j;
        int elems;
        int pattern;
	int *DA, *HA; 
	ORB_t t1, t2;
	Plan *p;
	CUDAMEMdata *d;
	p = (Plan *)plan;
	d = (CUDAMEMdata*)p->vptr;
	DA = d->devicearray;
        HA = d->hostarray;
	/* update execution count */
	p->exec_count++;
	// DeviceToDevice is not async wrt the device kernels
	// try uncommenting:
	// CUDA_CALL( cudaMemcpyAsync( (d->DA), (d->DB), (d->arraybytes), cudaMemcpyDeviceToDevice, 0) );
	ORB_read(t1);
	for (i=0; i<(d->nLoopCount); i++){
                for (elems = 1<<19 ; elems <((d->arrayelems)/8) ; elems = elems << 1){
                        for(j=0; j<256;j++){
                                pattern = j+(j<<8);
                                for(k=0; k < elems; k++){ // initialize the array
                                        d->hostarray[k] = pattern;
                                }
                                // copy the array
                                CUDA_CALL( cudaMemcpy( (d->devicearray), (d->hostarray), elems*sizeof(int), cudaMemcpyHostToDevice));
                                CUDA_CALL( cudaMemcpy( (d->hostarray), (d->devicearray), elems*sizeof(int), cudaMemcpyDeviceToHost)); 
                                for(k=0; k<elems; k++){ // initialize the array
                                        if(d->hostarray[k] != pattern){
                                                ret = 42;
                                        }
                                } // test
                        } // bit patterns
                } // elems
        } // loops



                
	ORB_read(t2);
	perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));
	// DeviceToHost is async wrt the device kernels
	// try uncommenting:
	// CUDA_CALL( cudaMemcpyAsync( (d->HA), (d->DA), (d->arraybytes), cudaMemcpyDeviceToHost, 0) );
        return ret;
}

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parseCUDAMEMPlan
 * \sa makeCUDAMEMPlan
 * \sa initCUDAMEMPlan
 * \sa execCUDAMEMPlan
 * \sa killCUDAMEMPlan
 */
int perfCUDAMEMPlan (void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	CUDAMEMdata *d;
	p = (Plan *)plan;
	d = (CUDAMEMdata *)p->vptr;
	if (p->exec_count > 0) {
		uint64_t M2 = (uint64_t)d->M * (uint64_t)d->M;
		opcounts[TIMER0] = (M2 * (2 * (uint64_t)d->M + 1)) * (uint64_t)d->nLoopCount * p->exec_count; // Count # of floating point operations
		opcounts[TIMER1] = 0;
		opcounts[TIMER2] = 0;
		
		perf_table_update(&p->timers, opcounts, p->name);
		
		double flops  = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		EmitLogfs(MyRank, 9999, "CUDAMEM plan performance:", flops, "MFLOPS", PRINT_SOME);
		EmitLog  (MyRank, 9999, "CUDAMEM execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info CUDAMEM_info = {
	"CUDAMEM",
	NULL,
	0,
	makeCUDAMEMPlan,
	parseCUDAMEMPlan,
	execCUDAMEMPlan,
	initCUDAMEMPlan,
	killCUDAMEMPlan,
	perfCUDAMEMPlan,
	{ "FLOPS", NULL, NULL }
};

/* PLAN 1 CUDAMEM GPUDEV#  LOOPCOUNT BLOCKFACTOR */
/* GPUDEV#      indicates to which device the load should be bound.                          Default: 0 */
/* LOOPCOUNT    is the number of DGEMM iterations called for a single execution of the load. Default: 8 */
/* BLOCKFACTOR  array sizes will be a multiple of this. If zero or unspecified it will default to  warps*cores   */
/* currently we run only with square matrices sized based on GPU memory  */
/* this can change if there's a good reason                              */

/**
 * \brief Allocates and returns the data struct for the plan
 * \param [in] m The input data for the plan.
 * \return void* Data struct 
 * \sa parseCUDAMEMPlan
 * \sa initCUDAMEMPlan
 * \sa execCUDAMEMPlan
 * \sa perfCUDAMEMPlan
 * \sa killCUDAMEMPlan
*/
void * makeCUDAMEMPlan(data *m) {
	Plan *p;
	CUDAMEMdata *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initCUDAMEMPlan;
		p->fptr_execplan = &execCUDAMEMPlan;
		p->fptr_killplan = &killCUDAMEMPlan;
		p->fptr_perfplan = &perfCUDAMEMPlan;
		p->name = CUDAMEM;
		d = (CUDAMEMdata *)malloc(sizeof(CUDAMEMdata));
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
