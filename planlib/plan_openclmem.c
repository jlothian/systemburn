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
#include <systemburn.h> // <- Necessary to include to get the Plan struct and other neat things.
#include <planheaders.h> // <- Add your header file (plan_OPENCL_MEM.h) to planheaders.h to be included. For uniformity, do not include here, and be sure to leave planheaders.h included.

//If there are other functions to be used by your module, include them somewhere outside of the four required functions. I put them here. Examples in plan_dstream.h and plan_stream.h

// Note: the comment blocks that begin /** and contain tags such as "\brief" are Doxygen comments, used for creating an HTML manual of SystemBurn.
// Feel free to edit these descriptions (these are greatly generalized), but do not delete them, so Doxygen can document the software.

#ifdef HAVE_PAPI
//Can add counters to the list, be sure NUM_PAPI_EVENTS contains the size
//of PAPI_COUNTERS and each counter has a corresponding unit even NULL
  #define NUM_PAPI_EVENTS 1
  #define PAPI_COUNTERS { PAPI_FP_OPS }
  #define PAPI_UNITS { "FLOPS" }
#endif //HAVE_PAPI

/**
 * \brief Allocates and returns the data struct for the plan
 * \param i The struct that holds the input data.
 * \return void* Data struct
 */
void *makeOPENCL_MEMPlan(data *i){   // <- Replace YOUR_NAME with the name of your module.
    Plan *p;     // <- Plan pointer, necessary to integrate with systemburn. Do not change.
    OPENCL_MEM_DATA *ip;     // <- Change YOUR_TYPE with the name of your data type defined in the header file.
    p = (Plan *)malloc(sizeof(Plan));  // <- Allocating the necessary Plan pointer. Do not change.
    assert(p);
    if(p){      // <- Checking the Plan pointer was allocated. Do not change.
        p->fptr_initplan = &initOPENCL_MEMPlan;         // <- Replace YOUR_NAME with the name of your module.
        p->fptr_execplan = &execOPENCL_MEMPlan;         // <- Replace YOUR_NAME with the name of your module.
        p->fptr_killplan = &killOPENCL_MEMPlan;         // <- Replace YOUR_NAME with the name of your module.
        p->fptr_perfplan = &perfOPENCL_MEMPlan;
        p->name = OPENCLMEM;                           // <- Replace YOUR_NAME with the name of your module.
        ip = (OPENCL_MEM_DATA *)malloc(sizeof(OPENCL_MEM_DATA));      // <- Change YOUR_TYPE to your defined data type.
        assert(ip);
        if(ip){
          if(i->isize == 1)
          {
            ip->planner_size = i->i[0];
          } else {
            ip->planner_size = (int)i->d[0];
          }
          //if input happens, make it happen here
          //ip->opencl_data = *i;            // <- Unless is it just an int, change this so that whatever field of your type uses an int get defined here.
        }
        (p->vptr) = (void *)ip;      // <- Setting the void pointer member of the Plan struct to your data structure. Only change if you change the name of ip earlier in this function.
    }
    return p;     // <- Returning the Plan pointer. Do not change.
} /* makeOPENCL_MEMPlan */

/************************
 * This is the place where the memory gets allocated, and data types get initialized to their starting values.
 ***********************/
/**
 * \brief Creates and initializes the working data for the plan
 * \param plan The Plan struct that holds the plan's data values.
 * \return Error flag value
 */
