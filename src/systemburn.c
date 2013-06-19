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
#include <load.h>
#include <initialization.h>
#include <comm.h>
#include <performance.h>
#include <planheaders.h>

/****************************************************************************//**
 * \file systemburn.c
 * \brief SystemBurn is a fully configureable threaded stress-test benchmark.
 * The system is arranged as an SPMD program. For the purposes of SystemBurn, we
 * presume to launch only one process per "node", where a "node" is defined
 * simply as a group of processor cores which share a single NUMA address space.
 * We permit the possibility that there may be multiple logical nodes per physical
 * node by this definition.
 *
 * Thus on each node we have one or more processor cores and we will load them
 * with one or more worker threads. If system monitoring capabilities are
 * available, a monitor thread will run in parallel with the worker threads. The
 * main program from which these threads are launched will serve two purposes:
 * that of the communication thread, and that of the scheduler/event handler
 * thread.
 *******************************************************************************/
int MyRank;
ThreadHandle *WorkerHandle;
ThreadHandle MonitorHandle;
TemperatureRange local_temp;

int comm_flag;
int verbose_flag;
int plancheck_flag;
int planperf_flag = 1;
int num_workers;
int thermal_panic;
int thermal_relaxation_time;
int monitor_frequency;
int monitor_output_frequency;
char temperature_path[ARRAY];

/**
 * \brief Main..
 */
