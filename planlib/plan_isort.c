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
//Can add counters to the list, be sure NUM_PAPI_EVENTS contains the size 
//of PAPI_COUNTERS and each counter has a corresponding unit even NULL
#define NUM_PAPI_EVENTS 1 
#define PAPI_COUNTERS { PAPI_FP_OPS } 
#define PAPI_UNITS { "FLOPS" } 
#endif //HAVE_PAPI

/**
 * \brief Swap two elements of an array
 * \param [in] array holds an array of integers
 * \param [in] idx1 is the first array index to swap
 * \param [in] idx2 is the second array index to swap
*/
void isort_swap(uint64_t *array, int idx1, int idx2){
        uint64_t t;
        t = array[idx1];
        array[idx1] = array[idx2];
        array[idx2] = t;
}

/** 
 * \brief partition an array
 * \param [in] array holds an array of integers
 * \param [in] left is the index of the leftmost element of the partition
 * \param [in] right is the index of the rightmost element of the partition
 * \param [in] pivot_index is the index of the element you wish to pivot on
*/
int isort_partition(uint64_t *array, int left, int right, int pivot_index){
        int i, st_index;
        uint64_t pivot_val;
        pivot_val = array[pivot_index];
        isort_swap(array, pivot_index, right); // move pivot to end of partition
        st_index = left;
        for(i=left; i<=(right -1); i++){
                if(array[i] < pivot_val){
                        isort_swap(array, i, st_index);
                        st_index++;
                }
        }
        isort_swap(array, st_index, right);
        return st_index;
}

/**
 * \brief run the quicksort algorithm on an array of integers
 * \param [in] array holds an array of integers
 * \param [in] left is the index of the leftmost partition of the array you want to sort
 * \param [in] right is the index of the rightmost partition of the array you want to sort
*/
int isort_quicksort(uint64_t *array, int left, int right) {
        int pivot_index, new_pivot_index;
        if(left < right) { // len(list) > 2
                pivot_index = (left+right)/2;
                new_pivot_index = isort_partition(array, left, right, pivot_index);
                isort_quicksort(array, left, new_pivot_index - 1);
                isort_quicksort(array, new_pivot_index+ 1, right);
        }
}


/**
 * \brief Allocates and returns the data struct for the plan
 * \param i The struct that holds the input data.
 * \return void* Data struct
 */
void * makeISORTPlan(data *i) {
	Plan *p;
	ISORTdata *ip;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initISORTPlan; 
		p->fptr_execplan = &execISORTPlan;
		p->fptr_killplan = &killISORTPlan;
		p->fptr_perfplan = &perfISORTPlan;
		p->name          = ISORT;
		ip=(ISORTdata*)malloc(sizeof(ISORTdata)); 
		assert(ip);
		if(ip) {
                        if(i->isize == 1){
                                ip->array_size = i->i[0]/sizeof(uint64_t); //
                        } else {
                                ip->array_size = i->d[0]/sizeof(uint64_t); //
                        }
                }
                (p->vptr)=(void*)ip; // <- Setting the void pointer member of the Plan struct to your data structure. Only change if you change the name of ip earlier in this function.
	}
	return p; // <- Returning the Plan pointer. Do not change.
}

/************************
 * This is the place where the memory gets allocated, and data types get initialized to their starting values.
 ***********************/
/**
 * \brief Creates and initializes the working data for the plan
 * \param plan The Plan struct that holds the plan's data values.
 * \return Error flag value
 */
