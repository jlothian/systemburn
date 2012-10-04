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
 * \param [in] m The input data for the plan.
 * \return void* Data struct 
 * \sa parsePV1Plan
 * \sa initPV1Plan
 * \sa execPV1Plan
 * \sa perfPV1Plan
 * \sa killPV1Plan
*/
void * makePV1Plan(data *m) {
	Plan *p;
	PV1data *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initPV1Plan;
		p->fptr_execplan = &execPV1Plan;
		p->fptr_killplan = &killPV1Plan;
		p->fptr_perfplan = &perfPV1Plan;
		p->name = PV1;
		d = (PV1data *)malloc(sizeof(PV1data));
		assert(d);
		if(d) {
			if(m->isize==1) d->M = m->i[0]/(4*sizeof(double));
			else d->M = m->d[0]/(4*sizeof(double));
		}
		(p->vptr)=(void*)d;
	}
	return p;
}

/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan Holds the data and memory for the plan.
 * \return int Error flag value
 * \sa parsePV1Plan 
 * \sa makePV1Plan
 * \sa execPV1Plan
 * \sa perfPV1Plan
 * \sa killPV1Plan
*/
int    initPV1Plan(void *plan) {
	int i;
	size_t M;
	int ret = make_error(ALLOC,generic_err);
	Plan *p;
	PV1data *d = NULL;
	p = (Plan *)plan;
	if (p) {
		d = (PV1data*)p->vptr;
		p->exec_count = 0;
		perftimer_init(&p->timers, NUM_TIMERS);
	}
	assert(d);
	if(d) {
		M = d->M;
		d->one   = (double*)  malloc(sizeof(double)*M);
		assert(d->one);
		d->two   = (double*)  malloc(sizeof(double)*M);
		assert(d->two);
		d->three = (double*)  malloc(sizeof(double)*M);
		assert(d->three);
		d->four = (double*)  malloc(sizeof(double)*M);
		assert(d->four);
		if(d->one && d->two && d->three && d->four) {
			for(i=0;i<d->M;i++) {
				d->one[i]   =1.0;
				d->two[i]   =2.0;
				d->three[i] =0.0;
				d->four[i]  =0.0;
			}
			d->random = rand();
			ret = ERR_CLEAN;
		}
	}
	return ret;
}

/**
 * \brief Frees the memory used in the plan
 * \param [in] plan Points to memory to be free'd.
 * \sa parsePV1Plan
 * \sa makePV1Plan
 * \sa initPV1Plan
 * \sa execPV1Plan
 * \sa perfPV1Plan
 */
void * killPV1Plan(void *plan) {
	Plan *p;
	PV1data *d;
	p = (Plan *)plan;
	assert(p);
	d = (PV1data*)p->vptr;
	assert(d);

	if(d->one)   free(d->one);
	if(d->two)   free(d->two);
	if(d->three) free(d->three);
	if(d->four)  free(d->four);
	free(d);
	free(p);
	return (void*)NULL;
}

/**
 * \brief A power hungry streaming computational algorithm on four arrays of 64bit values, which will operate with a memory footprint of "size" bytes.
 * \param [in] plan Holds the data and the memory for the plan.
 * \return int Error flag value
 * \sa parsePV1Plan
 * \sa makePV1Plan
 * \sa initPV1Plan
 * \sa perfPV1Plan
 * \sa killPV1Plan
 */
int execPV1Plan(void *plan) {
	register size_t i;
	ORB_t t1, t2;
	Plan *p;
	PV1data* d;
	p = (Plan *)plan;
	d = (PV1data*)p->vptr;
	assert(d);
	/* update execution count */
	p->exec_count++;
	
	ORB_read(t1);
	for(i=0; i<d->M ;i++) {
		d->one[i] = 0.5 + 0.5 * (0.9 * d->one[i] + (1.0+d->two[(d->random^i)&0xfff]) * d->three[i&0xff000]);
	}
	ORB_read(t2);
	perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));
	perftimer_accumulate(&p->timers, TIMER1, ORB_cycles_a(t2, t1));
	return ERR_CLEAN;
}

