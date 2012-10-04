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
#include <systemburn.h>
#include <planheaders.h>

/**
* \brief Allocates and returns the data struct for the plan
* \param [in] m The input data for the plan.
* \return void* Data struct 
* \sa parseFFT2Plan
* \sa initFFT2Plan
* \sa execFFT2Plan
* \sa perfFFT2Plan
* \sa killFFT2Plan
*/
void * makeFFT2Plan(data *m) {
	Plan *p;
	FFTdata *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p) {
		p->fptr_initplan = &initFFT2Plan;
		p->fptr_execplan = &execFFT2Plan;
		p->fptr_killplan = &killFFT2Plan;
		p->fptr_perfplan = &perfFFT2Plan;
		p->name = FFT2D;
		d = (FFTdata *)malloc(sizeof(FFTdata));
		assert(d);
		if(d) {
			if(m->isize) d->M = sqrt(m->i[0]/(3*sizeof(fftw_complex)));
			else d->M = sqrt(m->d[0]/(3*sizeof(fftw_complex)));
		}
		(p->vptr)=(void*)d;
	}
	// p=(Plan*)malloc(sizeof(Plan));
	return p;
}

/**
* \brief Creates and initializes the working data for the plan
* \param [in] plan Holds the data and memory for the plan.
* \return int Error flag value
* \sa parseFFT2Plan 
* \sa makeFFT2Plan
* \sa execFFT2Plan
* \sa perfFFT2Plan
* \sa killFFT2Plan
*/
int    initFFT2Plan(void *plan) {
	int i,k;
	size_t M;
	int ret = make_error(ALLOC,generic_err);
	Plan *p;
	FFTdata *d = NULL;
	p = (Plan *)plan;
	if (p) {
		d = (FFTdata*)p->vptr;
		p->exec_count = 0;
		perftimer_init(&p->timers, NUM_TIMERS);
	}
	if (d) {
		M = d->M;
		
		pthread_rwlock_wrlock(&FFTW_Lock);
		d->in_original = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*M*M);
		assert(d->in_original);
		d->out =         (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*M*M);
		assert(d->out);
		d->mid =         (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*M*M);
		assert(d->mid);
		if(d->in_original && d->out && d->mid) ret = make_error(0,specific_err);	// Error in getting the plan set
	
		d->forward =  fftw_plan_dft_2d(M,M,d->in_original,d->mid,FFTW_FORWARD, FFTW_ESTIMATE);
		d->backward = fftw_plan_dft_2d(M,M,d->mid,d->out,FFTW_BACKWARD, FFTW_ESTIMATE);
		pthread_rwlock_unlock(&FFTW_Lock);
		if(d->forward && d->backward) ret = ERR_CLEAN;
		
		srand(0);
		for(i=0;i<M;i++) {
			for(k=0;k<M;k++) {
				d->in_original[i*M+k][0]=rand();
				d->in_original[i*M+k][1]=rand();
			}
		}
	}
	return ret;
}

/**
* \brief Frees the memory used in the plan
* \param [in] plan Points to the memory for free-ing.
* \sa parseFFT2Plan
* \sa makeFFT2Plan
* \sa initFFT2Plan
* \sa execFFT2Plan
* \sa perfFFT2Plan
*/
void * killFFT2Plan(void *plan) {
	Plan *p;
	FFTdata *d;
	p = (Plan *)plan;
	assert(p);
	d = (FFTdata*)p->vptr;
	assert(d);
	pthread_rwlock_wrlock(&FFTW_Lock);
	if(d->in_original) fftw_free(d->in_original);
	if(d->out)         fftw_free(d->out);
	if(d->mid)         fftw_free(d->mid);
	if(d->forward)     fftw_destroy_plan(d->forward);
	if(d->backward)    fftw_destroy_plan(d->backward);
	pthread_rwlock_unlock(&FFTW_Lock);
	free(d);
	free(p);
	return (void*)NULL;
}

/**
* \brief A 2 dimensional complex fast Fourier transform in a memory footprint of "size" bytes.
* \param [in] plan Holds the data and memory for the plan.
* \return int Error flag value
* \sa parseFFT2Plan
* \sa makeFFT2Plan
* \sa initFFT2Plan
* \sa perfFFT2Plan
* \sa killFFT2Plan
*/
int execFFT2Plan(void *plan) {
	int i;
	ORB_t t1, t2;
	Plan *p;
	FFTdata* d;
	p = (Plan *)plan;
	d = (FFTdata*)p->vptr;
	assert(d);
	assert(d->forward);
	assert(d->backward);
	/* update execution count */
	p->exec_count++;
//	for(i=0;i<d->M;i++) {
	if(d->forward) {
		ORB_read(t1);
		fftw_execute(d->forward);
		ORB_read(t2);
		perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));
	}
	if(d->backward) {
		ORB_read(t1);
		fftw_execute(d->backward);
		ORB_read(t2);
		perftimer_accumulate(&p->timers, TIMER1, ORB_cycles_a(t2, t1));
	}
//	}
	return ERR_CLEAN;
}

/**
 * \brief Stores (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure containing the plan data.
 * \returns An integer error code.
 * \sa parseFFT2Plan
 * \sa makeFFT2Plan
 * \sa initFFT2Plan
 * \sa execFFT2Plan
 * \sa killFFT2Plan
 */
int perfFFT2Plan(void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t points;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	FFTdata *d;
	p = (Plan *)plan;
	d = (FFTdata *)p->vptr;
	if (p->exec_count > 0) {
		points = (uint64_t)d->M * (uint64_t)d->M;                          // Number of data points in the calculations.
		opcounts[TIMER0] = (10 * points * FFTlog2((uint64_t)d->M)) * p->exec_count; // Method used by authors of the fftw3 library - see http://www.fftw.org/speed
		opcounts[TIMER1] = (5 * points * FFTlog2(points)) * p->exec_count;
		opcounts[TIMER2] = 0;
		
		perf_table_update(&p->timers, opcounts, p->name, NULL);
		
		double flops_forward = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		EmitLogfs(MyRank, 9999, "FFT2D plan performance:", flops_forward, "MFLOPS", PRINT_SOME);
		EmitLog  (MyRank, 9999, "FFT2D execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
* \brief Reads the input file, and pulls out the necessary data for use in the plan
* \param [in] line The input line for the plan.
* \param [out] output Holds the data for the load.
* \return int True if the data was read, false if it wasn't
* \sa makeFFT2Plan 
* \sa initFFT2Plan
* \sa execFFT2Plan
* \sa perfFFT2Plan
* \sa killFFT2Plan
*/
int parseFFT2Plan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);
	output->name = FFT2D;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
* \brief Holds the custom error messages for the plan
*/
char* fft2_errs[] = {
	" plan setting errors:"
};

/**
* \brief The data structure for the plan. Holds the input and all used info. 
*/
plan_info FFT2_info = {
	"FFT2D",
	fft2_errs,
	1,
	makeFFT2Plan,
	parseFFT2Plan,
	execFFT2Plan,
	initFFT2Plan,
	killFFT2Plan,
	perfFFT2Plan,
	{ "FLOPS", "FLOPS", NULL }
};

