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
#ifndef __PLANHEADERS_H
#define __PLANHEADERS_H

#include <loadstruct.h>
#include <performance.h>

/*
 * Individual plan header files
 */
#include <plan_sleep.h>
#include <plan_stream.h>
#include <plan_stride.h>
#include <plan_gups.h>
#include <plan_pv1.h>
#include <plan_pv2.h>
#include <plan_pv3.h>
#include <plan_pv4.h>
#include <plan_comm.h>
#include <plan_write.h>
#include <plan_cba.h>
#include <plan_tilt.h>
#include <plan_isort.h>

#ifdef HAVE_BLAS
  #include <plan_dgemm.h>
#endif
#ifdef HAVE_FFTW3
  #include <plan_fftw.h>
#endif
#ifdef HAVE_CUBLAS
  #include <plan_dcublas.h>
  #include <plan_scublas.h>
  #include <plan_cudamem.h>
#endif
#ifdef HAVE_OPENCL
#include <plan_openclmem.h>
#include <plan_dopenclblas.h>
#endif

#define MSG_SIZE 100
#define WIDTH sizeof(int) * 8 / 2 //Find half of the bits used by an int variable.

#define ERR_CLEAN 0xDEADBEEF

// Test plancheck_flag to see if calculation error checking is enabled:
#define CHECK_CALC (plancheck_flag == 1)
#define DO_PERF (planperf_flag == 1)

/**
 * \brief This structure contains the "plan" information for a thread.
 * Specifically, the plan contains pointers to the interface functions
 * for the plan and a pointer to an opaque heap structure allocated by
 * this plan instantiation for its use.
 */
typedef struct {                         /* Plan struct allocated/initialized by a makeXXXPlan() */
    int   (*fptr_initplan)(void *);      /**< Points to a function for initializing plan data.         */
    int   (*fptr_execplan)(void *);      /**< Executes the plan with data from the init function.      */
    int   (*fptr_perfplan)(void *);      /**< Stores and displays performance data from the plan.      */
    void * (*fptr_killplan)(void *);     /**< Cleans up after the plan and returns performance data.   */
    void *vptr;                          /**< Pointer to the plan's specialized heap.                  */
    int   name;                          /**< Stores the plan ID number for reference.                 */
    PerfTimers timers;                   /**< Contains pairs of time stamps for measuring performance. */
    uint64_t exec_count;                 /**< Stores the number of times the exec function is called.  */

    #ifdef HAVE_PAPI
    int PAPI_EventSet;                              /* Holds the PAPI event set for this plan            */
    int PAPI_Num_Events;                            /* Holds the number of PAPI events for this plan     */
    long long PAPI_Times [TOTAL_PAPI_EVENTS];       /* Holds the timing data for the PAPI counters       */
    long long PAPI_Results [TOTAL_PAPI_EVENTS];     /* Holds the values collected from the PAPI counters */
    #endif
} Plan;

/**
 * \brief Holds the generic error types in readable form
 */
enum {
    ALLOC,
    CALC
};

/**
 * \brief Tells the error-maker whether to get the message from the generic list or from the custom list provided in the plan
 */
enum {
    generic_err,
    specific_err
};

/**
 * \brief Holds the names of the plans to use in conjunction with plan_list[]
 * \sa plan_list
 */
typedef enum {
    SYSTEM = -1,        // For use with error generation, plan # -1 gives the system error list.
    SLEEP,
    #ifdef HAVE_BLAS
    DGEMM,
    RDGEMM,
    #endif
    #ifdef HAVE_FFTW3
    FFT1D,
    FFT2D,
    #endif
    #ifdef HAVE_CUBLAS
    DCUBLAS,
    SCUBLAS,
    CUDAMEM,
    #endif
    DSTREAM,
    DSTRIDE,
    LSTREAM,
    LSTRIDE,
    GUPS,
    PV1,
    PV2,
    PV3,
    PV4,
    SBCOMM,
    WRITE,
    CBA,
    TILT,
    ISORT,
    #ifdef HAVE_OPENCL
    OPENCLMEM,
    DOPENCLBLAS,
    #endif
    UNKN_PLAN     /* Tells when the plan name is unrecognized.*/
} plan_choice;

#define ERR_FLAG_SIZE ((UNKN_PLAN) +1)  // Make space to push SYSTEM to 0 when arrayed
#define NUM_PLANS      (UNKN_PLAN)      // Determines the number of plans available.

#ifdef PLANS
  #ifndef USED
    #define USED
/* struct plan_info is defined in src/loadstruct.h */
/**
 * \brief Holds the information for all of the plans in SystemBurn. Accessed through the plan_choice enum
 * \sa plan_choice
 */
plan_info *plan_list[] = {
    &(SLEEP_info),
    #ifdef HAVE_BLAS
    &(DGEMM_info),
    &(RDGEMM_info),
    #endif
    #ifdef HAVE_FFTW3
    &(FFT1_info),
    &(FFT2_info),
    #endif
    #ifdef HAVE_CUBLAS
    &(DCUBLAS_info),
    &(SCUBLAS_info),
    &(CUDAMEM_info),
    #endif
    &(DSTREAM_info),
    &(DSTRIDE_info),
    &(LSTREAM_info),
    &(LSTRIDE_info),
    &(GUPS_info),
    &(PV1_info),
    &(PV2_info),
    &(PV3_info),
    &(PV4_info),
    &(COMM_info),
    &(WRITE_info),
    &(CBA_info),
    &(TILT_info),
    &(ISORT_info),
#ifdef HAVE_OPENCL
    &(OPENCL_MEM_info),
    &(DOPENCLBLAS_info),
#endif
    &(SLEEP_info)     // Default for unknown plans.
};
  #endif // USED
#endif // PLANS

extern plan_info *plan_list[];
extern char *generic_errors[]; // Initialized in planlib/plan_misc.c
#define GEN_SIZE 2
extern char *system_error[];
#define SYS_ERR_SIZE 3
extern void add_error(void *m, int name, int error);
extern int make_error(int i, int j);

#define MAX_GEN_VAL ((1 << WIDTH) - GEN_SIZE)
#define ABS(a) ((a < 0) ? -a : a)

#ifdef BUILD_MSG
/**
 * \brief Builds an error message, based on the parameters given. It either get the text from a generic list of messages or from a custom list provided within the plan definition.
 * \sa generic_errors
 */
char *build_msg(int plan,int error){
    char *message = (char *)malloc(sizeof(char) * MSG_SIZE);
    plan--;
    if((plan <= ERR_FLAG_SIZE) && (plan >= 0) ){
        strcpy(message, plan_list[plan]->name);
        if(error < GEN_SIZE ){
            strcat(message, generic_errors[error]);
        } else {
            error -= GEN_SIZE;
            if(plan_list[plan]->error && plan_list[plan]->error[error]){
                strcat(message,plan_list[plan]->error[error]);
            } else {
                strcat(message," unknown error:");
            }
        }
    } else if(plan < 0){
        if(system_error[error]){
            strcpy(message, system_error[error]);
        } else {
            strcpy(message,"Unkown error:");
        }
    } else {
        strcpy(message, "Unknown error:");
    }
    return message;
} // build_msg

#endif // ifdef BUILD_MSG

/*
 * Miscellaneous functions used in processing plans.
 */
extern plan_choice setPlan(char *planName);
extern char *printPlan(plan_choice plan);
extern data *get_sizes(char *line);
extern int key_gen(int a, int b);
extern int *key_conv(int a);
extern void tokenize_line(char *line, char ***tokens, int *count);

#endif /* __PLANHEADERS_H */
