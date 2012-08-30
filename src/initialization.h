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
	This header file contains the declarations of functions and data for
	the commandline interface of systemburn and the program configuration
	settings.
*/

#ifndef __INITIALIZATION_H
#define __INITIALIZATION_H


typedef enum {
	WORKERS,
	MAX_TEMP,
	REST_TIME,
	MONITOR_FREQ,
	MONITOR_OUT,
	UNKN_CONFIG
} configkey;

/* Global variables */
extern int  num_workers;
extern int  thermal_panic;
extern int  thermal_relaxation_time;
extern int  monitor_frequency;
extern int  monitor_output_frequency;
extern char temperature_path[ARRAY];

// extern int verbose_flag;
// extern int comm_flag;

/* Commandline and initialization functions. In initialization.c */
extern int  initialize(int argc, char* argv[], char **log_file, char **config_file, char ***load_names);
extern int  setLogName(char *log, char **log_file, int log_flag);
extern int  setLoadNames(int num_loads, char ***load_names, int index, int argc, char **argv);
extern int  setCommLoad(int buflen);
extern void freeLoadNames(int num_loads, char ***load_names);
extern int  setVerbosity(char *value);
extern void printHelpText();

/* Configuration info functions. In initialization.c */
extern int bcastConfig(int load_num);
extern int initConfigOptions(char *config, char **config_buffer);
extern int parseConfig(char *config_buffer, int config_filesize);
extern int parseConfigFile(int *numWork, int *maxTemp, int *relaxTime, int *monFreq, \
					int *monOut, char *tempPath, char *inString);
extern configkey configkeyCmp(char * name);

#endif /* __INITIALIZATION_H */
