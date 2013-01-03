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
#ifndef __COMM_H
#define __COMM_H

typedef enum {
    REDUCE_SUM,
    REDUCE_MIN,
    REDUCE_MAX
} reduction_op;

extern void *config_buffer_create(int buffer_size, int num_loads);
extern void *config_buffer_create_SHMEM(int buffer_size, int num_loads);
extern void *config_buffer_create_MPI(int buffer_size, int num_loads);
extern void config_buffer_destroy(void *buffer, int buffer_size, int *num_loads);
extern void config_buffer_destroy_SHMEM(void *buffer, int buffer_size, int *num_loads);
extern void config_buffer_destroy_MPI(void *buffer, int buffer_size, int *num_loads);
extern int bcastConfig_MPI(int load_num);
extern int bcastConfig_SHMEM(int load_num);
extern int buffer_pack(void *src, int length, int type_size, void *buff, int buff_len, int *buff_ind);
extern int buffer_unpack(void *buff, int buff_len, int *buff_ind, void *dest, int length, int type_size);
extern int load_buffer_size(Load *load, int *num_subloads);
extern void *load_buffer_create(Load *load, int num_subloads, int buffer_size);
extern void *load_buffer_create_SHMEM(Load *load, int num_subloads, int buffer_size);
extern void *load_buffer_create_MPI(Load *load, int num_subloads, int buffer_size);
extern int load_buffer_destroy(void *buffer, int buffer_size, Load *load);
extern int load_buffer_destroy_SHMEM(void *buffer, int buffer_size, Load *load);
extern int load_buffer_destroy_MPI(void *buffer, int buffer_size, Load *load);
extern int bcastLoad_MPI(Load *load);
extern int bcastLoad_SHMEM(Load *load);
extern int comm_broadcast_buffer_MPI(void *buffer, int buffer_size);
extern int comm_broadcast_buffer_SHMEM(void *buffer, int buffer_size);
extern void reduceTemps_MPI();
extern void reduceTemps_SHMEM();
extern void reduceFlags_MPI(int **local_flag);
extern void reduceFlags_SHMEM(int **local_flag);
extern void comm_table_reduce_MPI(void *table, int nrows, int ncols, reduction_op op);
extern void comm_table_reduce_SHMEM(void *table, int nrows, int ncols, reduction_op op);
extern void comm_setup(int *argc, char ***argv);
extern int comm_getrank();
extern void comm_broadcast_int(int *value);
extern void comm_abort(int e_code);
extern void comm_finalize();

#endif /* __COMM_H */
