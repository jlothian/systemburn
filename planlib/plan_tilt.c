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
#include <inttypes.h>

#ifdef HAVE_PAPI
#define NUM_PAPI_EVENTS 1 
#define PAPI_COUNTERS { PAPI_FP_OPS } 
#define PAPI_UNITS { "FLOPS" } 
#endif //HAVE_PAPI

#define M0 0x5555555555555555uL
#define M1 0x3333333333333333uL
#define M2 0x0f0f0f0f0f0f0f0fuL
#define M3 0x00ff00ff00ff00ffuL
#define M4 0x0000ffff0000ffffuL
#define M5 0x00000000ffffffffuL

#define TILTM(A,B,S,M) {\
	uint64_t z = ((A) ^ ((B) >> (S))) & (M);\
	(A) ^= z;  (B) ^= (z << (S));\
}

/**
 * \brief The tilt function that does most of the computation for Tilt
 * \param [in] p Array of values to run through Tilt
 */
void tilt (uint64_t p[]) {
	static uint64_t tmp[64];
	uint64_t *q;

	for (q = &tmp[0]; q < &tmp[64]; p++, q+=8) {
		uint64_t a, b, c, d, e, f, g, h;

		a = p[ 0];        b = p[ 8];        c = p[16];        d = p[24];
		TILTM(a,b,8,M3);  TILTM(c,d,8,M3);  TILTM(a,c,16,M4); TILTM(b,d,16,M4);

		e = p[32];        f = p[40];        g = p[48];        h = p[56];
		TILTM(e,f,8,M3);  TILTM(g,h,8,M3);  TILTM(e,g,16,M4); TILTM(f,h,16,M4);

		TILTM(a,e,32,M5); TILTM(b,f,32,M5); TILTM(c,g,32,M5); TILTM(d,h,32,M5);
		q[ 0] = a;        q[ 1] = b;        q[ 2] = c;        q[ 3] = d;
		q[ 4] = e;        q[ 5] = f;        q[ 6] = g;        q[ 7] = h;
	}
	/* reset pointer */
	p-=8;

	for (q = &tmp[0]; q < &tmp[8]; p+=8, q++) {
		uint64_t a, b, c, d, e, f, g, h;

		a = q[ 0];        b = q[ 8];        c = q[16];        d = q[24];
		TILTM(a,b,1,M0);  TILTM(c,d,1,M0);  TILTM(a,c,2,M1);  TILTM(b,d,2,M1);

		e = q[32];        f = q[40];        g = q[48];        h = q[56];
		TILTM(e,f,1,M0);  TILTM(g,h,1,M0);  TILTM(e,g,2,M1);  TILTM(f,h,2,M1);

		TILTM(a,e,4,M2);  TILTM(b,f,4,M2);  TILTM(c,g,4,M2);  TILTM(d,h,4,M2);
		p[ 0] = a;        p[ 1] = b;        p[ 2] = c;        p[ 3] = d;
		p[ 4] = e;        p[ 5] = f;        p[ 6] = g;        p[ 7] = h;
	}
}

/**
 * \brief Allocates and returns the data struct for the plan
 * \param [in] i Holds the input data for the plan.
 * \return void* Data struct 
 * \sa parseTiltPlan
 * \sa initTiltPlan
 * \sa execTiltPlan
 * \sa perfTiltPlan
 * \sa killTiltPlan
*/
void * makeTiltPlan(data *i) { 
	Plan *p; 
	TILT_data *ip; 
	p=(Plan*)malloc(sizeof(Plan)); 
	assert(p);
	if(p) { // <- Checking the Plan pointer was allocated. Do not change.
		p->fptr_initplan = &initTiltPlan; 
		p->fptr_execplan = &execTiltPlan; 
		p->fptr_killplan = &killTiltPlan;
		p->fptr_perfplan = &perfTiltPlan;
		p->name = TILT;
		ip=(TILT_data*)malloc(sizeof(TILT_data)); 
		assert(ip);
		if(ip) {
			ip->niter = i->i[0]; 
			ip->seed = i->i[1];
		}
		(p->vptr)=(void*)ip; 
	}
	return p; // <- Returning the Plan pointer. Do not change.
}

/************************
 * This is the place where the memory gets allocated, and data types get initialized to their starting values.
 ***********************/
/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan Holds the data and memory for the plan.
 * \return int Error flag value
 * \sa parseTiltPlan 
 * \sa makeTiltPlan
 * \sa execTiltPlan
 * \sa perfTiltPlan
 * \sa killTiltPlan
