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
#define MESSAGE_DECL
#define BUILD_MSG
#include <systemburn.h>
#include <planheaders.h>
#include <plan_sleep.h>
#include <initialization.h>
#include <comm.h>

/* \brief
 * Life Cycle of a Plan
 * Plans live in shared memory (globally visible on the node to any thread with a pointer to it).
 * Plans are created by the controller (boss) thread and ownership of the plan is passed to the worker thread.
 * the controller/boss thread is responsible for initializing the Plan into a sane state before handing it off.
 * The worker thread is responsible for periodically checking to see if his plan has been updated.
 * If the worker thread's plan has been updated, the worker should close-out his previous plan and start the new one
 *   - Closing out a previous plan includes freeing any resources which have been allocated for the plan.
 *   - Beginning a new plan includes an resource allocation and initialization required for the new plan.
 */

/** \brief Initializes the plan using the plan's init_Plan function.
   \param p Pointer to the plan being run by the worker.
 */
inline int InitPlan(Plan *p){
    if( p != NULL){
        return ((p->fptr_initplan)((void *)p));
    }
    return BAD;
}

/** \brief Executes the plan using the plan's exec_Plan function.
   \param p Pointer to the plan being run by the worker.
 */
inline int runPlan(Plan *p){
    if( p != NULL){
        return ((p->fptr_execplan)((void *)p));
    }
    return BAD;
}

/** \brief Stores and displays performance data for the plan with the plan's perf function.
 *  \param p Pointer to the plan being run by the worker.
 */
inline int perfPlan(Plan *p){
    if((p != NULL) && (p->fptr_perfplan != NULL) ){
        return ((p->fptr_perfplan)((void *)p));
    }
    return BAD;
}

/** \brief Kills the plan using the plan's kill_Plan function.
   \param p Pointer to the plan being run by the worker.
 */
inline void *killPlan(Plan *p){
    if( p != NULL){
        (p->fptr_killplan)((void *)p);
    }
    return (void *)NULL;
}

/** \brief Start up the worker threads that run the plans that compose the workforce of SystemBurn */
void StartWorkerThreads(){
    int i, one = 1;
    data p[1];
    p->i = &one;

    /* construct worker thread's control structure start them with sleep */
    for(i = 0; i < num_workers; i++){
        pthread_rwlock_init(&(WorkerHandle[i].Lock),0);
        pthread_attr_init(&(WorkerHandle[i].Attr));
        WorkerHandle[i].Num = i;
        WorkerHandle[i].Plan = (plan_list[SLEEP]->make)(p);
        pthread_create(&(WorkerHandle[i].ID), &(WorkerHandle[i].Attr), WorkerThread, &(WorkerHandle[i]));
        EmitLog(MyRank, SCHEDULER_THREAD, "Starting Worker Thread",WorkerHandle[i].Num, PRINT_OFTEN);
        #ifdef ASYNC_WORKERS
        pthread_detach(WorkerHandle[i].ID);
        #endif
    }
    return;
} /* StartWorkerThreads */

/** \brief Stop the workers */
void StopWorkerThreads(){
    int i;
    /* tell them all to finish */
    for(i = 0; i < num_workers; i++){
        EmitLog(MyRank, SCHEDULER_THREAD, "Stopping Worker Thread",WorkerHandle[i].Num, PRINT_OFTEN);
        pthread_rwlock_wrlock(&(WorkerHandle[i].Lock));
        WorkerHandle[i].Plan = NULL;
        pthread_rwlock_unlock(&(WorkerHandle[i].Lock));
        #ifndef ASYNC_WORKERS
        pthread_join(WorkerHandle[i].ID, NULL);
        #endif
    }
    return;
}

//#include<linux/getcpu.h>
//#include<sys/syscall.h>

/** \brief The main function for the worker threads, which determines what they run and what inputs they receive.
   \param p Pointer to the ThreadHandle struct that holds the information for the worker thread
 */
