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
#define NUM_PAPI_EVENTS 3 
#define PAPI_COUNTERS { PAPI_FP_OPS, PAPI_TOT_CYC, PAPI_FP_INS } 
#define PAPI_UNITS { "FLOPS", "CYCS", "FLIPS" } 
#endif //HAVE_PAPI


/**
 * \brief Checks the results of execDStreamPlan for errors.
 * \param [in] vptr Holds the data used to calculate the values used for error checking.
 * \return An integer error flag value.
 * \sa execDStreamPlan
 */
int StreamCheck(void *vptr) {
	DStreamdata *d;
	d = (DStreamdata*)vptr;
	double cutoff,asum,bsum,csum;
	double a=1, b=2, c=0;
	register int j;
	int ret=ERR_CLEAN;
	c=a;
	b=c * d->random;
	c=a+b;
	a=b+d->random * c;
	a*=(double) d->M; b*=(double) d->M; c*=(double) d->M;
	
	asum=bsum=csum=0.0;
	for(j=0;j<d->M;j++) {
		asum+= d->one[j];
		bsum+= d->two[j];
		csum+= d->three[j];
	}

	#ifndef abs
	#define abs(a) ((a)>=0 ? (a) : -(a))
	#endif

	cutoff = 1.e-8;
	if(abs(a-asum)/asum > cutoff) ret = make_error(CALC,generic_err);
	else if(abs(b-bsum)/bsum > cutoff) ret = make_error(CALC,generic_err);
	else if(abs(c-csum)/csum > cutoff) ret = make_error(CALC,generic_err);
	return ret;
}

/**
 * \brief Allocates and returns the data struct for the plan.
 * \param [in] m Holds the input data for the plan.
 * \return void* Data struct
 * \sa parseDStreamPlan
 * \sa initDStreamPlan
 * \sa execDStreamPlan
 * \sa perfDStreamPlan
 * \sa killDStreamPlan
*/
void * makeDStreamPlan(data *m) {
	Plan *p;
	DStreamdata *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initDStreamPlan;
		p->fptr_execplan = &execDStreamPlan;
		p->fptr_killplan = &killDStreamPlan;
		p->fptr_perfplan = &perfDStreamPlan;
		p->name = DSTREAM;
		d = (DStreamdata *)malloc(sizeof(DStreamdata));
		assert(d);
		if(d) {
			if(m->isize==1) d->M = m->i[0]/(5*sizeof(double));
			else d->M = m->d[0]/(5*sizeof(double));
		}
		(p->vptr)=(void*)d;
	}
	return p;
}