int  initISORTPlan(void *plan) { // <- Replace ISORT with the name of your module.
	if(!plan) return make_error(ALLOC, generic_err); // <- This is the error code for one of the malloc fails.
	Plan *p;
	ISORTdata *d;
	p = (Plan *)plan;

    #ifdef HAVE_PAPI
        int temp_event, i;
        int PAPI_Events [NUM_PAPI_EVENTS] = PAPI_COUNTERS;
        char* PAPI_units [NUM_PAPI_EVENTS] = PAPI_UNITS;
    #endif //HAVE_PAPI

	if (p) {
		d = (ISORTdata *)p->vptr;
		p->exec_count = 0;   // Initialize the plan execution count to zero.
                
                if(DO_PERF){
		    perftimer_init(&p->timers, NUM_TIMERS); // Initialize all performance timers to zero.

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
	if (d) {
                d->numbers = (uint64_t *)malloc(d->array_size*sizeof(uint64_t));
	}
	return ERR_CLEAN; // <- This indicates a clean run with no errors. Does not need to be changed.
}

/************************
 * This is where the plan gets executed. Place all operations here.
 ***********************/
/**
 * \brief <DESCRIPTION of your plan goes here..> 
 * \param plan The Plan struct that holds the plan's data values.
 * \return int Error flag value
 */
int execISORTPlan(void *plan) { // <- Replace ISORT with the name of your module.
    #ifdef HAVE_PAPI
        int k;
        long long start, end;
    #endif //HAVE_PAPI

        int64_t i;
	ORB_t t1, t2;     // Storage for timestamps, used to accurately find the runtime of the plan execution.
	Plan *p;
        ISORTdata *d;
	p = (Plan *)plan;
        d = (ISORTdata *)p->vptr;
	p->exec_count++;   // Update the execution counter stored in the plan.
	
        srand48(0);
        // seed the array with random numbers	
        for(i=0; i<d->array_size; i++){
                d->numbers[i] = lrand48();
        }
	
        if(DO_PERF){
        #ifdef HAVE_PAPI
            /* Start PAPI counters and time */
            TEST_PAPI(PAPI_reset(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
            start = PAPI_get_real_usec();
        #endif //HAVE_PAPI

	    ORB_read(t1);     // Store the timestamp for the beginning of the execution.
        } //DO_PERF

        fprintf(stderr, "Starting sort\n");
        for(i=0; i<d->array_size;i++){
                fprintf(stderr," %llu\n",d->numbers[i]);
        }
        fprintf(stderr,"\n");
        isort_quicksort(d->numbers,0,d->array_size-1);
        //qsort(d->numbers,d->array_size,sizeof(uint64_t),isort_cmp);
        fprintf(stderr, "Done sort\n");
        for(i=0; i<d->array_size;i++){
                fprintf(stderr," %llu\n",d->numbers[i]);
        }
        fprintf(stderr,"\n");
        sleep(5);
	
        if(DO_PERF){
	    ORB_read(t2);     // Store timestamp for the end of execution.

        #ifdef HAVE_PAPI
            end = PAPI_get_real_usec(); //PAPI time

            /* Collect PAPI counters and store time elapsed */
            TEST_PAPI(PAPI_accum(p->PAPI_EventSet, p->PAPI_Results), PAPI_OK, MyRank, 9999, PRINT_SOME);
            for(k=0; k<p->PAPI_Num_Events && k<TOTAL_PAPI_EVENTS; k++){
                p->PAPI_Times[k] += (end - start);
            }
        #endif //HAVE_PAPI

	    perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));  // Store the difference between the timestamps in the plan's timers.
	} //DO_PERF

	if (CHECK_CALC) {     // Evaluates to true if the '-t' option is passed on the commandline.
		if(DO_PERF){
                    ORB_read(t1);
                } //DO_PERF
		
		// ----------------------------------------------------------------
		// Optional: Check calculations performed in execution above.
		// ----------------------------------------------------------------
		
                if(DO_PERF){
		    ORB_read(t2);
		    perftimer_accumulate(&p->timers, TIMER1, ORB_cycles_a(t2, t1));
                } //DO_PERF
	}
	
	return ERR_CLEAN; // <- This inicates a clean run with no errors. Does not need to be changed.
}

/************************
 * This is where everything gets cleaned up. Be sure to free() your data types (free the members first) in addition to the ones included below.
 ***********************/
/**
 * \brief Frees the memory used in the plan
 * \param plan Holds the information and memory location for the plan.
 */
void * killISORTPlan(void *plan) {
	Plan *p; 
        ISORTdata *d;
	p = (Plan *)plan;

        if(DO_PERF){
        #ifdef HAVE_PAPI
            TEST_PAPI(PAPI_stop(p->PAPI_EventSet, NULL), PAPI_OK, MyRank, 9999, PRINT_SOME);
        #endif //HAVE_PAPI
        } //DO_PERF

        d = (ISORTdata *)(p->vptr);
        if(d->numbers){
                free(d->numbers);
        }
	free((void*)(p->vptr));
	free((void*)(plan));
        
	return (void*)NULL;
}

/************************
 * This is the parsing function that the program uses to read in your load information from the load file. This lets you have a custom number of int arguments for your module while still maintaining modularity.
 ***********************/
/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param line The line of input text.
 * \param output The struct that holds the values for the load.
 * \return True if the data was read, false if it wasn't
 */
int parseISORTPlan(char *line, LoadPlan *output) { // <- Replace ISORT with the name of your module.
	output->input_data = get_sizes(line);
        output->name = ISORT; // <- Replace ISORT with the name of your module.
        output->input_data; // <- Change NUMBER_OF_YOUR_MEMBERS to equal however many int fields your data type holds.
	return output->input_data->isize + output->input_data->csize + output->input_data->dsize; // <- Return flag to know whether the parsing succeeded or not. Do not change.
}

/**
 * \brief Calculate and store performance data for the plan.
 * \param plan The Plan structure that holds the plan's data.
 * \returns Success or failure in calculating the performance.
 */
int perfISORTPlan(void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	ISORTdata *d;
	p = (Plan *)plan;
	d = (ISORTdata *)p->vptr;
	if (p->exec_count > 0) {  // Ensures the plan has been executed at least once...
		// Assign appropriate plan-specific operation counts to the opcount[] array, such that the 
		// indices correspond with the timers used in the exec function.
                // FIXME: figure out real operation count, if applicable

		opcounts[TIMER0] = p->exec_count * 1; // Where operations can be a function of the input size.
		
		perf_table_update(&p->timers, opcounts, p->name);  // Updates the global table with the performance data.
            #ifdef HAVE_PAPI
		PAPI_table_update(p->name, p->PAPI_Results, p->PAPI_Times, p->PAPI_Num_Events);
            #endif //HAVE_PAPI
		
		double flops  = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;  // Example for computing MFLOPS
		EmitLogfs(MyRank, 9999, "YOUR_PLAN plan performance:", flops, "MFLOPS", PRINT_SOME);           // Displays calculated performance when the '-v2' command line option is passed.
		EmitLog  (MyRank, 9999, "YOUR_PLAN execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Holds the custom error messages for the plan
 */
// This is not necessary; if you do not have any errors other than allocation or computation, remove this and leave its spot NULL in the plan_info stuct below
char* YOUR_ERRORS[] = {
	"ERROR message goes here"
};

plan_info ISORT_info = {
	"ISORT", //ISORT
	NULL, //YOUR_ERRORS (if applicable. If not, leave NULL.)
	0, // Size of ^
	makeISORTPlan,
	parseISORTPlan,
	execISORTPlan,
	initISORTPlan,
	killISORTPlan,
	perfISORTPlan,
	{ NULL, NULL, NULL } //YOUR_UNITS strings naming the units of each timer value (leave NULL if unneeded)
};
