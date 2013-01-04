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
#ifndef __PLAN_PV4_H
#define __PLAN_PV4_H

#include <float.h>
#include <limits.h>
#include <loadstruct.h>

extern void *makePV4Plan(data *i);          /* creates a plan struct              */
extern int initPV4Plan(void *p);            /* inits plan's vptr                  */
extern int execPV4Plan(void *p);            /* run the sleep plan                 */
extern int perfPV4Plan(void *p);
extern void *killPV4Plan(void *p);          /* clean up & free plan & vptr        */
extern int parsePV4Plan(char *line, LoadPlan *output);
extern plan_info PV4_info;

#define ROTATION 50

/* PV4 caller data structure */
/**
 * \brief The data structure for the plan. Holds the input and all used info.
 */
typedef struct {
    size_t M;
    double *one, *two, *three, *four;
    int random;
} PV4data;

#endif /* __PLAN_PV4_H */
