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
#include <systemheaders.h>
#include <systemburn.h>
#include <planheaders.h>
#include <brand.h>

#ifdef HAVE_PAPI
  #define NUM_PAPI_EVENTS 1
  #define PAPI_COUNTERS { PAPI_FP_OPS }
  #define PAPI_UNITS { "FLOPS" }
#endif //HAVE_PAPI

/**
 * \brief Allocates and returns the data struct for the plan
 * \param [in] i The struct that holds the input data.
 * \return void* Data struct
 * \sa parseCBAPlan
 * \sa initCBAPlan
 * \sa execCBAPlan
 * \sa perfCBAPlan
 * \sa killCBAPlan
 */
void *makeCBAPlan(data *i){
    Plan *p;
    CBA_data *ip;
    p = (Plan *)malloc(sizeof(Plan));
    assert(p);
    if(p){      // <- Checking the Plan pointer was allocated. Do not change.
        p->fptr_initplan = &initCBAPlan;
        p->fptr_execplan = &execCBAPlan;
        p->fptr_killplan = &killCBAPlan;
        p->fptr_perfplan = &perfCBAPlan;
        p->name = CBA;
        ip = (CBA_data *)malloc(sizeof(CBA_data));
        assert(ip);
        if(ip){
            ip->niter = i->i[0];
            ip->seed = i->i[1];
            //ip->nrows = i->i[2];
            //ip->ncols = i->i[3];
            if(i->isize == 3){
                ip->nrows = (sqrt(i->i[2] / (2 * sizeof(uint64_t))));
            } else {
                ip->nrows = (sqrt(i->d[0] / (2 * sizeof(uint64_t))));
            }
            // ensure correct blocking:
            ip->nrows -= ip->nrows % BLOCKSIZE;
            ip->ncols = ip->nrows;
        }
        (p->vptr) = (void *)ip;
    }
    return p;     // <- Returning the Plan pointer. Do not change.
} /* makeCBAPlan */

/************************
 * This is the place where the memory gets allocated, and data types get initialized to their starting values.
 ***********************/

/**
 * \brief Creates and initializes the working data for the plan
 * \param [in] plan The struct that holds the plan's data values.
 * \return int Error flag value
 * \sa parseCBAPlan
 * \sa makeCBAPlan
 * \sa execCBAPlan
 * \sa perfCBAPlan
 * \sa killCBAPlan
 */
