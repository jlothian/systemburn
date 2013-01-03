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
#ifndef __PLAN_COMM_H
#define __PLAN_COMM_H

extern void *makeCommPlan(data *i);                     // <- Change Comm to your module's name.
extern int initCommPlan(void *p);                       // <- Change Comm to your module's name.
extern int execCommPlan(void *p);                       // <- Change Comm to your module's name.
extern int perfCommPlan(void *p);
extern void *killCommPlan(void *p);                     // <- Change Comm to your module's name.
extern int parseCommPlan(char *line, LoadPlan *output);         // <- Change Comm to your module's name.
extern plan_info COMM_info;

/**
 * \brief The data structure for the plan. Holds the input and all used info.
 */
typedef struct {
    size_t   buflen;
    int   NumRanks;
    int   ThisRankID;
    int   NumStages;
    int   NumMessages;
    int   istage;
    char *sendbufptr;
    char *recvbufptr;
} COMMdata;

extern char *comm_errs[];

#endif /* __PLAN_COMM_H */
