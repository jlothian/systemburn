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

// Declare and initialize the FFTW lock that must be 
// used by all plans using the fftw3 library.
pthread_rwlock_t FFTW_Lock = PTHREAD_RWLOCK_INITIALIZER;

/**
 * \brief Binary logarithm (n=2^i)
 * \param [in] n Target for solving the logarithm.
 * \returns i Power of 2 >= n
 */
uint64_t FFTlog2(uint64_t n) {
	uint64_t i=0;
	if (n<1) return 0xFFFFFFFFFFFFFFFF;
	while (n>>=1) i++;
	return i;
}

/**
 * \brief Allocates and returns the data struct for the plan
 * \param [in] m Holds the input data for the plan.
 * \return void* Data struct 
 * \sa parseFFT1Plan
 * \sa initFFT1Plan
 * \sa execFFT1Plan
 * \sa perfFFT1Plan
 * \sa killFFT1Plan
*/
void * makeFFT1Plan(data *m) {
	Plan *p;
	FFTdata *d;
	p=(Plan*)malloc(sizeof(Plan));
	assert(p);
	if(p){ 
	        p->fptr_initplan = &initFFT1Plan;
       		p->fptr_execplan = &execFFT1Plan;
        	p->fptr_killplan = &killFFT1Plan;
		p->fptr_perfplan = &perfFFT1Plan;
		p->name = FFT1D;
        	d = (FFTdata *)malloc(sizeof(FFTdata));
		assert(d);
		if(d) {
			if(m->isize==1) d->M = (size_t)(m->i[0]/(3*sizeof(fftw_complex)));
			else d->M = (size_t)(m->d[0]/(3.0*sizeof(fftw_complex)));
		}
	        (p->vptr)=(void*)d;
	}
        return p;
}

/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan Holds the data and the memory locations for the plan.
 * \return int Error flag value
 * \sa parseFFT1Plan 
 * \sa makeFFT1Plan
 * \sa execFFT1Plan
 * \sa perfFFT1Plan
 * \sa killFFT1Plan
*/
int    initFFT1Plan(void *plan) {
	int i;
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

		int sadj = sizeof(fftw_complex);
		//EmitLog(MyRank,100,"Allocating",sadj*M*3,0);

		pthread_rwlock_wrlock(&FFTW_Lock);
		d->in_original = (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*M);
		assert(d->in_original);
		d->out =         (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*M);
		assert(d->out);
		d->mid =         (fftw_complex*) fftw_malloc(sizeof(fftw_complex)*M);
		assert(d->mid);
		if(d->in_original && d->out && d->mid) ret = make_error(0,specific_err);	// Error in getting the plan set
 
		d->forward =  fftw_plan_dft_1d(M,d->in_original,d->mid,FFTW_FORWARD, FFTW_ESTIMATE);
		d->backward = fftw_plan_dft_1d(M,d->mid,d->out,FFTW_BACKWARD, FFTW_ESTIMATE);
		pthread_rwlock_unlock(&FFTW_Lock);
		if(d->forward && d->backward) ret = ERR_CLEAN;
		
		srand(0);
		for(i=0;i<M;i++) {
			d->in_original[i][0]=rand();
			d->in_original[i][1]=rand();
		}
	}
	return ret;
}

/**
 * \brief Frees the memory used in the plan
 * \param [in] plan Points to the locations of memory that need free-ing.
 * \sa parseFFT1Plan
 * \sa makeFFT1Plan
 * \sa initFFT1Plan
 * \sa execFFT1Plan
 * \sa perfFFT1Plan
 */
void * killFFT1Plan(void *plan) {
	Plan *p;
	FFTdata *d;
	p = (Plan *)plan;
	d = (FFTdata*)p->vptr;

	int sadk = sizeof(fftw_complex);
	//EmitLog(MyRank,100,"Freeing   ",sadk*d->M*3,0);

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
 * \brief A 1 dimensional complex fast Fourier transform in a memory footprint of "size" bytes. 
 * \param [in] plan Holds the data and the storage for the plan.
 * \return int Error flag value
 * \sa parseFFT1Plan
 * \sa makeFFT1Plan
 * \sa initFFT1Plan
 * \sa perfFFT1Plan
 * \sa killFFT1Plan
 */
int execFFT1Plan(void *plan) {
	int i;
	ORB_t t1, t2;
	Plan *p;
	FFTdata* d;
	p = (Plan *)plan;
	d = (FFTdata*)p->vptr;
	/* update execution count */
	p->exec_count++;
	
//	for(i=0;i<d->M;i++) {	// Was running so long that no performance data could be retrieved.
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
 * \sa parseFFT1Plan
 * \sa makeFFT1Plan
 * \sa initFFT1Plan
 * \sa execFFT1Plan
 * \sa killFFT1Plan
 */
int perfFFT1Plan(void *plan) {
	int ret = ~ERR_CLEAN;
	uint64_t opcounts[NUM_TIMERS];
	Plan *p;
	FFTdata *d;
	p = (Plan *)plan;
	d = (FFTdata *)p->vptr;
	if (p->exec_count > 0) {
		opcounts[TIMER0] = (5 * (uint64_t)d->M * FFTlog2(d->M)) * p->exec_count; // Method used by authors of the fftw3 library - see http://www.fftw.org/speed
		opcounts[TIMER1] = (5 * (uint64_t)d->M * FFTlog2(d->M)) * p->exec_count;
		opcounts[TIMER2] = 0;
		
		perf_table_update(&p->timers, opcounts, p->name, NULL);
		
		double flops_forward = ((double)opcounts[TIMER0]/perftimer_gettime(&p->timers, TIMER0))/1e6;
		EmitLogfs(MyRank, 9999, "FFT1D plan performance:", flops_forward, "MFLOPS", PRINT_SOME);
		EmitLog  (MyRank, 9999, "FFT1D execution count :", p->exec_count, PRINT_SOME);
		ret = ERR_CLEAN;
	}
	return ret;
}

/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line The input line for the plan.
 * \param [out] output Holds the data for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makeFFT1Plan 
 * \sa initFFT1Plan
 * \sa execFFT1Plan
 * \sa perfFFT1Plan
 * \sa killFFT1Plan
*/
int parseFFT1Plan(char *line, LoadPlan *output) {
	output->input_data = get_sizes(line);	
	output->name = FFT1D;
	return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief Holds the custom error messages for the plan
 */
char* fft1_errs[] = {
	" plan setting errors:"
};

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
plan_info FFT1_info = {
	"FFT1D",
	fft1_errs,
	1,
	makeFFT1Plan,
	parseFFT1Plan,
	execFFT1Plan,
	initFFT1Plan,
	killFFT1Plan,
	perfFFT1Plan,
	{ "FLOPS", "FLOPS", NULL }
};

