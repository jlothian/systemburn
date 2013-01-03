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
/*
        This file contains implementations for systemburn initialization functions
        and configuration settings functions.
 */

#include <systemburn.h>
#include <initialization.h>
#include <comm.h>

/**
   \brief	This function serves as a primary initialization phase for the systemburn benchmark.
        It opens several input files and an output log, using the input information to
        set the program's global variables.

        Input files are recieved through commandline options; the currently implemented
        options are:
        <p> &nbsp;	-c &lt;#&gt;		: enable communication load with buffer length #. </p>
        <p> &nbsp;	-f &lt;config file&gt;	: specifies a system dependent configuration file. </p>
        <p> &nbsp;	-l &lt;log file&gt;	: specifies the name of the desired log filename
                                                can specify "-" which causes output to be
                                                printed to standard out. </p>
        <p> &nbsp;	-n &lt;# of load files&gt;	: specifes the number of load files remaining in the
                                                commandline (which are non-option arguments). </p>

        All options require arguments and, if any (or all) options are omitted, these default
        files are opened:
                systemburn.config	: the default configuration file.
                systemburn.log		: the default log file.
                systemburn.load     : the default load file (with only 1 load set).

        For scalability, this function should only be executed by MPI process 0, which should
        then broadcast the values of the global variables to the other processes.

   \param argc Typical C variable: tells how many command line arguments were given to the program.
   \param argv Typical C variable: array that contains the command line arguents.
   \param log_file The user-specified file(s) where SystemBurn will print the output
   \param config_file The user-created file(s) that has the configuration settings for SystemBurn.
   \param load_names The user-created load file(s) that will be used by SystemBurn.
 */

// extern int comm_flag;

int initialize(int argc, char *argv[], char **log_file, char **config_file, char ***load_names){
    int   c = 0;
    int   buflen = 0;
    char *config = NULL;
    char *log = NULL;
    int   log_flag = 0;
    int   n = 0;
    char *verb = NULL;
    char  options[] = "c:f:l::n:tpv::h";

    /* Using commandline options and arguments, determine filenames of files to open. */
    while((c = getopt(argc, argv, options)) != -1){
        switch(c){
        case 'c':                       /* Input - comm_flag */
            buflen = atoi(optarg);
            break;
        case 'f':                       /* Input - configuration file with system information. */
            config = optarg;
            break;
        case 'l':                       /* Output - log file to record in. */
            log = optarg;
            log_flag = 1;
            break;
        case 'n':                       /* Input - the number of load file non-option arguments. */
            n = atoi(optarg);
            break;
        case 'v':
            verb = optarg;
            break;
        case 'p':                       /* disable collection of performance stats */
            planperf_flag = 0;
            break;
        case 't':
            plancheck_flag = 1;
            break;
        case 'h':                       /* Help - print some help information. */
            printHelpText();
            return ERR_CLEAN;
        case '?':
            fprintf(stderr, "Unknown option or missing argument.\n");
            break;
        }
    }

    /* Set a string to the program log's filename for later opening and use. */
    setLogName(log, log_file, log_flag);

    /* Set an output string to the specifed configuration file name for later use. */
    *config_file = config;

    /* Store the names of all the load files for later opening. */
    n = setLoadNames(n, load_names, optind, argc, argv);

    /* Set verbosity levels. */
    setVerbosity(verb);

    /* set communication load */
    setCommLoad(buflen);

    return n;           /* Return the number of loads to be run. */
} /* initialize */

/**
   \brief This function stores the names/paths of the load files in an array of strings.
   \param num_loads The number of load files that SystemBurn will run.
   \param load_names The list of load file(s) that SystemBurn will run.
   \param index The index in argv that starts the list of load files.
   \param argc Typical C variable: tells how many command line arguments were given to the program.
   \param argv Typical C variable: array that contains the command line arguents.
   \return num_loads
 */
int setLoadNames(int num_loads, char ***load_names, int index, int argc, char **argv){
    int i;
    if((num_loads <= 0) || (num_loads > (argc - index)) ){
        num_loads = argc - index;
    }

    if(num_loads > 0){
        /* allocate memory for n pointers to argv[]. */
        *load_names = (char **)malloc(num_loads * sizeof(char *));
        assert(*load_names);

        if(*load_names != NULL){
            for(i = 0; i < num_loads; i++){
                (*load_names)[i] = argv[i + index];
            }
        } else {                /* malloc() failed - abort this entire program run. */
            EmitLog(MyRank, SCHEDULER_THREAD, "Unable to allocate memory for the load file names.", -1, PRINT_ALWAYS);
            num_loads = 0;
        }
    } else {
        num_loads = 1;

        /* allocate memory for 1 array of scheduler structs */
        *load_names = (char **)malloc(1 * sizeof(char *));
        assert(*load_names);
        if(*load_names != NULL){
            *load_names[0] = NULL;
        } else {                /* malloc() failed - abort program. */
            EmitLog(MyRank, SCHEDULER_THREAD, "Unable to allocate memory for the load file's name.", -1, PRINT_ALWAYS);
            num_loads = 0;
        }
    }

    return num_loads;
} /* setLoadNames */

