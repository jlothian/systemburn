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
#ifndef __PLAN_TILT_H
#define __PLAN_TILT_H

extern void *makeTiltPlan(data *i);
extern int initTiltPlan(void *p);
extern int execTiltPlan(void *p);
extern int perfTiltPlan(void *p);
extern void *killTiltPlan(void *p);
extern int parseTiltPlan(char *line, LoadPlan *output);
extern plan_info TILT_info;

//If there are other functions to be used by your module, declare the prototypes here. Examples in plan_dstream.h and plan_istream.h

/**
 * \brief The data structure for the plan. Holds the input and all used info.
 */
typedef struct {
    int niter;
    int seed;
    uint64_t *arr;
} TILT_data;

#endif /* __PLAN_TILT_H */
