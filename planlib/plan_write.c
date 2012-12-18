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

#define MIBYTE 1048576

/**
 * \brief Allocates and returns the data struct for the plan
 * \param [in] i Holds the input data for the plan.
 * \return void* Data struct 
 * \sa parseWritePlan
 * \sa initWritePlan
 * \sa execWritePlan
 * \sa perfWritePlan
 * \sa killWritePlan
*/
void * makeWritePlan(data *i) {
	Plan *p;
	Writedata *ip;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) { // <- Checking the Plan pointer was allocated. Do not change.
		p->fptr_initplan = &initWritePlan;
		p->fptr_execplan = &execWritePlan;
		p->fptr_killplan = &killWritePlan;
		p->fptr_perfplan = &perfWritePlan;
		p->name = WRITE;
		ip=(Writedata*)malloc(sizeof(Writedata));
		assert(ip);
		if(ip) {
			if(i->isize==1) ip->mb = i->i[0]/MIBYTE;
			else ip->mb = i->d[0]/MIBYTE;
			if(i->csize>0) ip->str = strdup(i->c[0]);
			else ip->str = strdup("systemburn.iotest.out.");
		}
		(p->vptr)=(void*)ip;
	}
	return p;
}

/************************
 * This is the place where the memory gets allocated, and data types get 
 * initialized to their starting values.
 ***********************/
/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan Holds the data and memory for the plan.
 * \return int Error flag value
 * \sa parseWritePlan 
 * \sa makeWritePlan
 * \sa execWritePlan
 * \sa perfWritePlan
 * \sa killWritePlan
*/
int  initWritePlan(void *plan) {
	int ret = make_error(ALLOC,generic_err);
	Plan *p;
	Writedata * wi = NULL;
	p = (Plan *)plan;

    #ifdef HAVE_PAPI
        int temp_event, i;
        int PAPI_Events [NUM_PAPI_EVENTS] = PAPI_COUNTERS;
        char* PAPI_units [NUM_PAPI_EVENTS] = PAPI_UNITS;
    #endif //HAVE_PAPI

	if (p) {
		wi = (Writedata *)p->vptr;
		p->exec_count = 0;
                if(DO_PERF){
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
                } //DO_PERF
	}
	if (wi) {
		wi->buff_len = (wi->mb * MIBYTE) / sizeof(int);
		wi->buff = (int *)malloc(wi->buff_len * sizeof(int));
		assert(wi->buff);
		int i;
		for (i = 0; i < wi->buff_len; i++) {
			wi->buff[i] = 42;
		}
		
		char rank_string[6];
		char *fname;
		fname = (char*)malloc(sizeof(char)*(strlen(wi->str)+20));
		snprintf(rank_string, sizeof(rank_string)-1, "%d", MyRank);
		//fprintf(stderr, "rank_string: %s\n", rank_string);

		strcpy(fname,wi->str);
		strcat(fname,rank_string);
		wi->fdout = open(fname, O_CREAT | O_WRONLY, 0644);
		free(fname);
		if(wi->fdout < 0) {
			perror("File creation.");
			return make_error(0,specific_err);
		}
		ret = ERR_CLEAN;
	}
	return ret;
}

/************************
 * This is where the plan gets executed. Place all operations here.
 ***********************/
/**
 * \brief An I/O load which writes "megabytes" to files with "string" as the basename (this can be a path).
 * \param [in] plan Holds the data for the plan.
 * \return int Error flag value
 * \sa parseWritePlan
 * \sa makeWritePlan
 * \sa initWritePlan
 * \sa perfWritePlan
 * \sa killWritePlan
 */
