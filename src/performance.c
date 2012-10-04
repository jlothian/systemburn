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
#include <performance.h>
#include <comm.h>

/* Helper function to handle PAPI errors */
inline void handle_PAPI_error(int val){
    char* message;

    sprintf(message, "PAPI error %d: %s\n", val, PAPI_strerror(val));
    EmitLog(MyRank, SCHEDULER_THREAD, message, -1, PRINT_ALWAYS);
    //exit(1);
}

inline void PAPIRes_init(long long *results){
    int i;

    for(i=0; i<NUM_PAPI_EVENTS; i++){
        results[i] = 0LL;
    }   
}

/*
 * Functions for performance measurement and calculation.
 */

/**
 * \brief Initializes the specified timer(s) to zero.
 * If a specific timer number is given, only that timer is initialized, but
 * in the case that NUM_TIMERS is given as the timer number, all timers
 * will be initialized.
 *
 * \param [in] timers A reference to an existing PerfTimers structure containing the timer(s) to be initialized.
 * \param [in] timer_num The index of the timer to be initialized, or NUM_TIMERS.
 */
inline void perftimer_init (PerfTimers *timers, perf_time_index timer_num) {
	if (timers != NULL) {
		if (timer_num < NUM_TIMERS) {
			timers->time_count[timer_num] = (ORB_tick_t)0;
		} else {
			int i;
			for (i = 0; i < NUM_TIMERS; i++) {
				timers->time_count[i] = (ORB_tick_t)0;
			}
		}
	}
}

/**
 * \brief Adds to the current count of the timer.
 * 
 * \param [in,out] timers A reference to an existing PerfTimers structure containing the timer to be updated.
 * \param [in] timer_num The index of the timer to be updated, must be < NUM_TIMERS.
 * \param [in] ticks The elapsed number of timer ticks to be added to the timer.
 */
inline void perftimer_accumulate (PerfTimers *timers, perf_time_index timer_num, ORB_tick_t ticks) {
	if (timers != NULL && timer_num < NUM_TIMERS) timers->time_count[timer_num] += ticks;
}

/**
 * \brief Returns the number of clock ticks counted by the specified timer.
 * 
 * \param [in] timers A reference to an existing PerfTimers structure containing the timer to be read from.
 * \param [in] timer_num The index of the timer to be read, must be < NUM_TIMERS.
 * \returns The number of clock ticks elapsed between the start and stop time.
 */
inline uint64_t perftimer_getcount (PerfTimers *timers, perf_time_index timer_num) {
	if (timers != NULL && timer_num < NUM_TIMERS)
		return (uint64_t)timers->time_count[timer_num];
}

/**
 * \brief Returns the number of seconds counted by the specified timer.
 *
 * \param [in] timers A reference to an existing PerfTimers structure containing the timer to be read from.
 * \param [in] timer_num The index of the timer to be read, must be < NUM_TIMERS.
 * \returns The number of seconds elapsed between the start and stop time.
 */
inline double perftimer_gettime (PerfTimers *timers, perf_time_index timer_num) {
	if (timers != NULL && timer_num < NUM_TIMERS)
		return (double)timers->time_count[timer_num]/ORB_REFFREQ;
}

/*
 * Global data structures for storing performance data.
 */
/**
 * \brief Stores integer timer and operation counts for on node access.
 * Each plan is given a set of timer accumulators, with each timer associated with an operation count.
 */
uint64_t         perf_data_int  [NUM_PLANS][2*NUM_TIMERS];
/**
 * \brief Stores floating point timer and operation counts for use in multinode reduction operations.
 */
double           perf_data_dbl  [NUM_PLANS][2*NUM_TIMERS];
/**
 * \brief Stores multinode maximum performance data (op count / timer time).
 */
double           perf_data_max  [NUM_PLANS][NUM_TIMERS];
/**
 * \brief Stores multinode minimum performance data (op count / timer time).
 */
double           perf_data_min  [NUM_PLANS][NUM_TIMERS];
/**
 * \brief Stores references to strings labeling the units associated with the stored operation counts.
 */
