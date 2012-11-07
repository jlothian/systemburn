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
#include <systemburn.h>
#include <planheaders.h>

/* size of random array */
#define RSIZE 128
/* used by random number generator */
#define POLY 0x0000000000000007ULL
#define PERIOD 1317624576693539401LL



#ifdef HAVE_PAPI
#define NUM_PAPI_EVENTS 2 
#define PAPI_COUNTERS { PAPI_FP_OPS, PAPI_TOT_CYC } 
#define PAPI_UNITS { "FLOPS", "CYCS" } 
#endif //HAVE_PAPI


// binary logarithm -- slow but only called once per GUPS plan.
/**
 * \brief Binary logarithm (n=2^i)
 * \param [in] n Target for solving the logarithm.
 * \return i Power of 2 >= n
 */
uint64_t GUPSlog2(uint64_t n) {
	uint64_t i=0;
	if (n<1) return 0xFFFFFFFFFFFFFFFF;
	while (n>>=1) i++;
	return i;
}

/**
 * \brief Allocates and returns the data struct for the plan
 * \param [in] m Holds the input data for the plan.
 * \return A reference to the created Plan structure. 
 * \sa parseGUPSPlan
 * \sa initGUPSPlan
 * \sa execGUPSPlan
 * \sa perfGUPSPlan
 * \sa killGUPSPlan
*/
void * makeGUPSPlan(data *m) {
	Plan *p;
	GUPSdata *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initGUPSPlan;
		p->fptr_execplan = &execGUPSPlan;
		p->fptr_killplan = &killGUPSPlan;
		p->fptr_perfplan = &perfGUPSPlan;
		p->name = GUPS;
		d = (GUPSdata*)malloc(sizeof(GUPSdata));
		assert(d);
		if(d) {
			if (m->isize==1)
				d->tbl_log_size = GUPSlog2((uint64_t)(m->i[0])/sizeof(uint64_t));
			else
				d->tbl_log_size = GUPSlog2((uint64_t)(m->d[0])/sizeof(uint64_t));
		}
		(p->vptr)=(void*)d;
	}
	return p;
}

/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan A pointer to a Plan structure that holds the data and the memory for the plan.
 * \return int Error flag value
 * \sa parseGUPSPlan 
 * \sa makeGUPSPlan
 * \sa execGUPSPlan
 * \sa perfGUPSPlan
 * \sa killGUPSPlan
*/
int  initGUPSPlan(void *plan) {
	int ret = make_error(ALLOC,generic_err);

        int retval, i;
#ifdef HAVE_PAPI
        int PAPI_Events [NUM_PAPI_EVENTS] = PAPI_COUNTERS;
        char* PAPI_units [NUM_PAPI_EVENTS] = PAPI_UNITS;
#endif //HAVE_PAPI

	Plan *p;
	GUPSdata *d = NULL;
	p = (Plan *)plan;
	// If the plan is valid, initialize plan variables.
	if (p) {
		d = (GUPSdata*)p->vptr;
		p->exec_count = 0;
		perftimer_init(&p->timers, NUM_TIMERS);

#ifdef HAVE_PAPI
                /* Initialize plan's PAPI data */
                p->PAPI_EventSet = PAPI_NULL;
                retval = PAPI_create_eventset(&p->PAPI_EventSet);
                if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
                
                retval = PAPI_add_events(p->PAPI_EventSet, PAPI_Events, NUM_PAPI_EVENTS);
                if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
                PAPIRes_init(p->PAPI_Results, p->PAPI_Times);
                PAPI_set_units(p->name, PAPI_units, NUM_PAPI_EVENTS);
#endif //HAVE_PAPI
                //creat initializer for results?
	}
	// If the GUPS heap is valid, initialize GUPS variables.
	if (d) {
		d->tbl_size     = (1L << d->tbl_log_size);
		d->sub_log_size = (d->tbl_log_size/3);
		d->sub_size     = (1 << d->sub_log_size);
		d->num_updates  = (4 * d->tbl_size);

		//EmitLog(MyRank, 10, "Allocating",sizeof(uint64_t)*(d->tbl_size +d->sub_size+RSIZE),0); 
    
		d->tbl = (uint64_t*)malloc(sizeof(uint64_t)*d->tbl_size);
		assert(d->tbl);
		d->sub = (uint64_t*)malloc(sizeof(uint64_t)*d->sub_size);
		assert(d->sub);
		d->random = (uint64_t*)malloc(sizeof(uint64_t)*RSIZE);
		assert(d->random);
		if (d->tbl && d->sub && d->random) {
			/* initialize substitution table */
			d->sub[0] = 0;
			for (i = 1; i < d->sub_size; i++)
				d->sub[i] = d->sub[i-1] + 0x0123456789abcdefULL;
                        /* initialize main table */
                        for (i = 0; i < d->tbl_size; i++)
                                d->tbl[i] = i;
			ret = ERR_CLEAN;
		}
	}
	return ret;
}

