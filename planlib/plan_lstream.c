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

/**
 * \brief Checks the values calulated in exec to see if there are any errors.
 * \param vptr The data used for the error checking.
 * \returns An integer error code.
 * \sa execLStreamPlan
 */
int LStreamCheck(void *vptr) {
	LStreamdata *d;
	d = (LStreamdata*)vptr;
	long int cutoff,asum,bsum,csum;
	register int j;
	long int a=1, b=2, c=0;
	int ret = ERR_CLEAN;
	c=a;
	b=d->random * c;
	c=a+b;
	a=b+d->random * c;
	a*=d->M; b*=d->M; c*=d->M;

	asum=bsum=csum=0.0;
	for(j=0; j<d->M;j++) {
		asum+=(long int) d->one[j];
		bsum+=(long int) d->two[j];
		csum+=(long int) d->three[j];
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
 * \brief Allocates and returns the data struct for the plan
 * \param m Holds the input data for the plan.
 * \return void* Data struct 
 * \sa parseLStreamPlan
 * \sa initLStreamPlan
 * \sa execLStreamPlan
 * \sa perfLStreamPlan
 * \sa killLStreamPlan
*/
void * makeLStreamPlan(data *m) {
	Plan *p;
	LStreamdata *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initLStreamPlan;
		p->fptr_execplan = &execLStreamPlan;
		p->fptr_killplan = &killLStreamPlan;
		p->fptr_perfplan = &perfLStreamPlan;
		p->name = LSTREAM;
		d = (LStreamdata *)malloc(sizeof(LStreamdata));
		assert(d);
		if(d) {
			if(m->isize==1) d->M = m->i[0]/(5*sizeof(long int));
			else d->M = m->d[0]/(5*sizeof(long int));
		}
		(p->vptr)=(void*)d;
	}
	return p;
}

/**
 * \brief Creates and initializes the working data for the plan
 * \param plan Holds the data and memory for the plan.
 * \return int Error flag value
 * \sa parseLStreamPlan 
 * \sa makeLStreamPlan
 * \sa execLStreamPlan
 * \sa perfLStreamPlan
 * \sa killLStreamPlan
*/
int    initLStreamPlan(void *plan) {
	size_t M;
	int ret = make_error(ALLOC,generic_err);
	Plan *p;
	LStreamdata *d = NULL;
	p = (Plan *)plan;
	if (p) {
		d = (LStreamdata*)p->vptr;
		p->exec_count = 0;
		perftimer_init(&p->timers, NUM_TIMERS);
	}
	if (d) {
		M = d->M;
		d->one   = (long int*)  malloc(sizeof(long int)*M);
		assert(d->one);
		d->two   = (long int*)  malloc(sizeof(long int)*M);
		assert(d->two);
		d->three = (long int*)  malloc(sizeof(long int)*M);
		assert(d->three);
		d->four  = (long int*)  malloc(sizeof(long int)*M);
		assert(d->four);
		d->five  = (long int*)  malloc(sizeof(long int)*M);
		assert(d->five);
		if(d->one && d->two && d->three && d->four && d->five) ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Frees the memory used in the plan
 * \param plan Points to the memory to be free'd.
 * \sa parseLStreamPlan
 * \sa makeLStreamPlan
 * \sa initLStreamPlan
 * \sa execLStreamPlan
 * \sa perfLStreamPlan
 */
void * killLStreamPlan(void *plan) {
	Plan *p;
	LStreamdata *d;
	p = (Plan *)plan;
	d = (LStreamdata*)p->vptr;

	if(d->one)   free(d->one);
	if(d->two)   free(d->two);
	if(d->three) free(d->three);
	if(d->four)  free(d->four);
	if(d->five)  free(d->five);
	free(d);
	free(p);
	return (void*)NULL;
}

/**
 * \brief Streaming integer vector operations run to consume "size" bytes of memory. 
 * \param plan Holds the data and memory for the plan.
 * \return int Error flag value
 * \sa parseLStreamPlan
 * \sa makeLStreamPlan
 * \sa initLStreamPlan
 * \sa perfLStreamPlan
 * \sa killLStreamPlan
 */
int execLStreamPlan(void *plan) {
	int ret = ERR_CLEAN;
	register size_t i;
	ORB_t t1, t2;
	Plan *p;
	LStreamdata* d;
	p = (Plan *)plan;
	d = (LStreamdata*)p->vptr;
	
	/* update execution count */
	p->exec_count++;

        for(i=0;i<d->M;i++) {
                d->one[i]   =1;
                d->two[i]   =2;
                d->three[i] =0;
                d->four[i]  =1;
                d->five[i]  =0;
                d->random   =rand();
        }

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

	//Shifting Scale
	for(i=0; i<d->M ;i++)
		d->five[i] = d->four[i] << d->random;
	ORB_read(t2);
	perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));
	perftimer_accumulate(&p->timers, TIMER1, ORB_cycles_a(t2, t1));
	
	if (CHECK_CALC) {
		ORB_read(t1);
		ret = LStreamCheck(d);
		ORB_read(t2);
		perftimer_accumulate(&p->timers, TIMER2, ORB_cycles_a(t2, t1));
	}
	return ret;
}

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parseLStreamPlan
 * \sa makeLStreamPlan
 * \sa initLStreamPlan
 * \sa execLStreamPlan
 * \sa killLStreamPlan
 */
int perfLStreamPlan (void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	LStreamdata *d;
	p = (Plan *)plan;
	d = (LStreamdata *)p->vptr;
	if (p->exec_count > 0) {
		// Reference: http://www.cs.virginia.edu/stream/ref.html "Counting Bytes and FLOPS"
		opcounts[TIMER0] = (0 + 1 + 1 + 2) * d->M * p->exec_count;                    // Count # of integer operations
		opcounts[TIMER1] = (2 + 2 + 3 + 3) * sizeof(long int) * d->M * p->exec_count; // Count # of bytes transferred to and from memory
		opcounts[TIMER2] = (3 * d->M + 7) * p->exec_count;                            // Count integer operations in checking stage (needs work)
		
		perf_table_update(&p->timers, opcounts, p->name);
		
		double ips  = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		double mbps = ((double)opcounts[TIMER1]/perftimer_gettime(&p->timers, TIMER1))/1e6;
		EmitLogfs(MyRank, 9999, "LSTREAM plan performance:", ips, "MI64OPS", PRINT_SOME);
		EmitLogfs(MyRank, 9999, "LSTREAM plan performance:", mbps, "MB/s", PRINT_SOME);
		EmitLog  (MyRank, 9999, "LSTREAM execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line Input line for the plan.
 * \param [out] output Holds the data for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makeLStreamPlan
 * \sa initLStreamPlan
 * \sa execLStreamPlan
 * \sa perfLStreamPlan
 * \sa killLStreamPlan
*/
int parseLStreamPlan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);	
	output->name = LSTREAM;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info LSTREAM_info = {
	"LSTREAM",
	NULL,
	0,
	makeLStreamPlan,
	parseLStreamPlan,
	execLStreamPlan,
	initLStreamPlan,
	killLStreamPlan,
	perfLStreamPlan,
	{ "I64OPS", "B/s", NULL }
};

