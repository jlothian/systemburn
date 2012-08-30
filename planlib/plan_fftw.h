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
#ifndef __PLAN_FFTW_H
#define __PLAN_FFTW_H

#include <fftw3.h>
#include <systemheaders.h>
#include <loadstruct.h>

extern char* fft1_errs[];
extern char* fft2_errs[];

extern uint64_t FFTlog2(uint64_t n);

extern void * makeFFT1Plan(data *i);	/* creates a plan struct       */
extern int    initFFT1Plan(void *p);	/* inits plan's vptr           */
extern int    execFFT1Plan(void *p);	/* run the sleep plan          */
extern int    perfFFT1Plan(void *p);
extern void * killFFT1Plan(void *p);	/* clean up & free plan & vptr */
extern int    parseFFT1Plan(char *line, LoadPlan *output);
extern plan_info FFT1_info;

/* FFT caller data structure */
/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
typedef struct {
	size_t M;
	fftw_complex *in_original, *mid, *out;
	fftw_plan forward, backward;
} FFTdata;


// Lock is defined and initialized in plan_fftw1d.c
extern pthread_rwlock_t FFTW_Lock;

/* FFT2D Functions */

extern void * makeFFT2Plan(data *i);	/* creates a plan struct       */
extern int    initFFT2Plan(void *p);	/* inits plan's vptr           */
extern int    execFFT2Plan(void *p);	/* run the sleep plan          */
extern int    perfFFT2Plan(void *p);
extern void * killFFT2Plan(void *p);	/* clean up & free plan & vptr */
extern int    parseFFT2Plan(char *line, LoadPlan *output);
extern plan_info FFT2_info;

#endif /* __PLAN_FFTW_H */