*/
int  initTiltPlan(void *plan) { 
	int ret = make_error(ALLOC,generic_err);
	int i;
	brand_t br;
	Plan *p;
	TILT_data *ti = NULL;
	p = (Plan *)plan;

    #ifdef HAVE_PAPI
        int temp_event, i;
        int PAPI_Events [NUM_PAPI_EVENTS] = PAPI_COUNTERS;
        char* PAPI_units [NUM_PAPI_EVENTS] = PAPI_UNITS;
    #endif //HAVE_PAPI

	if (p) {
		ti = (TILT_data *)p->vptr;
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
	if(ti) {
		ti->arr=(uint64_t *)malloc(64*sizeof(uint64_t));
		assert(ti->arr);
		brand_init(&br, ti->seed);
		for (i = 0; i < 64; i++) 
			ti->arr[i] = brand(&br);
		ret = ERR_CLEAN;
	}
	return ret; 
}

/************************
 * This is where the plan gets executed. Place all operations here.
 ***********************/
/**
 * \brief A bit-twiddling load with a small memory footprint, "niter" iterations at a time.
 * \param [in] plan Holds the data for the plan.
 * \return int Error flag value
 * \sa parseTiltPlan
 * \sa makeTiltPlan
 * \sa initTiltPlan
 * \sa perfTiltPlan
 * \sa killTiltPlan
 */
int execTiltPlan(void *plan) {
    #ifdef HAVE_PAPI
        int k;
        long long start, end;
    #endif //HAVE_PAPI

	int i;
	ORB_t t1, t2;
	Plan *p;
	TILT_data *ti;
	p = (Plan *)plan;
	ti = (TILT_data *)p->vptr;
	/* update execution count */
	p->exec_count++;
	
    #ifdef HAVE_PAPI
        /* Start PAPI counters and time */
        TEST_PAPI(PAPI_reset(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
        start = PAPI_get_real_usec();
    #endif //HAVE_PAPI

	ORB_read(t1);
	for (i = 0; i < ti->niter; i++)
		tilt(ti->arr);
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
	return ERR_CLEAN; 
}

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parseTiltPlan
 * \sa makeTiltPlan
 * \sa initTiltPlan
 * \sa execTiltPlan
 * \sa killTiltPlan
 */
int perfTiltPlan (void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	TILT_data *d;
	p = (Plan *)plan;
	d = (TILT_data *)p->vptr;
	if (p->exec_count > 0) {
		opcounts[TIMER0] = (uint64_t)d->niter * p->exec_count; // Count # of calls to tilt()
		opcounts[TIMER1] = 0;
		opcounts[TIMER2] = 0;
		
		perf_table_update(&p->timers, opcounts, p->name);
            #ifdef HAVE_PAPI
		PAPI_table_update(p->name, p->PAPI_Results, p->PAPI_Times, p->PAPI_Num_Events);
            #endif //HAVE_PAPI
		
		double ips  = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		EmitLogfs(MyRank, 9999, "TILT plan performance:", ips, "Million Trips/s", PRINT_SOME);
		EmitLog  (MyRank, 9999, "TILT execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/************************
 * This is where everything gets cleaned up. Be sure to free() your data types (free the members first) in addition to the ones included below.
 ***********************/
/**
 * \brief Frees the memory used in the plan
 * \param [in] plan Points to the memory to be free'd
 * \sa parseTiltPlan
 * \sa makeTiltPlan
 * \sa initTiltPlan
 * \sa execTiltPlan
 * \sa perfTiltPlan
 */
void * killTiltPlan(void *plan) { 
	Plan *p; // <- Plan type. Do not change.
	TILT_data *ti;
	p = (Plan *)plan; 

    #ifdef HAVE_PAPI
        TEST_PAPI(PAPI_stop(p->PAPI_EventSet, NULL), PAPI_OK, MyRank, 9999, PRINT_SOME);
    #endif //HAVE_PAPI

	ti = (TILT_data *)p->vptr;
	free((void*)(ti->arr));
	free((void*)(p->vptr)); 
	free((void*)(plan)); 
	return (void*)NULL; 
}

/************************
 * This is the parsing function that the program uses to read in your load information from the load file. This lets you have a custom number of int arguments for your module while still maintaining modularity.
 ***********************/
/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line Input line for the plan.
 * \param [out] output Holds the data for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makeTiltPlan 
 * \sa initTiltPlan
 * \sa execTiltPlan
 * \sa perfTiltPlan
 * \sa killTiltPlan
*/
int parseTiltPlan(char *line, LoadPlan *output) { 
	output->name = TILT;
	output->input_data = get_sizes(line);
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info TILT_info = {
	"TILT",
	NULL,
	0,
	makeTiltPlan,
	parseTiltPlan,
	execTiltPlan,
	initTiltPlan,
	killTiltPlan,
	perfTiltPlan,
	{ "Trips/s", NULL, NULL }
};