void *WorkerThread(void *p){
    long cpucoreid, numcpucores;
    int init_flag, run_flag, perf_flag;
    int one = 1;
    data sleep_pass[1];
    sleep_pass->i = &one;
    ThreadHandle *MyHandle = (ThreadHandle *)p;
    Plan *WorkerPlan, *BossPlan;
    #ifdef LINUX_PLACEMENT
    int affin_flag;
    cpucoreid = sched_getcpu();
    //syscall(__NR_getcpu(&cpucoreid, NULL, NULL));
    numcpucores = sysconf(_SC_NPROCESSORS_ONLN);
    EmitLog(MyRank, MyHandle->Num, "Starting on processor core", cpucoreid, PRINT_SOME);
    #else
    cpucoreid = -1;
    numcpucores = -1;
    EmitLog(MyRank, MyHandle->Num, "Starting...", -1, PRINT_SOME);
    #endif
    WorkerPlan = NULL;
    for(;; ){
        pthread_rwlock_rdlock( &(MyHandle->Lock) );
        BossPlan = MyHandle->Plan;
        pthread_rwlock_unlock( &(MyHandle->Lock) );
        if(BossPlan == NULL){
            if(DO_PERF){
                EmitLog(MyRank, MyHandle->Num, "Printing performance data.", -1, PRINT_SOME);
                perf_flag = perfPlan(WorkerPlan);
                if(perf_flag != ERR_CLEAN){
                    EmitLog(MyRank, MyHandle->Num, "Performance recording error flag triggered, error number:", perf_flag, PRINT_SOME);
                }
            }             //DO_PERF
            EmitLog(MyRank, MyHandle->Num, "Thread exiting", -1, PRINT_SOME);
            fprintf(stderr,"Killing a plan!\n");
            WorkerPlan = killPlan(WorkerPlan);                          /* clean up old plan       */
            fprintf(stderr,"... done Killing a plan!\n");
            pthread_exit((void *)0);
        } else {
            if(BossPlan != WorkerPlan){                                 /* if the plan was updated */
                if((WorkerPlan != NULL) && (WorkerPlan->name != SLEEP) ){
                    if(DO_PERF){
                        EmitLog(MyRank, MyHandle->Num, "Printing performance data.", -1, PRINT_SOME);
                        perf_flag = perfPlan(WorkerPlan);
                        if(perf_flag != ERR_CLEAN){
                            EmitLog(MyRank, MyHandle->Num, "Performance recording error flag triggered, error number:", perf_flag, PRINT_SOME);
                        }
                    }                     //DO_PERF
                }
                #ifdef LINUX_PLACEMENT
                numcpucores = sysconf(_SC_NPROCESSORS_ONLN);
                cpucoreid = sched_getcpu();
                //syscall(__NR_getcpu(&cpucoreid, NULL, NULL));

                cpu_set_t cpuset;
                affin_flag = pthread_getaffinity_np(MyHandle->ID, sizeof(cpu_set_t), &cpuset);
                if(affin_flag != 0){
                    add_error(MyHandle,SYSTEM,2);
                }

                EmitLog(MyRank, MyHandle->Num, "New plan detected. Switching plans on core", cpucoreid, PRINT_SOME);
                if(PRINT_RARELY <= verbose_flag){
                    int i;
                    EmitLog(MyRank, MyHandle->Num, "CPU set for this thread:", -1, PRINT_SOME);
                    printf("\t\t\t\t");
                    for(i = 0; i < numcpucores; i++){
                        if(CPU_ISSET(i, &cpuset)){
                            printf(" %d", i);
                        }
                    }
                    printf("\n");
                }
                #else /* ifdef LINUX_PLACEMENT */
                EmitLog(MyRank, MyHandle->Num, "New plan detected. Switching plans.", -1, PRINT_SOME);
                #endif /* ifdef LINUX_PLACEMENT */
                WorkerPlan = killPlan(WorkerPlan);                              /*     clean up old plan   */
                WorkerPlan = BossPlan;                                          /*     switch plans        */

                init_flag = InitPlan(WorkerPlan);                               /*     initialize new plan */
                if(init_flag != ERR_CLEAN){
                    add_error(MyHandle, WorkerPlan->name,init_flag);
                    EmitLog(MyRank, MyHandle->Num, "Initialization error flag triggered, error number:", init_flag, PRINT_ALWAYS);
                    MyHandle->Plan = (plan_list[SLEEP]->make)(sleep_pass);
                    continue;
                }
            }
            run_flag = runPlan(WorkerPlan);
            if(run_flag != ERR_CLEAN){
                add_error(MyHandle, WorkerPlan->name,run_flag);
                //MyHandle->Plan = (plan_list[SLEEP]->make)(sleep_pass);
                EmitLog(MyRank, MyHandle->Num, "Runtime error flag triggered, error number:", run_flag, PRINT_ALWAYS);
            }
        }
    }
    return((void *)0);     /* not reached */
} /* WorkerThread */