/**
 * \brief Frees the memory used in the plan
 * \param [in] plan Points to the Plan structure to be freed.
 * \sa parseGUPSPlan
 * \sa makeGUPSPlan
 * \sa initGUPSPlan
 * \sa execGUPSPlan
 * \sa perfGUPSPlan
 */
void * killGUPSPlan(void *plan) {
        int retval;

	Plan *p;
	GUPSdata *d;
	p = (Plan *)plan;
	d = (GUPSdata*)p->vptr;

	//EmitLog(MyRank,10, "Freeing   ",sizeof(uint64_t)*(d->tbl_size+d->sub_size+RSIZE),0);

	if(d->tbl) free((void*)(d->tbl));
	if(d->sub) free((void*)(d->sub));
	if(d->random) free((void*)(d->random));

        //retval = PAPI_stop(p->PAPI_EventSet, NULL);  //don't know if this will work
        //if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);

	free((void*)(d));
	free((void*)(p));
	return (void*)NULL;
}

/**
 * \brief Giga Updates Per Second - a random memory access benchmark on a table of "size" bytes. Note that "size" must be a power of 2, if it is not, it will be adjusted to the largest power of 2 which will fit within "size" bytes. 
 * \param [in] plan Plan structure holding the data and memory for the plan.
 * \return int Error flag value
 * \sa parseGUPSPlan
 * \sa makeGUPSPlan
 * \sa initGUPSPlan
 * \sa perfGUPSPlan
 * \sa killGUPSPlan
 */
int execGUPSPlan(void *plan) {
        /* PAPI vars */
        int retval, k;
        long long start, end;

	/* local vars */
	int64_t i,j;
	uint64_t tblsize,lsubsize,nupdates;
	uint64_t *tbl,*sub,*ran;
        int errflag=0, ret = ERR_CLEAN;
	Plan *p;
	GUPSdata *d;
	/* execution timers */
	ORB_t t1, t2;
	/* get values from data struct */
	p = (Plan *)plan;
	d = (GUPSdata*)p->vptr;
	tbl = d->tbl;
	sub = d->sub;
	ran = d->random;
	tblsize = d->tbl_size;
	lsubsize = d->sub_log_size;
	nupdates = d->num_updates;

	/* increment execution count */
	p->exec_count++;

	/* initialize random array */
	for (i = 0; i < RSIZE; i++)
		ran[i] = GUPS_startRNG((nupdates/RSIZE)*i);


#ifdef HAVE_PAPI
        /* Start PAPI counters and time */
        retval = PAPI_start(p->PAPI_EventSet);
        if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
        start = PAPI_get_real_usec();
#endif //HAVE_PAPI

	ORB_read(t1);
	/* perform updates to main table */
	for (i = 0; i < nupdates/RSIZE; i++) {
		for (j = 0; j < RSIZE; j++) {
			ran[j] = (ran[j] << 1) ^ ((int64_t)ran[j] < 0 ? POLY : 0);
			tbl[ran[j] & (tblsize-1)] ^= sub[ran[j] >> (64-lsubsize)];
		}
	}

	ORB_read(t2);
	perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));
#ifdef HAVE_PAPI
        end = PAPI_get_real_usec(); //PAPI time

        /* Collect PAPI counters and store time elapsed */
        retval = PAPI_accum(p->PAPI_EventSet, p->PAPI_Results);
        if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
        for(k=0; k<NUM_PAPI_EVENTS; k++){
            p->PAPI_Times[k] += (end - start);
        }
#endif //HAVE_PAPI


	/* verify results */
	if (CHECK_CALC) {
		uint64_t temp = 0x1;
                
#ifdef HAVE_PAPI
                /* Restart PAPI counters */
                retval = PAPI_reset(p->PAPI_EventSet);
                if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
                start = PAPI_get_real_usec();
#endif //HAVE_PAPI

		ORB_read(t1);
		for (i = 0; i < nupdates; i++) {
			temp = (temp << 1) ^ (((int64_t) temp < 0) ? POLY : 0);
			tbl[temp & (tblsize-1)] ^= sub[temp >> (64-lsubsize)];
		}
		for (i = 0; i < tblsize; i++) {
			if (tbl[i] != i){
			uint64_t a = (-1)^(1<<((sizeof(int)*8)-1));	// address mask
			uint64_t p = sysconf(_SC_PAGESIZE)-1;		// page mask
			EmitLog(MyRank, 9999, "GUPS Errors: wrong value in bit      ",(int)GUPSlog2(tbl[i]^i),0); 
			EmitLog(MyRank, 9999, "GUPS Errors:          table address: ",(int)((uint64_t)(&(tbl[i]))&a),0); 
			EmitLog(MyRank, 9999, "GUPS Errors:         offset in page: ",(int)((uint64_t)(&(tbl[i]))&p),0); 
			EmitLog(MyRank, 9999, "GUPS Errors:         patching value: ",i,0); 
			/* fix the erroneous value */
			tbl[i] = i;
			errflag=1;
			}
		}
		
                ORB_read(t2);
		perftimer_accumulate(&p->timers, TIMER1, ORB_cycles_a(t2, t1));

#ifdef HAVE_PAPI
                end = PAPI_get_real_usec(); //PAPI time
                /* Collect PAPI counters */
                retval = PAPI_accum(p->PAPI_EventSet, p->PAPI_Results);
                if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
                for(k=0; k<NUM_PAPI_EVENTS; k++){
                    p->PAPI_Times[k] += (end - start);
                }
#endif //HAVE_PAPI
		if(errflag==1)
			ret = make_error(CALC,generic_err);
	}
