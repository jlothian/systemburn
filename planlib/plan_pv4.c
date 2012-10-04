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
 * \brief Allocates and returns the data struct for the plan
 * \param [in] m Holds the input data for the plan.
 * \return void* Data struct 
 * \sa parsePV4Plan
 * \sa initPV4Plan
 * \sa execPV4Plan
 * \sa perfPV4Plan
 * \sa killPV4Plan
*/
void * makePV4Plan(data *m) {
	Plan *p;
	PV4data *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initPV4Plan;
		p->fptr_execplan = &execPV4Plan;
		p->fptr_killplan = &killPV4Plan;
		p->fptr_perfplan = &perfPV4Plan;
		p->name = PV4;
		d = (PV4data *)malloc(sizeof(PV4data));
		assert(d);
		if(d) {
			if(m->isize) d->M = m->i[0]/(4*sizeof(double));
			else d->M = m->d[0]/(4*sizeof(double));
		}
		(p->vptr)=(void*)d;
	}
	return p;
}

#define MASKA 0x7
#define MASKB 0xfff

/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan Holds the data and memory for the plan.
 * \return int Error flag value
 * \sa parsePV4Plan 
 * \sa makePV4Plan
 * \sa execPV4Plan
 * \sa perfPV4Plan
 * \sa killPV4Plan
*/
int    initPV4Plan(void *plan) {
	int i;
	size_t M;
	int ret = make_error(ALLOC,generic_err);
	Plan *p;
	PV4data *d = NULL;
	p = (Plan *)plan;
	if (p) {
		d = (PV4data*)p->vptr;
		p->exec_count = 0;
		perftimer_init(&p->timers, NUM_TIMERS);
	}
	assert(d);
	if(d) {
		M = d->M;

		//EmitLog(MyRank,1,"Allocating",sizeof(double)*4*M,0);

		d->one   = (double*)  malloc(sizeof(double)*M);
		assert(d->one);
		d->two   = (double*)  malloc(sizeof(double)*M);
		assert(d->two);
		d->three = (double*)  malloc(sizeof(double)*M);
		assert(d->three);
		d->four  = (double*)  malloc(sizeof(double)*M);
		assert(d->four);
		//if(d->one && d->two) {
		if(d->one && d->two && d->three && d->four) {
			for(i=0;i<M;i++) {
				d->one[i]   = 0.0;
				d->two[i]   = 0.0;
				d->three[i] = 0.0;
				d->four[i]  = 0.0;
			}
			d->random = rand();
			ret = ERR_CLEAN;
		}
	}
	return ret;
}

/**
 * \brief Frees the memory used in the plan
 * \param [in] plan Points to the memory to be free'd.
 * \sa parsePV4Plan
 * \sa makePV4Plan
 * \sa initPV4Plan
 * \sa execPV4Plan
 * \sa perfPV4Plan
 */
void * killPV4Plan(void *plan) {
	Plan *p;
	PV4data *d;
	p = (Plan *)plan;
	assert(p);
	d = (PV4data*)p->vptr;
	assert(d);

	//EmitLog(MyRank,1,"Freeing   ",sizeof(double)*4*d->M,0);

	if(d->one)   free(d->one);
	if(d->two)   free(d->two);
	if(d->three) free(d->three);
	if(d->four)  free(d->four);
	free(d);
	free(p);

	return (void*)NULL;
}

/**
 * \brief A power hungry streaming computational algorithm on one array of 64bit values. It is intended to run in a smaller memory footprint which will be contained in L2 cache, not inducing main memory traffic. 
 * \param [in] plan Holds the data for the plan.
 * \return int Error flag value
 * \sa parsePV4Plan
 * \sa makePV4Plan
 * \sa initPV4Plan
 * \sa perfPV4Plan
 * \sa killPV4Plan
 */