/** \brief This function initializes the WorkerHandle structure's flags to zero. */
void initWorkerFlags(){
    int i, j, k;
    for(i = 0; i < num_workers; i++){
        WorkerHandle[i].Flag = (int **)malloc(sizeof(int *) * ERR_FLAG_SIZE);
        assert(WorkerHandle[i].Flag);
        // Allocate storage for "System" (worker thread management) errors.
        WorkerHandle[i].Flag[0] = (int *)calloc((SYS_ERR_SIZE), sizeof(int));
        assert(WorkerHandle[i].Flag[0]);
        // Allocate storage for Plan errors.
        for(j = 1; j < ERR_FLAG_SIZE; j++){
            WorkerHandle[i].Flag[j] = (int *)malloc(sizeof(int) * (plan_list[j - 1]->esize + GEN_SIZE));
            assert(WorkerHandle[i].Flag[j]);
            for(k = 0; k < plan_list[j - 1]->esize + GEN_SIZE; k++){
                WorkerHandle[i].Flag[j][k] = 0;
            }
        }
    }
} /* initWorkerFlags */

/** \brief Sets up the error flags for scheduler thread. */
int **initErrorFlags(){
    int i, j;
    int **flags = NULL;

    flags = (int **)malloc(sizeof(int *) * (size_t)ERR_FLAG_SIZE);
    assert(flags);
    // Allocate storage for System errors.
    flags[0] = (int *)calloc((SYS_ERR_SIZE), sizeof(int));
    assert(flags[0]);
    // Allocate storage for Plan errors.
    for(i = 1; i < ERR_FLAG_SIZE; i++){
        flags[i] = (int *)malloc(sizeof(int) * (size_t)(GEN_SIZE + plan_list[i - 1]->esize));
        assert(flags[i]);
        for(j = 0; j < plan_list[i - 1]->esize + GEN_SIZE; j++){
            flags[i][j] = 0;
        }
    }
    return flags;
} /* initErrorFlags */

/**
 * \brief This function uses a reduction routine to gather and summarize error flags from all SPMD processes.
 * \param local_flag The array of error flags that each worker holds, telling SystemBurn which errors were encountered and how often
 */
void reduceFlags(int **local_flag){
    /* uses either of two comm libraries, functions located in comm.c */
    collectLocalFlags(local_flag);
    #ifdef HAVE_SHMEM
    reduceFlags_SHMEM(local_flag);
    #else
    reduceFlags_MPI(local_flag);
    #endif
}

/**
 * \brief Collects all error flags from the worker threads within a node.
 * \param [out] local_flag A 2D array of error flags to which is added new flags collected from the workers.
 */
void collectLocalFlags(int **local_flag){
    int i, j, k;

    for(i = 0; i < num_workers; i++){
        pthread_rwlock_wrlock(&(WorkerHandle[i].Lock));
        // Collect System flags first
        j = 0;
        for(k = 0; k < SYS_ERR_SIZE; k++){
            local_flag[j][k] += WorkerHandle[i].Flag[j][k];
            WorkerHandle[i].Flag[j][k] = 0;
        }
        // Collect Plan flags
        for(j = 1; j < ERR_FLAG_SIZE; j++){
            for(k = 0; k < plan_list[j - 1]->esize + GEN_SIZE; k++){
                local_flag[j][k] += WorkerHandle[i].Flag[j][k];
                WorkerHandle[i].Flag[j][k] = 0;
            }
        }
        pthread_rwlock_unlock(&(WorkerHandle[i].Lock));
    }
} /* collectLocalFlags */

/**
 * \brief This function prints the messages associated with each error flag.
 * \param all_flags The array that will hold all of the error flags from the run
 */
void printFlags(int **all_flags){
    int noError = 0;
    int i,j;

    // Print System errors, if needed.
    for(j = 0; j < SYS_ERR_SIZE; j++){
        //printf("Error flag[0][%d] = %d\n", j, all_flags[0][j]);
        if(all_flags[0][j] > 0){
            EmitLog(MyRank, SCHEDULER_THREAD, build_msg(0,j), all_flags[0][j], PRINT_ALWAYS);
        } else {
            noError++;
        }
    }
    // Print Plan errors
    for(i = 1; i < ERR_FLAG_SIZE; i++){
        for(j = 0; j < plan_list[i - 1]->esize + GEN_SIZE; j++){
            //printf("Error flag[%d][%d] = %d\n", i, j, all_flags[i][j]);
            if(all_flags[i][j] > 0){
                EmitLog(MyRank, SCHEDULER_THREAD, build_msg(i,j), all_flags[i][j], PRINT_ALWAYS);
            } else {
                noError++;
            }
        }
    }
    /*
       if (noError == ERR_FLAG_SIZE) {
       EmitLog(MyRank, SCHEDULER_THREAD, "No flagged errors have occured in this time period.", -1, PRINT_ALWAYS);
       }
     */
} /* printFlags */