/**
 * \brief Creates and initializes the working data for the plan.
 * \param [in] plan Holds the data and storage locations for the plan.
 * \return int Error flag value
 * \sa parseDStreamPlan 
 * \sa makeDStreamPlan
 * \sa execDStreamPlan
 * \sa perfDStreamPlan
 * \sa killDStreamPlan
*/
int    initDStreamPlan(void *plan) {
	size_t M;
	int ret = make_error(ALLOC,generic_err);
	Plan *p;
	DStreamdata *d = NULL;
	p = (Plan *)plan;

        int retval, i;
#ifdef HAVE_PAPI
        int PAPI_Events [NUM_PAPI_EVENTS] = PAPI_COUNTERS;
        char* PAPI_units [NUM_PAPI_EVENTS] = PAPI_UNITS;
#endif //HAVE_PAPI

	if (p) {
		d = (DStreamdata*)p->vptr;
		p->exec_count = 0;
		perftimer_init(&p->timers, NUM_TIMERS);

                /* Initialize plan's PAPI data */
#ifdef HAVE_PAPI
                p->PAPI_EventSet = PAPI_NULL;
                retval = PAPI_create_eventset(&p->PAPI_EventSet);
                if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
                
                //retval = PAPI_add_events(p->PAPI_EventSet, PAPI_Events, NUM_PAPI_EVENTS);
                //if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
                
                //TODO: use loop version as below
                for(i=0; i<TOTAL_PAPI_EVENTS && i<NUM_PAPI_EVENTS; i++){
                    retval = PAPI_add_event(p->PAPI_EventSet, PAPI_Events[i]);
                    if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
                }

                PAPIRes_init(p->PAPI_Results, p->PAPI_Times);
                PAPI_set_units(p->name, PAPI_units, NUM_PAPI_EVENTS);
        
                retval = PAPI_start(p->PAPI_EventSet);
                if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
                //creat initializer for results?
#endif //HAVE_PAPI
	}
	if(d) {
		M = d->M;

		//EmitLog(MyRank,101,"Allocating",sizeof(double)*M*3,0);

		d->one   = (double*)  malloc(sizeof(double)*M);
		assert(d->one);
		d->two   = (double*)  malloc(sizeof(double)*M);
		assert(d->two);
		d->three = (double*)  malloc(sizeof(double)*M);
		assert(d->three);
		d->four  = (double*)  malloc(sizeof(double)*M);
		assert(d->four);
		d->five  = (double*)  malloc(sizeof(double)*M);
		assert(d->five);
		if(d->one && d->two && d->three && d->four && d->five) ret = ERR_CLEAN;
	}
        return ret;
}

/**
 * \brief Frees the memory used in the plan.
 * \param [in] plan Points to the to-be-free'd data for the plan.
 * \sa parseDStreamPlan
 * \sa makeDStreamPlan
 * \sa initDStreamPlan
 * \sa execDStreamPlan
 * \sa perfDStreamPlan
 */
void * killDStreamPlan(void *plan) {
	int retval;
        
        Plan *p;
	DStreamdata *d;
	p = (Plan *)plan;
	assert(p);
	d = (DStreamdata*)p->vptr;
	assert(d);

	//EmitLog(MyRank,101,"Freeing   ",sizeof(double)*d->M*3,0);

	if(d->one)   free(d->one);
	if(d->two)   free(d->two);
	if(d->three) free(d->three);
	if(d->four)  free(d->four);
	if(d->five)  free(d->five);

#ifdef HAVE_PAPI
        retval = PAPI_stop(p->PAPI_EventSet, NULL);  //don't know if this will work
        if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
#endif //HAVE_PAPI
	free(d);
	free(p);
	return (void*)NULL;
}

/**
 * \brief Streaming double precision floating point vector operations run to consume "size" bytes of memory.
 * \param [in] plan Holds the data and storage locations for the plan.
 * \return int Error flag value
 * \sa parseDStreamPlan
 * \sa makeDStreamPlan
 * \sa initDStreamPlan
 * \sa perfDStreamPlan
 * \sa killDStreamPlan
 */
