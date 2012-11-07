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
#ifndef __PERFORMANCE_H
#define __PERFORMANCE_H

/*
 * Declarations visible to plans used for gathering performance data
 * ------------------------------------------------------------------------------------------------
 */

#include <config.h>
#include <orbtimer.h>

#ifdef HAVE_PAPI
/* PAPI header */
#include <papi.h>  
#endif

/**
 * \brief Indices for use with the performance timers.
 * \sa PerfTimers
 */
typedef enum {
	TIMER0 = 0,
	TIMER1,
	TIMER2,
	NUM_TIMERS  /**< The last element in the enum, evaluates to the total number of timers available to a plan. */
} perf_time_index;

/**
 * \brief Opaque object containing accumulators for recording total plan execution durations.
 */
typedef struct {
	ORB_tick_t time_count[NUM_TIMERS];
} PerfTimers;

/*
 * Functions for performance measurement and calculation.
 */
extern void     perftimer_init       (PerfTimers *timers, perf_time_index timer_num);
extern void     perftimer_accumulate (PerfTimers *timers, perf_time_index timer_num, ORB_tick_t ticks);
extern uint64_t perftimer_getcount   (PerfTimers *timers, perf_time_index timer_num);
extern double   perftimer_gettime    (PerfTimers *timers, perf_time_index timer_num);

/*
 * Declaration for performance data visible to the worker thread and scheduler
 * ------------------------------------------------------------------------------------------------
 */

#define GLOBAL 0
#define LOCAL  1

/*
 * Global data structures for storing performance data.
 */
extern uint64_t         perf_data_int  [][2*NUM_TIMERS];
extern double           perf_data_dbl  [][2*NUM_TIMERS];
extern char *           perf_data_unit [][NUM_TIMERS];
extern pthread_rwlock_t perf_data_lock [];



#ifdef HAVE_PAPI
/* PAPI specific functions and variables */
#define TOTAL_PAPI_EVENTS 4
//#define PAPI_COUNTERS { PAPI_FP_OPS, PAPI_TOT_CYC } //PAPI_SR_INS }

//typedef struct {
//    long long event_count[NUM_PAPI_EVENTS];
//} PAPI_Results;

extern void PAPIRes_init(long long *result, long long *times);
extern void PAPI_table_update(int plan_id, long long *results, long long *timers, int PAPI_num);
extern void PAPI_set_units(int plan_id, char** units, int PAPI_num);
//extern long long PAPI_Data [][2*NUM_PAPI_EVENTS];
extern long long PAPI_Data [][2*TOTAL_PAPI_EVENTS];
extern char* PAPI_data_unit [][TOTAL_PAPI_EVENTS]; //look at bottom of plan for example of plan data struct
#endif
/* END PAPI */



/*
 * Functions for performance data retrieval and analysis.
 */
extern void performance_init  ();
extern void perf_table_init   ();
extern void perf_table_print  (int scope_flag, int print_priority);
extern void perf_table_reduce ();
extern void perf_table_maxreduce ();
extern void perf_table_minreduce ();
extern void perf_table_minmax_populate(void *table, int nrows, int ncols);
extern void perf_table_minmax_print(void *table, int nrows, int ncols, int is_minimum);
extern void perf_table_update (PerfTimers *timers, uint64_t *opcounts, int plan_id);

#endif /* __PERFORMANCE_H */