#ifdef HAVE_PAPI
        /* Stop PAPI counters */
        retval = PAPI_stop(p->PAPI_EventSet, NULL);
        if(retval != PAPI_OK) PAPI_EmitLog(retval, MyRank, 9999, PRINT_SOME);
#endif //HAVE_PAPI

	return ret;
}

/**
 * \brief Stores (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure containing the plan data.
 * \returns An integer error code.
 * \sa parseGUPSPlan
 * \sa makeGUPSPlan
 * \sa initGUPSPlan
 * \sa execGUPSPlan
 * \sa killGUPSPlan
 */
int perfGUPSPlan(void *plan) {
        char* buffer;

	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	GUPSdata *d;
	p = (Plan *)plan;
	d = (GUPSdata *)p->vptr;
	if (p->exec_count > 0) {
		opcounts[TIMER0] = (uint64_t)d->num_updates * p->exec_count;
		opcounts[TIMER1] = 0;
		opcounts[TIMER2] = 0;
		
                /* Additionally, passing PAPI_Results to be collected */
		perf_table_update(&p->timers, opcounts, p->name);
#ifdef HAVE_PAPI
		PAPI_table_update(p->name, p->PAPI_Results, p->PAPI_Times, NUM_PAPI_EVENTS);
#endif //HAVE_PAPI
		
                //TODO: add place to present PAPI data
		double gups = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/(1e9);
		EmitLogfs(MyRank, 9999, "GUPS plan performance:", gups, "GUPS", PRINT_SOME);
		EmitLog  (MyRank, 9999, "GUPS execution count :", p->exec_count, PRINT_SOME);
		
                //sprintf(buffer, "GUPS PAPI data : ES = %d\t R1 = %llu\t R2 = %llu\t\n", p->PAPI_EventSet, p->PAPI_Results[0], p->PAPI_Results[1]);
                //sprintf(buffer, "GUPS PAPI data : ES = %d\t Results = %p\n", p->PAPI_EventSet, p->PAPI_Results);
                //EmitLog  (MyRank, 9999, buffer, 0, PRINT_SOME);
		//EmitLog  (MyRank, 9999, "GUPS PAPI ES :", p->PAPI_EventSet, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line The input line for the plan.
 * \param [out] output Holds the data for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makeGUPSPlan 
 * \sa initGUPSPlan
 * \sa execGUPSPlan
 * \sa perfGUPSPlan
 * \sa killGUPSPlan
*/
int parseGUPSPlan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);
        output->name = GUPS;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info GUPS_info = {
	"GUPS",
	NULL,
	0,
	makeGUPSPlan,
	parseGUPSPlan,
	execGUPSPlan,
	initGUPSPlan,
	killGUPSPlan,
	perfGUPSPlan,
	{ "UPS", NULL, NULL }
};

/**
 * \brief GUPS running code
 */
uint64_t GUPS_startRNG(int64_t n) {
	int i, j;
	uint64_t m2[64];
	uint64_t temp, ran;

	while (n < 0) n += PERIOD;
	while (n > PERIOD) n -= PERIOD;
	if (n == 0) return 0x1;

	temp = 0x1;
	for (i=0; i<64; i++) {
		m2[i] = temp;
		temp = (temp << 1) ^ ((int64_t) temp < 0 ? POLY : 0);
		temp = (temp << 1) ^ ((int64_t) temp < 0 ? POLY : 0);
	}

	for (i=62; i>=0; i--)
		if ((n >> i) & 1) break;

	ran = 0x2;
	while (i > 0) {
		temp = 0;
		for (j=0; j<64; j++) {
			if ((ran >> j) & 1)
				temp ^= m2[j];
		}
		ran = temp;
		i -= 1;
		if ((n >> i) & 1)
			ran = (ran << 1) ^ ((int64_t) ran < 0 ? POLY : 0);
	}

	return ran;
}