char *           perf_data_unit [NUM_PLANS][NUM_TIMERS];
/**
 * \brief A locking data structure that allows multiple worker threads to update the table without creating conflicts.
 */
pthread_rwlock_t perf_data_lock [NUM_PLANS];


/* PAPI data structure */
long long PAPI_Data [NUM_PLANS][NUM_PAPI_EVENTS]; 



/*
 * Functions for performance data retrieval and analysis.
 */

/**
 * \brief Sets up the performance data gathering system.
 * Calls functions to calibrate the system timers and to initialize the global
 * table for storing performance data.
 */
void performance_init () {
        int retval;

	if (MyRank == ROOT) EmitLog(MyRank, SCHEDULER_THREAD, "Calibrating performance timers.", -1, PRINT_ALWAYS);
	ORB_calibrate();
        
        /* Initialize PAPI and PAPI threads */
        retval = PAPI_library_init(PAPI_VER_CURRENT);
        if(retval != PAPI_VER_CURRENT) handle_PAPI_error(retval);

        retval = PAPI_thread_init(pthread_self);
        if(retval != PAPI_OK) handle_PAPI_error(retval);

        /*
        retval = PAPI_create_eventset(&PAPI_EventSet);
        if(retval != PAPI_OK) handle_PAPI_error(retval);

        retval = PAPI_add_events(PAPI_EventSet, PAPI_Events, NUM_PAPI_EVENTS);
        if(retval != PAPI_OK) handle_PAPI_error(retval);

        retval = PAPI_start(PAPI_EventSet);
        if(retval != PAPI_OK) handle_PAPI_error(retval);
        */

        /* Initialize performance tables */
        perf_table_init();
}

/**
 * \brief Performs setup operations on the node's performance data table.
 */
void perf_table_init () {
	int i, j;
	for (i = 0; i < NUM_PLANS; i++) {
		for (j = 0; j < 2*NUM_TIMERS; j++) {
			perf_data_int[i][j] = 0;
			perf_data_dbl[i][j] = 0.0;
		}
		for (j = 0; j < NUM_TIMERS; j++) {
			perf_data_unit[i][j] = plan_list[i]->perf_units[j];
		}
                /* Initialize PAPI data structure */
                for(j = 0; j < NUM_PAPI_EVENTS; j++){
                    PAPI_Data[i][j] = 0LL;
                }
		pthread_rwlock_init(&perf_data_lock[i],0);
	}
}

/**
 * \brief Prints all data currently stored in the global performance data table.
 * 
 * \param [in] scope_flag Determines whether to print performance data from the local node or aggregate data from all nodes.
 * \param [in] print_priority Used to determine the verbosity level required to activate the print.
 */
