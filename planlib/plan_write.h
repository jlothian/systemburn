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
#ifndef __PLAN_WRITE_H
#define __PLAN_WRITE_H

#include <fcntl.h>

extern void * makeWritePlan(data *i);
extern int    initWritePlan(void *p);
extern int    execWritePlan(void *p);
extern int    perfWritePlan(void *p);
extern void * killWritePlan(void *p);
extern int    parseWritePlan(char *line, LoadPlan *output);
extern plan_info WRITE_info;

//Below is a sample data structure. This is optional, but your module will probably need some form of input to function.

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
typedef struct {
  int mb;          // Size of file to write in megabytes.
  int *buff;
  uint64_t buff_len;
  char* str;

  int fdout;
} Writedata;

#endif /* __PLAN_WRITE_H */
