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
#ifndef __PLAN_DGEMM_H
#define __PLAN_DGEMM_H

#include <loadstruct.h>

extern void * makeDGEMMPlan(data *i);	/* creates a plan struct       */
extern int    initDGEMMPlan(void *p);	/* inits plan's vptr           */
extern int    execDGEMMPlan(void *p);	/* run the sleep plan          */
extern int    perfDGEMMPlan(void *p);
extern void * killDGEMMPlan(void *p);	/* clean up & free plan & vptr */
extern int    parseDGEMMPlan(char *line, LoadPlan *output);
extern plan_info DGEMM_info;

/* Macros, Enums, Prototypes for DGEMM */
#define CBLAS_INDEX size_t  /* this may vary between platforms */
enum CBLAS_ORDER {CblasRowMajor=101, CblasColMajor=102};
enum CBLAS_TRANSPOSE {CblasNoTrans=111, CblasTrans=112, CblasConjTrans=113};
enum CBLAS_UPLO {CblasUpper=121, CblasLower=122};
enum CBLAS_DIAG {CblasNonUnit=131, CblasUnit=132};
enum CBLAS_SIDE {CblasLeft=141, CblasRight=142};
void cblas_dgemm(const enum CBLAS_ORDER Order,
		 const enum CBLAS_TRANSPOSE TransA,
		 const enum CBLAS_TRANSPOSE TransB,
		 const int M, const int N, const int K,
		 const double alpha,
		 const double *A, const int lda,
		 const double *B, const int ldb,
		 const double beta,
		 double *C, const int ldc);

/* DGEMM caller data structure */
/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
typedef struct {
	int M;
	double *A, *B, *C;
} DGEMMdata;

/**
 * \brief The data structure for the plan. Holds the input and all used info. 
 */
extern void * makeRDGEMMPlan(data *i);
extern int    initRDGEMMPlan(void *p);
extern int    execRDGEMMPlan(void *p);
extern int    perfRDGEMMPlan(void *p);
extern void * killRDGEMMPlan(void *p);
extern int    parseRDGEMMPlan(char *line, LoadPlan *output);
extern plan_info RDGEMM_info;

typedef struct {
	int M, N;
	double *A, *B, *C;
} RDGEMMdata;

#endif /* __PLAN_DGEMM_H */