/* Need PAPI here, to diplay - keep it simple stupid */
void perf_table_print (int scope_flag, int print_priority) {
	if (print_priority <= verbose_flag) {
		int i, j, k;
		char line[150], temp[128], prefixes[] = " kMGTPE";
		double timer, opcount, max, min;
		
		// Print the appropriate header to the table.
		if (scope_flag == LOCAL) {
			printf("\nPERF:\tNode %d Performance Summary:\n", MyRank);
			printf("PERF:\t %-8s %-s\n", "Plan", "Average Performance");
		} else {
			printf("\nPERF:\tAll Node Performance Summary:\n");
			printf("PERF:\t%10s", " ");
			for (i = 0; i < NUM_TIMERS; i++) {
				printf("%12c Timer %-2d %12c", ' ', i, ' ');
			}
			printf("\n");
			printf("PERF:\t %-8s ", "Plan");
			for (i = 0; i < NUM_TIMERS; i++) {
				printf("%6s / %6s / %6s %-9s", "Min ", "Ave ", "Max ", "Units");
			}
			printf("\n");
		}
		
		// Print the performance data table.
		for (i = 0; i < NUM_PLANS; i++) {
			k = -1;
			line[0] = '\0';
			for (j = 0; j < NUM_TIMERS; j++) {
				if (scope_flag == LOCAL) {
					pthread_rwlock_rdlock(&perf_data_lock[i]);
					timer   = (double)perf_data_int[i][2*j];
					opcount = (double)perf_data_int[i][2*j+1];
					pthread_rwlock_unlock(&perf_data_lock[i]);
				} else {
					pthread_rwlock_rdlock(&perf_data_lock[i]);
					timer   = perf_data_dbl[i][2*j];
					opcount = perf_data_dbl[i][2*j+1];
					max     = perf_data_max[i][j];
					min     = perf_data_min[i][j];
					pthread_rwlock_unlock(&perf_data_lock[i]);
				}
				
				// If we have a valid time, print something, otherwise, skip.
				if (timer > 0.0) {
					k = 0;
					// Get the number of seconds recorded.
					double perf = timer/ORB_REFFREQ;
					// If an opcount was taken, calculate ops/sec and get a unit prefix.
					if (opcount > 0.0) {
						perf = (double)opcount/perf;
						while (perf >= 1.0e3 && k < strlen(prefixes)) {
							perf /= 1.0e3;
							k++;
						}
					}
					if (scope_flag == LOCAL) {
						snprintf(temp, 127, "%-6.2f %c%-8s", perf, prefixes[k], perf_data_unit[i][j]);
					} else {
						max = max/pow(1.0e3, k);
						min = min/pow(1.0e3, k);
						snprintf(temp, 127, "%6.2f / %6.2f / %6.2f %c%-8s", min, perf, max, prefixes[k], perf_data_unit[i][j]);
					}
					strncat(line, temp, 150-strlen(line)-1);
				}
			}
			if (k >= 0) {
				printf("PERF:\t %-8s %s\n", plan_list[i]->name, line);
			}

                        /* Simple PAPI results print - add to main print loop */
                        for(j=0; j<NUM_PAPI_EVENTS; j++){
                            if(PAPI_Data[i][j] > 0LL)
                                printf("PAPI val #%d: %llu\n", j, PAPI_Data[i][j]);
                        }

		}
		printf("\n");
	}
}

/**
 * \brief Calls communication functions to collect performance data from all nodes.
 */
void perf_table_reduce () {
#ifdef HAVE_SHMEM
	comm_table_reduce_SHMEM(perf_data_dbl, NUM_PLANS, 2*NUM_TIMERS, REDUCE_SUM);
#else // MPI
	comm_table_reduce_MPI(perf_data_dbl, NUM_PLANS, 2*NUM_TIMERS, REDUCE_SUM);
#endif
}

/**
 * \brief Uses communication calls to determine each plan's maximum performance for the run and display that data.
 */
void perf_table_maxreduce () {
	perf_table_minmax_populate(perf_data_max, NUM_PLANS, NUM_TIMERS);
	
#ifdef HAVE_SHMEM
	comm_table_reduce_SHMEM(perf_data_max, NUM_PLANS, NUM_TIMERS, REDUCE_MAX);
#else // MPI
	comm_table_reduce_MPI(perf_data_max, NUM_PLANS, NUM_TIMERS, REDUCE_MAX);
#endif
	
// 	if (MyRank == ROOT) {
// 		perf_table_minmax_print(perf_data_max, NUM_PLANS, NUM_TIMERS, 0);
// 	}
}

/**
 * \brief Uses communication calls to determine each plan's minimum performance for the run and display that data.
 */
void perf_table_minreduce () {
	perf_table_minmax_populate(perf_data_min, NUM_PLANS, NUM_TIMERS);
	
#ifdef HAVE_SHMEM
	comm_table_reduce_SHMEM(perf_data_min, NUM_PLANS, NUM_TIMERS, REDUCE_MIN);
#else // MPI
	comm_table_reduce_MPI(perf_data_min, NUM_PLANS, NUM_TIMERS, REDUCE_MIN);
#endif
	
// 	if (MyRank == ROOT) {
// 		perf_table_minmax_print(perf_data_min, NUM_PLANS, NUM_TIMERS, 1);
// 	}
}