int execDStreamPlan(void *plan) {
        /* PAPI vars */
        int retval, k;
        long long start, end;
        //char message[512];

	register int i;
	int ret = ERR_CLEAN;
	ORB_t t1,t2;
	Plan *p;
	DStreamdata* d;
	p = (Plan *)plan;
	d = (DStreamdata*)p->vptr;
	assert(d);
	
	/* update execution count */
	p->exec_count++;

        for(i=0;i<d->M;i++) {
                d->one[i]   =1.0;
                d->two[i]   =2.0;
                d->three[i] =0.0;
                d->four[i]  =1.0;
                d->five[i]  =0.0;
                d->random   =rand();
        }
        
#ifdef HAVE_PAPI
        /* Start PAPI counters and time */
        retval = PAPI_reset(p->PAPI_EventSet);
        if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
        start = PAPI_get_real_usec();
#endif //HAVE_PAPI

        ORB_read(t1);
	//Copy
	for(i=0; i<d->M ;i++)
		d->three[i] = d->one[i]; 

	//Scale
	for(i=0; i<d->M ;i++)
		d->two[i] = d->three[i] * d->random;

	//Add
	for(i=0; i<d->M ;i++)
		d->three[i] = d->one[i] + d->two[i];

	//Triad
	for(i=0; i<d->M ;i++)
		d->one[i] = d->two[i] + d->random * d->three[i];

	ORB_read(t2);
#ifdef HAVE_PAPI
        end = PAPI_get_real_usec(); //PAPI time

        /* Collect PAPI counters and store time elapsed */
        retval = PAPI_accum(p->PAPI_EventSet, p->PAPI_Results);
        if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
        for(k=0; k<NUM_PAPI_EVENTS && k<TOTAL_PAPI_EVENTS; k++){
            p->PAPI_Times[k] += (end - start);
            //snprintf(message, 512, "PAPI TEST: val = %llu\t time = %llu\n", p->PAPI_Results[k], p->PAPI_Times[k]);
            //EmitLog(MyRank, 9999, message, -1, PRINT_ALWAYS);
        }
#endif //HAVE_PAPI

	perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));
	perftimer_accumulate(&p->timers, TIMER1, ORB_cycles_a(t2, t1));
	
        //TODO: put PAPI in here as well
	if (CHECK_CALC) {
		ORB_read(t1);
		ret = StreamCheck(d);
		ORB_read(t2);
		perftimer_accumulate(&p->timers, TIMER2, ORB_cycles_a(t2, t1));
	}
	return ret;
}

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parseDStreamPlan
 * \sa makeDStreamPlan
 * \sa initDStreamPlan
 * \sa execDStreamPlan
 * \sa killDStreamPlan
 */
int perfDStreamPlan (void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	DStreamdata *d;
	p = (Plan *)plan;
	d = (DStreamdata *)p->vptr;
	if (p->exec_count > 0) {
		// Reference: http://www.cs.virginia.edu/stream/ref.html "Counting Bytes and FLOPS"
		opcounts[TIMER0] = (0 + 1 + 1 + 2) * d->M * p->exec_count;                  // Count # of floating point operations for each execution loop.
		opcounts[TIMER1] = (2 + 2 + 3 + 3) * sizeof(double) * d->M * p->exec_count; // Count # of bytes transferred to and from memory
		opcounts[TIMER2] = (3 * d->M + 7) * p->exec_count;                          // FLOPs count for checking stage (needs work...)
		
		perf_table_update(&p->timers, opcounts, p->name);
#ifdef HAVE_PAPI
		PAPI_table_update(p->name, p->PAPI_Results, p->PAPI_Times, NUM_PAPI_EVENTS);
#endif //HAVE_PAPI

	        //TODO: Insert some PAPI info here as well	
		
		double flops = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		double mbps  = ((double)opcounts[TIMER1]/perftimer_gettime(&p->timers, TIMER1))/1e6;
		EmitLogfs(MyRank, 9999, "DSTREAM plan performance:", flops, "MFLOPS", PRINT_SOME);
		EmitLogfs(MyRank, 9999, "DSTREAM plan performance:", mbps, "MB/s", PRINT_SOME);
		EmitLog  (MyRank, 9999, "DSTREAM execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan.
 * \param [in] line The line of input for the plan.
 * \param [out] output Holds the data and memory locations for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makeDStreamPlan 
 * \sa initDStreamPlan
 * \sa execDStreamPlan
 * \sa perfDStreamPlan
 * \sa killDStreamPlan
*/
int parseDStreamPlan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);
	output->name = DSTREAM;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info DSTREAM_info = {
	"DSTREAM",
	NULL,
	0,
	makeDStreamPlan,
	parseDStreamPlan,
	execDStreamPlan,
	initDStreamPlan,
	killDStreamPlan,
	perfDStreamPlan,
	{ "FLOPS", "B/s", NULL }/*,
	{ "FLOPS", "CYC" },
        NUM_PAPI_EVENTS*/
};

