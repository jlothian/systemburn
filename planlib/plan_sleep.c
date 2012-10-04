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
 * \param [in] i Contains input data for the plan.
 * \return void* Plan data structure. 
 * \sa parseSleepPlan
 * \sa initSleepPlan
 * \sa execSleepPlan
 * \sa perfSleepPlan
 * \sa killSleepPlan
*/
void * makeSleepPlan(data *i) {
	Plan *p;
	int *ip;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initSleepPlan;
		p->fptr_execplan = &execSleepPlan;
		p->fptr_killplan = &killSleepPlan;
		p->fptr_perfplan = &perfSleepPlan;
		p->name = SLEEP;
		ip=(int*)malloc(sizeof(int));
		assert(ip);
		if(ip) *ip=i->i[0];
		(p->vptr)=(void*)ip;
	}
	return p;
}

/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan The Plan data structure, containing all input data.
 * \return int Error flag value
 * \sa parseSleepPlan 
 * \sa makeSleepPlan
 * \sa execSleepPlan
 * \sa perfSleepPlan
 * \sa killSleepPlan
*/
int  initSleepPlan(void *plan) {
	int ret = make_error(ALLOC,generic_err);
	Plan *p = (Plan *)plan;
	if(p) {
		ret = ERR_CLEAN;
		p->exec_count = 0;
		perftimer_init(&p->timers, NUM_TIMERS);
	}
	return ret;
}

/**
 * \brief Puts a thread to sleep for N seconds at a time.
 * \param [in] plan the Plan data structure, with input data.
 * \return int Error flag value
 * \sa parseSleepPlan
 * \sa makeSleepPlan
 * \sa initSleepPlan
 * \sa perfSleepPlan
 * \sa killSleepPlan
 */
int execSleepPlan(void *plan) {
	ORB_t t1, t2;
	Plan *p = (Plan *)plan;
	p->exec_count++;
	ORB_read(t1);
	sleep(*((int*)p->vptr));
	ORB_read(t2);
	perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));
	return ERR_CLEAN;
}

/**
 * \brief Frees the memory used in the plan
 * \param [in] plan The Plan data structure to be freed.
 * \sa parseSleepPlan
 * \sa makeSleepPlan
 * \sa initSleepPlan
 * \sa execSleepPlan
 * \sa perfSleepPlan
 */
void * killSleepPlan(void *plan) {
	Plan *p;
	p = (Plan *)plan;
	free((void*)(p->vptr));
	free((void*)(plan));
	return (void*)NULL;
}

/**
 * \brief Stores (and optionally displays) performance data for the plan.
 * \param [in] plan A pointer to the Plan structure, containing data for the plan.
 * \returns An integer error flag.
 * \sa parseSleepPlan
 * \sa makeSleepPlan
 * \sa initSleepPlan
 * \sa execSleepPlan
 * \sa killSleepPlan
 */
int perfSleepPlan(void *plan) {
	int ret = ~ERR_CLEAN;
	Plan *p = (Plan *)plan;
	if (p->exec_count > 0) {
		uint64_t opcount[NUM_TIMERS] = { 0, 0, 0 };
		
		// Don't record the time, so no print is performed...
		// perf_table_update(&p->timers, opcount, p->name, NULL);
		
		double time = perftimer_gettime(&p->timers, TIMER0);
		EmitLogfs(MyRank, 9999, "Sleep time:", time, "Seconds", PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line A string containing the input file line to be parsed.
 * \param [out] output A pre-allocated LoadPlan data structure to be initialized from the input file line.
 * \return int True if the data was read, false if it wasn't
 * \sa makeSleepPlan 
 * \sa initSleepPlan
 * \sa execSleepPlan
 * \sa perfSleepPlan
 * \sa killSleepPlan
*/
int parseSleepPlan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);
	output->name = SLEEP;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info SLEEP_info = {
	"SLEEP",
	NULL,
	0,
	makeSleepPlan,
	parseSleepPlan,
	execSleepPlan,
	initSleepPlan,
	killSleepPlan,
	perfSleepPlan,
	{ "Seconds", NULL, NULL }
};

