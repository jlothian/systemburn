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

#define MAKE_PLANS

#include <systemburn.h>
#include <load.h>
#include <initialization.h>
#include <planheaders.h>

/****************************************
 * 	Basic scheduling functions	*
 ***************************************/

/********************************************************
 * \brief
 *  	WorkerSched will assign plans to individual	*
 * 	workerthreads as dictated by the load file. 	*
 * 	WorkerSched is run for every load. 		*
 * \param load Pointer to the load for which workers    *
 *             are made.                                *
 *******************************************************/

int WorkerSched(Load *load) {
	int i = 0, k = 0;
	int one = 1;
	int ret = ERR_CLEAN;
	Plan *p = NULL;
	SubLoad  *subload_ptr = NULL;
	LoadPlan *plan_ptr    = NULL;
	
#ifdef LINUX_PLACEMENT
	int affin_err = 0;
	int cores = sysconf(_SC_NPROCESSORS_ONLN);
	cpu_set_t *cpuset;
	cpuset = (cpu_set_t *)malloc(load->num_cpusets * sizeof(cpu_set_t));
	assert(cpuset);

	EmitLog(MyRank, SCHEDULER_THREAD, "Number of cores on this machine:", cores, PRINT_RARELY);

	SetMasks(cores, cpuset, load);
        SetCPUSetLens(cores, cpuset, load);
#endif

	/* If the load requires more threads than available, warn the user that only a part of the load will run. */
	if (load->num_threads > num_workers)
		EmitLog(MyRank, SCHEDULER_THREAD, "Too few worker threads available to run the full load.", -1, PRINT_ALWAYS);

	/* Assign plans and cpusets to threads, one by one. */
	subload_ptr = load->front;
	while (subload_ptr != NULL && k < num_workers) {
		plan_ptr = subload_ptr->first;
		while (plan_ptr != NULL && k < num_workers) {
			char *plan_name = printPlan(plan_ptr->name);
			if(plan_ptr->input_data->isize) {
				EmitLog(MyRank, SCHEDULER_THREAD, plan_name, plan_ptr->input_data->i[0], PRINT_OFTEN);
			} else if (plan_ptr->input_data->dsize) {
				EmitLog(MyRank, SCHEDULER_THREAD, plan_name, plan_ptr->input_data->d[0], PRINT_OFTEN);
			}

			/* Function switch */
			p = (*plan_list[ plan_ptr->name ]->make) (plan_ptr->input_data);
			assert(p);
			if(p == NULL) add_error(&WorkerHandle[k],SYSTEM,0);
//			p->name=plan_ptr->name;		// Moved into individual plan make functions 

			pthread_rwlock_wrlock(&(WorkerHandle[k].Lock));
			WorkerHandle[k].Plan = p;
			pthread_rwlock_unlock(&(WorkerHandle[k].Lock));
#ifdef LINUX_PLACEMENT
			if (cpuset != NULL) {
				affin_err = pthread_setaffinity_np((WorkerHandle[k].ID), sizeof(cpu_set_t), &(cpuset[i]));
				if (affin_err != 0) add_error(&WorkerHandle[k],SYSTEM,2);
			}
#endif
			k++;
			plan_ptr = plan_ptr->next;
		}
		i++;
		subload_ptr = subload_ptr->next;
	}
	
	if (k < num_workers) {
		for (i = k; i < num_workers; i++) {
			data pass[1];
			pass->i = &one;
			p = (*plan_list[SLEEP]->make)(pass);
			
			pthread_rwlock_wrlock(&(WorkerHandle[i].Lock));
			WorkerHandle[i].Plan = p;
			pthread_rwlock_unlock(&(WorkerHandle[i].Lock));
		}
	}

#ifdef LINUX_PLACEMENT
	if (cpuset != NULL)
		free(cpuset);
	else
		ret = ALLOC;
#endif

	return ret; 	
}

#ifdef LINUX_PLACEMENT
/**
 * \brief MaskBlock will set the affinity mask for the cpu sets in a block type fashion.
 * \param numcpucores The number of cores that can be set
 * \param cpuset What configuration the cores are set to
 * \param num_cpusets Number of sets to process
 */
