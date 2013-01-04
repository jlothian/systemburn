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
#ifndef __PLAN_SLEEP_H
#define __PLAN_SLEEP_H

#include <loadstruct.h>

extern void *makeSleepPlan(data *i);    /* creates a plan struct       */
extern int initSleepPlan(void *p);      /* inits plan's vptr           */
extern int execSleepPlan(void *p);      /* run the sleep plan          */
extern void *killSleepPlan(void *p);    /* clean up & free plan & vptr */
extern int perfSleepPlan(void *p);
extern int parseSleepPlan(char *line, LoadPlan *output);
extern plan_info SLEEP_info;

#endif /* __PLAN_SLEEP_H */
