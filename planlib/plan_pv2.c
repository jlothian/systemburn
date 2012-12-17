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
                 
#ifdef HAVE_PAPI
#define NUM_PAPI_EVENTS 1 
#define PAPI_COUNTERS { PAPI_FP_OPS } 
#define PAPI_UNITS { "FLOPS" } 
#endif //HAVE_PAPI

/**
 * \brief Allocates and returns the data struct for the plan
 * \param [in] m Holds the input data for the plan.
 * \return void* Data struct 
 * \sa parsePV2Plan
 * \sa initPV2Plan
 * \sa execPV2Plan
 * \sa perfPV2Plan
 * \sa killPV2Plan
*/
void * makePV2Plan(data *m) {
	Plan *p;
	PV2data *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initPV2Plan;
		p->fptr_execplan = &execPV2Plan;
		p->fptr_killplan = &killPV2Plan;
		p->fptr_perfplan = &perfPV2Plan;
		p->name = PV2;
		d = (PV2data *)malloc(sizeof(PV2data));
		assert(d);
		if(d) {
			if(m->isize==1) d->M = m->i[0]/sizeof(double);
			else d->M = m->d[0]/sizeof(double);
		}
		(p->vptr)=(void*)d;
	}
	return p;
}

#define MASKA 0x7fff
#define MASKB 0xfff

/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan Holds the data and memory for the plan.
 * \return int Error flag value
 * \sa parsePV2Plan 
 * \sa makePV2Plan
 * \sa execPV2Plan
 * \sa perfPV2Plan
 * \sa killPV2Plan
*/
int    initPV2Plan(void *plan) {
	int i;
	size_t M;
	int ret = make_error(ALLOC,generic_err);
	Plan *p;
	PV2data *d = NULL;
	p = (Plan *)plan;

    #ifdef HAVE_PAPI
        int temp_event, i;
        int PAPI_Events [NUM_PAPI_EVENTS] = PAPI_COUNTERS;
        char* PAPI_units [NUM_PAPI_EVENTS] = PAPI_UNITS;
    #endif //HAVE_PAPI

	if (p) {
		d = (PV2data*)p->vptr;
		p->exec_count = 0;
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
	}
	assert(d);
	if(d) {
		M = d->M;
		d->one   = (double*)  malloc(sizeof(double)*M);
		assert(d->one);
		d->two   = (double*)  malloc(sizeof(double)*(MASKA+2));
		assert(d->two);
		d->three = (double*)  malloc(sizeof(double)*2);
		assert(d->three);
		d->four = (double*)  malloc(sizeof(double)*2);
		assert(d->four);
		//if(d->one && d->two) {
		if(d->one && d->two && d->three && d->four) {
			for(i=0;i<d->M;i++) {
				d->one[i]   =0.0;
			}
			for(i=0;i<MASKA+1;i++) {
				d->two[i]   =0.0;
			}
			d->three[0] =MASKA;
			d->four[0]  =MASKB;
			d->random = rand();
			ret = ERR_CLEAN;
		}
	}
	return ret;
}

/**
 * \brief Frees the memory used in the plan
 * \param [in] plan Points to the memory to be free'd.
 * \sa parsePV2Plan
 * \sa makePV2Plan
 * \sa initPV2Plan
 * \sa execPV2Plan
 * \sa perfPV2Plan
 */
void * killPV2Plan(void *plan) {
	Plan *p;
	PV2data *d;
	p = (Plan *)plan;
	assert(p);
	d = (PV2data*)p->vptr;
	assert(d);

    #ifdef HAVE_PAPI
        TEST_PAPI(PAPI_stop(p->PAPI_EventSet, NULL), PAPI_OK, MyRank, 9999, PRINT_SOME);
    #endif //HAVE_PAPI

	if(d->one)   free(d->one);
	if(d->two)   free(d->two);
	if(d->three) free(d->three);
	if(d->four)  free(d->four);
	free(d);
	free(p);

	return (void*)NULL;
}

/**
 * \brief A power hungry streaming computational algorithm on one array of 64bit values, which will operate with a memory footprint of "size" bytes. This load was tuned to a quadcore Intel "Nehalem" processor, but may be suitable for loading multiple x86-64 cores until the memory system is saturated. It is intended to be run with a footprint large enough to require main memory access. 
 * \param [in] plan Holds the data for the plan.
 * \return int Error flag value
 * \sa parsePV2Plan
 * \sa makePV2Plan
 * \sa initPV2Plan
 * \sa perfPV2Plan
 * \sa killPV2Plan
 */