void MaskBlock(int numcpucores, cpu_set_t *cpuset, int num_cpusets) {
	int i, x; 
	for(i=0; i<numcpucores; i++) {
		x = floor(((i*num_cpusets)/numcpucores)); 
		CPU_SET(i,&(cpuset[x]));
	}

}

/**
 * \brief MaskRoundRobin sets the affinity mask for the cpu sets in a round-robin fashion.
 * \param numcpucores The number of cores that can be set
 * \param cpuset What configuration the cores are set to
 * \param num_cpusets Number of sets to process         
 */ 
void MaskRoundRobin(int numcpucores, cpu_set_t *cpuset, int num_cpusets) {
	int i; 
	for(i=0; i<numcpucores; i++) {
		CPU_SET(i,&(cpuset[i%num_cpusets])); 
	}

}

/** \brief Sets the cpu mask as specified by the Load structure.
 \param numcpucores Number of cores available to be set
 \param cpus List of CPUs to be set
 \param input The load struct that the mask is created for
 */
void MaskSpecific(int numcpucores, cpu_set_t *cpus, Load *input) {
	int i, j, all_flag = 0;
	SubLoad *subload_ptr = input->front;
	
	for (i = 0; i < input->num_cpusets; i++) {
		if (subload_ptr->cpuset_len > 0) {
			for (j = 0; j < subload_ptr->cpuset_len; j++) {
				if (subload_ptr->cpuset[j] >= 0)
					CPU_SET(subload_ptr->cpuset[j], &cpus[i]);
				else
					all_flag = 1;
			}
		} else {
			all_flag = 1;
		}
		/* If the cpuset array in the load struct is NULL or contains a negative value	*/
		/* then set this subload to run on all processors.				*/
		if (all_flag) {
			for (j = 0; j < numcpucores; j++) {
				CPU_SET(j, &cpus[i]);
			}
		}
		
		subload_ptr = subload_ptr->next;
	}
}

/** \brief Initialize an array of cpusets to zero.
 \param cpuset List of CPU sets to initialize
 \param num_cpusets Size of cpuset
 */
void ZeroMask(cpu_set_t *cpuset, int num_cpusets) {
	int i;
	for(i=0; i<num_cpusets; i++) {
		CPU_ZERO(&(cpuset[i]));
        }
}

/** \brief Sets the cpu mask
 \param numcpucores Number of cores available to be set
 \param cpuset List of CPU sets to use
 \param input The load struct the mask is created for
 */
void SetMasks(int numcpucores, cpu_set_t *cpuset, Load *input) {
	if (cpuset != NULL) {
		schedules sched = input->scheduling;
	
		ZeroMask(cpuset, input->num_cpusets);

		switch (sched) {
			case ROUND_ROBIN:
				MaskRoundRobin(numcpucores, cpuset, input->num_cpusets);
				break;
			case BLOCK:
				MaskBlock(numcpucores, cpuset, input->num_cpusets);
				break;
			case SUBLOAD_SPECIFIC:
				MaskSpecific(numcpucores, cpuset, input);
				break;
			default:
				MaskBlock(numcpucores, cpuset, input->num_cpusets);
				break;
		}
        } 
}	

/** \brief Sets the cpuset lengths and integer representations for subloads
 \param numcpucores Number of cores available to be set
 \param cpuset CPU sets in use
 \param input The load struct to calculate lengths for
 */
void SetCPUSetLens(int numcpucores, cpu_set_t *cpuset, Load *input){
        int i,j,k;
        SubLoad *subload_ptr = input->front;
        for(i=0; i<input->num_cpusets; i++){
                subload_ptr->cpuset_len = CPU_COUNT(&cpuset[i]);
                subload_ptr->cpuset = (int *)malloc(subload_ptr->cpuset_len*sizeof(int));
                k=0;
                for(j=0; j<numcpucores; j++){
                        if(CPU_ISSET(j, &cpuset[i])){
                                subload_ptr->cpuset[k]=j;
                                k++;
                        }
                }
                subload_ptr = subload_ptr->next;
        }

}

#endif /* LINUX_PLACEMENT */