int main(int argc, char **argv, char **envp){
    int i, last;
    Load load_data;
    char **load_names = NULL;
    int load_filesize = 0;
    char *load_buffer = NULL;
    int config_filesize = 0;
    char *config_buffer = NULL;
    char *config_file = NULL;
    char *log_file = NULL;
    int num_loads = 0;
    struct timeval StartTime, CurrentTime;

    int **errorFlags;
    int err = ERR_CLEAN;
    int pflag = 0;
    Plan *CommPlan;
    int iflag;
    unsigned int nap;

    comm_setup(&argc, &argv);
    MyRank = comm_getrank();

    last = 0;

    num_loads = initialize(argc, argv, &log_file, &config_file, &load_names);

    /* Initialize ROOT's global variables using input files. */
    if(MyRank == ROOT){
        config_filesize = initConfigOptions(config_file, &config_buffer);         /* parse configuration file. */
        if((config_filesize <= 0) || (config_buffer == NULL) ){
            EmitLog(MyRank, SCHEDULER_THREAD, "Aborting run - A config file could not be opened/read.", -1, PRINT_ALWAYS);
            config_filesize = 0;
            if(config_buffer != NULL){
                free(config_buffer);
            }
        }
        comm_broadcast_int(&config_filesize);
    } else {
        comm_broadcast_int(&config_filesize);
        config_buffer = getFileBuffer(config_filesize);
    }

    if(config_filesize == 0){
        comm_finalize();
        exit(1);
    }

    err = broadcast_buffer(config_buffer, config_filesize);

    err += parseConfig(config_buffer, config_filesize);
    free(config_buffer);

    if(MyRank == ROOT){
        if(PRINT_RARELY <= verbose_flag){           /* Print status info. */
            printf("num_loads                = %d\n", num_loads);
            printf("num_workers              = %d\n", num_workers);
            printf("thermal_panic            = %d\n", thermal_panic);
            printf("thermal_relaxation_time  = %d\n", thermal_relaxation_time);
            printf("monitor_frequency        = %d\n", monitor_frequency);
            printf("monitor_output_frequency = %d\n", monitor_output_frequency);
            printf("temperature_path         = %s\n", temperature_path);
            for(i = 0; i < num_loads; i++){
                printf("load_names[%d]           = %s\n", i, load_names[i]);
            }
            printf("log_file                 = %s\n", log_file);
        }
    }

    // num_loads = bcastConfig(num_loads); // Broadcast global variables from ROOT to all others.
    if(num_loads <= 0){                     // If there are no loads to run, exit the program.
        comm_finalize();
        exit(0);
    }

    /* Initialize the communication load if it is to be run */
    if(comm_flag != 0){
        data pass;
        pass.isize = 1;
        pass.i = &comm_flag;
        CommPlan = makeCommPlan(&pass);
    } else {
        CommPlan = 0;
    }
    if(CommPlan != 0){
        iflag = (CommPlan->fptr_initplan)(CommPlan);
    }

    /* Initialize the array of ThreadHandle structures for the workers. */
    WorkerHandle = (ThreadHandle *)malloc(num_workers * sizeof(ThreadHandle));
    if(WorkerHandle == NULL){
        EmitLog(MyRank, SCHEDULER_THREAD, "Aborting run - Insufficient memory for the WorkerHandle struct", -1, PRINT_ALWAYS);
        comm_finalize();
        exit(1);
    }

    errorFlags = initErrorFlags();
    initWorkerFlags();

    if(DO_PERF){
        performance_init();
    }     //DO_PERF

    if(MyRank == ROOT){
        EmitLog(MyRank, SCHEDULER_THREAD, "Initialization complete. Beginning run.", -1, PRINT_ALWAYS);
    }

    StartMonitorThread();
    StartWorkerThreads();

    sleep(thermal_relaxation_time);     /* idle for a baseline */
    reduceTemps();
    sleep(thermal_relaxation_time);

    /*********************************************************************************
    * The following code will subscribe plans and their sizes to worker threads.
    * This is a temporary load distribution being that it comes from a single load.
    * In the future we would like to have several loads lined up to distribute to the
    * worker threads in a simlar fashion. This will allow us to run the benchmark
    * for different time durations depending on the total load schedule.
    *********************************************************************************/
    nap = monitor_output_frequency / 4;
    if(nap < 1){
        nap = 1;
    }
    for(i = 0; i < num_loads; i++){
        if(MyRank == ROOT){                                     // Pull load data from a load file.
            assert(load_names);
            load_filesize = initLoadOptions(load_names[i], &load_buffer);
            if((load_filesize <= 0) || (load_buffer == NULL) ){
                // /* Redundant */fprintf(stderr, "This load file could not be opened/read... trying next (if available).\n");
                EmitLog(MyRank, SCHEDULER_THREAD, "This load file could not be opened/read... trying next (if available).", -1, PRINT_ALWAYS);
                load_filesize = 0;
                if(load_buffer != NULL){
                    free(load_buffer);
                }
            }
            comm_broadcast_int(&load_filesize);
            if(load_filesize == 0){
                continue;
            }
        } else {
            comm_broadcast_int(&load_filesize);
            if(load_filesize == 0){
                continue;
            }
            load_buffer = getFileBuffer(load_filesize);
        }
        err = broadcast_buffer(load_buffer, load_filesize);
        err += parseLoad(load_buffer, &load_data);
        free(load_buffer);
        // err = bcastLoad(&load_data);             // Broadcast the load structure to all nodes (processes).
        err = WorkerSched(&load_data);                                  // Assign the load to worker threads and check for errors
        if(MyRank == ROOT){
            printLoad(&load_data);                                      // Print the load data to the terminal.
        }
        if(err != ERR_CLEAN){
            errorFlags[SYSTEM + 1][err]++;
        }
#define SB_CONTINUE      0x0
#define SB_LAST_TRIP     0x1
#define SB_DO_REDUCTIONS 0x2
        if(MyRank == ROOT){                                             // ROOT notes when we start this load
            gettimeofday(&StartTime, NULL);
            last = StartTime.tv_sec;
        }
        do {            // DELAY WHILE LOAD RUNS: loop while the load executes until ROOT's clock says stop.  Sleep if CommPlan isn't valid.
            if((comm_flag != 0) && (CommPlan) && (CommPlan->fptr_execplan) && (CommPlan->vptr)){
                iflag = (CommPlan->fptr_execplan)(CommPlan);                             // run an iteration of the comm plan if enabled
            } else {
                gettimeofday(&CurrentTime, NULL);
                if(nap + CurrentTime.tv_sec < StartTime.tv_sec + load_data.runtime){
                    sleep(nap);
                } else {
                    sleep((StartTime.tv_sec + load_data.runtime) - CurrentTime.tv_sec);
                }
            }
            if(MyRank == ROOT){
                gettimeofday(&CurrentTime, NULL);
                pflag = ((CurrentTime.tv_sec > last + monitor_output_frequency) << 1) | (CurrentTime.tv_sec < StartTime.tv_sec + load_data.runtime);
            }
            comm_broadcast_int(&pflag);
            if(pflag & SB_DO_REDUCTIONS){
                assert(errorFlags);
                reduceFlags(errorFlags);
                reduceTemps();
                if(MyRank == ROOT){
                    last = CurrentTime.tv_sec;
                }
            }
        } while(pflag & SB_LAST_TRIP);
        // LOAD COMPLETE
        if(MyRank == ROOT){
            EmitLog(MyRank, SCHEDULER_THREAD, "Elapsed time for this load:", CurrentTime.tv_sec - StartTime.tv_sec, PRINT_ALWAYS);
        }
        freeLoad(&load_data);
    }
    sleep(thermal_relaxation_time);
    reduceTemps();
    StopWorkerThreads();                        // tell them all to finish

    // Clean up the Communication Plan
    if((comm_flag != 0) && (CommPlan) && (CommPlan->vptr)){
        if(CommPlan->fptr_perfplan){
            iflag = (CommPlan->fptr_perfplan)(CommPlan);
        }
        if(CommPlan->fptr_killplan){
            CommPlan = (CommPlan->fptr_killplan)(CommPlan);
        }
    }

    sleep(thermal_relaxation_time);

    if(DO_PERF){
        sleep(30);
        perf_table_print(LOCAL, PRINT_OFTEN);
        perf_table_reduce();

        perf_table_maxreduce();
        perf_table_minreduce();

        if(MyRank == ROOT){
            perf_table_print(GLOBAL, PRINT_ALWAYS);
        }
    } //DO_PERF

    if(MyRank == ROOT){
        EmitLog(MyRank, SCHEDULER_THREAD, "Run Completed. Exiting.", -1, PRINT_ALWAYS);
    }

    comm_finalize();
    exit(0);
} /* main */