/**
   \brief This function handles the details of assigning a log file name obtained from the commandline to a variable for later use.
   \param log Name of a log file.
   \param log_file List of log files to be used.
   \param log_flag Tells what kind of output to use.
   \return int 0 or 1 based on whether it works
 */
int setLogName(char *log, char **log_file, int log_flag){
    if(log != NULL){
        *log_file = log;
    } else if(log_flag > 0){
        *log_file = NULL;
    } else {
        *log_file = (char *)malloc(2 * sizeof(char));
        assert(*log_file);
        if(*log_file != NULL){
            strcpy(*log_file, "-");
        } else {
            return BAD;
        }
    }
    return GOOD;
}

/**
   \brief Frees the space associated with the loads
   \param num_loads The number of load files to be used by SystemBurn.
   \param load_names The list of load files to use.
 */
void freeLoadNames(int num_loads, char ***load_names){
    if(load_names != NULL){
        if(*load_names != NULL){
            free(*load_names);
        }
    }
}

/**
   \brief Tells the EmitLog what to print, and when
   \param value The verbosity level that determines exactly how much output is printed
 */
int setVerbosity(char *value){
    if(value != NULL){
        verbose_flag = atoi(value);
    } else {
        verbose_flag = 0;
    }

    return GOOD;
}

/**
   \brief Used for the communication load
   \param buflen The size of the buffer to be used in the communication load
 */
int setCommLoad(int buflen){
    if((buflen > 0) && (buflen < 10000000)){
        comm_flag = buflen;
    } else {
        comm_flag = 0;
    }
    return GOOD;
}

/**
   \brief Helpful to new users
 */
void printHelpText(){
    printf("Usage: systemburn [OPTION]... [FILE]...\n");
    printf("Run systemburn loads stored in FILEs (systemburn.load in the current directory is the default).\n");
    printf("\n");
    printf("Options:\n");
    printf("  -f <config file>   Opens a file which supplies system configuration info. If this option\n");
    printf("                          is not specified, the default file opened is systemburn.config.\n");
    printf("  -l <log file>      Allows the user to specify a filename to store program log info in.\n");
    printf("                          the default file is systemburn.log. (This option is currently unavailable)\n");
    printf("  -c <comm msgsize>  Runs a communication load with specific message sizes.\n");
    printf("  -n <# load files>  The number of files specifed as non-option arguments to systemburn.\n");
    printf("  -v <output level>  Determines the amount of output, with 0 the default and 3 the most.\n");
    printf("  -p                 Disable calculation and output of performance statistics.\n");
    printf("  -t                 Enables calculation checks in running load, when available.\n");
    printf("  -h                 Displays this help text and exits.\n");
    printf("\n");
    printf("Exit Status:\n");
    printf("  0  if OK,\n  1  if serious error (memory allocation failure, inaccessible input file...)\n");
    printf("\n");
} /* printHelpText */

/* This function performs a one-time broadcast of global configuration data to all MPI processes.
   int bcastConfig(int load_num) {
   #ifdef HAVE_SHMEM
        return bcastConfig_SHMEM(load_num);
   #else
        return bcastConfig_MPI(load_num);
   #endif
   }
 */

/**
   \brief This functions opens the configuration file and assigns the values in it to global variables.
   \param config The name of the configuration file to be loaded into SystemBurn
   \param buffer The output buffer containing a copy of the config file.
   \return The size in bytes of the config buffer.
 */
int initConfigOptions(char *config, char **config_buffer){
    FILE *configFile = NULL;
    char filename[ARRAY];
    int filesize, ret = BAD;

    if(config == NULL){                 /* No configuration file argument - loading default file. */
        strncpy(filename, "systemburn.config", ARRAY);
    } else {
        strncpy(filename, config, ARRAY);
    }

    filesize = fsize(filename);

    if(filesize > 0){
        configFile = fopen(filename, "r");

        if(configFile != NULL){
            (*config_buffer) = getFileBuffer(filesize);
            int tmp = readFile(*config_buffer, filesize, configFile);
            fclose (configFile);
        }
    }

    return filesize;
} /* initConfigOptions */

