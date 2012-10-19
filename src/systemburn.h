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
#ifndef __SYSTEMBURN_H
#define __SYSTEMBURN_H

#include <config.h>
#include <systemheaders.h>
#include <sys/time.h>
#include <planheaders.h>

/* global macros */

#define ROOT                        0
#define ABSOLUTE_ZERO            -273
#define SCHEDULER_THREAD           -1
#define MONITOR_THREAD             -2

#define PRINT_ALWAYS                0
#define PRINT_OFTEN                 1
#define PRINT_SOME                  2
#define PRINT_RARELY                3

/* typedefs */

/** \brief Temperature struct used by the monitor thread */
typedef struct {
        float min,max,avg;
} TemperatureRange;

/*
 * Plan struct used to be here. Moved to planlib/planheaders.h
 *
 * As it is only used in worker.c and the plan library, there
 * did not appear to be any point in keeping it visible to 
 * everyone.
 */

/**
 * \brief This structure contains the "identity" information for the thread and a pointer to the next Plan it should execute.
 */
typedef struct {
	pthread_t        ID;     /**< Thread ID                                               */
	pthread_attr_t   Attr;   /**< Thread attributes for this thread                       */
	pthread_rwlock_t Lock;   /**< Read-write lock for this structure                      */
	int              Num;    /**< Thread number on this MPI task                          */
	void *           Plan;   /**< Pointer to Plan. NULL means terminate worker            */
	int**            Flag;   /**< Error flags for individual workers                      */
	int              Status; /**< Determines if the thread has run a plan it's been given */
} ThreadHandle;

/* global data */

extern TemperatureRange local_temp;
extern ThreadHandle MonitorHandle;
extern ThreadHandle *WorkerHandle;
extern int          MyRank;
extern int          comm_flag;
extern int          verbose_flag;
extern int          plancheck_flag;

enum { BAD, GOOD};
enum { SINGLE=1, ARRAY=255};

/* functions */ 

/* Monitor thread functions. In monitor.c */
extern void * MonitorThread(void * vptr);
extern void StartMonitorThread();
extern void CheckTemperatureRange(TemperatureRange *T);
extern void EmergencyStop(int errorcode);
extern void reduceTemps();

/* Worker thread functions. In worker.c */
extern void * WorkerThread(void *threadarg);
extern void   StopWorkerThreads();
extern void   StartWorkerThreads();
extern void   initWorkerFlags();
extern void   reduceFlags(int **local_flags);
extern void   collectLocalFlags(int **local_flag);
extern void   printFlags(int **all_flags);
extern int ** initErrorFlags();

/* General purpose printing/output functions. In utility.c */
extern void   PAPI_EmitLog(int val, int rank, int tnum, int debug);
extern void   EmitLog(int rank, int tnum, char *text, int data, int debug);
extern void   EmitLog3(int rank, int tnum, char *text, int data1, int data2, int data3, int debug);
extern void   EmitLog3f(int rank, int tnum, char *text, float data1, float data2, float data3, int debug);
extern void   EmitLogfs(int rank, int tnum, char *text, double data1, char *data2, int debug);
extern int    fsize(char *path);
extern int    readFile(char *buffer, int size, FILE *stream);
extern char * getFileBuffer(int filesize);
extern int    strgetline(char *out, int out_len, char *source, int source_offset);

#endif /* __SYSTEMBURN_H */