int execPV2Plan(void *plan) {
    #ifdef HAVE_PAPI
        int k;
        long long start, end;
    #endif //HAVE_PAPI

	register double *A,Ai,S,T,U,V,W,X,Y,Z;
	register long   *B,Bi,ia,ib,i,j,k,l;
	register long    MA,MB,PY,M;
	ORB_t t1, t2;
	Plan *p;
	PV2data* d;
	p = (Plan *)plan;
	d = (PV2data*)p->vptr;
	assert(d);
	/* update execution count */
	p->exec_count++;
	
	A  = (double *) d->one;
	B  = (long *)   d->two;
	MA =            d->three[0];
	MB =            d->four[0];
	PY =            d->random;
	M  =            d->M;
	Ai = S = T = U = V = W = X = Y = Z = A[0]/PY;
	ib = j = k = l = A[0] ;
	
    #ifdef HAVE_PAPI
        /* Start PAPI counters and time */
        TEST_PAPI(PAPI_reset(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
        start = PAPI_get_real_usec();
    #endif //HAVE_PAPI

	ORB_read(t1);
	for(i=0; i<M; i+=1) {
		     ia    = i;
		     Ai    = A[ia];
		     Bi    = B[ib];
		     ib    = (i+1) & MA;
		     S   = ((U*U)+X)   + ((V*V)+U)   + ((W*W)+V)     + ((X*X)+W)   + ((U+U)*V)   + ((V+V)*W)     + ((W+W)*X);
		     T   = ((U+V)*W)   * ((V+W)*X)   * ((W+X)*U)     * ((X+U)*V)   * ((U*V)+W)   * ((V*W)+X)     * ((W*X)+U);
		     j  =  ((j^PY)>>4) | ((k^PY)<<7) | ((l^PY)>>8)   | ((j|PY)<<4) | ((k|PY)<<9) | ((l|PY)<<3)   | ((j&PY)>>5) | ((k&PY)>>8) | ((l&PY)>>2)  | (~j^PY)   | (~k&PY)   | (~l|PY);
		     k  =  ((k^MA)>>8) | ((l^MA)<<3) | ((j^MA)>>2)   | ((k|MA)<<8) | ((l|MA)<<5) | ((j|MA)<<5)   | ((k&MA)>>7) | ((l&MA)>>9) | ((j&MA)>>3)  | (~k^MA)   | (~l&MA)   | (~j|MA);
                     l  =  ((l^MB)>>2) | ((j^MB)<<5) | ((k^MB)>>4)   | ((l|MB)<<2) | ((j|MB)<<1) | ((k|MB)<<7)   | ((l&MB)>>3) | ((j&MB)>>4) | ((k&MB)>>1)  | (~l^MB)   | (~j&MB)   | (~k|MB);
		     A[ia] = Ai * S;
		     U     = S;
                     V     = W;
		     W     = X;
		     X     = Y;
		     Y     = Z;
		     Z     = T;
		     B[ib] = Bi & j;
		     j     = k+1;
	}
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
	perftimer_accumulate(&p->timers, TIMER1, ORB_cycles_a(t2, t1));
	return ERR_CLEAN;
}

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parsePV2Plan
 * \sa makePV2Plan
 * \sa initPV2Plan
 * \sa execPV2Plan
 * \sa killPV2Plan
 */
int perfPV2Plan (void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	PV2data *d;
	p = (Plan *)plan;
	d = (PV2data *)p->vptr;
	if (p->exec_count > 0) {
		opcounts[TIMER0] = d->M * p->exec_count;                                             // Count # of trips through inner loop
		opcounts[TIMER1] = (d->M * (2 * sizeof(double) + 2 * sizeof(long))) * p->exec_count; // Count # of bytes of memory transfered (2 r, 2 w)
		opcounts[TIMER2] = 0;
		
		perf_table_update(&p->timers, opcounts, p->name);
            #ifdef HAVE_PAPI
		PAPI_table_update(p->name, p->PAPI_Results, p->PAPI_Times, p->PAPI_Num_Events);
            #endif //HAVE_PAPI
		
		double trips  = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		double mbps   = ((double)opcounts[TIMER1]/perftimer_gettime(&p->timers, TIMER1))/1e6;
		EmitLogfs(MyRank, 9999, "PV2 plan performance:", trips, "Million Trips/s", PRINT_SOME);
		EmitLogfs(MyRank, 9999, "PV2 plan performance:", mbps,  "MB/s",   PRINT_SOME);
		EmitLog  (MyRank, 9999, "PV2 execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line Input line for the plan.
 * \param [out] output Holds the data and info for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makePV2Plan 
 * \sa initPV2Plan
 * \sa execPV2Plan
 * \sa perfPV2Plan
 * \sa killPV2Plan
*/
int parsePV2Plan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);	
	output->name = PV2;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info PV2_info = {
	"PV2",
	NULL,
	0,
	makePV2Plan,
	parsePV2Plan,
	execPV2Plan,
	initPV2Plan,
	killPV2Plan,
	perfPV2Plan,
	{ "Trips/s", "B/s", NULL }
};

