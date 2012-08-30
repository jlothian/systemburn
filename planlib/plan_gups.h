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
#ifndef __PLAN_GUPS_H
#define __PLAN_GUPS_H

#include <systemheaders.h>

extern void * makeGUPSPlan(data *m);
extern int    initGUPSPlan(void *p);
extern int    execGUPSPlan(void *p);
extern void * killGUPSPlan(void *p);
extern int    perfGUPSPlan(void *p);
extern int    parseGUPSPlan(char *line, LoadPlan *output);
extern plan_info GUPS_info;
extern uint64_t GUPS_startRNG(int64_t n);

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
typedef struct {
	int tbl_log_size, sub_log_size;
	uint64_t tbl_size, sub_size, num_updates;
	uint64_t *tbl, *sub, *random;
} GUPSdata;

#endif /* __PLAN_GUPS_H */