int execPV4Plan(void *plan) {
	register double *a,*b,*c,*d;
	// register double ta,tb,tc,td;
	register long   i,j,k,l;
	register long   M;
	ORB_t t1, t2;
	Plan *p;
	PV4data* data;
	p = (Plan *)plan;
	data = (PV4data*)p->vptr;
	assert(data);
	/* update execution count */
	p->exec_count++;
	
	a  = (double *) data->one;
	b  = (double *) data->two;
	c  = (double *) data->three;
	d  = (double *) data->four;
	M  =            data->M;
	ORB_read(t1);
	for(j=0; j<10000; j++) {
		for(i=0; i<M; i+=1) {
			for(k=0;k<8;k++)
			a[i] = ( (((a[i]*b[i])+c[i])*((a[i]*c[i])+d[i])) + ((a[i]*d[i])+b[i]) )
			     * ( (((d[i]*c[i])+d[i])*((b[i]*d[i])+a[i])) + ((c[i]+d[i])*a[i]) )
			     + ( (((c[i]*d[i])+a[i])*((b[i]+d[i])*a[i])) + ((b[i]+c[i])*d[i]) );	
			// accum answers into scalar temps for more work per loop
			// ta   = ( (((a[i]*b[i])+c[i])*((a[i]*c[i])+d[i])) + ((a[i]*d[i])+b[i]) )
			//      * ( (((d[i]*c[i])+d[i])*((b[i]*d[i])+a[i])) + ((c[i]+d[i])*a[i]) )
			//      + ( (((c[i]*d[i])+a[i])*((b[i]+d[i])*a[i])) + ((b[i]+c[i])*d[i]) );	
			// tb   = ( (((ta*b[i])+c[i])*((ta*c[i])+d[i])) + ((ta*d[i])+b[i]) )
			//      * ( (((d[i]*c[i])+d[i])*((b[i]*d[i])+ta)) + ((c[i]+d[i])*ta) )
			//      + ( (((c[i]*d[i])+ta)*((b[i]+d[i])*ta)) + ((b[i]+c[i])*d[i]) );	
			// tc   = ( (((ta*tb)+c[i])*((ta*c[i])+d[i])) + ((ta*d[i])+tb) )
			//      * ( (((d[i]*c[i])+d[i])*((tb*d[i])+ta)) + ((c[i]+d[i])*ta) )
			//      + ( (((c[i]*d[i])+ta)*((tb+d[i])*ta)) + ((tb+c[i])*d[i]) );	
			// td   = ( (((ta*tb)+tc)*((ta*tc)+d[i])) + ((ta*d[i])+tb) )
			//      * ( (((d[i]*tc)+d[i])*((tb*d[i])+ta)) + ((tc+d[i])*ta) )
			//      + ( (((tc*d[i])+ta)*((tb+d[i])*ta)) + ((tb+tc)*d[i]) );	
			// a[i] = ( (((ta*tb)+tc)*((ta*tc)+td)) + ((ta*td)+tb) )
			//      * ( (((td*tc)+td)*((tb*td)+ta)) + ((tc+td)*ta) )
			//      + ( (((tc*td)+ta)*((tb+td)*ta)) + ((tb+tc)*td) );	
		}
	}
	ORB_read(t2);
	perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));
	perftimer_accumulate(&p->timers, TIMER1, ORB_cycles_a(t2, t1));
	return ERR_CLEAN;
}

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parsePV4Plan
 * \sa makePV4Plan
 * \sa initPV4Plan
 * \sa execPV4Plan
 * \sa killPV4Plan
 */
int perfPV4Plan (void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	PV4data *d;
	p = (Plan *)plan;
	d = (PV4data *)p->vptr;
	if (p->exec_count > 0) {
		uint64_t times = 10000 * d->M * 8;
		opcounts[TIMER0] = times * p->exec_count;                        // Count # of trips through inner loop
		opcounts[TIMER1] = (times * 5 * sizeof(double)) * p->exec_count; // Count # of bytes of memory transfered (4 r, 1 w)
		opcounts[TIMER2] = 0;
		
		perf_table_update(&p->timers, opcounts, p->name, NULL);
		
		double trips  = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		double mbps   = ((double)opcounts[TIMER1]/perftimer_gettime(&p->timers, TIMER1))/1e6;
		EmitLogfs(MyRank, 9999, "PV4 plan performance:", trips, "Million Trips/s", PRINT_SOME);
		EmitLogfs(MyRank, 9999, "PV4 plan performance:", mbps,  "MB/s",   PRINT_SOME);
		EmitLog  (MyRank, 9999, "PV4 execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line The input line for the plan.
 * \param [out] output Holds the information for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makePV4Plan 
 * \sa initPV4Plan
 * \sa execPV4Plan
 * \sa perfPV4Plan
 * \sa killPV4Plan
*/
int parsePV4Plan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);	
	output->name = PV4;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info PV4_info = {
	"PV4",
	NULL,
	0,
	makePV4Plan,
	parsePV4Plan,
	execPV4Plan,
	initPV4Plan,
	killPV4Plan,
	perfPV4Plan,
	{ "Trips/s", "B/s", NULL }
};

