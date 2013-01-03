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
#ifndef __BRAND_H
#define __BRAND_H

#include <stdint.h>

/* some masking macros */

#define _ZERO64       0uL
#define _maskl(x)     (((x) == 0) ? _ZERO64   : ((~_ZERO64) << (64 - (x))))
#define _maskr(x)     (((x) == 0) ? _ZERO64   : ((~_ZERO64) >> (64 - (x))))
#define _mask(x)      (((x) < 64) ? _maskl(x) : _maskr(2 * 64 - (x)))

/* PRNG */

#define _BR_RUNUP_      128L
#define _BR_LG_TABSZ_    7L
#define _BR_TABSZ_      (1L << _BR_LG_TABSZ_)

/**
 * \brief The struct for holding the BRand info
 */
typedef struct {
    uint64_t hi, lo, ind;
    uint64_t tab[_BR_TABSZ_];
} brand_t;

#define _BR_64STEP_(H,L,A,B) { \
        uint64_t x; \
        x = H ^ (H << A) ^ (L >> (64 - A)); \
        H = L | (x >> (B - 64)); \
        L = x << (128 - B); \
}

uint64_t brand(brand_t *p);

void brand_init(brand_t *p, uint64_t val);

double rfraction(brand_t *p);
#endif // ifndef __BRAND_H

