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
/**
        This file contains implementations for load structure manipulation functions:
                A load file parsing function,
                A load structure printing function,
                A load structure broadcasting function,
                A load structure cleanup function,
                And several supporting functions.
 */

#define PARSE_PLANS
#define FIND_PLANS
#define PLANS

#include <systemburn.h>
#include <load.h>
#include <initialization.h>
#include <planheaders.h>
#include <comm.h>

// Left here for reference on how to access plan_list and it's info
/*
   plan_info* plan_list[] = {
        &(SLEEP_info),
   #ifdef HAVE_BLAS
        &(DGEMM_info),
        &(RDGEMM_info),
   #endif
   #ifdef HAVE_FFTW3
        &(FFT1_info),
        &(FFT2_info),
   #endif
        &(DSTREAM_info),
        &(DSTRIDE_info),
        &(LSTREAM_info),
        &(LSTRIDE_info),
        &(GUPS_info),
        &(PV1_info),
        &(PV2_info),
        &(PV3_info),
        &(COMM_info),
        &(STRING_info),
        &(SLEEP_info) // Default for unknown plans.
   };
 */

/* This function is called to broadcast a single load assignment to each MPI process.
   int bcastLoad(Load *load) {
   #ifdef HAVE_SHMEM
        return bcastLoad_SHMEM(load);
   #else
        return bcastLoad_MPI(load);
   #endif
   }
 */

/**
   \brief This function opens a single load file to be parsed. Should be called only on root.
   \param load_name Name of the load file to read.
   \param output A buffer containing the load file.
   \returns The size in bytes of the load file.
 */
int initLoadOptions(char *load_name, char **output){
    int filesize = 0;
    char filename[ARRAY];
    FILE *loadFile = NULL;

    if(load_name == NULL){              /* No load file arguments - loading default file. */
        strncpy(filename, "systemburn.load", ARRAY);
    } else {
        strncpy(filename, load_name, ARRAY);
    }

    filesize = fsize(filename);

    if(filesize > 0){
        loadFile = fopen(filename, "r");

        if(loadFile != NULL){
            /* Copy to buffer and close the file. */

            *output = getFileBuffer(filesize);
            int tmp = readFile(*output, filesize, loadFile);
            fclose(loadFile);
        }
    } else {
        EmitLog(MyRank, SCHEDULER_THREAD, "AIeeeeee...  couldn't open load file",-1, PRINT_ALWAYS);
    }
    return filesize;
} /* initLoadOptions */

/**
   \brief Broadcasts the contents of a memory buffer to all nodes using one of two network libraries.
   \param [in,out] buffer The data to be broadcast: input recieved from root and output on all other nodes.
   \param [in] buffer_size The size in bytes of the buffer.
   \returns Error codes from the communication libraries.
 */
int broadcast_buffer(void *buffer, int buffer_size){
    #ifdef HAVE_SHMEM
    return comm_broadcast_buffer_SHMEM(buffer, buffer_size);
    #else
    return comm_broadcast_buffer_MPI(buffer, buffer_size);
    #endif
}

/**
   \brief Reads the load file and pulls the data from it.
   \param inString A bufferd copy of the load file to be read
   \param output Where the information is stored for later use.
 */
