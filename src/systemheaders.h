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
#ifndef __SYSTEMHEADERS_H
#define __SYSTEMHEADERS_H

#define _GNU_SOURCE
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <inttypes.h>
#include <time.h>
#ifdef LINUX_PLACEMENT
  #include <utmpx.h>     /* for sched_getcpu */
  #include <sched.h>
#endif

#endif /* __SYSTEMHEADERS_H */
