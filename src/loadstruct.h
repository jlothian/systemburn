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
#ifndef __LOADSTRUCT_H
#define __LOADSTRUCT_H

/** \brief This struct allows the user to input various forms of data, be they ints, doubles or strings.	*/

typedef struct data {
    int isize;
    int csize;
    int dsize;
    int *i;
    char **c;
    double *d;
} data;

/* Defines a structure to store a load specification in:	*/

/** \brief This structure stores plans and sizes associated with a specific socket
        or specified as free threads within the node.               */
typedef struct LoadPlan {
    int name;                   /* The name enum for this LoadPlan, indicates the load module to be run.	*/
    data *input_data;           /* Pointer to the size parameter(s) of the load module indicated by name.	*/
    struct LoadPlan *next;      /* Pointer to the next LoadPlan in the linked list.				*/
} LoadPlan;

/** \brief This struct holds the information for the plans and allows SystemBurn to communicate with them in a modular fashion */
typedef struct {
    char *name;
    char **error;
    int esize;
    void * (*make)(data * i);
    int   (*parse)(char *line, LoadPlan *output);
    int   (*exec)(void *p);
    int   (*init)(void *p);
    void * (*kill)(void *p);
    int   (*perf)(void *p);
    char *perf_units[];
} plan_info;

/** \brief Struct that holds the information for a given SubLoad. */
typedef struct SubLoad {
    LoadPlan *first;            /* Points to the first LoadPlan in a linked list.				*/
    LoadPlan *last;             /* Points to the last LoadPlan in the linked list to assist fast adding.	*/
    int num_plans;              /* The number of plans contained in the plans[] and sizes[] arrays.		*/
    int *cpuset;                /* The cpu core numbers in the cpuset for this subload. (NULL for unbound)	*/
    int cpuset_len;             /* The number of cores (elements) in the cpuset array. (0 for unbound)		*/
    struct SubLoad *next;       /* Points to the next subload in the linked list.				*/
} SubLoad;

/** \brief This structure stores one entire load to be replicated on each node.     */
typedef struct {
    SubLoad *front;             /* Points to the first subload in a linked list of subloads.			*/
    SubLoad *back;              /* Point to the last subload in the linked list (for fast adding of subloads).  */
    int num_threads;            /* The number of threads running the plan (sum of num_plans across groups).	*/
    int num_cpusets;            /* All the cpusets used by the plan (the number of groups in the array).	*/
    int runtime;                /* The amount of time (in seconds) that the load should be run.			*/
    int scheduling;             /* The type of scheduling to be applied to this load: BLOCK, ROUND_ROBIN...	*/
} Load;

#endif /* __LOADSTRUCT_H */
