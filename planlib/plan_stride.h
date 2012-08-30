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
#ifndef __PLAN_STRIDE_H
#define __PLAN_STRIDE_H

#include <loadstruct.h>

extern void * makeDStridePlan(data *i);             /* creates a plan struct       */
extern int    initDStridePlan(void *p);        	  /* inits plan's vptr           */
extern int    execDStridePlan(void *p);        	  /* run the sleep plan          */
extern int    perfDStridePlan(void *p);
extern void * killDStridePlan(void *p);       	  /* clean up & free plan & vptr */
extern void   Set(double *a, double *b,int m); 	  /* initialize the arrays       */
extern void   Fill(double *a);                 	  /* fill the cache memory       */
extern int    parseDStridePlan(char *line, LoadPlan *output);
extern plan_info DSTRIDE_info;

#define REPEAT   10
#define CACHE    7500000
#define TEST     100

/* DStride caller data structure */
/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
typedef struct {
	size_t M;
	double *one, *two, *three;//, *four, *five;
} DStridedata;

/*LStride stuff. */

extern void * makeLStridePlan(data *i);             /* creates a plan struct       */
extern int    initLStridePlan(void *p);           /* inits plan's vptr           */
extern int    execLStridePlan(void *p);           /* run the sleep plan          */
extern int    perfLStridePlan(void *p);
extern void * killLStridePlan(void *p);           /* clean up & free plan & vptr */
extern void   LSet(long int *a, long int *b,int m);       /* initialize the arrays       */
extern void   LFill(long int *a);                         /* fill the cache memory       */
extern int    parseLStridePlan(char *line, LoadPlan *output);
extern plan_info LSTRIDE_info;

/* LStride caller data structure */
/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
typedef struct {
	size_t M;
	long int *one, *two, *three;//, *four, *five;
} LStridedata;

#endif /* __PLAN_STRIDE_H */
