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
#include <initialization.h>
#include <planheaders.h>
#include <comm.h>

#ifdef HAVE_SHMEM
  #include <mpp/shmem.h>
#else
  #include <mpi.h>
#endif

#ifdef HAVE_SHMEM
  #ifdef SLU       /* Cray-like implementation */
  #  define   SHMEM_NUM_PES  num_pes()
  #  define   SHMEM_MY_PE    my_pe()
  #else	           /* SGI-like implementation */
  # define    SHMEM_NUM_PES _num_pes()
  # define    SHMEM_MY_PE   _my_pe()
  #endif
#endif

#ifndef MAX
#define MAX(A,B) ( ((A) > (B)) ? (A) : (B) )
#endif

/*
 * Abstractions of SPMD network communication functions to
 * facilitate the inclusion of multiple libraries.
 * NOTE: much of the code below was buggy and commented out
 *       in favor of simply having each node read the 
 *       command line, config file, and load file. should
 *       this prove to be scaling problem (it may) the files
 *       are small enough that you can bcast the whole file
 *       and heave each node parse it independently
*/

/*
 * functions requiring comm:
 * 	- main
 * 	- bcastConfig (in initialization.c)
 * 	- bcastLoad (in load.c)
 * 	- reduceTemps (in monitor.c)
 * 	- reduceFlags (in worker.c)
 *	- perf_reduce (in performance.c)
 * 	- comm test plan
 */

/* Symmtric workspace data required by broadcasts */
#ifdef HAVE_SHMEM
static long   pSync[SHMEM_BCAST_SYNC_SIZE];
static long   qSync[SHMEM_REDUCE_SYNC_SIZE];
static long   rSync[SHMEM_REDUCE_SYNC_SIZE];
static float  pWrkFloat[SHMEM_REDUCE_MIN_WRKDATA_SIZE];
static int    bint1, bint2;
static float  tmin, tavg, tmax;
static float  lmin, lavg, lmax;
#endif

/**
 * \brief Sets up the communication systems for SystemBurn 
 * \param argc Typical C variable: number of arguments in the command line
 * \param argv Typical C variable: array of command line arguments
 */
void comm_setup(int *argc, char ***argv) {
#ifdef HAVE_SHMEM
	int i;
	start_pes(0);
	for (i = 0; i < SHMEM_BCAST_SYNC_SIZE; i++) {
		pSync[i] = SHMEM_SYNC_VALUE;
	}
	for (i = 0; i < SHMEM_REDUCE_SYNC_SIZE; i++) {
		rSync[i] = SHMEM_SYNC_VALUE;
		qSync[i] = SHMEM_SYNC_VALUE;
	}
#else
	MPI_Init(argc, argv);
#endif
}

/**
 * \brief Retrieves the rank of the calling process
 * \return rank
 */
int comm_getrank() {
#ifdef HAVE_SHMEM
	return SHMEM_MY_PE;
#else
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	return rank;
#endif
}

/**
 * \brief Send a value out to all of the threads
 * \param value Value to be send (broadcast) to all the threads
 */
void comm_broadcast_int(int *value) {
#ifdef HAVE_SHMEM
	static int debug = 0;
	int i, commsize = SHMEM_NUM_PES;
	assert(value);
	bint1=*value;
	bint2=*value;
	shmem_barrier_all();
	shmem_broadcast32(&bint2, &bint1, SINGLE, ROOT, 0, 0, commsize, pSync);
	shmem_barrier_all();
	bint1=bint2;
	*value=bint1;
	debug++;
#else
	MPI_Bcast((void*)value, SINGLE, MPI_INT, ROOT, MPI_COMM_WORLD);
#endif
}

/**
 * \brief Drop everything and abort the run
 * \param e_code The error code that explains why the program was aborted
 */
void comm_abort(int e_code) {
#ifndef HAVE_SHMEM
	MPI_Abort(MPI_COMM_WORLD, e_code);
#endif
}

/**
 * \brief Wrap up the run, freeing the threads
 */
void comm_finalize() {
#ifdef HAVE_SHMEM
	shmem_barrier_all();
#  ifdef SLU
	shmem_finalize();
#  endif
#else
	MPI_Finalize();
#endif
}

/**
 * \brief Gather the values of the temperatures from the various nodes to check if any are overheating - MPI
 */
void reduceTemps_MPI() {
#ifndef HAVE_SHMEM
        int commsize;
	float tmin, tavg, tmax;
	MPI_Reduce(&local_temp.min, &tmin, 1, MPI_FLOAT, MPI_MIN, ROOT, MPI_COMM_WORLD);
	MPI_Reduce(&local_temp.avg, &tavg, 1, MPI_FLOAT, MPI_SUM, ROOT, MPI_COMM_WORLD);
        MPI_Reduce(&local_temp.max, &tmax, 1, MPI_FLOAT, MPI_MAX, ROOT, MPI_COMM_WORLD);
	if(MyRank == ROOT) {
		MPI_Comm_size(MPI_COMM_WORLD,&commsize);
		tavg = tavg / commsize;
		if (tmin <= tmax) {
			EmitLog3f(MyRank, SCHEDULER_THREAD, "The minimum/average/maximum temperature across all cores (C) ", tmin, tavg, tmax, PRINT_ALWAYS);
		} else {
			EmitLog(MyRank, SCHEDULER_THREAD, "No valid temperature monitoring interface on this platform.", -1, PRINT_ALWAYS);
		}
        }
#endif
}

/**
 * \brief Gather the values of the temperatures from the various nodes to check if any are overheating - SHMEM 
 */