int initOPENCL_MEMPlan(void *plan){   // <- Replace YOUR_NAME with the name of your module.
    if(!plan){
        return make_error(ALLOC, generic_err);           // <- This is the error code for one of the malloc fails.
    }
    Plan *p;
    OPENCL_MEM_DATA *d;
    p = (Plan *)plan;

    #ifdef HAVE_PAPI
    int temp_event, i;
    int PAPI_Events [NUM_PAPI_EVENTS] = PAPI_COUNTERS;
    char *PAPI_units [NUM_PAPI_EVENTS] = PAPI_UNITS;
    #endif //HAVE_PAPI

    if(p){
        d = (OPENCL_MEM_DATA *)p->vptr;
        p->exec_count = 0;           // Initialize the plan execution count to zero.
        perftimer_init(&p->timers, NUM_TIMERS);         // Initialize all performance timers to zero.

        #ifdef HAVE_PAPI
        /* Initialize plan's PAPI data */
        p->PAPI_EventSet = PAPI_NULL;
        p->PAPI_Num_Events = 0;

        TEST_PAPI(PAPI_create_eventset(&p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);

        //Add the desired events to the Event Set; ensure the dsired counters
        //  are on the system then add, ignore otherwise
        for(i = 0; i < TOTAL_PAPI_EVENTS && i < NUM_PAPI_EVENTS; i++){
            temp_event = PAPI_Events[i];
            if(PAPI_query_event(temp_event) == PAPI_OK){
                p->PAPI_Num_Events++;
                TEST_PAPI(PAPI_add_event(p->PAPI_EventSet, temp_event), PAPI_OK, MyRank, 9999, PRINT_SOME);
            }
        }

        PAPIRes_init(p->PAPI_Results, p->PAPI_Times);
        PAPI_set_units(p->name, PAPI_units, NUM_PAPI_EVENTS);

        TEST_PAPI(PAPI_start(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
        #endif     //HAVE_PAPI
    }
    if(d){
      cl_int error;
      /*
      cl_uint num_platforms = 0;
      cl_uint num_devices = 0;
      cl_platform_id *platforms = NULL;
      cl_device_id *devices = NULL;
      cl_context context = NULL;
      */
      error = clGetPlatformIDs(0, NULL,&(d->num_platforms));
      printf("error is: %i\n", error);
      assert(error == CL_SUCCESS);

      d->platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id)*d->num_platforms);
      error = clGetPlatformIDs(d->num_platforms, d->platforms, NULL);
      assert(error == CL_SUCCESS);

      error = clGetDeviceIDs(d->platforms[0],CL_DEVICE_TYPE_ALL, 0, NULL, &(d->num_devices));
      assert(error == CL_SUCCESS);

      d->devices = (cl_device_id *)malloc(sizeof(cl_device_id)*d->num_devices);
      error = clGetDeviceIDs(d->platforms[0],CL_DEVICE_TYPE_ALL, d->num_devices, d->devices, NULL);
      assert(error == CL_SUCCESS);

      d->context = clCreateContext(NULL, d->num_devices, d->devices, NULL, NULL, &error);
      assert(error == CL_SUCCESS);
        // Initialize plan specific data here.
    }
    return ERR_CLEAN;     // <- This indicates a clean run with no errors. Does not need to be changed.
} /* initOPENCL_MEMPlan */

/************************
 * This is where everything gets cleaned up. Be sure to free() your data types (free the members first) in addition to the ones included below.
 ***********************/
/**
 * \brief Frees the memory used in the plan
 * \param plan Holds the information and memory location for the plan.
 */
void *killOPENCL_MEMPlan(void *plan){   // <- Replace YOUR_NAME with the name of your module.
    Plan *p;     // <- Plan type. Do not change.
    p = (Plan *)plan;     // <- Setting the Plan pointer. Do not change.

    if(DO_PERF){
        #ifdef HAVE_PAPI
        TEST_PAPI(PAPI_stop(p->PAPI_EventSet, NULL), PAPI_OK, MyRank, 9999, PRINT_SOME);
        #endif //HAVE_PAPI
    }

    free((void *)(p->vptr));    // <- Freeing the used void pointer member of Plan. Do not change.
    free((void *)(plan));    // <- Freeing the used Plan pointer. Do not change.
    return (void *)NULL;    // <- Return statement to ensure nice exit from module.
}

/************************
 * This is where the plan gets executed. Place all operations here.
 ***********************/
/**
 * \brief <DESCRIPTION of your plan goes here..>
 * \param plan The Plan struct that holds the plan's data values.
 * \return int Error flag value
 */
int execOPENCL_MEMPlan(void *plan){  // <- Replace YOUR_NAME with the name of your module.
    #ifdef HAVE_PAPI
    int k;
    long long start, end;
    #endif //HAVE_PAPI

    ORB_t t1, t2;         // Storage for timestamps, used to accurately find the runtime of the plan execution.
    Plan *p;
    p = (Plan *)plan;
    p->exec_count++;       // Update the execution counter stored in the plan.

    #ifdef HAVE_PAPI
    /* Start PAPI counters and time */
    TEST_PAPI(PAPI_reset(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
    start = PAPI_get_real_usec();
    #endif //HAVE_PAPI

    ORB_read(t1);         // Store the timestamp for the beginning of the execution.

    // --------------------------------------------
    // Plan is executed here...
    // --------------------------------------------

    ORB_read(t2);         // Store timestamp for the end of execution.

    #ifdef HAVE_PAPI
    end = PAPI_get_real_usec();     //PAPI time

    /* Collect PAPI counters and store time elapsed */
    TEST_PAPI(PAPI_accum(p->PAPI_EventSet, p->PAPI_Results), PAPI_OK, MyRank, 9999, PRINT_SOME);
    for(k = 0; k < p->PAPI_Num_Events && k < TOTAL_PAPI_EVENTS; k++){
        p->PAPI_Times[k] += (end - start);
    }
    #endif //HAVE_PAPI

    perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));      // Store the difference between the timestamps in the plan's timers.

    if(CHECK_CALC){           // Evaluates to true if the '-t' option is passed on the commandline.
        ORB_read(t1);

        // ----------------------------------------------------------------
        // Optional: Check calculations performed in execution above.
        // ----------------------------------------------------------------

        ORB_read(t2);
        perftimer_accumulate(&p->timers, TIMER1, ORB_cycles_a(t2, t1));
    }

    return ERR_CLEAN;     // <- This inicates a clean run with no errors. Does not need to be changed.
} /* execOPENCL_MEMPlan */

/**
 * \brief Calculate and store performance data for the plan.
 * \param plan The Plan structure that holds the plan's data.
 * \returns Success or failure in calculating the performance.
 */
int perfOPENCL_MEMPlan(void *plan){
    int ret = ~ERR_CLEAN;
    uint64_t opcounts[NUM_TIMERS];
    Plan *p;
    OPENCL_MEM_DATA *d;
    p = (Plan *)plan;
    d = (OPENCL_MEM_DATA *)p->vptr;
    if(p->exec_count > 0){        // Ensures the plan has been executed at least once...
        // Assign appropriate plan-specific operation counts to the opcount[] array, such that the
        // indices correspond with the timers used in the exec function.
      opcounts[TIMER0] = p->exec_count; //* YOUR_OPERATIONS_PER_EXECUTION;         // Where operations can be a function of the input size.

        perf_table_update(&p->timers, opcounts, p->name);          // Updates the global table with the performance data.
        #ifdef HAVE_PAPI
        PAPI_table_update(p->name, p->PAPI_Results, p->PAPI_Times, p->PAPI_Num_Events);
        #endif     //HAVE_PAPI

        double flops = ((double)opcounts[TIMER0] / perftimer_gettime(&p->timers, TIMER0)) / 1e6;       // Example for computing MFLOPS
        EmitLogfs(MyRank, 9999, "YOUR_PLAN plan performance:", flops, "MFLOPS", PRINT_SOME);                   // Displays calculated performance when the '-v2' command line option is passed.
        EmitLog  (MyRank, 9999, "YOUR_PLAN execution count :", p->exec_count, PRINT_SOME);
        ret = ERR_CLEAN;
    }
    return ret;
} /* perfOPENCL_MEMPlan */

/************************
 * This is the parsing function that the program uses to read in your load information from the load file. This lets you have a custom number of int arguments for your module while still maintaining modularity.
 ***********************/
/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param line The line of input text.
 * \param output The struct that holds the values for the load.
 * \return True if the data was read, false if it wasn't
 */
int parseOPENCL_MEMPlan(char *line, LoadPlan *output){  // <- Replace YOUR_NAME with the name of your module.
    output->input_data = get_sizes(line);
    output->name = OPENCLMEM;     // <- Replace YOUR_NAME with the name of your module.
    output->input_data;     // <- Change NUMBER_OF_YOUR_MEMBERS to equal however many int fields your data type holds.
    return output->input_data->isize + output->input_data->csize + output->input_data->dsize;     // <- Return flag to know whether the parsing succeeded or not. Do not change.
}

/**
 * \brief Holds the custom error messages for the plan
 */
// This is not necessary; if you do not have any errors other than allocation or computation, remove this and leave its spot NULL in the plan_info stuct below
char *PLAN_OPENCL_ERRORS[] = {
    "ERROR message goes here"
};

plan_info OPENCL_MEM_info = {
    "OPENCL_MEM",     //OPENCL_MEM
    NULL,     //YOUR_ERRORS (if applicable. If not, leave NULL.)
    0,     // Size of ^
    makeOPENCL_MEMPlan,
    parseOPENCL_MEMPlan,
    execOPENCL_MEMPlan,
    initOPENCL_MEMPlan,
    killOPENCL_MEMPlan,
    perfOPENCL_MEMPlan,
    { NULL, NULL, NULL }     //YOUR_UNITS strings naming the units of each timer value (leave NULL if unneeded)
};