/**
 * \brief Calculates and collects node local performance data and stores in a table.
 *
 * \param [out] table A pre-allocated table for storing the local performance data in.
 * \param [in] nrows The number of rows in the table.
 * \param [in] ncols The number of columns in the table.
 */
void perf_table_minmax_populate(void *table, int nrows, int ncols) {
	int i, j;
	double timer, opcount;
	double *ptable = (double *)table;
	
	for (i = 0; i < nrows; i++) {
		for (j = 0; j < ncols; j++) {
			ptable[i*ncols + j] = 0.0;
			pthread_rwlock_rdlock(&perf_data_lock[i]);
			timer   = (double)perf_data_int[i][2*j];
			opcount = (double)perf_data_int[i][2*j+1];
			pthread_rwlock_unlock(&perf_data_lock[i]);
			if (timer > 0.0) {
				double temp = timer/ORB_REFFREQ;
				if (opcount > 0.0) {
					temp = opcount/temp;
				}
				ptable[i*ncols + j] = temp;
			}
		}
	}
}

/**
 * \brief Displays the minimum/maximum performance data after collection.
 *
 * \param [in] table The table containing the performance data to be printed.
 * \param [in] nrows The number of rows in the table.
 * \param [in] ncols The number of columns in the table.
 * \param [in] is_minimum Indicates whether minimum (1) or maximum values (0) are being printed.
 */
void perf_table_minmax_print(void *table, int nrows, int ncols, int is_minimum) {
	char prefixes[] = " kMGTPE";
	char  line[120], temp[30], label[4];
	double *ptable = (double *)table;
	int i, j, k;
	
	if (is_minimum) {
		strcpy(label, "Min");
	} else {
		strcpy(label, "Max");
	}
	
	printf("  %-10s %-s %-s\n", "Plan", label, "Performance");
	for (i = 0; i < nrows; i++) {
		k = -1;
		line[0] = '\0';
		for (j = 0; j < ncols; j++) {
			double perf = ptable[i*ncols + j];
			if (perf > 0.0) {
				k = 0;
				while (perf >= 1.0e3 && k < strlen(prefixes)) {
					perf /= 1.0e3;
					k++;
				}
				snprintf(temp, 30, "%-9.4f %c%-10s", perf, prefixes[k], perf_data_unit[i][j]);
				strncat(line, temp, 120-strlen(line)-1);
			}
		}
		if (k >= 0) {
			printf("  %-10s %s\n", plan_list[i]->name, line);
		}
	}
	printf("\n");
}

/**
 * \brief Adds performance data to the node's table.
 * 
 * \param [in] timers Contains accumulated timer counts for several execution sections of the plan.
 * \param [in] opcounts Contains the operation counts for the execution sections of the plan, corresponding to the timer counts.
 * \param [in] plan_id Identifies the plan whose table entry should be updated.
 */
void perf_table_update (PerfTimers *timers, uint64_t *opcounts, int plan_id, long long *results) {
	int i, retval;
	uint64_t perf_count[2*NUM_TIMERS];
	
	for (i = 0; i < NUM_TIMERS; i++) {
		perf_count[2*i] = perftimer_getcount(timers, i);
		perf_count[2*i+1] = opcounts[i];
	}
	pthread_rwlock_wrlock(&perf_data_lock[plan_id]);
	for (i = 0; i < 2*NUM_TIMERS; i++) {
		perf_data_int[plan_id][i] += perf_count[i];
		perf_data_dbl[plan_id][i] += (double)perf_count[i];
	}

        /* Update PAPI data structure */
        if(results){
            for(i=0; i<NUM_PAPI_EVENTS; i++){
                PAPI_Data[plan_id][i] += results[i];
            }
        }
        //retval = PAPI_accum(PAPI_EventSet, PAPI_Data[plan_id]);
        //if(retval != PAPI_OK) handle_PAPI_error(retval);

        pthread_rwlock_unlock(&perf_data_lock[plan_id]);
}