int parseLoad(char *inString, Load *output){
    int ret = BAD;

    /* Initialize the output Load structure: */
    output->front = NULL;
    output->back = NULL;
    output->num_threads = 0;
    output->num_cpusets = 0;
    output->runtime = 0;
    output->scheduling = INITIAL;

    /* Temporary storage declarations: */
    SubLoad **temp_subload = NULL;
    LoadPlan temp_plan;
    #ifdef LINUX_PLACEMENT
    int *temp_cpuset = NULL;
    #endif
    char line_buffer[ARRAY];
    int str_offset = 0;
    char temp_string[ARRAY];
    int temp_int;

    /* Error checking variable: */
    int error_flag = GOOD;
    int line_number = 0;

    /* Keyword switch variable: */
    keyword key = NONE;

    /* Internally used indices: */
    int load_index = 0;
    int subload_ind = 0;
    int subload_copies = 0;
    int plan_copies = 0;
    #ifdef LINUX_PLACEMENT
    int cpuset_len = 0;
    #endif /* LINUX_PLACEMENT */
    int flag = 0;

    /* Position flags: */
    int inside_load = 0;                /* If parsing in between '{' and '}' set to 1, else, set to 0.	*/
    int inside_subload = 0;             /* If between '[' and ']', set to 1, else, set to 0.		*/
    /* Gets a line of text from the input file repeatedly, until there are no more. */
    while((str_offset = strgetline(line_buffer, ARRAY, inString, str_offset)) <= strlen(inString)){
        line_number++;
        /* Get the first word in the line and compare to the keyword list. */
        flag = sscanf(line_buffer, "%s", temp_string);
        if(flag > 0){
            if(temp_string[0] != '#'){
                key = keywordCmp(temp_string);
                switch(key){
                case LOAD_START:                                /* LOAD_START or '{' - indicates the beginning of a full load entry.	*/
                    inside_load = 1;
                    break;
                case LOAD_END:                                  /* LOAD_END or '}' - indicates the end of a full load entry.		*/
                    inside_load = 0;
                    load_index++;
                    break;
                case SUB_START:                                 /* SUB_START or '[' - indicates the beginning of a subload entry.	*/
                    if(inside_load){
                        inside_subload = 1;
                    } else if(MyRank == ROOT){
                        EmitLog(MyRank, SCHEDULER_THREAD, "Load File Error: Not inside a load, skipping SUB_START on line", line_number, PRINT_ALWAYS);
                    }
                    break;
                case SUB_END:                                   /* SUB_END or ']' - indicates the end of a subload entry.		*/
                    if(inside_subload){
                        assignSubLoad(output, &subload_ind, &temp_subload, subload_copies);
                        inside_subload = 0;
                    } else if(MyRank == ROOT){
                        EmitLog(MyRank, SCHEDULER_THREAD, "Load File Error: Not inside a subload, skipping SUB_END on line", line_number, PRINT_ALWAYS);
                    }
                    break;
                case RUNTIME:                                   /* RUNTIME - followed by the integer number of seconds to run the load.	*/
                    if(inside_load){
                        runtimeLine(line_buffer, &temp_int);
                        output->runtime = temp_int;
                    } else if(MyRank == ROOT){
                        EmitLog(MyRank, SCHEDULER_THREAD, "Load File Error: Not inside a load, skipping RUNTIME on line", line_number, PRINT_ALWAYS);
                    }
                    break;
                case SCHEDULE:                                  /* SCHEDULE - followed by a string that indicates cpu masking.		*/
                    if(inside_load){
                        scheduleLine(line_buffer, &temp_int);
                        output->scheduling = (schedules)temp_int;
                    } else if(MyRank == ROOT){
                        EmitLog(MyRank, SCHEDULER_THREAD, "Load File Error: Not inside a load, skipping SCHEDULE on line", line_number, PRINT_ALWAYS);
                    }
                    break;
                case SUBLOAD:                                   /* SUBLOAD - followed by the number of copies of the following subload.	*/
                    if(inside_load){
                        assignSubLoad(output, &subload_ind, &temp_subload, subload_copies);
                        subloadLine(line_buffer, &subload_copies);
                        allocSubload(&temp_subload, &subload_copies);
                    } else if(MyRank == ROOT){
                        EmitLog(MyRank, SCHEDULER_THREAD, "Load File Error: Not inside a load, skipping SUBLOAD on line", line_number, PRINT_ALWAYS);
                    }
                    break;
                case PLAN:                                      /* PLAN - followed by the # of copies of this plan and the plan spec.	*/
                    if(inside_subload){
                        planLine(line_buffer, &temp_plan, &plan_copies);
                        assignPlan(temp_subload, subload_copies, &temp_plan, plan_copies);
                    } else if(MyRank == ROOT){
                        EmitLog(MyRank, SCHEDULER_THREAD, "Load File Error: Not inside a subload, skipping PLAN on line", line_number, PRINT_ALWAYS);
                    }
                    break;
                case MASK:                                      /* MASK - followed by the cpu core numbers assigned to that subload.	*/
                    #ifdef LINUX_PLACEMENT
                    if(inside_subload){
                        maskLine(line_buffer, &temp_cpuset, &cpuset_len);
                        assignMask(temp_subload, subload_copies, temp_cpuset, cpuset_len);
                    } else if(MyRank == ROOT){
                        EmitLog(MyRank, SCHEDULER_THREAD, "Load File Error: Not inside a subload, skipping MASK on line", line_number, PRINT_ALWAYS);
                    }
                    #endif /* LINUX_PLACEMENT */
                    break;
                default:                                        /* An unrecognized keyword is skipped.					*/

                    break;
                }
            }
        }
    }

    if(output != NULL){
        ret = GOOD;
    }

    return ret;
} /* parseLoad */

