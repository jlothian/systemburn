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

/* Helper function to handle PAPI errors */
inline void PAPI_EmitLog(int val, int rank, int tnum, int debug){
    char message[512];

    snprintf(message, 512, "PAPI error %d: %s\n", val, PAPI_strerror(val));
    EmitLog(rank, tnum, message, -1, PRINT_ALWAYS);
    //exit(1);
}

/** \brief Outputs given info to stdout in order to let the user know what is going on.
 \param rank The rank of the calling thread
 \param tnum The thread number of the calling thread
 \param text The message to be printed
 \param data (Optional) Must be given, but if it is 0 then it is ignored. Integer data to print with the message.
 \param debug Tells whether or not to print the message, based on the verbosity level
 */
void EmitLog(int rank, int tnum, char *text, int data, int debug) {
	if (debug <= verbose_flag) {
		struct timeval CurrentTime;
		struct tm montime;
		char tstring[ARRAY];
		char *tname;
		char *tshort;
		if (tnum == -2) {
			tname = "Monitor";
			tshort = "MONTR";
		} else if (tnum == -1) {
			tname = "Scheduler";
			tshort = "SCHED";
		} else {
			tname = "Worker";
			tshort = NULL;
		}
		gettimeofday(&CurrentTime, NULL);
		localtime_r(&CurrentTime.tv_sec, &montime);
		strftime(tstring, sizeof(tstring), "%Y.%m.%d.%H:%M:%S", &montime);
		if ( tnum < 0 ) {
			if (data >= 0) {
				printf("%s R:%05d T_%5s : %s %d\n", tstring, rank, tshort, text, data);
			} else {
				printf("%s R:%05d T_%5s : %s\n",    tstring, rank, tshort, text);
			}
		} else {
			if (data >= 0) {
				printf("%s R:%05d W_%05d : %s %d\n", tstring, rank, tnum, text, data);
			} else {
				printf("%s R:%05d W_%05d : %s\n",    tstring, rank, tnum, text);
			}
		}
	}
}

/** \brief Same as EmitLog, except it takes/outputs more data to the screen.
 \param rank The rank of the calling thread
 \param tnum The thread number of the calling thread
 \param text The message to be printed
 \param data1 Integer data to print with the message.
 \param data2 Integer data to print with the message.
 \param data3 Integer data to print with the message.
 \param debug Tells whether or not to print the message, based on the verbosity level
 \sa EmitLog
 */
void EmitLog3(int rank, int tnum, char *text, int data1, int data2, int data3, int debug) {
	if (debug <= verbose_flag) {
		struct timeval CurrentTime;
		struct tm montime;
		char tstring[ARRAY];
		char *tname;
		char *tshort;
		if (tnum == -2) {
			tname = "Monitor";
			tshort = "MONTR";
		} else if (tnum == -1) {
			tname = "Scheduler";
			tshort = "SCHED";
		} else {
			tname = "Worker";
			tshort = NULL;
		}
		gettimeofday(&CurrentTime, NULL);
		localtime_r(&CurrentTime.tv_sec, &montime);
		strftime(tstring, sizeof(tstring), "%Y.%m.%d.%H:%M:%S", &montime);
		if ( tnum < 0 ) {
			printf("%s R:%05d T_%5s : %s %d / %d / %d\n", tstring, rank, tshort, text, data1,data2,data3);
		} else {
			printf("%s R:%05d W_%05d : %s %d / %d / %d\n", tstring, rank, tnum, text, data1,data2,data3);
		}
	}
}

/** \brief Same as EmitLog3, except the data used are float types.
 \param rank The rank of the calling thread
 \param tnum The thread number of the calling thread
 \param text The message to be printed
 \param data1 Float data to print with the message.
 \param data2 Float data to print with the message.
 \param data3 Float data to print with the message.
 \param debug Tells whether or not to print the message, based on the verbosity level
 \sa Emitlog3
 */
void EmitLog3f(int rank, int tnum, char *text, float data1, float data2, float data3, int debug) {
	if (debug <= verbose_flag) {
		struct timeval CurrentTime;
		struct tm montime;
		char tstring[ARRAY];
		char *tname;
		char *tshort;
		if (tnum == -2) {
			tname = "Monitor";
			tshort = "MONTR";
		} else if (tnum == -1) {
			tname = "Scheduler";
			tshort = "SCHED";
		} else {
			tname = "Worker";
			tshort = NULL;
		}
		gettimeofday(&CurrentTime, NULL);
		localtime_r(&CurrentTime.tv_sec, &montime);
		strftime(tstring, sizeof(tstring), "%Y.%m.%d.%H:%M:%S", &montime);
		if ( tnum < 0 ) {
			printf("%s R:%05d T_%5s : %s %4.1f / %4.1f / %4.1f\n",  tstring, rank, tshort, text, data1, data2, data3);
		} else {
			printf("%s R:%05d W_%05d : %s %4.1f / %4.1f / %4.1f\n", tstring, rank, tnum,   text, data1, data2, data3);
		}
	}
}

