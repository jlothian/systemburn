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
#include <systemheaders.h> // <- Good to include since it has the basic headers in it.
#include <systemburn.h>    // <- Necessary to include to get the Plan struct and other neat things.
#include <planheaders.h>   // <- Add your header file (plan_Comm.h) to planheaders.h to be included. For uniformity, do not include here, and be sure to leave planheaders.h included.
#ifdef HAVE_SHMEM
  #  include <mpp/shmem.h>
  #ifdef SLU             /* Cray-like implementation */
  #  define   SHMEM_NUM_PES  num_pes()
  #  define   SHMEM_MY_PE    my_pe()
  #else                  /* SGI-like implementation */
  # define    SHMEM_NUM_PES _num_pes()
  # define    SHMEM_MY_PE   _my_pe()
  #endif
#else
	#include <mpi.h>
#endif

#ifdef HAVE_PAPI
#define NUM_PAPI_EVENTS 1 
#define PAPI_COUNTERS { PAPI_FP_OPS } 
#define PAPI_UNITS { "FLOPS" } 
#endif //HAVE_PAPI

/**
 * \brief Holds the custom error messages for the plan
 */
char* comm_errs[] = {
	" MPI error:"
};

/**
 * \brief Finds the next higher power of 2.
 * \param n The limit for the search.
 * \return c The next power of 2 greater than n.
 */
int comm_ceil2(int n) {
	int c = 1;
	while (c < n) c *= 2;
	return c;
}

/**
 * \brief Allocates and returns the data struct for the plan
 * \param dp The input data for the plan.
 * \return void* Data struct
 * \sa parseCommPlan
 * \sa initCommPlan
 * \sa execCommPlan
 * \sa perfCommPlan
 * \sa killCommPlan
 */
void *makeCommPlan(data *dp) {
	Plan *p;
	COMMdata *ip;
	p = (Plan*)malloc(sizeof(Plan));
	assert(p);
	if (p) {
		p->fptr_initplan = &initCommPlan;
		p->fptr_execplan = &execCommPlan;
		p->fptr_killplan = &killCommPlan;
		p->fptr_perfplan = &perfCommPlan;
		p->name          = SBCOMM;
		ip               = (COMMdata*)malloc(sizeof(COMMdata));
		assert(ip);
		if (ip) {
			ip->buflen = dp->i[0]/2;
			(p->vptr)=(void*)ip;
		}
	}
	return p;
}

/************************
 * This is the place where the memory gets allocated, and data types get initialized to their starting values.
 ***********************/
/**
 * \brief Creates and initializes the working data for the plan
 * \param plan The data and memory location for the plan.
 * \return int Error flag value
 * \sa parseCommPlan 
 * \sa makeCommPlan
 * \sa execCommPlan
 * \sa perfCommPlan
 * \sa killCommPlan
*/
int  initCommPlan(void *plan) {
	int i, ierr, NumRanks, ThisRankID;
	size_t buflen;
	int ret = make_error(ALLOC,generic_err);
	Plan *p;
	COMMdata *d = NULL;
	p = (Plan *)plan;

#ifdef HAVE_PAPI
        int temp_event, i;
        int PAPI_Events [NUM_PAPI_EVENTS] = PAPI_COUNTERS;
        char* PAPI_units [NUM_PAPI_EVENTS] = PAPI_UNITS;
#endif //HAVE_PAPI

	if (p) d = (COMMdata*)p->vptr;
	assert(d);
	
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
	
	buflen = d->buflen;
	ierr  = 0;
#ifdef HAVE_SHMEM
	NumRanks         = SHMEM_NUM_PES;
	ThisRankID       = SHMEM_MY_PE;
#else // MPI
	ierr += MPI_Comm_size(MPI_COMM_WORLD, &NumRanks);
	ierr += MPI_Comm_rank(MPI_COMM_WORLD, &ThisRankID);
#endif
	if (ierr != 0) return make_error(0,specific_err); // MPI error
	d -> NumRanks    = NumRanks;
	d -> ThisRankID  = ThisRankID;
	d -> NumStages   = comm_ceil2(NumRanks);
	d -> NumMessages = 50000;
	d -> istage      = 0;
#ifdef HAVE_SHMEM
	d -> sendbufptr  = (char *)shmalloc(buflen);
#else // MPI
	d -> sendbufptr  = (char *)malloc(buflen);
#endif
	d -> recvbufptr  = (char *)malloc(buflen);
	if ( d->sendbufptr  && d->recvbufptr ) {
		for (i=0; i<buflen; i++) {
			(d->sendbufptr)[i] = 0;
			(d->recvbufptr)[i] = 0;
		}
		ret = ERR_CLEAN;
	}
	return ret;
}

/************************
 * This is where the plan gets executed. Place all operations here.
 ***********************/
/**
 * \brief Where the plan is executed
 * \param [in] plan The data and memory location for the plan.
 * \return int Error flag value
 * \sa parseCommPlan
 * \sa makeCommPlan
 * \sa initCommPlan
 * \sa perfCommPlan
 * \sa killCommPlan
 */