/**
   \brief Compares keyword strings and returns the correct enum value.
   \param input String to compare to the load file keywords.
 */
keyword keywordCmp(char *input){
    keyword temp = NONE;

    if((input[0] == '{') || (strcmp(input, "LOAD_START") == 0) ){
        temp = LOAD_START;
    } else if((input[0] == '}') || (strcmp(input, "LOAD_END") == 0) ){
        temp = LOAD_END;
    } else if((input[0] == '[') || (strcmp(input, "SUB_START") == 0) ){
        temp = SUB_START;
    } else if((input[0] == ']') || (strcmp(input, "SUB_END") == 0) ){
        temp = SUB_END;
    } else if(strcmp(input, "RUNTIME") == 0){
        temp = RUNTIME;
    } else if(strcmp(input, "SCHEDULE") == 0){
        temp = SCHEDULE;
    } else if(strcmp(input, "SUBLOAD") == 0){
        temp = SUBLOAD;
    } else if(strcmp(input, "PLAN") == 0){
        temp = PLAN;
    } else if(strcmp(input, "MASK") == 0){
        temp = MASK;
    } else {
        temp = NONE;
    }

    return temp;
} /* keywordCmp */

/** \brief Parse a RUNTIME line.
   \param line RUNTIME to read
   \param runtime Will hold the total runtime for SystemBurn
 */
int runtimeLine(char *line, int *runtime){
    int temp_runtime = 0;
    int flag = BAD;

    flag = sscanf(line, " RUNTIME %d", &temp_runtime);

    if(flag == GOOD){
        *runtime = temp_runtime;
    } else {
        *runtime = 0;
        flag = BAD;
    }

    return flag;
}

/** \brief Parse a SCHEDULE line.
   \param line SCHEDULE line to be read.
   \param schedule Struct that holds the scheduling information for the run.
 */
int scheduleLine(char *line, schedules *schedule){
    char schedString[ARRAY];
    schedules temp = UNKN_SCHED;
    int flag = BAD;

    flag = sscanf(line, " SCHEDULE %s", schedString);

    if(flag == GOOD){
        temp = setSchedule(schedString);
        *schedule = temp;
    } else {
        *schedule = UNKN_SCHED;
        flag = BAD;
    }

    return flag;
} /* scheduleLine */

/** \brief Parse a SUBLOAD line
   \param line SUBLOAD line to be read.
   \param subloads Array that holds the number of subloads for this load
 */
int subloadLine(char *line, int *subloads){
    int temp = 0;
    int flag = BAD;

    flag = sscanf(line, " SUBLOAD %d", &temp);

    if(flag == GOOD){
        *subloads = temp;
    } else {
        *subloads = 1;
        flag = BAD;
    }

    return flag;
}

/** \brief Allocate memory for the number of copies of a subload specified in the most recent SUBLOAD line.
   \param output Holds the information for all of the subloads in this load.
   \param subloads Tells the number of copies of the given subload.
 */
