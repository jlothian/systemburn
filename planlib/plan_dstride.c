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
 * \brief Sets the values of a and b to 0
 * \param a Array to be initialized.
 * \param b Array to be initialized.
 * \param m Size of a and b.
 */
void Set(double* a, double* b, int m) {
	int i;
	for(i=0;i<m;i++) {
		a[i] = b[i] = 0.0;
	}
}

/**
 * \brief Fills the indices of a with random numbers
 * \param a Array to be initialized.
 */
void Fill(double* a) {
	int i;
	for(i=0;i<CACHE;i++) {
		a[i] = rand();
	}
}

/**
 * \brief Allocates and returns the data struct for the plan
 * \param [in] m Holds the input data for the plan.
 * \return void* Data struct 
 * \sa parseDStridePlan
 * \sa initDStridePlan
 * \sa execDStridePlan
 * \sa perfDStridePlan
 * \sa killDStridePlan
*/
void * makeDStridePlan(data *m) {
	Plan *p;
	DStridedata *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initDStridePlan;
		p->fptr_execplan = &execDStridePlan;
		p->fptr_killplan = &killDStridePlan;
		p->fptr_perfplan = &perfDStridePlan;
		p->name = DSTRIDE;
		d = (DStridedata *)malloc(sizeof(DStridedata));
		assert(d);
		if(d) {
			if(m->isize==1) d->M = m->i[0]/(2*sizeof(double));
			else d->M = m->d[0]/(2*sizeof(double));
		}
		(p->vptr)=(void*)d;
	}
	return p;
}

/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan Holds the data and storage locations for the plan.
 * \return int Error flag value
 * \sa parseDStridePlan 
 * \sa makeDStridePlan
 * \sa execDStridePlan
 * \sa perfDStridePlan
 * \sa killDStridePlan
*/
int    initDStridePlan(void *plan) {
	size_t M;
	int ret = make_error(ALLOC,generic_err);
	Plan *p;
	DStridedata *d = NULL;
	p = (Plan *)plan;
	if (p) {
		d = (DStridedata*)p->vptr;
		p->exec_count = 0;
		perftimer_init(&p->timers, NUM_TIMERS);
	}
	if (d) {
		M = d->M;
		d->one   = (double*) malloc(sizeof(double)*M);
		assert(d->one);
		d->two   = (double*) malloc(sizeof(double)*M);
		assert(d->two);
		d->three = (double*) malloc(sizeof(double)*CACHE);
		assert(d->three);
		if(d->one && d->two && d->three) ret = ERR_CLEAN;
	}
	srand(0);
	return ret;
}

/**
 * \brief Frees the memory used in the plan
 * \param [in] plan Points to the memory locations for free'ing.
 * \sa parseDStridePlan
 * \sa makeDStridePlan
 * \sa initDStridePlan
 * \sa execDStridePlan
 * \sa perfDStridePlan
 */
void * killDStridePlan(void *plan) {
	Plan *p;
	DStridedata *d;
	p = (Plan *)plan;
	d = (DStridedata*)p->vptr;

	if(d->one)   free(d->one);
	if(d->two)   free(d->two);
	if(d->three) free(d->three);
	free(d);
	free(p);
	return (void*)NULL;
}

/**
 * \brief A double precision floating point load which accesses memory with changing stride, using "size" bytes of memory. 
 * \param [in] plan Holds the data and the storage locations for the plan.
 * \return int Error flag value
 * \sa parseDStridePlan
 * \sa makeDStridePlan
 * \sa initDStridePlan
 * \sa perfDStridePlan
 * \sa killDStridePlan
 */
int execDStridePlan(void *plan) {
	Plan *p;
	DStridedata* d;
	p = (Plan *)plan;
	d = (DStridedata*)p->vptr;
	int i,k,r, Inc[] = {1,2,8,64,512,1028,2056,4112,16448,32896};
	double sum=0;
	int ret = ERR_CLEAN;
	ORB_t t1, t2;
	/* update execution count */
	p->exec_count++;

	Set(d->one,d->two,d->M);
	Fill(d->three);
	ORB_read(t1);
	for(k=0;k<REPEAT;k++) {
		for(r=0;r<10;r++) {
			for(i=0;i<d->M;i+=Inc[r]) {
				sum+=d->one[i]*d->two[i];
			}
			sum+=d->three[0];
		}
		sum++;
	}
	ORB_read(t2);
	perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));
	perftimer_accumulate(&p->timers, TIMER1, ORB_cycles_a(t2, t1));
	if (CHECK_CALC) {
		ret = (sum == (REPEAT * (1.0 + 10.0 * d->three[0]))) ? ERR_CLEAN : make_error(CALC,generic_err);
	}
	return ret;
}

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parseDStridePlan
 * \sa makeDStridePlan
 * \sa initDStridePlan
 * \sa execDStridePlan
 * \sa killDStridePlan
 */
int perfDStridePlan (void *plan) {
	int ret = ~ERR_CLEAN;
	int i, Inc[] = {1,2,8,64,512,1028,2056,4112,16448,32896};
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	DStridedata *d;
	p = (Plan *)plan;
	d = (DStridedata *)p->vptr;
	if (p->exec_count > 0) {
		uint64_t count = 0;
		for (i = 0; i < 10; i++) {
			count += d->M / Inc[i];
		}
		
		opcounts[TIMER0] = (2 * count + 10 + 1) * REPEAT * p->exec_count;     // Count # of floating point operations
		opcounts[TIMER1] = ((2 * count * REPEAT) + (1 * 10 * REPEAT)) * p->exec_count; // Count memory accesses.
		opcounts[TIMER2] = 0;
		
		perf_table_update(&p->timers, opcounts, p->name);
		
		double flops = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		double mbps  = ((double)opcounts[TIMER1]/perftimer_gettime(&p->timers, TIMER1))/1e6;
		EmitLogfs(MyRank, 9999, "DSTRIDE plan performance:", flops, "MFLOPS", PRINT_SOME);
		EmitLogfs(MyRank, 9999, "DSTRIDE plan performance:", mbps, "MB/s", PRINT_SOME);
		EmitLog  (MyRank, 9999, "DSTRIDE execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line The input line for the plan.
 * \param [out] output Holds the data and the storage locations for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makeDStridePlan 
 * \sa initDStridePlan
 * \sa execDStridePlan
 * \sa perfDStridePlan
 * \sa killDStridePlan
*/
int parseDStridePlan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);
	output->name = DSTRIDE;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info DSTRIDE_info = {
	"DSTRIDE",
	NULL,
	0,
	makeDStridePlan,
	parseDStridePlan,
	execDStridePlan,
	initDStridePlan,
	killDStridePlan,
	perfDStridePlan,
	{ "FLOPS", "B/s", NULL }
};