int execCommPlan(void *plan) {
#ifdef HAVE_PAPI
        int k;
        long long start, end;
#endif //HAVE_PAPI

	int thatRankID, i;
	ORB_t t1, t2;
	Plan *p;
	COMMdata *d;
	p = (Plan *)plan;
	d = (COMMdata *)p->vptr;
	thatRankID = d->ThisRankID ^ d->istage;
#ifdef HAVE_SHMEM
	static int sync;
	int other;
	sync = MyRank;
	shmem_barrier_all();
	if ((thatRankID < d->NumRanks) && (thatRankID != d->ThisRankID)) {
		/* update execution count */
		p->exec_count++;
		
#ifdef HAVE_PAPI
                /* Start PAPI counters and time */
                TEST_PAPI(PAPI_reset(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
                start = PAPI_get_real_usec();
#endif //HAVE_PAPI
		
		ORB_read(t1);
		for (i = 0; i < d->NumMessages; i++) {
			shmem_getmem(d->recvbufptr, d->sendbufptr, d->buflen, thatRankID);
		}
		
		//shmem_int_p(&sync, d->ThisRankID, thatRankID);
		other = shmem_int_swap(&sync, d->ThisRankID, thatRankID); // Replaced because swap is atomic.
		// If wanted to check, could compare other to thatRankID (should be equal).
		shmem_int_wait_until(&sync, SHMEM_CMP_EQ, thatRankID);
		sync = d->ThisRankID;
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
	}
	shmem_barrier_all();
#else // MPI
	int ierr;
	MPI_Status mpistatus;
	if ((thatRankID < d->NumRanks) && (thatRankID != d->ThisRankID)) {
		ierr = 0;
		/* update execution count */
		p->exec_count++;
		
#ifdef HAVE_PAPI
                /* Start PAPI counters and time */
                TEST_PAPI(PAPI_reset(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
                start = PAPI_get_real_usec();
#endif //HAVE_PAPI
		
		ORB_read(t1);
		for (i = 0; i < d->NumMessages; i++) {
			ierr += MPI_Sendrecv(d->sendbufptr, d->buflen, MPI_BYTE, thatRankID, 0,
					     d->recvbufptr, d->buflen, MPI_BYTE, thatRankID, 0,
					     MPI_COMM_WORLD, &mpistatus);
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

		perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2,t1));
		if (ierr != 0) return make_error(0,specific_err); // MPI error
	}
#endif
	d->istage = (d->istage+1)%d->NumStages;
	return ERR_CLEAN;
}

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parseCommPlan
 * \sa makeCommmPlan
 * \sa initCommPlan
 * \sa execCommPlan
 * \sa killCommPlan
 */
int perfCommPlan(void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	COMMdata *d;
	p = (Plan *)plan;
	d = (COMMdata *)p->vptr;
	if (p->exec_count > 0) {
		opcounts[TIMER0] = (uint64_t)d->NumMessages * (uint64_t)d->buflen * p->exec_count; // Count # of bits transferred.
		opcounts[TIMER1] = 0;
		opcounts[TIMER2] = 0;
		
		perf_table_update(&p->timers, opcounts, p->name);
		
#ifdef HAVE_PAPI
		PAPI_table_update(p->name, p->PAPI_Results, p->PAPI_Times, p->PAPI_Num_Events);
#endif //HAVE_PAPI
		
		double mbps = (((double)opcounts[TIMER0])/perftimer_gettime(&p->timers, TIMER0))/1e6;
		EmitLogfs(MyRank, 9999, "COMM plan performance:", mbps, "MB/s", PRINT_SOME);
		EmitLog  (MyRank, 9999, "COMM execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/************************
 * This is where everything gets cleaned up.
 ***********************/
/**
 * \brief Frees the memory used in the plan
 * \param [in] plan The data and memory locations for the plan.
 * \sa parseCommPlan
 * \sa makeCommPlan
 * \sa initCommPlan
 * \sa execCommPlan
 * \sa perfCommPlan
 */
void * killCommPlan(void *plan) {
	Plan     *p;
	COMMdata *d;
	p = (Plan *)plan;
	d = (COMMdata*)p->vptr;

#ifdef HAVE_PAPI
        TEST_PAPI(PAPI_stop(p->PAPI_EventSet, NULL), PAPI_OK, MyRank, 9999, PRINT_SOME);
#endif //HAVE_PAPI

#ifdef HAVE_SHMEM
	if (d->sendbufptr)shfree((void*)(d->sendbufptr));
#else // MPI
	if (d->sendbufptr) free((void*)(d->sendbufptr));
#endif
	if (d->recvbufptr) free((void*)(d->recvbufptr));
	free((void*)(d));
	free((void*)(p));
	return (void*)NULL;
}

/************************
 * This is the parsing function that the program uses to read the load information from the load file.
 ***********************/
/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line The line of input data.
 * \param [out] output The data and storage for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makeCommPlan 
 * \sa initCommPlan
 * \sa execCommPlan
 * \sa perfCommPlan
 * \sa killCommPlan
*/
int parseCommPlan(char *line, LoadPlan *output) {
        output->name = SBCOMM;
        output->input_data = get_sizes(line);
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief Holds the custom error messages for the plan
 */
plan_info COMM_info = {
	"COMM",
	comm_errs,
	1,
	makeCommPlan,
	parseCommPlan,
	execCommPlan,
	initCommPlan,
	killCommPlan,
	perfCommPlan,
	{ "B/s", NULL, NULL }
};