int allocSubload(SubLoad ***output, int *subloads){
    int i, flag = BAD;

    if((*subloads) <= 0){
        *subloads = 1;
    }

    /* Allocate an array of SubLoad pointers - allows a separate allocation for each SubLoad struct. */
    *output = (SubLoad **)malloc(*subloads * sizeof(SubLoad *));
    assert(*output);
    if(*output != NULL){
        for(i = 0; i < *subloads; i++){
            /* Allocate a single SubLoad - allows for individual freeing of subloads. */
            (*output)[i] = (SubLoad *)malloc(sizeof(SubLoad));
            assert((*output)[i]);
            if((*output)[i] == NULL){
                for(i = i - 1; i >= 0; i--){
                    free((*output)[i]);
                }
                free(*output);
                *output = NULL;
                break;
            }
        }
    }

    if(*output != NULL){
        flag = GOOD;

        /* Initialize the allocated subloads. */
        for(i = 0; i < *subloads; i++){
            (*output)[i]->num_plans = 0;
            (*output)[i]->cpuset_len = 0;
            (*output)[i]->first = NULL;
            (*output)[i]->last = NULL;
            (*output)[i]->cpuset = NULL;
            (*output)[i]->next = NULL;
        }
    } else {
        *subloads = 0;
    }

    return flag;
} /* allocSubload */

/** \brief Parse a PLAN line.
   \param line PLAN line to be read.
   \param output Struct that holds the information for the plan listed
   \param plans The number of copies of the given plan that should be run
 */
int planLine(char *line, LoadPlan *output, int *plans){
    int plan_copies = 0;
    char plan_name[ARRAY];
    plan_choice temp = UNKN_PLAN;
    int flag = 0;
    flag = sscanf(line, " PLAN %d%s", &plan_copies, plan_name);
    if(flag == 2){
        flag = GOOD;
        *plans = plan_copies;
        temp = setPlan(plan_name);
        if(temp == UNKN_PLAN){
            EmitLog(MyRank, SCHEDULER_THREAD, strcat(plan_name, " plan was not recognized, sleeping."), -1, PRINT_ALWAYS);
        }
        /* Initialize LoadPlan structure. */
        output->name = UNKN_PLAN;
        output->input_data = NULL;
//	output->isize = output->csize = output->dsize = 0;
        output->next = NULL;
        /* Uses an array of function pointers to call the parsing function for each plan. */
        (*plan_list[temp]->parse)(line, output);
    } else {
        *plans = 0;
        flag = BAD;
    }
    return flag;
} /* planLine */

/** \brief Assigns copies of the most recently stored LoadPlan in current SubLoad(s).
   \param output Holds the information for the calling subload
   \param subloads Tells how many copies of the subload will be running
   \param input The plans that the subload will be running.
   \param plans The number of copies of the plans
 */
int assignPlan(SubLoad **output, int subloads, LoadPlan *input, int plans){
    int i, j, flag = 1;
    LoadPlan *plan_ptr = NULL;

    for(i = 0; i < subloads; i++){
        for(j = 0; j < plans; j++){
            plan_ptr = output[i]->last;
            if(plan_ptr == NULL){
                output[i]->first = output[i]->last = planCopy(input);
            } else {
                plan_ptr->next = planCopy(input);
                output[i]->last = plan_ptr->next;
            }
            if(output[i]->last != NULL){
                output[i]->num_plans++;
            } else {
                output[i]->last = plan_ptr;
                flag = 0;
                break;
            }
        }
        if(output[i]->last == NULL){
            break;
        }
    }

    freePlan(input);

    return flag;
} /* assignPlan */

/** \brief Parses a MASK line.
   \param line MASK line to be read.
   \param cpuset Stores the mask values
   \param set_len Number of mask values
 */
