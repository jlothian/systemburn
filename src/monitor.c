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
#include <comm.h>
#include <fcntl.h>

/*******************************************************************************
* This is a simple example of a typical activity for the monitor thread:
* checking the temperature. This particular implementation is specific
* to Linux systems with ACPI * enabled. If ACPI is not enabled, the check
* returns an approximation to absolute zero.  * The calling code should be
* smart enough to recognize this indicates a problem.
*******************************************************************************/

#define MAX_ID           0x0010
#define MAX_BUS          0x0100
#define BUFLEN           0x0400
#define NOT_YET          (-1)
#define MAX_CORES        0x0040
int MAX_HARDWARE_SENSORS = MAX_CORES * 4;
int CoreTemp[MAX_CORES * 8];
int ProcSensor;
int FILES_OPENED = NOT_YET;
int ProcTemp = 0;
int nTemps = 0;

/**
 * \brief Checks the temperatures of the nodes in the computer
 *
 * \param T Holds the data for the temperatures on a node: min, max, and avg
 */
void CheckTemperatureRange(TemperatureRange *T){
    char filename[BUFLEN],rdbuf[BUFLEN];
    float t;
    int count,core,sensor,bus,id,fd,sen;
    count = 0;
    T->max = 0.0;
    T->avg = 0.0;
    T->min = 1000.0;
    if(FILES_OPENED == NOT_YET){
        FILES_OPENED = 0;
// #ifndef USE_PROC_TEMP
        // look for thermal sensors on Intel cores -- new style
        // these are under /sys/devices/platform/coretemp.*/temp1_input
        for(core = 0; core < MAX_HARDWARE_SENSORS; core++){
            for(sen = 0; sen < 5; sen++){
                snprintf(filename, BUFLEN, "/sys/bus/platform/devices/coretemp.%d/temp%1d_input", core,sen);
                fd = open(filename, 0, O_RDONLY);
                if(fd != -1){
                    CoreTemp[nTemps++] = fd;
                    FILES_OPENED++;
                }
            }
        }
        // look for thermal sensors on Intel cores -- old style
        // these are under /sys/devices/platform/coretemp.*/temp1_input
        for(core = 0; core < MAX_HARDWARE_SENSORS; core++){
            for(sen = 0; sen < 5; sen++){
                snprintf(filename, BUFLEN, "/sys/devices/platform/coretemp.%d/temp%1d_input", core,sen);
                fd = open(filename, 0, O_RDONLY);
                if(fd != -1){
                    CoreTemp[nTemps++] = fd;
                    FILES_OPENED++;
                }
            }
        }
        // look for thermal sensors on AMD cores
        // these are under /sys/devices/pci0000:00/0000:00:*.*/temp1_input
        for(bus = 0; bus < MAX_BUS; bus++){
            for(id = 0; id < MAX_ID; id++){
                for(sen = 0; sen < 5; sen++){
                    snprintf(filename, BUFLEN, "/sys/devices/pci0000:00/0000:00:%2x.%1x/temp%1d_input",bus,id,sen);
                    fd = open(filename, 0, O_RDONLY);
                    if(fd != -1){
                        CoreTemp[nTemps++] = fd;
                        FILES_OPENED++;
                    }
                }
            }
        }
// #endif // USE_PROC_TEMP
        // if NOTHING else is available, try falling back to the old /proc interface for older kernels
        if(FILES_OPENED == 0){
            ProcSensor = open("/proc/acpi/thermal_zone/THM/temperature", 0, O_RDONLY);
            if(ProcSensor != -1){
                ProcTemp = 1;
            }
        }
    }
    if(FILES_OPENED > 0){
        for(sensor = 0; sensor < FILES_OPENED; sensor++){
            lseek(CoreTemp[sensor],0,SEEK_SET);
            read(CoreTemp[sensor],rdbuf,BUFLEN);
            t = ((double)atoi(rdbuf)) / 1000.0;
            T->min = (T->min < t) ? T->min : t;
            T->max = (T->max > t) ? T->max : t;
            T->avg += t;
        }
        T->avg = (T->avg) / FILES_OPENED;
    } else if(ProcTemp == 1){
        lseek(ProcSensor,0,SEEK_SET);
        read(ProcSensor,rdbuf,BUFLEN);
        t = ((double)atof(rdbuf + 13));
        T->min = t;
        T->max = t;
        T->avg = t;
    } else {
        T->min = 1000.;
        T->max = 0.;
        T->avg = 0.;
    }
} /* CheckTemperatureRange */

/**
 * \brief The monitor thread sleeps in a loop periodically waking up to update the thermal state of it's node.
 * If there's a problem, it can call the emergency stop routine.
 */
void *MonitorThread(void *vptr){
    CheckTemperatureRange(&local_temp);
    if(local_temp.min > local_temp.max){
        EmitLog(MyRank, MONITOR_THREAD, "Cannot access core temperatures. Monitor exiting.", -1, PRINT_RARELY);
        pthread_exit((void *)NULL);
    }
    for(;; ){     /* monitor loop */
        sleep(monitor_frequency);
        /* various system state monitiors */
        CheckTemperatureRange(&local_temp);
        if(local_temp.max >= thermal_panic){
            EmergencyStop(1);
        }
    }
    return (void *)NULL;
}

/**
 * \brief This is a nearly useless little piece of code which exists to provide the main program with a simple abstraction for starting the monitor.
 */
void StartMonitorThread(){
    /* construct monitor thread's control structure and launch */
    pthread_rwlock_init(&(MonitorHandle.Lock),0);
    MonitorHandle.Num = -1;
    MonitorHandle.Plan = NULL;
    pthread_create(&(MonitorHandle.ID), NULL, MonitorThread, &(MonitorHandle));
    #ifdef ASYNC_WORKERS
    pthread_detach(MonitorHandle.ID);
    #endif
    return;
}

/**
 * \brief reduceTemps() uses a reduction to compute the min/avg/max of core temperatures observed on all nodes.
 * Unfortunately, the use of MPI and issues with some MPI libraries not playing
 * well with pthreads means that this is called by the scheduler thread.
 */
void reduceTemps(){
/* uses either of two communication libraries, functions located in comm.c */
    #ifdef HAVE_SHMEM
    reduceTemps_SHMEM();
    #else
    reduceTemps_MPI();
    #endif
}

/**
 * \brief If the monitor thread is running, it can issue an emergency stop, if the system state exceeds operating parameters.
 * We aren't worrying about graceful shutdown here...
 * The intent is to kill everything immediately to preserve the system
 *
 * \param errorcode Tells the user why SystemBurn is exiting unexpectedly.
 */
void EmergencyStop(int errorcode){
    StopWorkerThreads();
    comm_abort(errorcode);
    exit(errorcode);
    return;     /* not bloody likely :-) */
}