int execWritePlan(void *plan) {
    #ifdef HAVE_PAPI
        int k;
        long long start, end;
    #endif //HAVE_PAPI

	ssize_t ret;
	ORB_t t1, t2;
	Plan *p;
	Writedata *wi;
	p = (Plan *)plan;
	wi = (Writedata *)p->vptr;
	/* update execution count */
	p->exec_count++;
	// Rewind the file descriptor.
	//fprintf(stderr, "Seeking...\n");
	lseek(wi->fdout, 0, SEEK_SET);
	//fprintf(stderr, "Writing...\n");
	int block_size = 1024;
	int num_blocks = wi->buff_len * sizeof(int) / block_size;
	int i;

        if(DO_PERF){
#ifdef HAVE_PAPI
                /* Start PAPI counters and time */
                TEST_PAPI(PAPI_reset(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
                start = PAPI_get_real_usec();
#endif //HAVE_PAPI
                ORB_read(t1);
        }

	ret = write(wi->fdout, wi->buff, wi->buff_len * sizeof(int));
	
        if(DO_PERF){
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

	if(ret != wi->buff_len * sizeof(int)) {  // < 0) {
		perror("Write failure. ");
		return make_error(1,specific_err);
	}
	
// 	for(i = 0; i < num_blocks; i++) {
// 		fprintf(stderr, "Writing block number %d of %d\n", i, num_blocks);
// 		ret = write(wi->fdout, &(wi->buff[i*block_size]), block_size);
// 		if(ret < 0) {
// 			perror("Write failure");
// 			return ERR_WRITE_FILE;
// 		}
// 	}
	return ERR_CLEAN;
}

/************************
 * This is where everything gets cleaned up. Be sure to free() your data types
 * (free the members first) in addition to the ones included below.
 ***********************/
/**
 * \brief Frees the memory used in the plan
 * \param [in] plan Points to the memory to be free'd.
 * \sa parseWritePlan
 * \sa makeWritePlan
 * \sa initWritePlan
 * \sa execWritePlan
 * \sa perfWritePlan
 */
void * killWritePlan(void *plan) {
	Plan *p;
	Writedata *wi;
	p = (Plan *)plan;

    #ifdef HAVE_PAPI
        TEST_PAPI(PAPI_stop(p->PAPI_EventSet, NULL), PAPI_OK, MyRank, 9999, PRINT_SOME);
    #endif //HAVE_PAPI

	wi = (Writedata *)p->vptr;
	close(wi->fdout);
	if(wi->str)  free(wi->str);
	if(wi->buff) free(wi->buff);
	free((void*)(p->vptr));
	free((void*)(plan));
	return (void*)NULL;
}

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parseWritePlan
 * \sa makeWritePlan
 * \sa initWritePlan
 * \sa execWritePlan
 * \sa killWritePlan
 */
int perfWritePlan(void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	Writedata *d;
	p = (Plan *)plan;
	d = (Writedata *)p->vptr;
	if (p->exec_count > 0) {
		opcounts[TIMER0] = d->mb * MIBYTE * p->exec_count; // Count # of bytes written.
		opcounts[TIMER1] = 0;
		opcounts[TIMER2] = 0;
		
		perf_table_update(&p->timers, opcounts, p->name);
            #ifdef HAVE_PAPI
		PAPI_table_update(p->name, p->PAPI_Results, p->PAPI_Times, p->PAPI_Num_Events);
            #endif //HAVE_PAPI
		
		double mbs = (((double)opcounts[TIMER0])/perftimer_gettime(&p->timers, TIMER0))/1e6;
		EmitLogfs(MyRank, 9999, "Write plan performance:", mbs, "MB/s", PRINT_SOME);
		EmitLog  (MyRank, 9999, "Write execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/************************
 * This is the parsing function that the program uses to read in your load 
 * information from the load file. This lets you have a custom number of int 
 * arguments for your module while still maintaining modularity.
 ***********************/
/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line Input line for the plan.
 * \param [out] output Holds the information for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makeWritePlan 
 * \sa initWritePlan
 * \sa execWritePlan
 * \sa perfWritePlan
 * \sa killWritePlan
*/
int parseWritePlan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);
	output->name = WRITE;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief Holds the custom error messages for the plan
 */
char* write_errs[] = {
	" file openning errors:",
	" file writing errors:",
};

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info WRITE_info = {
	"WRITE",
	write_errs,
	2,
	makeWritePlan,
	parseWritePlan,
	initWritePlan,
	execWritePlan,
	killWritePlan,
	perfWritePlan,
	{ "B/s", NULL, NULL }
};