int maskLine(char *line, int **cpuset, int *set_len){
    int char_count, count = 0;
    int temp_int;
    char *place = line;
    int temp[2 * num_workers];
    int i, flag = BAD;
    *set_len = 0;

    sscanf(place, " MASK %d%n", &temp_int, &char_count);
    do {
        if(count < 2 * num_workers){
            temp[count] = temp_int;
            count++;
        }
        place += char_count;
    } while(sscanf(place, "%d%n", &temp_int, &char_count) > 0);

    /* Allocates an array to store the mask values. */
    (*cpuset) = (int *)malloc(count * sizeof(int));
    assert(*cpuset);
    if(*cpuset != NULL){
        flag = GOOD;
        *set_len = count;
        for(i = 0; i < count; i++){
            (*cpuset)[i] = temp[i];
        }
    }
    return flag;
} /* maskLine */

/** \brief Assign the most recently allocated mask to the current SubLoad(s).
   \param output Holds the info for the subload
   \param subloads The number of copies of the subload.
   \param cpuset The cpu mask that will be applied to the subload.
   \param set_len The number of mask values
 */
int assignMask(SubLoad **output, int subloads, int *cpuset, int set_len){
    int i, flag = BAD;

    for(i = 0; i < subloads; i++){
        if(output[i]->cpuset == NULL){
            flag = GOOD;
            output[i]->cpuset = cpuset;
            output[i]->cpuset_len = set_len;
            break;
        }
    }

    if((flag == 0) && (cpuset != NULL) ){
        free(cpuset);
    }

    return flag;
} /* assignMask */

/** \brief Assign the current SubLoad(s) to the output Load structure.
   \param load The structure for the current load
   \param index Keeps track of what position the list is currently at.
   \param temp_subload Array that holds the subloads that will be assigned to the current load.
   \param subloads The number of subloads to be assigned.
 */
int assignSubLoad(Load *load, int *index, SubLoad ***temp_subload, int subloads){
    int i, flag = 0;
    SubLoad *subload_ptr = NULL;

    if(*temp_subload != NULL){
        for(i = 0; i < subloads; i++){
            subload_ptr = load->back;
            if(subload_ptr == NULL){
                load->front = load->back = (*temp_subload)[i];
            } else {
                subload_ptr->next = (*temp_subload)[i];
                load->back = subload_ptr->next;
            }
            if(load->back != NULL){
                flag++;
                load->num_cpusets++;
                (*index)++;
                load->num_threads += (load->back)->num_plans;
            } else {
                load->back = subload_ptr;
            }
        }

        free(*temp_subload);
        *temp_subload = NULL;

        if(flag == i){
            flag = GOOD;
        } else {
            flag = BAD;
        }
    } else {
        flag = BAD;
    }

    return flag;
} /* assignSubLoad */

/** \brief Allocates a copy of it's input LoadPlan and returns a pointer to the copy.
   \param input Creates a copy of the plan to be assigned to the load
 */
LoadPlan *planCopy(LoadPlan *input){
    int i;
    LoadPlan *temp;
    temp = (LoadPlan *)malloc(sizeof(LoadPlan));
    assert(temp);
    if((temp != NULL) && (input != NULL) ){
        temp->name = input->name;
        temp->next = NULL;
        if(input->input_data != NULL){
            temp->input_data = (data *)malloc(sizeof(data));
            assert(temp->input_data);
            temp->input_data->isize = input->input_data->isize;
            temp->input_data->csize = input->input_data->csize;
            temp->input_data->dsize = input->input_data->dsize;
            temp->input_data->i = (int *)    malloc(temp->input_data->isize * sizeof(int));
            temp->input_data->c = (char **)  malloc(temp->input_data->csize * sizeof(char **));
            temp->input_data->d = (double *) malloc(temp->input_data->dsize * sizeof(double));
            if(temp->input_data != NULL){
                if(input->input_data->i != NULL){
                    for(i = 0; i < temp->input_data->isize; i++){
                        temp->input_data->i[i] = input->input_data->i[i];
                    }
                }
                if(input->input_data->c != NULL){
                    for(i = 0; i < temp->input_data->csize; i++){
                        temp->input_data->c[i] = strdup(input->input_data->c[i]);
                    }
                }
                if(input->input_data->d != NULL){
                    for(i = 0; i < temp->input_data->dsize; i++){
                        temp->input_data->d[i] = input->input_data->d[i];
                    }
                }
            } else {
                free(temp);
                temp = NULL;
            }
        }
    }

    return temp;
} /* planCopy */