/**
 * \brief Same as EmitLog, but prints a double and a string.
 * \param rank The rank of the calling thread
 * \param tnum The thread number of the calling thread
 * \param text The message to be printed
 * \param data1 Float data to print with the message.
 * \param data2 String to print with the message.
 * \param debug Tells whether or not to print the message, based on the verbosity level
 * \sa Emitlog
 */
void EmitLogfs(int rank, int tnum, char *text, double data1, char *data2, int debug) {
	if (debug <= verbose_flag) {
		struct timeval CurrentTime;
		struct tm montime;
		char tstring[ARRAY];
		char *tname;
		char *tshort;
		if (tnum == -2) {
			tname = "Monitor";
			tshort = "MONTR";
		} else if (tnum == -1) {
			tname = "Scheduler";
			tshort = "SCHED";
		} else {
			tname = "Worker";
			tshort = NULL;
		}
		gettimeofday(&CurrentTime, NULL);
		localtime_r(&CurrentTime.tv_sec, &montime);
		strftime(tstring, sizeof(tstring), "%Y.%m.%d.%H:%M:%S", &montime);
		if ( tnum < 0 ) {
			printf("%s R:%05d T_%5s : %s %8.4f %s\n",  tstring, rank, tshort, text, data1, data2);
		} else {
			printf("%s R:%05d W_%05d : %s %8.4f %s\n", tstring, rank, tnum,   text, data1, data2);
		}
	}
}

/**
 \brief Examines an open file stream and returns the size of the file in bytes.
 \param [in] path The path of the file to be examined.
 \returns The size in bytes of the file.
 */
int fsize(char *path) {
	long long filesize = 0;
	int err;
	struct stat file_stats;
	
	/* Ensure file path is valid. */
	assert(path);
	
	/* Get file statistics. */
	err = stat(path, &file_stats);
	
	/* On success: */
	if (err == 0) {
		filesize = (long long)file_stats.st_size;
	}
	
	/* Return filesize (# of bytes). */
	return (int)filesize;
}

/**
 \brief Allocates a memory buffer for storing a copy of the load file.
 \param [in] filesize The size in bytes of the buffer to be created.
 \returns A pointer to the allocated buffer.
 */
char * getFileBuffer(int filesize) {
	char *buffer = NULL;
	const size_t PAGE_SIZE = 4096;
	size_t buffer_size = (filesize / PAGE_SIZE + 1) * PAGE_SIZE;
	if (filesize > 0) {
		/* Allocate memory for the buffer. */
		buffer = (char *)malloc(buffer_size * sizeof(char));
		assert(buffer);
	}
	
	return buffer;
}

/**
 \brief Fills a memory buffer with data from the open load file.
 \param [out] buffer The pre-allocated memory buffer.
 \param [in] size The size in bytes of data to be read from the file.
 \param [in] stream The file stream to be read from.
 \returns The number of bytes of data placed in the buffer (the size parameter or less).
 */
int readFile(char *buffer, int size, FILE *stream) {
	int ret = 0;
	
	/* Ensure file stream and buffer are valid. */
	assert(stream);
	assert(buffer);
	
	/* Read size bytes from the stream. */
	ret = fread(buffer, sizeof(char), size, stream);
	
	/* Place a null terminator at the end of the string. */
	buffer[ret+1] = '\0';
	
	/* Return the number of bytes read. */
	return ret;
}

/**
 \brief Copies and outputs the first newline terminated string occuring in the input string.
 Imitates the fgets() function, only this time for reading from strings.
 \param [out] out The copied line of the input string.
 \param [in] out_len The maximum length of the output string.
 \param [in] source The original source string to be copied from.
 \param [in] source_offset The element offset from the front of the string to begin the copy.
 \returns The element offset of the character immediately following the most recently encountered newline.
 */
int strgetline(char *out, int out_len, char *source, int source_offset) {
	char *start = source + source_offset;
	int c = 0;
	while ((start[c] != '\n') && (c < out_len-1) && (start[c] != '\0')) {
		out[c] = start[c];
		c++;
	}
	out[c] = '\0';
	
	return source_offset+c+1;
}
