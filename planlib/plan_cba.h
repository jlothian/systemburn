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
#ifndef __PLAN_CBA_H
#define __PLAN_CBA_H

#include <brand.h>

/**** From cpu_suite cba.h **********/
#define BLOCKSIZE    32L            /* MUST be a power of 2
                                     * and <= 64 due to our test params */
#define NITERS       60 * 64L         /* should be a multiple of 64 */
#define PAD          16L

/* restricted pointers */
#ifndef RESTRICT
/* if no support */
  #define RESTRICT
#endif

#define CBA_M1 0x5555555555555555uL
#define CBA_M2 0x3333333333333333uL
#define CBA_M3 0x0f0f0f0f0f0f0f0fuL

#ifdef USE_POP3

  #define POPCNT3(A,B,C,Z) { \
        uint64_t X,Y; \
        X = ( (A)       &CBA_M1) + ( (B)       &CBA_M1) + ( (C)       &CBA_M1); \
        Y = (((A) >> 1) & CBA_M1) + (((B) >> 1) & CBA_M1) + (((C) >> 1) & CBA_M1); \
        Z = (X & CBA_M2) + ((X >> 2) & CBA_M2) + \
            (Y & CBA_M2) + ((Y >> 2) & CBA_M2); \
        Z = (Z & CBA_M3) + ((Z >> 4) & CBA_M3); \
        Z = Z + (Z >> 8); \
        Z = Z + (Z >> 16); \
        Z = Z + (Z >> 32); \
        Z &= 0xff; \
}

#else /* undef USE_POP3 */

  #define _MASK1BIT  0x5555555555555555uL
  #define _MASK2BITS 0x3333333333333333uL
  #define _MASK4BITS 0x0f0f0f0f0f0f0f0fuL

static int64_t _popcnt(uint64_t x){
    x = x - ((x >> 1) & _MASK1BIT);
    x = ((x >> 2) & _MASK2BITS) + (x & _MASK2BITS);
    x = ((x >> 4) + x) & _MASK4BITS;
    x = (x >>  8) + x;
    x = (x >> 16) + x;
    x = (x >> 32) + x;

    return x & 0xff;
}

//#else /* undef USE_POP3 */

/* #define your _popcnt() macro here */

//#include <nmmintrin.h>
//#include <x86intrin.h>
//#define _popcnt(x) _mm_popcnt_u64(x);

#endif // ifdef USE_POP3

int64_t cnt_bit_arr(uint64_t *arr, int64_t nrow, int64_t ncol, uint64_t *out,
                    int64_t niters);
int64_t cnt_bit_arr_nb(uint64_t *arr, int64_t nrow, int64_t ncol, uint64_t *out,
                       int64_t niters);
void blockit(uint64_t *data, int64_t nrow, int64_t ncol, uint64_t *work);

extern void *makeCBAPlan(data *i);
extern int initCBAPlan(void *p);
extern int execCBAPlan(void *p);
extern int perfCBAPlan(void *p);
extern void *killCBAPlan(void *p);
extern int parseCBAPlan(char *line, LoadPlan *output);
extern plan_info CBA_info;

//If there are other functions to be used by your module, declare the prototypes here. Examples in plan_dstream.h and plan_istream.h

//Below is a sample data structure. This is optional, but your module will probably need some form of input to function.
/**
 * \brief The data structure for the plan. Holds the input and all used info.
 */
typedef struct {
    int niter;
    int seed;
    int nrows;
    int ncols;
    brand_t br;
    uint64_t *data, *chk, *work, *out;
} CBA_data;

#endif /* __PLAN_CBA_H */
