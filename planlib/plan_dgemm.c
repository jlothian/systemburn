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

/* currently we force loading with square matrices only */
/* this can change if there's a good reason             */
/* void * makeDGEMMPlan(int m, int k, int n) {          */
/**
 * \brief Allocates and returns the data struct for the plan
 * \param [in] m Holds the input data for the plan.
 * \return void* Data struct 
 * \sa parseDGEMMPlan
 * \sa initDGEMMPlan
 * \sa execDGEMMPlan
 * \sa perfDGEMMPlan
 * \sa killDGEMMPlan
*/
void * makeDGEMMPlan(data *m) {
	Plan *p;
	DGEMMdata *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initDGEMMPlan;
		p->fptr_execplan = &execDGEMMPlan;
		p->fptr_killplan = &killDGEMMPlan;
		p->fptr_perfplan = &perfDGEMMPlan;
		p->name = DGEMM;
		d = (DGEMMdata *)malloc(sizeof(DGEMMdata));
		assert(d);
		if(d) { 
			if(m->isize==1) d->M = sqrt(m->i[0]/(3*sizeof(double)));
			else d->M = sqrt(m->d[0]/(3*sizeof(double)));
		}
		(p->vptr)=(void*)d;
	}
	/* delay allocating any further structures... NUMA */
	return p;
}

/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line The line of input for the plan.
 * \param [out] output Holds the data and memory locations for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makeDGEMMPlan
 * \sa initDGEMMPlan
 * \sa execDGEMMPlan
 * \sa perfDGEMMPlan
 * \sa killDGEMMPlan 
*/
int parseDGEMMPlan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);
        output->name = DGEMM;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief Creates and initializes the working data for the plan.
 * \param [in] plan Holds the data and structures for the plan.
 * \return int Error flag value
 * \sa parseDGEMMPlan 
 * \sa makeDGEMMPlan
 * \sa execDGEMMPlan
 * \sa perfDGEMMPlan
 * \sa killDGEMMPlan
*/
int   initDGEMMPlan(void *plan) {
	int i;
	size_t M;
	int ret = make_error(ALLOC,generic_err);
	Plan *p;
	DGEMMdata *d = NULL;
	p = (Plan *)plan;

#ifdef HAVE_PAPI
        int temp_event, k;
        int PAPI_Events [NUM_PAPI_EVENTS] = PAPI_COUNTERS;
        char* PAPI_units [NUM_PAPI_EVENTS] = PAPI_UNITS;
#endif //HAVE_PAPI

	if (p) {
		d = (DGEMMdata*)p->vptr;
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
                        for(k=0; k<TOTAL_PAPI_EVENTS && k<NUM_PAPI_EVENTS; k++){
                            temp_event = PAPI_Events[k];
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
		M = d->M;

//		EmitLog(MyRank,11,"Allocating 1000*",((sizeof(double)*M)*M*3)/1000,0);

		d->A = (double *)malloc((sizeof(double)*M)*M);
		assert(d->A);
		d->B = (double *)malloc((sizeof(double)*M)*M);
		assert(d->B);
		d->C = (double *)malloc((sizeof(double)*M)*M);
		assert(d->C);
		
		if(d->A && d->B && d->C) {
			for(i=0; i<M*M; i++) {
				d->A[i] = 2.0;
				d->B[i] = 4.5;
			}
			ret = ERR_CLEAN;
		}
	}
	return ret;
}

/**
 * \brief Frees the memory used in the plan.
 * \param [in] plan Holds the location of the plan to be free'd.
 * \sa parseDGEMMPlan
 * \sa makeDGEMMPlan
 * \sa initDGEMMPlan
 * \sa execDGEMMPlan
 * \sa perfDGEMMPlan
 */
void * killDGEMMPlan(void *plan) {
	Plan *p;
	DGEMMdata *d;
	p = (Plan *)plan;
	d = (DGEMMdata*)p->vptr;
        int retval;

//	EmitLog(MyRank,11,"Freeing   ",sizeof(double)*d->M*d->M*3,0);

        if(DO_PERF){
#ifdef HAVE_PAPI
        TEST_PAPI(PAPI_stop(p->PAPI_EventSet, NULL), PAPI_OK, MyRank, 9999, PRINT_SOME);
#endif //HAVE_PAPI
        } //DO_PERF

	if(d->C) free((void*)(d->C));
	if(d->B) free((void*)(d->B));
	if(d->A) free((void*)(d->A));

	free((void*)(d));
	free((void*)(p));
	return (void*)NULL;
}

/**
 * \brief A double precision matrix multiplication benchmark which will run to consume "size" bytes of memory.
 * \param [in] plan Holds the data for the plan's execution.
 * \return int Error flag value
 * \sa parseDGEMMPlan
 * \sa makeDGEMMPlan
 * \sa initDGEMMPlan
 * \sa perfDGEMMPlan
 * \sa killDGEMMPlan
 */
int execDGEMMPlan(void *plan) {
#ifdef HAVE_PAPI
        int k;
        long long start, end;
#endif //HAVE_PAPI

	int M, K, N, lda, ldb, ldc;
	double *A, *B, *C;
	double alpha, beta;
	ORB_t t1, t2;
	Plan *p;
	DGEMMdata *d;
	p = (Plan *)plan;
	d = (DGEMMdata*)p->vptr;
	/* update execution counter */
	p->exec_count++;
	alpha = 1.0;
	beta = 0.0;
	A = d->A;
	B = d->B;
	C = d->C;
	M = K = N = lda = ldb = ldc = d->M;

        if (DO_PERF){
#ifdef HAVE_PAPI
                /* Start PAPI counters and time */
                TEST_PAPI(PAPI_reset(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
                start = PAPI_get_real_usec();
#endif //HAVE_PAPI

                ORB_read(t1);
        } //DO_PERF

	cblas_dgemm( CblasRowMajor, CblasNoTrans, CblasNoTrans, M, N, K, alpha, A, lda, B, ldb, beta, C, ldc); 

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
	return ERR_CLEAN;
}

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parseDGEMMPlan
 * \sa makeDGEMMPlan
 * \sa initDGEMMPlan
 * \sa execDGEMMPlan
 * \sa killDGEMMPlan
 */
int perfDGEMMPlan (void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	DGEMMdata *d;
	p = (Plan *)plan;
	d = (DGEMMdata *)p->vptr;
	if (p->exec_count > 0) {
		uint64_t M2 = (uint64_t)d->M * (uint64_t)d->M;
		opcounts[TIMER0] = (M2 * (2ULL * (uint64_t)d->M + 1ULL)) * p->exec_count; // Count # of floating point operations
		opcounts[TIMER1] = 0;
		opcounts[TIMER2] = 0;
		
		perf_table_update(&p->timers, opcounts, p->name);
#ifdef HAVE_PAPI
		PAPI_table_update(p->name, p->PAPI_Results, p->PAPI_Times, p->PAPI_Num_Events);
#endif //HAVE_PAPI
		
		double flops  = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		EmitLogfs(MyRank, 9999, "DGEMM plan performance:", flops, "MFLOPS", PRINT_SOME);
		EmitLog  (MyRank, 9999, "DGEMM execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info DGEMM_info = {
	"DGEMM",
	NULL,
	0,
	makeDGEMMPlan,
	parseDGEMMPlan,
	execDGEMMPlan,
	initDGEMMPlan,
	killDGEMMPlan,
	perfDGEMMPlan,
	{ "FLOPS", NULL, NULL }
};

/* if this were a standalone benchmark, the implementation of cblas_dgemm() would appear below... */