int initCBAPlan(void *plan){
    int ret = make_error(ALLOC,generic_err);
    int i;
    int nrow, ncol;
    Plan *p;
    CBA_data *ci = NULL;
    p = (Plan *)plan;

    #ifdef HAVE_PAPI
    int temp_event, k;
    int PAPI_Events [NUM_PAPI_EVENTS] = PAPI_COUNTERS;
    char *PAPI_units [NUM_PAPI_EVENTS] = PAPI_UNITS;
    #endif //HAVE_PAPI

    if(p){
        ci = (CBA_data *)p->vptr;
        p->exec_count = 0;
        if(DO_PERF){
            perftimer_init(&p->timers, NUM_TIMERS);

            #ifdef HAVE_PAPI
            /* Initialize plan's PAPI data */
            p->PAPI_EventSet = PAPI_NULL;
            p->PAPI_Num_Events = 0;

            TEST_PAPI(PAPI_create_eventset(&p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);

            //Add the desired events to the Event Set; ensure the dsired counters
            //  are on the system then add, ignore otherwise
            for(k = 0; k < TOTAL_PAPI_EVENTS && k < NUM_PAPI_EVENTS; k++){
                temp_event = PAPI_Events[k];
                if(PAPI_query_event(temp_event) == PAPI_OK){
                    p->PAPI_Num_Events++;
                    TEST_PAPI(PAPI_add_event(p->PAPI_EventSet, temp_event), PAPI_OK, MyRank, 9999, PRINT_SOME);
                }
            }

            PAPIRes_init(p->PAPI_Results, p->PAPI_Times);
            PAPI_set_units(p->name, PAPI_units, NUM_PAPI_EVENTS);

            TEST_PAPI(PAPI_start(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
            #endif //HAVE_PAPI
        }         //DO_PERF
    }
    if(ci){
        brand_init(&(ci->br), ci->seed);

        nrow = ci->nrows;
        ncol = ci->ncols;

        ci->niter *= 64;          /* we'll do iterations in blocks of 64 */

        if((ci->ncols % BLOCKSIZE) != 0){
            return make_error(0,specific_err);
            //fprintf(stderr, "ERROR (plan_cba): BLOCKSIZE (%ld) must divide"
            //" ncol (%ld)\n", BLOCKSIZE, ncol);
        }
        assert ((NITERS % 64) == 0);

        ci->work = (uint64_t *)calloc((size_t)((nrow * ncol + PAD + NITERS) * 2),
                                      sizeof(uint64_t));
        ret = (ci->work == NULL) ? make_error(ALLOC,generic_err) : ERR_CLEAN;

        ci->out = &(ci->work[nrow * ncol + PAD]);
        ci->data = &(ci->out[NITERS]);
        ci->chk = &(ci->data[nrow * ncol + PAD]);

        for(i = 0; i < (nrow * ncol); i++){
            ci->data[i] = brand(&(ci->br));
        }

        blockit (ci->data, nrow, ncol, ci->work);
    }
    return ret;
} /* initCBAPlan */

/************************
 * This is where the plan gets executed. Place all operations here.
 ***********************/
/**
 * \brief A bit-twiddling load which will run within the given bytes of memory.
 * \param [in] plan The struct that holds the plan's data values.
 * \return int Error flag value
 * \sa parseCBAPlan
 * \sa makeCBAPlan
 * \sa initCBAPlan
 * \sa perfCBAPlan
 * \sa killCBAPlan
 */
int execCBAPlan(void *plan){
    #ifdef HAVE_PAPI
    int k;
    long long start, end;
    #endif //HAVE_PAPI

    int i, j;
    int niters;
    ORB_t t1, t2;
    Plan *p;
    CBA_data *ci;

    p = (Plan *)plan;
    ci = (CBA_data *)p->vptr;

    /* update execution count */
    p->exec_count++;

    for(i = 0; i < ci->niter; i += NITERS){
        niters = ci->niter - i;
        if(niters > NITERS){
            niters = NITERS;
        }

        for(j = 0; j < niters; j++){
            /* pick NITERS random rows in the range 1..(nrow-1) */
            ci->out[j] = 1 + (brand(&(ci->br)) % (ci->nrows - 1));
            ci->out[j] <<= 48;              /* store index in high 16 bits */
        }

        if(DO_PERF){
            #ifdef HAVE_PAPI
            /* Start PAPI counters and time */
            TEST_PAPI(PAPI_reset(p->PAPI_EventSet), PAPI_OK, MyRank, 9999, PRINT_SOME);
            start = PAPI_get_real_usec();
            #endif //HAVE_PAPI

            ORB_read(t1);
        }         //DO_PERF
        cnt_bit_arr (ci->work, ci->nrows, ci->ncols, ci->out, niters);
        if(DO_PERF){
            ORB_read(t2);

            #ifdef HAVE_PAPI
            end = PAPI_get_real_usec();             //PAPI time

            /* Collect PAPI counters and store time elapsed */
            TEST_PAPI(PAPI_accum(p->PAPI_EventSet, p->PAPI_Results), PAPI_OK, MyRank, 9999, PRINT_SOME);
            for(k = 0; k < p->PAPI_Num_Events && k < TOTAL_PAPI_EVENTS; k++){
                p->PAPI_Times[k] += (end - start);
            }
            #endif //HAVE_PAPI

            perftimer_accumulate(&p->timers, TIMER0, ORB_cycles_a(t2, t1));
        }         //DO_PERF
    }

    return ERR_CLEAN;
} /* execCBAPlan */

/**
 * \brief Calculates (and optionally displays) performance data for the plan.
 * \param [in] plan The Plan structure that contains all the plan data.
 * \returns An integer error code.
 * \sa parseCBAPlan
 * \sa makeCBAPlan
 * \sa initCBAPlan
 * \sa execCBAPlan
 * \sa killCBAPlan
 */
int perfCBAPlan(void *plan){
    int ret = ~ERR_CLEAN;
    uint64_t opcounts[NUM_TIMERS];
    Plan *p;
    CBA_data *d;
    p = (Plan *)plan;
    d = (CBA_data *)p->vptr;
    if(p->exec_count > 0){
        opcounts[TIMER0] = ((uint64_t)d->ncols * (uint64_t)d->niter * 8ULL) * p->exec_count;         // count # of bytes processed
        opcounts[TIMER1] = 0;
        opcounts[TIMER2] = 0;

        perf_table_update(&p->timers, opcounts, p->name);
        #ifdef HAVE_PAPI
        PAPI_table_update(p->name, p->PAPI_Results, p->PAPI_Times, p->PAPI_Num_Events);
        #endif //HAVE_PAPI

        double ips = ((double)opcounts[TIMER0] / perftimer_gettime(&p->timers, TIMER0)) / 1e9;
        EmitLogfs(MyRank, 9999, "CBA plan performance:", ips, "GB/s", PRINT_SOME);
        EmitLog  (MyRank, 9999, "CBA execution count :", p->exec_count, PRINT_SOME);
        ret = ERR_CLEAN;
    }
    return ret;
} /* perfCBAPlan */

/************************
 * This is where everything gets cleaned up. Be sure to free() your data types (free the members first) in addition to the ones included below.
 ***********************/
/**
 * \brief Frees the memory used in the plan
 * \param plan Holds the information and memory location for the plan.
 * \sa parseCBAPlan
 * \sa makeCBAPlan
 * \sa initCBAPlan
 * \sa execCBAPlan
 * \sa perfCBAPlan
 */
void *killCBAPlan(void *plan){
    Plan *p;     // <- Plan type. Do not change.
    CBA_data *ci;
    p = (Plan *)plan;
    ci = (CBA_data *)p->vptr;

    if(DO_PERF){
        #ifdef HAVE_PAPI
        TEST_PAPI(PAPI_stop(p->PAPI_EventSet, NULL), PAPI_OK, MyRank, 9999, PRINT_SOME);
        #endif //HAVE_PAPI
    }     //DO_PERF

    free((void *)(ci->work));
    free((void *)(p->vptr));
    free((void *)(plan));
    return (void *)NULL;
} /* killCBAPlan */

/************************
 * This is the parsing function that the program uses to read in your load information from the load file. This lets you have a custom number of int arguments for your module while still maintaining modularity.
 ***********************/
/**
 * \brief Reads the input file, and pulls out the necessary data for use in the plan
 * \param [in] line The line of input text.
 * \param [out] output The struct that holds the values for the load.
 * \return int True if the data was read, false if it wasn't
 * \sa makeCBAPlan
 * \sa initCBAPlan
 * \sa execCBAPlan
 * \sa perfCBAPlan
 * \sa killCBAPlan
 */
int parseCBAPlan(char *line, LoadPlan *output){
    output->input_data = get_sizes(line);
    output->name = CBA;
    return (output->input_data->isize + output->input_data->csize + output->input_data->dsize > 0);
}

/**
 * \brief Holds the custom error messages for the plan
 */
char *cba_errs[] = {
    " blocking failed:"
};

plan_info CBA_info = {
    "CBA",
    cba_errs,
    1,
    makeCBAPlan,
    parseCBAPlan,
    execCBAPlan,
    initCBAPlan,
    killCBAPlan,
    perfCBAPlan,
    { "B/s", NULL, NULL }
};

/**
 * \brief the main operating function for CBA
 */
static
void block_iter(uint64_t *RESTRICT accum, uint64_t *RESTRICT arr,
                uint64_t *RESTRICT out, int64_t niters){
    int64_t i, j;
    uint64_t ind0, ind1, ind2, ind3, s0, s1, s2, s3;

    for(i = 0; i < niters; i += 4){
        s0 = out[i + 0];
        ind0 = s0 >> 48;
        s1 = out[i + 1];
        ind1 = s1 >> 48;
        s2 = out[i + 2];
        ind2 = s2 >> 48;
        s3 = out[i + 3];
        ind3 = s3 >> 48;

        j = 0;
        #ifdef USE_POP3
        for(; j < (BLOCKSIZE - 2); j += 3){
            uint64_t x, y, z, popc;

            x = accum[j + 0];
            y = accum[j + 1];
            z = accum[j + 2];

            x ^= arr[ind0 * BLOCKSIZE + j + 0];
            y ^= arr[ind0 * BLOCKSIZE + j + 1];
            z ^= arr[ind0 * BLOCKSIZE + j + 2];
            POPCNT3(x,y,z,popc);
            s0 += popc;

            x ^= arr[ind1 * BLOCKSIZE + j + 0];
            y ^= arr[ind1 * BLOCKSIZE + j + 1];
            z ^= arr[ind1 * BLOCKSIZE + j + 2];
            POPCNT3(x,y,z,popc);
            s1 += popc;

            x ^= arr[ind2 * BLOCKSIZE + j + 0];
            y ^= arr[ind2 * BLOCKSIZE + j + 1];
            z ^= arr[ind2 * BLOCKSIZE + j + 2];
            POPCNT3(x,y,z,popc);
            s2 += popc;

            x ^= arr[ind3 * BLOCKSIZE + j + 0];
            y ^= arr[ind3 * BLOCKSIZE + j + 1];
            z ^= arr[ind3 * BLOCKSIZE + j + 2];
            POPCNT3(x,y,z,popc);
            s3 += popc;

            accum[j + 0] = x;
            accum[j + 1] = y;
            accum[j + 2] = z;
        }
        #endif /* ifdef USE_POP3 */
        for(; j < BLOCKSIZE; j++){
            uint64_t x = accum[j];

            x ^= arr[ind0 * BLOCKSIZE + j];
            s0 += _popcnt(x);
            x ^= arr[ind1 * BLOCKSIZE + j];
            s1 += _popcnt(x);
            x ^= arr[ind2 * BLOCKSIZE + j];
            s2 += _popcnt(x);
            x ^= arr[ind3 * BLOCKSIZE + j];
            s3 += _popcnt(x);
            accum[j] = x;
        }
        out[i + 0] = s0;
        out[i + 1] = s1;
        out[i + 2] = s2;
        out[i + 3] = s3;
    }
} /* block_iter */

/**
 * \brief Operates on the blocked data
 */
int64_t cnt_bit_arr(uint64_t *arr, int64_t nrow, int64_t ncol, uint64_t *out,
                    int64_t niters){
    int64_t i;

    /* call the rows of the matrix r(0) ... r(ncol-1)
     * let the random row indices (stored in out[])
     *    be i(0) ... i(NITERS-1) [note that each index is > 0]
     * we will accumulate each random row into r(0)
     *    and return the number of ones in the result
     */

    for(i = 0; i < ncol; i += BLOCKSIZE){
        block_iter (&arr[i * nrow], &arr[i * nrow], out, niters);
    }

    for(i = 0; i < niters; i++){
        out[i] &= _maskr(48);
    }

    return 0;
}

/* non-blocked code */
/**
 * \brief Operates on the non-blocked data
 */
int64_t cnt_bit_arr_nb(uint64_t *arr, int64_t nrow, int64_t ncol, uint64_t *out,
                       int64_t niters){
    int64_t i, j;

    for(i = 0; i < niters; i++){
        uint64_t ind, s;

        s = out[i];
        ind = s >> 48;

        for(j = 0; j < ncol; j++){
            uint64_t x = arr[j];

            x ^= arr[ind * ncol + j];
            s += _popcnt(x);

            arr[j] = x;
        }
        out[i] = s;
    }

    for(i = 0; i < niters; i++){
        out[i] &= _maskr(48);
    }

    return 0;
} /* cnt_bit_arr_nb */

/* convert data into blocked format */
/**
 * \brief Converts the data into a block format
 */
void blockit(uint64_t *data, int64_t nrow, int64_t ncol, uint64_t *work){
    int64_t b, i, j;

    /* loop over blocks */
    for(b = 0; b < (ncol / BLOCKSIZE); b++){
        /* loop over rows in block */
        for(i = 0; i < nrow; i++){
            for(j = 0; j < BLOCKSIZE; j++){
                *work++ = data[b * BLOCKSIZE + i * ncol + j];
            }
        }
    }
}