/** \brief This function prints all the details of a load structure to the file pointer specified by OUTPUT_PLACE.
   \param input The load to be printed.
 */
void printLoad(Load *input){
    FILE *out = OUTPUT_PLACE;
    int i, j, k = 0;
    int k_flag, count;
    char *plan_name = NULL;
    char size_str[ARRAY];
    char buffer[ARRAY];
    SubLoad *sub_index[OUTPUT_STEP];
    LoadPlan *plan_index[OUTPUT_STEP];
    int      plan_flags[OUTPUT_STEP] = {
        0
    };

    /* Initialize the subload index array - because the subloads are stored as a linked list, and output
       will be in multiple columns, the subloads will be indexed by an array of pointers, a pointer per column.	*/
    sub_index[0] = input->front;
    for(i = 1; i < OUTPUT_STEP; i++){
        if(sub_index[i - 1] != NULL){
            sub_index[i] = sub_index[i - 1]->next;
        } else {
            sub_index[i] = NULL;
        }
    }

    /* Print information common to the entire load. */
    fprintf(out, "\nGeneral load information:\n");
    if(comm_flag == 0){
        fprintf(out, "   Comm Disabled\n");
    } else {
        fprintf(out, "   Comm Enabled:  %d byte test message length\n", comm_flag);
    }
    fprintf(out, "   Load runtime:  %d\n", input->runtime);
    fprintf(out, "   Thread count:  %d\n", input->num_threads);
    fprintf(out, "   Subload count: %d\n", input->num_cpusets);
    fprintf(out, "   Load schedule: ");
    printSchedule(out, input->scheduling);
    fprintf(out, "\n\n");

    /* Print SubLoads in columns, with OUTPUT_STEP determining the number of columns. */
    for(i = 0; i < input->num_cpusets; i += OUTPUT_STEP){
        fprintf(out, "  ");
        for(j = 0; j < OUTPUT_STEP && sub_index[j] != NULL; j++){
            fprintf(out, "Subload %2d: %26c", j + i, ' ');
        }
        fprintf(out, "\n\n   ");
        for(j = 0; j < OUTPUT_STEP && sub_index[j] != NULL; j++){
            fprintf(out, "Number of threads: %3d %14c", sub_index[j]->num_plans, ' ');
        }
        fprintf(out, "\n\n   ");
        for(j = 0; j < OUTPUT_STEP && sub_index[j] != NULL; j++){
            fprintf(out, "Plan %5c Size(s) %18c", ' ', ' ');
        }
        fprintf(out, "\n   ");
        /* Initialize plan index array - like the subloads, the plans are also in a linked list, and each
           column of output needs a pointer to that subload's plans.						*/
        for(j = 0; j < OUTPUT_STEP; j++){
            if(sub_index[j] != NULL){
                plan_flags[j] = 0;
                plan_index[j] = sub_index[j]->first;
            } else {
                plan_flags[j] = 1;
                plan_index[j] = NULL;
            }
        }
        /* Display the plans for each subload, or dashes if any subload has no more plans. */
        while(sumArr(plan_flags, OUTPUT_STEP) < OUTPUT_STEP){
            for(j = 0; j < OUTPUT_STEP && sub_index[j] != NULL; j++){
                if(plan_index[j] != NULL){
                    plan_name = printPlan(plan_index[j]->name);
                    fprintf(out, "%-9s", plan_name);
                    size_str[0] = '\0';
                    for(k = 0; k < plan_index[j]->input_data->isize; k++){
                        snprintf(buffer, 12, "%d ", plan_index[j]->input_data->i[k]);
                        strncat(size_str, buffer, 27 - strlen(size_str));
                    }
                    for(k = 0; k < plan_index[j]->input_data->csize; k++){
                        snprintf(buffer,2 + strlen(plan_index[j]->input_data->c[k]), "%s ", plan_index[j]->input_data->c[k]);
                        strncat(size_str, buffer, 27 - strlen(size_str));
                    }
                    for(k = 0; k < plan_index[j]->input_data->dsize; k++){
                        snprintf(buffer, 15, "%-14.1f ", plan_index[j]->input_data->d[k]);
                        strncat(size_str, buffer, 27 - strlen(size_str));
                    }
                    fprintf(out, "  %-26s", size_str);
                    plan_index[j] = plan_index[j]->next;
                } else {
                    fprintf(out, "%2c-%10c-%23c", ' ', ' ', ' ');
                }
                if(plan_index[j] == NULL){
                    plan_flags[j] = 1;
                }
            }
            fprintf(out, "\n   ");
        }
        fprintf(out, "\n   ");
        for(j = 0; j < OUTPUT_STEP && sub_index[j] != NULL; j++){
            fprintf(out, "cpuset size: %2d %21c", sub_index[j]->cpuset_len, ' ');
        }

        /* Print the cpuset of each subload, with 8 integers per line and as many lines as necessary. */
        fprintf(out, "\n   ");
        k_flag = 0;
#define CPUSET_OUTPUT_STEP 8
        /* Prints the first 8 (maximum) members of each subload's cpuset. */
        for(j = 0; j < OUTPUT_STEP && sub_index[j] != NULL; j++){
            fprintf(out, "cpuset: ");
            for(k = 0; k < sub_index[j]->cpuset_len && k < CPUSET_OUTPUT_STEP; k++){
                fprintf(out, "%-3d", sub_index[j]->cpuset[k]);
            }
            if(k == CPUSET_OUTPUT_STEP){
                k_flag = 1;
            }
            fprintf(out, "%*c", 29 - k * 3, ' ');
        }
        count = 1;
        /* Prints the remaining lines of 8 (max) cpuset entries. */
        while(k_flag){
            k_flag = 0;
            fprintf(out, "\n   ");
            for(j = 0; j < OUTPUT_STEP && sub_index[j] != NULL; j++){
                if(sub_index[j]->cpuset_len > CPUSET_OUTPUT_STEP * count){
                    fprintf(out, "%8c", ' ');
                    for(k = CPUSET_OUTPUT_STEP * count; k < sub_index[j]->cpuset_len && k < CPUSET_OUTPUT_STEP * (count + 1); k++){
                        fprintf(out, "%-3d", sub_index[j]->cpuset[k]);
                    }
                    fprintf(out, "%*c", 29 - (k - CPUSET_OUTPUT_STEP * count) * 3, ' ');
                    if(k == CPUSET_OUTPUT_STEP * (count + 1)){
                        k_flag = 1;
                    }
                } else {
                    fprintf(out, "%37c", ' ');
                }
            }
            count++;
        }
        fprintf(out, "\n\n");

        /* Increments the subload indices to the next OUTPUT_STEP subloads. */
        if(sub_index[OUTPUT_STEP - 1] != NULL){
            sub_index[0] = sub_index[OUTPUT_STEP - 1]->next;
            for(j = 1; j < OUTPUT_STEP; j++){
                if(sub_index[j - 1] != NULL){
                    sub_index[j] = sub_index[j - 1]->next;
                } else {
                    sub_index[j] = NULL;
                }
            }
        } else {
            break;
        }
    }
} /* printLoad */