/**
   \brief Handles error checking for parsing the configuration file from a buffer.
   \param [in] config_buffer The string buffer containing the configuration file.
   \param [in] config_filesize The size in bytes of the input buffer.
   \returns An error code depending on whether a complete config file was parsed.
 */
int parseConfig(char *config_buffer, int config_filesize){
    int flag, workers, panic_temp, relax_time, mon_freq, mon_output;
    char tempInfo[ARRAY];
    int ret = GOOD;

    workers = panic_temp = relax_time = mon_freq = mon_output = 0;

    /* Parse the string buffer for configuration values. */
    flag = parseConfigFile(&workers, &panic_temp, &relax_time, &mon_freq, &mon_output, tempInfo, config_buffer);

    /* Assign the contents of the configFile to the appropriate global variables. */
    num_workers = workers;
    thermal_panic = panic_temp;
    thermal_relaxation_time = relax_time;
    monitor_frequency = mon_freq;
    monitor_output_frequency = mon_output;
    /* BUG: strcpy(temperature_path, tempInfo); */

    /* parseConfigFile already handles setting defaults, so just let the user know that they are being used. */
    if(flag != GOOD){
        if(MyRank == ROOT){
            EmitLog(MyRank, SCHEDULER_THREAD, "Invalid/incomplete configuration file. Using default values where needed.", -1, PRINT_ALWAYS);
        }
        ret = BAD;
    }
    return ret;
} /* parseConfig */

/**
   \brief This function handles parsing of the configuration file.
   \param numWork The number of worker threads to be spawned
   \param maxTemp The maximum temperature that SystemBurn will allow the computer to reach
   \param relaxTime The amount of time that SystemBurn will sleep in between plans to cool off.
   \param monFreq How often the monitor thread will check the system temperatures.
   \param monOut How often the monitor thread will output the min/max/avg temps. to the screen.
   \param tempPath Where to find the temperature monitoring files.
   \param inFile
   \return int 0 or 1 based on success of the function.
 */
int parseConfigFile(int *numWork, int *maxTemp, int *relaxTime, int *monFreq, \
                    int *monOut, char *tempPath, char *inString){
    int flag, count = 0;
    int ret = GOOD;
    char line_buffer[ARRAY];
    char temp_string[ARRAY];
    int temp_int;
    int str_offset = 0;
    configkey key;

    /* Set default values for the configuration variables. */
    *numWork = 16;
    *maxTemp = 70;
    *relaxTime = 15;
    *monFreq = 1;
    *monOut = 10;

    /* Parses the config buffer, ignoring everything on a line after a '#' symbol. */
    while((str_offset = strgetline(line_buffer, ARRAY, inString, str_offset)) <= strlen(inString)){
        flag = sscanf(line_buffer, "%s %d", temp_string, &temp_int);
        if(flag == 2){
            if(temp_string[0] != '#'){
                key = configkeyCmp(temp_string);
                switch(key){
                case WORKERS:
                    *numWork = temp_int;
                    count++;
                    break;
                case MAX_TEMP:
                    *maxTemp = temp_int;
                    count++;
                    break;
                case REST_TIME:
                    *relaxTime = temp_int;
                    count++;
                    break;
                case MONITOR_FREQ:
                    *monFreq = temp_int;
                    count++;
                    break;
                case MONITOR_OUT:
                    *monOut = temp_int;
                    count++;
                    break;
                default:
                    break;
                }
            }
        }
    }

    if(count < 5){
        ret = BAD;
    }

    return ret;
} /* parseConfigFile */

/**
   \brief Converts the string name into a variable
   \param name The thread name that will be converted into a key
 */
configkey configkeyCmp(char *name){
    configkey ret;

    if(strcmp(name, "WORKERS") == 0){
        ret = WORKERS;
    } else if(strcmp(name, "MAX_TEMP") == 0){
        ret = MAX_TEMP;
    } else if(strcmp(name, "REST_TIME") == 0){
        ret = REST_TIME;
    } else if(strcmp(name, "MONITOR_FREQ") == 0){
        ret = MONITOR_FREQ;
    } else if(strcmp(name, "MONITOR_OUT") == 0){
        ret = MONITOR_OUT;
    } else {
        ret = UNKN_CONFIG;
    }

    return ret;
} /* configkeyCmp */