void reduceTemps_SHMEM() {
#ifdef HAVE_SHMEM
	int commsize;
	// int i;
	// assert(pWrkFloat != NULL);
	// long *pSync = (long *)shmalloc(_SHMEM_REDUCE_SYNC_SIZE * sizeof(long));
	// assert(pSync != NULL);
	// for (i = 0; i < _SHMEM_REDUCE_SYNC_SIZE; i++) pSync[i] = _SHMEM_SYNC_VALUE;	

	// Can use pWrkFloat with length = SHMEM_REDUCE_MIN_WRKDATA_SIZE, because none of these
	// reductions will ever be longer than length 1.

	lmin = local_temp.min;
	lavg = local_temp.avg;
	lmax = local_temp.max;
	commsize = SHMEM_NUM_PES;
	shmem_barrier_all();
	shmem_float_min_to_all(&tmin, &lmin, 1, 0, 0, commsize, pWrkFloat, rSync);
	shmem_barrier_all();
	shmem_float_sum_to_all(&tavg, &lavg, 1, 0, 0, commsize, pWrkFloat, rSync);
	shmem_barrier_all();
	shmem_float_max_to_all(&tmax, &lmax, 1, 0, 0, commsize, pWrkFloat, rSync);
	shmem_barrier_all();
	
	if (MyRank == ROOT) {
		tavg = tavg / commsize;
		if (tmin <= tmax) {
			EmitLog3f(MyRank, SCHEDULER_THREAD, "The minimum/average/maximum temperature across all cores (C) ", tmin, tavg, tmax, PRINT_ALWAYS);
		} else {
			EmitLog(MyRank, SCHEDULER_THREAD, "No valid temperature monitoring interface on this platform.", -1, PRINT_ALWAYS);
		}
	}
	
	// shfree(pWrkFloat);
	// shfree(pSync);
#endif
}

/**
 * \brief Gather the error flags from the threads to report - MPI
 * \param local_flag 2D array of ints that holds the flag counters for the plans' errors. Keeps count of how many times an error was encountered.
 */
void reduceFlags_MPI(int **local_flag) {
#ifndef HAVE_SHMEM
	int i, j, k;
	int **sum = NULL;
	
	sum = (int **)malloc(ERR_FLAG_SIZE * sizeof(int*));
	assert(sum);
	// Allocate space for system flags first.
	sum[0] = (int *)calloc((SYS_ERR_SIZE), sizeof(int));
	assert(sum[0]);
	// Allocate space for plan flags.
	for (j = 1; j < ERR_FLAG_SIZE; j++) {
		sum[j] = (int*)calloc((plan_list[j-1]->esize+GEN_SIZE), sizeof(int));
		assert(sum[j]);
	}
	
	// Reduce system flags.
	MPI_Reduce(local_flag[0], sum[0], (SYS_ERR_SIZE), MPI_INT, MPI_SUM, ROOT, MPI_COMM_WORLD);
	// Reduce Plan flags.
	for(i = 1; i < ERR_FLAG_SIZE; i++) {
		MPI_Reduce(local_flag[i], sum[i], (plan_list[i-1]->esize + GEN_SIZE), MPI_INT, MPI_SUM, ROOT, MPI_COMM_WORLD);
	}

	if (MyRank == ROOT) {
		printFlags(sum);
	}

	for(i = 0; i < ERR_FLAG_SIZE; i++) free(sum[i]);
	free(sum);
#endif
}

/**
 * \brief Gather the error flags from the threads to report - SHMEM
 * \param local_flag 2D array of ints that holds the flag counters for the plans' errors. Keeps count of how many times an error was encountered.
 */
void reduceFlags_SHMEM(int **local_flag) {
#ifdef HAVE_SHMEM
	int i, j, k, commsize;
	int **sum, *temp;
	int largest = 0;

	// dangerous crap here.....
	sum  = (int **)shmalloc(ERR_FLAG_SIZE * sizeof(int*));
	assert(sum);
	// Allocate space for system flags.
	sum[0] = (int *)shmalloc((SYS_ERR_SIZE) * sizeof(int));
	assert(sum[0]);
	largest = SYS_ERR_SIZE;
	// Allocate space for plan flags.
	for (j = 1; j < ERR_FLAG_SIZE; j++) {
		sum[j] = (int*)shmalloc((plan_list[j-1]->esize+GEN_SIZE) * sizeof(int));
		
		largest = (plan_list[j-1]->esize > largest) ? plan_list[j-1]->esize : largest;
	}

	temp = (int *)shmalloc((largest + GEN_SIZE) * sizeof(int));
	assert(temp);

	// assert(pWrkInt != NULL);
	// long *pSync = (long *)shmalloc(_SHMEM_REDUCE_SYNC_SIZE * sizeof(long));
	// assert(pSync != NULL);
	// for (i = 0; i < _SHMEM_REDUCE_SYNC_SIZE; i++) pSync[i] = _SHMEM_SYNC_VALUE;	
	commsize = SHMEM_NUM_PES;

	// Allocate a work array of appropriate size:
	int pWrkLength = MAX(largest/2 + 1, SHMEM_REDUCE_MIN_WRKDATA_SIZE);
	int *pWrk = (int *)shmalloc(pWrkLength * sizeof(int));
	assert(pWrk);
	
	for (i = 0; i < SYS_ERR_SIZE; i++) {
		sum[0][i] = local_flag[0][i];
	}
	for (j = 1; j < ERR_FLAG_SIZE; j++) {
		for(k=0;k<plan_list[j-1]->esize+GEN_SIZE;k++) {
			sum[j][k] = local_flag[j][k];
		}
	}
	
	// Reduce system flags.
	shmem_barrier_all();
	shmem_int_sum_to_all(temp, sum[0], (SYS_ERR_SIZE), 0, 0, commsize, pWrk, rSync);
	shmem_barrier_all();
	for (j = 0; j < (SYS_ERR_SIZE); j++) {
		sum[0][j] = temp[j];
	}
	// Reduce plan flags.
	for(i=1;i<ERR_FLAG_SIZE;i++) {
		shmem_int_sum_to_all(temp, sum[i], plan_list[i-1]->esize+GEN_SIZE, 0, 0, commsize, pWrk, rSync);
		shmem_barrier_all();
		for (j = 0; j < (plan_list[i-1]->esize + GEN_SIZE); j++) {
			sum[i][j] = temp[j];
		}
	}
	
	if (MyRank == ROOT) {
		printFlags(sum);
	}
	
	for(i=0;i<ERR_FLAG_SIZE;i++) shfree(sum[i]);
	shfree(sum);
	shfree(temp);
	shfree(pWrk);
	// shfree(pSync);
#endif
}