// NOTES:
// 46 / 53 / 60 nehalem d->one[i] = 0.9 * d->two[i] + ((d->random^i)&0xfff) * d->three[(i&0xffff)];
// 46 / 54 / 60 nehalem d->one[i] = d->four[(i*d->random)%M] * d->two[i&0xff] + ((d->random^i)&0xfff) * d->three[i&0xfffff];
// 48 / 56 / 62 nehalem d->one[i] = 0.9 * d->one[i] + ((d->random^i)&0xfff) * d->three[(i&0xff)];
// 48 / 56 / 62 nehalem d->one[i] = 0.9 * d->one[i] + ((d->random^i)&0xfff) * d->three[(i&0xffff)];
// 48 / 56 / 62 nehalem d->one[i] = 0.9 * d->one[i] + (1.0+((d->random^i)&0xfff)) * d->three[i&0xfff0];
// 48 / 57 / 64 nehalem d->one[i] = 0.9 * d->one[i] + (1.0+((d->random^i)&0xfff)) * d->three[i&0xff000];
// 49 / 56 / 62 nehalem d->one[(i^d->random)&0xfffff] = 0.9 * d->one[i] + (1.0+((d->random^i)&0xfff)) * d->three[i&0xff000];
// 49 / 56 / 62 nehalem d->one[i] = (0.9*(0.9 * d->one[i] + (1.0+((d->random^i)&0xfff)) * d->three[(i&0xff00)]);
// 49 / 57 / 64 nehalem d->one[i] = 0.9 * d->one[(i&0xffffffff)] + (1.0+((d->random^i)&0xfff)) * d->three[((i^d->random)&0xff00)];
// 50 / 57 / 63 nehalem d->one[i] = 0.9 * d->one[i&0xffffffff] + ((d->random^i)&0xfff) * d->three[(i&0xff0)];
// 50 / 57 / 63 nehalem d->one[i] = 0.9 * d->one[i] + ((d->random^i)&0xfff) * d->three[(i&0xff0)];
// 50 / 58 / 64 nehalem d->one[i] = 0.9 * d->one[i&0xffffffff] + (1.0+((d->random^i)&0xfff)) * d->three[(i&0xff00)];
// 51 / 58 / 64 nehalem d->one[i] = 0.9 * d->one[i] + (1.0+((d->random^i)&0xfff)) * d->three[(i&0xff00)];
// 51 / 58 / 64 nehalem d->one[i] = 0.9 * d->one[i] + (1.0+d->two[(d->random^i)&0xfff]) * d->three[i&0xff000];
// 51 / 58 / 64 nehalem d->one[i] = 0.5 + 0.5 * (0.9 * d->one[i] + (1.0+d->two[(d->random^i)&0xfff]) * d->three[i&0xff000]);

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parsePV1Plan
 * \sa makePV1Plan
 * \sa initPV1Plan
 * \sa execPV1Plan
 * \sa killPV1Plan
 */
int perfPV1Plan (void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	PV1data *d;
	p = (Plan *)plan;
	d = (PV1data *)p->vptr;
	if (p->exec_count > 0) {
		opcounts[TIMER0] = d->M * p->exec_count;                        // Count # trips through inner loop.
		opcounts[TIMER1] = (d->M * 4 * sizeof(double)) * p->exec_count; // Count # of bytes of memory transfered (3 r, 1 w)
		opcounts[TIMER2] = d->M * p->exec_count;                        // Count # of floating point operations in checking loop.
		
		perf_table_update(&p->timers, opcounts, p->name);
		
		double flops  = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		double mbps   = ((double)opcounts[TIMER1]/perftimer_gettime(&p->timers, TIMER1))/1e6;
		EmitLogfs(MyRank, 9999, "PV1 plan performance:", flops, "Million Trips/s", PRINT_SOME);
		EmitLogfs(MyRank, 9999, "PV1 plan performance:", mbps,  "MB/s",   PRINT_SOME);
		EmitLog  (MyRank, 9999, "PV1 execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line Input line for the plan.
 * \param [out] output Holds the data for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makePV1Plan 
 * \sa initPV1Plan
 * \sa execPV1Plan
 * \sa perfPV1Plan
 * \sa killPV1Plan
*/
int parsePV1Plan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);	
	output->name = PV1;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info PV1_info = {
	"PV1",
	NULL,
	0,
	makePV1Plan,
	parsePV1Plan,
	execPV1Plan,
	initPV1Plan,
	killPV1Plan,
	perfPV1Plan,
	{ "Trips/s", "B/s", NULL }
};

