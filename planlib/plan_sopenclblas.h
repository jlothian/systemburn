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
#ifndef __PLAN_SOPENCLBLAS_H
#define __PLAN_SOPENCLBLAS_H

#include <CL/cl.h>

extern void *makeSOPENCLBLASPlan(data *i);                        // <- Change YOUR_NAME to your module's name.
extern int initSOPENCLBLASPlan(void *p);                          // <- Change YOUR_NAME to your module's name.
extern int execSOPENCLBLASPlan(void *p);                          // <- Change YOUR_NAME to your module's name.
extern void *killSOPENCLBLASPlan(void *p);                        // <- Change YOUR_NAME to your module's name.
extern int parseSOPENCLBLASPlan(char *line, LoadPlan *output);    // <- Change YOUR_NAME to your module's name.
extern int perfSOPENCLBLASPlan(void *p);
extern plan_info SOPENCLBLAS_info;
//If there are other functions to be used by your module, declare the prototypes here. Examples in plan_dstream.h and plan_istream.h

//Below is a sample data structure. This is optional, but your module will probably need some form of input to function.

typedef struct {
    //int planner_size;
    cl_mem A;
    float *A_buffer;
    cl_mem B;
    float *B_buffer;
    cl_mem C;
    float *C_buffer;
    size_t M;
    cl_command_queue opencl_queue;
    cl_uint device_id;
    int loop_count;
    cl_ulong device_memory;
    cl_program program;
    cl_kernel kernel;
    cl_uint num_platforms;
    cl_uint num_devices;
    cl_platform_id *platforms;
    cl_device_id *devices;
    cl_context context;
} SOPENCLBLAS_DATA;

#endif /* __PLAN_DOPENCLBLAS_H */