/**
 * \brief Reduces a table of data from each node to a single table on the root node by performing the specified operation. MPI implementation.
 * 
 * \param [in,out] table The table of data to be reduced. The resulting table is then copied into it as output.
 * \param [in] nrows The number of rows in the table.
 * \param [in] ncols The number of columns in the table.
 * \param [in] op The reduction operation to perform (sum, min, max).
 */
void comm_table_reduce_MPI(void *table, int nrows, int ncols, reduction_op op) {
#ifndef HAVE_SHMEM
	double *receive_buffer = NULL;
	MPI_Op operation;
	
	switch (op) {
		case REDUCE_SUM:
			operation = MPI_SUM;
			break;
		case REDUCE_MIN:
			operation = MPI_MIN;
			break;
		case REDUCE_MAX:
			operation = MPI_MAX;
			break;
		default:
			operation = MPI_SUM;
	}
	
	if (MyRank == ROOT) {
		receive_buffer = (double *)malloc(nrows * ncols * sizeof(double));
		assert(receive_buffer);
	}
	
	MPI_Reduce(table, receive_buffer, nrows*ncols, MPI_DOUBLE, operation, ROOT, MPI_COMM_WORLD);
	
	if (MyRank == ROOT) {
		memcpy(table, receive_buffer, nrows * ncols * sizeof(double));
		free(receive_buffer);
	}
#endif
}

/**
 * \brief Reduces a table of data from each node to a single table on the root node by performing the specified operation. SHMEM implementation.
 * 
 * \param [in,out] table The table of data to be reduced. The resulting table is then copied into it as output.
 * \param [in] nrows The number of rows in the table.
 * \param [in] ncols The number of columns in the table.
 * \param [in] op The reduction operation to perform (sum, min, max).
 */
void comm_table_reduce_SHMEM(void *table, int nrows, int ncols, reduction_op op) {
#ifdef HAVE_SHMEM
	int commsize = SHMEM_NUM_PES;
	double *send_buffer;
	double *receive_buffer;
	double *pWrk;
	send_buffer = (double *)shmalloc(nrows * ncols * sizeof(double));
	assert(send_buffer);
	receive_buffer = (double *)shmalloc(nrows * ncols * sizeof(double));
	assert(receive_buffer);

	// Allocate a work array of appropriate size.
	int pWrkLength = MAX((nrows * ncols)/2 + 1, SHMEM_REDUCE_MIN_WRKDATA_SIZE);
	pWrk = (double *)shmalloc(pWrkLength * sizeof(double));
	assert(pWrk);
	
	memcpy(send_buffer, table, nrows * ncols * sizeof(double));
	
	shmem_barrier_all();
	switch (op) {
		case REDUCE_SUM:
			shmem_double_sum_to_all(receive_buffer, send_buffer, nrows * ncols, 0, 0, commsize, pWrk, rSync);
			break;
		case REDUCE_MIN:
			shmem_double_min_to_all(receive_buffer, send_buffer, nrows * ncols, 0, 0, commsize, pWrk, rSync);
			break;
		case REDUCE_MAX:
			shmem_double_max_to_all(receive_buffer, send_buffer, nrows * ncols, 0, 0, commsize, pWrk, rSync);
			break;
		default:
			shmem_double_sum_to_all(receive_buffer, send_buffer, nrows * ncols, 0, 0, commsize, pWrk, rSync);
	}
	shmem_barrier_all();
	
	memcpy(table, receive_buffer, nrows * ncols * sizeof(double));
	
	shfree(send_buffer);
	shfree(receive_buffer);
	shfree(pWrk);
#endif
}

/**
 \brief Broadcasts a memory buffer from the root node to all nodes. MPI implementation.
 \param [in,out] buffer The data to be broadcast: input recieved from root and output on all other nodes.
 \param [in] buffer_size The size in bytes of the buffer.
 \returns Error codes from the communication libraries.
 */
int comm_broadcast_buffer_MPI(void *buffer, int buffer_size) {
	int error = 0;
#ifndef HAVE_SHMEM
	error = MPI_Bcast(buffer, buffer_size, MPI_CHAR, ROOT, MPI_COMM_WORLD);
#endif
	return error;
}

/**
 \brief Broadcasts a memory buffer from the root node to all nodes. SHMEM implementation.
 \param [in,out] buffer The data to be broadcast: input recieved from root and output on all other nodes.
 \param [in] buffer_size The size in bytes of the buffer.
 \returns Error codes from the communication libraries.
 */
int comm_broadcast_buffer_SHMEM(void *buffer, int buffer_size) {
	int error = 0;
#ifdef HAVE_SHMEM
	int commsize = SHMEM_NUM_PES;
	uint64_t *sbuf;
	size_t sbuf_len;
	
	/* Allocate a symmetric buffer. */
	sbuf_len = (buffer_size/sizeof(uint64_t) + 1);
	sbuf = (uint64_t *)shmalloc(sbuf_len * sizeof(uint64_t));
	assert(sbuf);
	
	/* On root, copy data into send buffer. */
	if (MyRank == ROOT) memcpy(sbuf, buffer, buffer_size);
	
	/* Synchronize PEs and broadcast. */
	shmem_barrier_all();
	shmem_broadcast64(sbuf, sbuf, sbuf_len, ROOT, 0, 0, commsize, pSync);
	shmem_barrier_all();
	
	/* Every PE that is not root, copy to buffer. */
	if (MyRank != ROOT) memcpy(buffer, sbuf, buffer_size);
	
	/* Free symmetric buffer. */
	shfree(sbuf);
#endif
	return error;
}