/** \brief A simple array summing function for use in the printLoad() function.
   \param array Array to be summed.
   \param n Number of elements in array.
 */
int sumArr(int *array, int n){
    int i, sum = 0;
    for(i = 0; i < n; i++){
        sum += array[i];
    }
    return sum;
}

/** \brief Frees the memory allocated to a LoadPlan structure.
   \param input Plan to be free'd
 */
void freePlan(LoadPlan *input){
    if(input->input_data){
        if(input->input_data->isize){
            free(input->input_data->i);
        }
        if(input->input_data->csize){
            free(input->input_data->c);
        }
        if(input->input_data->dsize){
            free(input->input_data->d);
        }
        free(input->input_data);
    }
    input->input_data = NULL;
    input->name = UNKN_PLAN;
    input->next = NULL;
} /* freePlan */

/** \brief Frees the memory allocated to a SubLoad structure.
   \param input Subload to be free'd
 */
void freeSubLoad(SubLoad *input){
    LoadPlan *plan_ptr1 = input->first;
    LoadPlan *plan_ptr2 = NULL;

    while(plan_ptr1 != NULL){
        plan_ptr2 = plan_ptr1;
        plan_ptr1 = plan_ptr1->next;
        freePlan(plan_ptr2);
        free(plan_ptr2);
    }
    input->first = NULL;
    input->last = NULL;
    input->num_plans = 0;
    if(input->cpuset != NULL){
        free(input->cpuset);
        input->cpuset = NULL;
    }
    input->cpuset_len = 0;
    input->next = NULL;
} /* freeSubLoad */

