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
/*
	This header file contains declarations for the parameters, structures, and functions
	used by or manipulating systemburn Load objects.
*/

#ifndef __LOAD_H
#define __LOAD_H

#include <loadstruct.h>
#include <planheaders.h>

/* Determines the number of subloads to display in a row, when printing. */
#define OUTPUT_STEP 3
#ifndef OUTPUT_PLACE
#define OUTPUT_PLACE stdout
#endif

typedef enum {
	LOAD_START,
	LOAD_END,
	SUB_START,
	SUB_END,
	RUNTIME,
	SCHEDULE,
	SUBLOAD,
	PLAN,
	MASK,
	NONE
} keyword;

typedef enum {
	INITIAL = -1,
	BLOCK,
	ROUND_ROBIN,
	SUBLOAD_SPECIFIC,
	UNKN_SCHED
} schedules;

/* Load structure related functions. In load.c */
extern int  bcastLoad(Load *load);
extern int broadcast_buffer(void *buffer, int buffer_size);
extern int initLoadOptions(char *load_name, char **output);
extern void printLoad(Load *input);
extern void printSchedule(FILE *out, schedules sched);
extern void freePlan(LoadPlan *input);
extern void freeSubLoad(SubLoad *input);
extern void freeLoad(Load *input);
extern schedules setSchedule(char *schedName);

/* Load struct parsing functions. */
extern int parseLoad(char *inString, Load *output);
extern keyword keywordCmp(char *input);
extern int runtimeLine(char *line, int *runtime);
extern int scheduleLine(char *line, schedules *schedule);
extern int subloadLine(char *line, int *subloads);
extern int allocSubload(SubLoad ***output, int *subloads);
extern int planLine(char *line, LoadPlan *output, int *plans);
extern int assignPlan(SubLoad **output, int subloads, LoadPlan *input, int plans);
extern int assignSubLoad(Load *load, int *index, SubLoad ***temp_subload, int subloads);
extern LoadPlan * planCopy(LoadPlan *input);
extern int sumArr(int *array, int n);


/* Scheduling functions. In schedule.c */
extern int  WorkerSched(Load *load);

#ifdef LINUX_PLACEMENT
/* CPUset management */
extern int  maskLine(char *line, int **cpuset, int *set_len);
extern int  assignMask(SubLoad **output, int subloads, int *cpuset, int set_len);
extern void MaskBlock(int numcpucores, cpu_set_t *cpuset, int num_cpusets);
extern void MaskRoundRobin(int numcpucores, cpu_set_t *cpuset, int num_cpusets);
extern void MaskSpecific(int numcpucores, cpu_set_t *cpus, Load *input);
extern void ZeroMask(cpu_set_t *cpuset, int num_cpusets);
extern void SetMasks(int numcpucores, cpu_set_t *cpuset, Load *input);
extern void SetCPUSetLens(int numcpucores, cpu_set_t *cpuset, Load *input);
#endif /* LINUX_PLACEMENT */

#endif /* __LOAD_H */