// Intentional?

/*
void *config_buffer_create_SHMEM(int buffer_size, int num_loads) {
	int *int_buffer = NULL;
	int buff_index = 0;
#ifdef HAVE_SHMEM
	int_buffer = (int *)shmalloc(buffer_size * sizeof(int));
	assert(int_buffer);
	if (MyRank == ROOT) {
		buff_index=0;
		// for (buff_index = 0; buff_index < ARRAY; buff_index++) {
			// int_buffer[buff_index] = (int)temperature_path[buff_index]; // JAK this should be an array of char....
		// }
		
		int_buffer[buff_index++] = num_workers;
		int_buffer[buff_index++] = num_loads;
		int_buffer[buff_index++] = thermal_panic;
		int_buffer[buff_index++] = thermal_relaxation_time;
		int_buffer[buff_index++] = monitor_frequency;
		int_buffer[buff_index++] = monitor_output_frequency;
		int_buffer[buff_index++] = verbose_flag;
		int_buffer[buff_index]   = comm_flag;
	}
#endif
	return (void *)int_buffer;
}

void *config_buffer_create_MPI(int buffer_size, int num_loads) {
	int buff_index = 0;
	char *char_buffer = NULL;
#ifndef HAVE_SHMEM
	char_buffer = (char *)malloc(buffer_size);
	assert(char_buffer);
	
	if (MyRank == ROOT) {
		MPI_Pack(&num_workers,              SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
		MPI_Pack(&num_loads,                SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
		MPI_Pack(&thermal_panic,            SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
		MPI_Pack(&thermal_relaxation_time,  SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
		MPI_Pack(&monitor_frequency,        SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
		MPI_Pack(&monitor_output_frequency, SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
		MPI_Pack(&verbose_flag,             SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
		MPI_Pack(&comm_flag,                SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
	}
#endif
	return (void *)char_buffer;
}

void *config_buffer_create(int buffer_size, int num_loads) {
#ifdef HAVE_SHMEM
	return config_buffer_create_SHMEM(buffer_size, num_loads);
#else
	return config_buffer_create_MPI(buffer_size, num_loads);
#endif
}

void config_buffer_destroy_SHMEM(void *buffer, int buffer_size, int *num_loads) {
#ifdef HAVE_SHMEM
	int buff_index = 0;
	int *int_buffer = (int *)buffer;
	
	if (MyRank != ROOT) {
		// for (buff_index = 0; buff_index < ARRAY; buff_index++) {
			// temperature_path[buff_index] = (char)int_buffer[buff_index];
		// }
		
		buff_index=0;
		num_workers              = int_buffer[buff_index++];
		*num_loads               = int_buffer[buff_index++];
		thermal_panic            = int_buffer[buff_index++];
		thermal_relaxation_time  = int_buffer[buff_index++];
		monitor_frequency        = int_buffer[buff_index++];
		monitor_output_frequency = int_buffer[buff_index++];
		verbose_flag             = int_buffer[buff_index++];
		comm_flag                = int_buffer[buff_index];
	}
	
	shfree(buffer);
	buffer = NULL;
#endif
	return;
}

void config_buffer_destroy_MPI(void *buffer, int buffer_size, int *num_loads) {
#ifndef HAVE_SHMEM
	int buff_index = 0;
	char *char_buffer = (char *)buffer;
	
	if (MyRank != ROOT) {
		MPI_Unpack(char_buffer, buffer_size, &buff_index, &num_workers,              SINGLE, MPI_INT,  MPI_COMM_WORLD);
		MPI_Unpack(char_buffer, buffer_size, &buff_index, num_loads,                 SINGLE, MPI_INT,  MPI_COMM_WORLD);
		MPI_Unpack(char_buffer, buffer_size, &buff_index, &thermal_panic,            SINGLE, MPI_INT,  MPI_COMM_WORLD);
		MPI_Unpack(char_buffer, buffer_size, &buff_index, &thermal_relaxation_time,  SINGLE, MPI_INT,  MPI_COMM_WORLD);
		MPI_Unpack(char_buffer, buffer_size, &buff_index, &monitor_frequency,        SINGLE, MPI_INT,  MPI_COMM_WORLD);
		MPI_Unpack(char_buffer, buffer_size, &buff_index, &monitor_output_frequency, SINGLE, MPI_INT,  MPI_COMM_WORLD);
		MPI_Unpack(char_buffer, buffer_size, &buff_index, &verbose_flag,             SINGLE, MPI_INT,  MPI_COMM_WORLD);
		MPI_Unpack(char_buffer, buffer_size, &buff_index, &comm_flag,                SINGLE, MPI_INT,  MPI_COMM_WORLD);
	}
	
	free(buffer);
	buffer = NULL;
#endif
	return;
}

void config_buffer_destroy(void *buffer, int buffer_size, int *num_loads) {
#ifdef HAVE_SHMEM
	config_buffer_destroy_SHMEM(buffer, buffer_size, num_loads);
#else
	config_buffer_destroy_MPI(buffer, buffer_size, num_loads);
#endif
	return;
}

int bcastConfig_MPI(int load_num) {
#ifndef HAVE_SHMEM
	char *buffer;
	int buff_sz = 0;
	
	// create buffer of proper size
	buff_sz = sizeof(int)*8;
	
	// Process 0: pack all global parameters into a buffer and send to all other processes.
	buffer = (char *)config_buffer_create(buff_sz, load_num);
	
	// Call MPI broadcast routine. 
	MPI_Bcast(buffer,           buff_sz, MPI_PACKED, ROOT, MPI_COMM_WORLD);
	MPI_Bcast(temperature_path, ARRAY,   MPI_CHAR,   ROOT, MPI_COMM_WORLD);
	
	// All nonzero processes: receive broadcast and unpack from the buffer into global variables.
	config_buffer_destroy((void *)buffer, buff_sz, &load_num);
	
#endif
	
	// Return the number of loads to be run.
	return load_num;
}

int bcastConfig_SHMEM(int load_num) {
#ifdef HAVE_SHMEM
	int *buffer;
	int buff_sz = 0;
	int commsize;
	// long *pSync;
	int i;
	
	buff_sz = 8 + ARRAY;
	commsize = SHMEM_NUM_PES;
	
	buffer = (int *)config_buffer_create(buff_sz, load_num);
	
	// pSync = (long *)shmalloc(_SHMEM_BCAST_SYNC_SIZE * sizeof(long));
	// assert(pSync);
	// for (i = 0; i < _SHMEM_BCAST_SYNC_SIZE; i++) { pSync[i] = _SHMEM_SYNC_VALUE; }
	
	shmem_barrier_all();
	
	shmem_broadcast32(buffer, buffer, buff_sz, ROOT, 0, 0, commsize, pSync);
	
	config_buffer_destroy((void *)buffer, buff_sz, &load_num);
	
	shmem_barrier_all();
	
	// shfree(pSync);
#endif
	return load_num;
}

int buffer_pack(void *src, int length, int type_size, void *buff, int buff_len, int *buff_ind) {
	char *source = (char *)src;
	char *buffer = (char *)buff;
	int i, b = *buff_ind;

	for (i = 0; (i < length * type_size) && (b < buff_len); i++, b++) {
		buffer[b] = source[i];
	}

	*buff_ind = b;
	return (b == buff_len) && (i < length * type_size);
}

int buffer_unpack(void *buff, int buff_len, int *buff_ind, void *dest, int length, int type_size) {
	char *destination = (char *)dest;
	char *buffer      = (char *)buff;
	int i, b = *buff_ind;

	for (i = 0; (i < length * type_size) && (b < buff_len); i++, b++) {
		destination[i] = buffer[b];
	}
	
	*buff_ind = b;
	return (b == buff_len) && (i < length * type_size);
}

int load_buffer_size(Load *load, int *num_subloads) {
	int buffer_size = 0;
	int nsubloads = 0;
	int i;
	SubLoad *subload_ptr = NULL;
	LoadPlan   *plan_ptr = NULL;
	
	// count subloads and calculate buffer_size
	// Note: since all the values to be packed are integers,
	//      we can just count the integers and then
	//      multiply by sizeof(int) at the end
	if (MyRank == ROOT) {
		// size of load struct + num_subloads
		buffer_size = 5 * sizeof(int);
		// loop through subloads
		subload_ptr = load->front;
		while (subload_ptr != NULL) {
			// count subloads
			nsubloads++;
			// size of subload
			buffer_size += (2 + subload_ptr->cpuset_len) * sizeof(int);
			// loop through load plans
			plan_ptr = subload_ptr->first;
			while (plan_ptr != NULL) {
				// size of load plan
				buffer_size += 4 * sizeof(int);
				buffer_size += plan_ptr->input_data->isize * sizeof(int);
				buffer_size += plan_ptr->input_data->csize * sizeof(int);	// the length of each string in the array
				buffer_size += plan_ptr->input_data->dsize * sizeof(double);
				for (i = 0; i < plan_ptr->input_data->csize; i++) {
					buffer_size += (strlen(plan_ptr->input_data->c[i]) + 1) * sizeof(char);
				}
				plan_ptr = plan_ptr->next;
			}

			subload_ptr = subload_ptr->next;
		}
	}

	*num_subloads = nsubloads;
	return buffer_size;
}

void *load_buffer_create_SHMEM(Load *load, int num_subloads, int buffer_size) {
	char *char_buffer = NULL;
	int i, buff_index = 0;
	int string_len, err = 0;
	SubLoad *subload_ptr = NULL;
	LoadPlan *plan_ptr = NULL;
#ifdef HAVE_SHMEM
	char_buffer = (char *)shmalloc(buffer_size * sizeof(char));
	assert(char_buffer);
	
	if (MyRank == ROOT) {
		err += buffer_pack(&(load->num_threads), SINGLE, sizeof(int), char_buffer, buffer_size, &buff_index);
		err += buffer_pack(&(load->num_cpusets), SINGLE, sizeof(int), char_buffer, buffer_size, &buff_index);
		err += buffer_pack(&(load->runtime)    , SINGLE, sizeof(int), char_buffer, buffer_size, &buff_index);
		err += buffer_pack(&(load->scheduling) , SINGLE, sizeof(int), char_buffer, buffer_size, &buff_index);
		err += buffer_pack(&(num_subloads)     , SINGLE, sizeof(int), char_buffer, buffer_size, &buff_index);
		
		subload_ptr = load->front;
		while (subload_ptr != NULL) {
			err += buffer_pack(&(subload_ptr->num_plans) , SINGLE, sizeof(int), char_buffer, buffer_size, &buff_index);
			err += buffer_pack(&(subload_ptr->cpuset_len), SINGLE, sizeof(int), char_buffer, buffer_size, &buff_index);
			err += buffer_pack(subload_ptr->cpuset , subload_ptr->cpuset_len, sizeof(int), char_buffer, buffer_size, &buff_index);
			
			plan_ptr = subload_ptr->first;
			while (plan_ptr != NULL) {
				err += buffer_pack(&(plan_ptr->name) , SINGLE, sizeof(int), char_buffer, buffer_size, &buff_index);
				err += buffer_pack(&(plan_ptr->input_data->isize) , SINGLE, sizeof(int), char_buffer, buffer_size, &buff_index);
				err += buffer_pack(&(plan_ptr->input_data->csize) , SINGLE, sizeof(int), char_buffer, buffer_size, &buff_index);
				err += buffer_pack(&(plan_ptr->input_data->dsize) , SINGLE, sizeof(int), char_buffer, buffer_size, &buff_index);
				err += buffer_pack(plan_ptr->input_data->i , plan_ptr->input_data->isize, sizeof(int), char_buffer, buffer_size, &buff_index);
				for (i = 0; i < plan_ptr->input_data->csize; i++) {
					string_len = strlen(plan_ptr->input_data->c[i]) + 1;
					err += buffer_pack(&string_len, SINGLE, sizeof(int), char_buffer, buffer_size, &buff_index);
					err += buffer_pack(plan_ptr->input_data->c[i] , string_len, sizeof(char), char_buffer, buffer_size, &buff_index);
				}
				err += buffer_pack(plan_ptr->input_data->d , plan_ptr->input_data->dsize, sizeof(double), char_buffer, buffer_size, &buff_index);
				
				plan_ptr = plan_ptr->next;
			}
			
			subload_ptr = subload_ptr->next;
		}
		
		if (err > 0) {
			EmitLog(MyRank, SCHEDULER_THREAD, "Overran the buffer while packing the load structure.", -1, PRINT_ALWAYS);
		}
	}
#endif
	return (void *)char_buffer;
}

void *load_buffer_create_MPI(Load *load, int num_subloads, int buffer_size) {
	char *char_buffer = NULL;
	int buff_index = 0;
	int i, string_len;
	SubLoad *subload_ptr = NULL;
	LoadPlan *plan_ptr = NULL;
#ifndef HAVE_SHMEM
	
	char_buffer = (char *)malloc(buffer_size * sizeof(char));
	assert(char_buffer);
	
	if (MyRank == ROOT) {
		// Pack the load structure into a buffer, so that it can be passed as a single message.
		MPI_Pack(&(load->num_threads), SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
		MPI_Pack(&(load->num_cpusets), SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
		MPI_Pack(&(load->runtime),     SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
		MPI_Pack(&(load->scheduling),  SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
		// add nsubloads to allow correct unpacking on other side
		MPI_Pack(&num_subloads, SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);

		subload_ptr = load->front;
		while (subload_ptr != NULL) {
			MPI_Pack(&(subload_ptr->num_plans),  SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
			MPI_Pack(&(subload_ptr->cpuset_len), SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
			MPI_Pack(subload_ptr->cpuset, subload_ptr->cpuset_len, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
			plan_ptr = subload_ptr->first;
			while (plan_ptr != NULL) {
				MPI_Pack(&(plan_ptr->name),      SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
				MPI_Pack(&(plan_ptr->input_data->isize), SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
				MPI_Pack(&(plan_ptr->input_data->csize), SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
				MPI_Pack(&(plan_ptr->input_data->dsize), SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
				MPI_Pack(plan_ptr->input_data->i, plan_ptr->input_data->isize,  MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
				for(i = 0; i < plan_ptr->input_data->csize; i++) {
					string_len = strlen(plan_ptr->input_data->c[i])+1;
					MPI_Pack(&string_len, SINGLE, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
					MPI_Pack(plan_ptr->input_data->c, string_len, MPI_CHAR, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
				}
				MPI_Pack(plan_ptr->input_data->d, plan_ptr->input_data->dsize, MPI_INT, char_buffer, buffer_size, &buff_index, MPI_COMM_WORLD);
				
				plan_ptr = plan_ptr->next;
			}
			
			subload_ptr = subload_ptr->next;
		}
	}
	
#endif
	return (void *)char_buffer;
}

void *load_buffer_create(Load *load, int num_subloads, int buffer_size) {
#ifdef HAVE_SHMEM
	return load_buffer_create_SHMEM(load, num_subloads, buffer_size);
#else
	return load_buffer_create_MPI(load, num_subloads, buffer_size);
#endif
}

int load_buffer_destroy_SHMEM(void *buffer, int buffer_size, Load *load) {
	char *char_buffer = (char *)buffer;
	int buff_index = 0;
	int i, j, k, string_len;
	int alloc_err = GOOD;
	SubLoad *subload_ptr = NULL;
	LoadPlan *plan_ptr = NULL;
	int nsubloads;
#ifdef HAVE_SHMEM
	if (MyRank != ROOT) {
		buffer_unpack(char_buffer, buffer_size, &buff_index, &(load->num_threads), SINGLE, sizeof(int));
		buffer_unpack(char_buffer, buffer_size, &buff_index, &(load->num_cpusets), SINGLE, sizeof(int));
		buffer_unpack(char_buffer, buffer_size, &buff_index, &(load->runtime)    , SINGLE, sizeof(int));
		buffer_unpack(char_buffer, buffer_size, &buff_index, &(load->scheduling) , SINGLE, sizeof(int));
		buffer_unpack(char_buffer, buffer_size, &buff_index, &(nsubloads)        , SINGLE, sizeof(int));
		
		load->front = (SubLoad *)malloc(sizeof(SubLoad));
		assert(load->front);
		
		subload_ptr = load->front;
		for (i = 0; (i < nsubloads) && (subload_ptr != NULL) && (alloc_err != BAD); i++) {
			buffer_unpack(char_buffer, buffer_size, &buff_index, &(subload_ptr->num_plans) , SINGLE, sizeof(int));
			buffer_unpack(char_buffer, buffer_size, &buff_index, &(subload_ptr->cpuset_len), SINGLE, sizeof(int));
			
			subload_ptr->cpuset = (int *)malloc(subload_ptr->cpuset_len * sizeof(int));
			assert(subload_ptr->cpuset);
			if (subload_ptr->cpuset != NULL) {
				buffer_unpack(char_buffer, buffer_size, &buff_index, subload_ptr->cpuset, subload_ptr->cpuset_len, sizeof(int));
			} else {
				alloc_err = BAD;
			}
			
			subload_ptr->first = (LoadPlan *)malloc(sizeof(LoadPlan));
			assert(subload_ptr->first);
			plan_ptr = subload_ptr->first;
			for (j = 0; (j < subload_ptr->num_plans) && (plan_ptr != NULL) && (alloc_err != BAD); j++) {
				buffer_unpack(char_buffer, buffer_size, &buff_index, &(plan_ptr->name), SINGLE, sizeof(int));
				
				plan_ptr->input_data = (data *)malloc(sizeof(data));
				assert(plan_ptr->input_data);
				if (plan_ptr->input_data != NULL) {
					buffer_unpack(char_buffer, buffer_size, &buff_index, &(plan_ptr->input_data->isize), SINGLE, sizeof(int));
					buffer_unpack(char_buffer, buffer_size, &buff_index, &(plan_ptr->input_data->csize), SINGLE, sizeof(int));
					buffer_unpack(char_buffer, buffer_size, &buff_index, &(plan_ptr->input_data->dsize), SINGLE, sizeof(int));
					
					plan_ptr->input_data->i = (int*)    malloc(sizeof(int)    * plan_ptr->input_data->isize);
					plan_ptr->input_data->c = (char**)  malloc(sizeof(char*)  * plan_ptr->input_data->csize);
					plan_ptr->input_data->d = (double*) malloc(sizeof(double) * plan_ptr->input_data->dsize);
					
					buffer_unpack(char_buffer, buffer_size, &buff_index, plan_ptr->input_data->i, plan_ptr->input_data->isize, sizeof(int));
					for (k = 0; k < plan_ptr->input_data->csize; k++) {
						buffer_unpack(char_buffer, buffer_size, &buff_index, &string_len, SINGLE, sizeof(int));
						plan_ptr->input_data->c[k] = (char*)malloc(string_len * sizeof(char));
						buffer_unpack(char_buffer, buffer_size, &buff_index, plan_ptr->input_data->c[k], string_len, sizeof(char));
					}
					buffer_unpack(char_buffer, buffer_size, &buff_index, plan_ptr->input_data->d, plan_ptr->input_data->dsize, sizeof(double));
				} else {
					alloc_err = BAD;
				}
				
				if (j != subload_ptr->num_plans-1) {
					plan_ptr->next = (LoadPlan *)malloc(sizeof(LoadPlan));
					assert(plan_ptr->next);
				} else {
					subload_ptr->last = plan_ptr;
					plan_ptr->next = NULL;
				}
				
				plan_ptr = plan_ptr->next;
			}
			
			if (i != nsubloads-1) {
				subload_ptr->next = (SubLoad *)malloc(sizeof(SubLoad));
				assert(subload_ptr->next);
			} else {
				load->back = subload_ptr;
				subload_ptr->next = NULL;
			}
			
			subload_ptr = subload_ptr->next;
		}
		
		if (alloc_err == BAD) {	// malloc() failed - deallocate all load structures and abort this process. 
			EmitLog(MyRank, SCHEDULER_THREAD, "Unable to allocate memory for the load structure.", -1, PRINT_ALWAYS);
			freeLoad(load);
		}
	} 
	shfree(buffer);
	buffer = NULL;
#endif
	return alloc_err;
}

int load_buffer_destroy_MPI(void *buffer, int buffer_size, Load *load) {
	int buff_index = 0;
	int nsubloads;
	SubLoad *subload_ptr = NULL;
	LoadPlan *plan_ptr = NULL;
	int i, j, k;
	int string_len;
	int alloc_err = GOOD;
	char *char_buffer = (char *)buffer;
#ifndef HAVE_SHMEM
	
	if (MyRank != ROOT) {
		// Unpack the buffer into a newly allocated Load structure.
		MPI_Unpack(buffer, buffer_size, &buff_index, &(load->num_threads), SINGLE, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(buffer, buffer_size, &buff_index, &(load->num_cpusets), SINGLE, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(buffer, buffer_size, &buff_index, &(load->runtime),     SINGLE, MPI_INT, MPI_COMM_WORLD);
		MPI_Unpack(buffer, buffer_size, &buff_index, &(load->scheduling),  SINGLE, MPI_INT, MPI_COMM_WORLD);
		// get number of subloads 
		MPI_Unpack(buffer, buffer_size, &buff_index, &nsubloads,  SINGLE, MPI_INT, MPI_COMM_WORLD);
		
		// Allocate memory for, error check, unpack, and initialize the Load structure.
		load->front = (SubLoad *)malloc(sizeof(SubLoad));
		assert(load->front);
		subload_ptr = load->front;
		// unpack subloads for this load
		for (i = 0; (i < nsubloads) && (subload_ptr != NULL) && (alloc_err != BAD); i++) {
			MPI_Unpack(buffer, buffer_size, &buff_index, &(subload_ptr->num_plans),  SINGLE, MPI_INT, MPI_COMM_WORLD);
			MPI_Unpack(buffer, buffer_size, &buff_index, &(subload_ptr->cpuset_len), SINGLE, MPI_INT, MPI_COMM_WORLD);
			// get cpusets
			subload_ptr->cpuset = (int *)malloc(subload_ptr->cpuset_len * sizeof(int));
			assert(subload_ptr->cpuset);
			if (subload_ptr->cpuset != NULL)
				MPI_Unpack(buffer, buffer_size, &buff_index, subload_ptr->cpuset, subload_ptr->cpuset_len, MPI_INT, MPI_COMM_WORLD);
			else
				alloc_err = BAD;

			// Allocate memory for the first LoadPlan in this SubLoad.
			subload_ptr->first = (LoadPlan *)malloc(sizeof(LoadPlan));
			assert(subload_ptr->first);
			plan_ptr = subload_ptr->first;
			// unpack loadplans for this subload
			for (j = 0; (j < subload_ptr->num_plans) && (plan_ptr != NULL) && (alloc_err != BAD); j++) {
				MPI_Unpack(buffer, buffer_size, &buff_index, &(plan_ptr->name), SINGLE, MPI_INT, MPI_COMM_WORLD);
				// unpack plan input data
				plan_ptr->input_data = (data *)malloc(sizeof(data));
				assert(plan_ptr->input_data);
				if (plan_ptr->input_data != NULL) {
					MPI_Unpack(buffer, buffer_size, &buff_index, &(plan_ptr->input_data->isize), SINGLE, MPI_INT, MPI_COMM_WORLD);
					MPI_Unpack(buffer, buffer_size, &buff_index, &(plan_ptr->input_data->csize), SINGLE, MPI_INT, MPI_COMM_WORLD);
					MPI_Unpack(buffer, buffer_size, &buff_index, &(plan_ptr->input_data->dsize), SINGLE, MPI_INT, MPI_COMM_WORLD);
					plan_ptr->input_data->i = (int*)    malloc(plan_ptr->input_data->isize * sizeof(int));
					plan_ptr->input_data->c = (char**)  malloc(plan_ptr->input_data->csize * sizeof(char*));
					plan_ptr->input_data->d = (double*) malloc(plan_ptr->input_data->dsize * sizeof(double));
					MPI_Unpack(buffer, buffer_size, &buff_index, plan_ptr->input_data->i, plan_ptr->input_data->isize, MPI_INT, MPI_COMM_WORLD);
					for(k = 0; k < plan_ptr->input_data->csize; k++) {
						MPI_Unpack(buffer, buffer_size, &buff_index, &string_len, SINGLE, MPI_INT, MPI_COMM_WORLD);
						plan_ptr->input_data->c[k] = (char *)malloc(string_len * sizeof(char));
						assert(plan_ptr->input_data->c[k]);
						MPI_Unpack(buffer, buffer_size, &buff_index, plan_ptr->input_data->c[k], string_len, MPI_CHAR, MPI_COMM_WORLD);
					}
					MPI_Unpack(buffer, buffer_size, &buff_index, plan_ptr->input_data->d, plan_ptr->input_data->dsize, MPI_INT, MPI_COMM_WORLD);
				} else {
					alloc_err = BAD;
				}
				
				// allocate the next loadplan unless this is the last iteration
				if (j != subload_ptr->num_plans-1) {
					plan_ptr->next = (LoadPlan *)malloc(sizeof(LoadPlan));
					assert(plan_ptr->next);
				} else {
					subload_ptr->last = plan_ptr;
					plan_ptr->next = NULL;
				}
				// iterate to the next loadplan
				plan_ptr = plan_ptr->next;
			}
			
				
			// allocate the next subload unless this is the last iteration
			if (i != nsubloads-1) {
				subload_ptr->next = (SubLoad *)malloc(sizeof(SubLoad));
				assert(subload_ptr->next);
			} else {
				load->back = subload_ptr;
				subload_ptr->next = NULL;
			}
			// iterate to the next subload
			subload_ptr = subload_ptr->next;
		}
		
		if (alloc_err == BAD) {	// malloc() failed - deallocate all load structures and abort this process.
			EmitLog(MyRank, SCHEDULER_THREAD, "Unable to allocate memory for the load structure.", -1, PRINT_ALWAYS);
			freeLoad(load);
		}
	}
	
	free(buffer);
	buffer = NULL;
#endif
	return alloc_err;
}

int load_buffer_destroy(void *buffer, int buffer_size, Load *load) {
#ifdef HAVE_SHMEM
	return load_buffer_destroy_SHMEM(buffer, buffer_size, load);
#else
	return load_buffer_destroy_MPI(buffer, buffer_size, load);
#endif
}

int bcastLoad_MPI(Load *load) {
	char *buffer;
	int buffer_size = 0;
	int flag = 0;
	int alloc_err = GOOD;
	int nsubloads = 0;
#ifndef HAVE_SHMEM
	
	buffer_size = load_buffer_size(load, &nsubloads);
	
	// create the buffer to distribute the load
	MPI_Bcast(&buffer_size, SINGLE, MPI_INT, ROOT, MPI_COMM_WORLD);
	
	buffer = (char *)load_buffer_create(load, nsubloads, buffer_size);
	assert(buffer);
	
	flag = MPI_Bcast(buffer, buffer_size, MPI_PACKED, ROOT, MPI_COMM_WORLD);
	
	alloc_err = load_buffer_destroy((void *)buffer, buffer_size, load);
	
#endif
	return (flag && alloc_err);
}

int bcastLoad_SHMEM(Load *load) {
	char *buffer;
	int buffer_size = 0;
	int alloc_err = GOOD;
	int nsubloads = 0;
	// long *pSync;
	int i;
	int commsize;
#ifdef HAVE_SHMEM

	// pSync = (long *)shmalloc(_SHMEM_BCAST_SYNC_SIZE * sizeof(long));   // hinky
	// assert(pSync);
	// for (i = 0; i < _SHMEM_BCAST_SYNC_SIZE; i++) { pSync[i] = _SHMEM_SYNC_VALUE; }

	shmem_barrier_all();
	commsize = SHMEM_NUM_PES;
	buffer_size = load_buffer_size(load, &nsubloads);
	shmem_broadcast32(&buffer_size, &buffer_size, SINGLE, ROOT, 0, 0, commsize, pSync); // hinky -- buffersize is not symmetric
	shmem_barrier_all();
	buffer = (char *)load_buffer_create(load, nsubloads, buffer_size);
	assert(buffer);
	shmem_broadcast32(buffer, buffer, buffer_size, ROOT, 0, 0, commsize, qSync);
	shmem_barrier_all();
	alloc_err = load_buffer_destroy((void *)buffer, buffer_size, load);
#endif
	return alloc_err;
}
*/
