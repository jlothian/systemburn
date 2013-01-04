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
#ifndef __PLAN_STREAM_H
#define __PLAN_STREAM_H

#include <float.h>
#include <limits.h>
#include <loadstruct.h>

extern void *makeDStreamPlan(data *i);  /* creates a plan struct              */
extern int initDStreamPlan(void *p);    /* inits plan's vptr                  */
extern int execDStreamPlan(void *p);    /* run the sleep plan                 */
extern int perfDStreamPlan(void *p);
extern void *killDStreamPlan(void *p);  /* clean up & free plan & vptr        */
extern int StreamCheck(void *p);        /* check for memory errors in results */
extern int parseDStreamPlan(char *line, LoadPlan *output);
extern plan_info DSTREAM_info;

#define ROTATION 50

/* DStream caller data structure */
/**
 * \brief The data structure for the plan. Holds the input and all used info.
 */
typedef struct {
    size_t M;
    int random;
    double *one, *two, *three, *four, *five;
} DStreamdata;

/*LStream stuff */

extern void *makeLStreamPlan(data *i);  /* creates a plan struct              */
extern int initLStreamPlan(void *p);    /* inits plan's vptr                  */
extern int execLStreamPlan(void *p);    /* run the sleep plan                 */
extern int perfLStreamPlan(void *p);
extern void *killLStreamPlan(void *p);  /* clean up & free plan & vptr        */
extern int LStreamCheck(void *p);       /* check for memory errors in results */
extern int parseLStreamPlan(char *line, LoadPlan *output);
extern plan_info LSTREAM_info;

/* Lstream caller data structure */
/**
 * \brief The data structure for the plan. Holds the input and all used info.
 */
typedef struct {
    size_t M;
    int random;
    long int *one, *two, *three, *four, *five;
} LStreamdata;

#endif /* __PLAN_STREAM_H */