/** \brief Frees all memory dynamically allocated to a Load structure.
   \param input Load to be free'd
 */
void freeLoad(Load *input){
    SubLoad *sub_ptr1 = input->front;
    SubLoad *sub_ptr2 = NULL;

    while(sub_ptr1 != NULL){
        sub_ptr2 = sub_ptr1;
        sub_ptr1 = sub_ptr1->next;
        freeSubLoad(sub_ptr2);
        free(sub_ptr2);
    }
    input->front = NULL;
    input->back = NULL;
    input->num_cpusets = 0;
    input->num_threads = 0;
    input->runtime = 0;
    input->scheduling = INITIAL;
} /* freeLoad */

/** \brief Takes a plan name in string form and returns the enum value.
   \param planName String that will be matched with the known plan names, returning the index value of the plan
 */
plan_choice setPlan(char *planName){
    int i;
    int found = 0;
    for(i = 0; i < UNKN_PLAN; i++){
        if(!strcmp(planName, plan_list[i]->name)){
            found = 1;
            break;
        }
    }
    if(!found){
        return UNKN_PLAN;
    } else  {
        return i;
    }
}

/** \brief Takes a plan enum value and returns its name as a string.
   \param plan Enumerator value that gives the index of the plan; used to print the string name of the plan.
 */
char *printPlan(plan_choice plan){
    char *plan_name = NULL;
    plan_name = plan_list[plan]->name;
    return plan_name;
}

/** \brief Takes a schedule name as a string and returns the enum value.
   \param schedName The string representation of the scheduling scheme to be used.
 */
schedules setSchedule(char *schedName){
    schedules sched;

    if(strcmp(schedName, "BLOCK") == 0){
        sched = BLOCK;
    } else if(strcmp(schedName, "ROUND_ROBIN") == 0){
        sched = ROUND_ROBIN;
    } else if(strcmp(schedName, "SUBLOAD_SPECIFIC") == 0){
        sched = SUBLOAD_SPECIFIC;
    } else {
        sched = UNKN_SCHED;
    }

    return sched;
}

/** \brief Prints the name of the input schedule enum value.
   \param out File to print to
   \param sched Enumerator value that equates to the string version of the schedule type.
 */
void printSchedule(FILE *out, schedules sched){
    switch(sched){
    case BLOCK:
        fprintf(out, "BLOCK");
        break;
    case ROUND_ROBIN:
        fprintf(out, "ROUND_ROBIN");
        break;
    case SUBLOAD_SPECIFIC:
        fprintf(out, "SUBLOAD_SPECIFIC");
        break;
    default:
        fprintf(out, "UNKN_SCHED");
        break;
    }
}

