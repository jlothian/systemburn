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
#include <planheaders.h>

/**
 * \brief A list of generic error messages that can be used by all of the plans.
 */
char *generic_errors[] = {
    " plan memory allocation errors:",
    " calculation errors:"
};

/**
 * \brief A list of system errors that should only be accessed from the scheduler; they indicate errors before the plans begin execution.
 */
char *system_error[] = {
    "Plan structure allocation failures:",
    "Cpuset allocation failures:",
    "Thread cpu affinity mask errors:"
};

/**
 * \brief Combines two numbers a and b into a key
 * \param a First value
 * \param b Second value
 * \return int Combined key.
 * \sa key_conv
 */
int key_gen(int a, int b){
    if(b < 0){
        a++;
    }
    return (a << (sizeof(int) * 4)) + b;
}

/**
 * \brief Takes a key and splits it into two variables, held in ret
 * \param a Key previously created with key_gen.
 * \sa key_gen
 * \return ret An int[2] that holds the two values hidden in the key
 */
int *key_conv(int a){
    int *ret = (int *)malloc(2 * sizeof(int));
    ret[0] = a >> (sizeof(int) * 4);
    ret[1] = a - (ret[0] << sizeof(int) * 4);
    return ret;
}

/**
 * \brief Creates an error code. If spec is true, then it uses a specific error from the plan
 * \param error Error code, either with the enum (ALLOC..) or the index in a plan's custom array of error messages.
 * \param spec Specific? If true, then the message is pulled from the plan's list of error notifications. If false, it uses the given set of messages.
 * \return ret The error code, used by the error generator
 */
int make_error(int error, int spec){
    int ret = 0;
    if(spec){
        ret = error + GEN_SIZE;
    } else {
        ret = error;
    }
    return ret;
}

/**
 * \brief Adds an error to the array that holds a running list of errors encountered during the run
 * \param m Points to the calling thread.
 * \param name The name of the plan where the error occured.
 * \param error Index of the error message.
 */
inline void add_error(void *m, int name, int error){
    ThreadHandle *me = (ThreadHandle *)m;
    pthread_rwlock_wrlock(&(me->Lock));
    me->Flag[name + 1][error]++;
    pthread_rwlock_unlock(&(me->Lock));
}

/**
 * \brief Parses the load file, and pulls the input information for use in the plan
 * \param line The input line to be parsed.
 * \returns The data structure that holds the input values for the plan.
 */
data *get_sizes(char *line){
    // Set up s to be able to hold the input given in the load file

    char **s;
    double *bytes;
    char *line_copy;
    int i,j;
    int num_tokens;

    line_copy = (char *)malloc((strlen(line) + 1) * sizeof(char));
    assert(line_copy);
    strcpy(line_copy, line);

    tokenize_line(line_copy, &s, &num_tokens);

    bytes = (double *)malloc(num_tokens * sizeof(double));
    assert(bytes);

    // This will loop through the inputs, making a key to be used later when separating the inputs into their types
    for(j = 0; j < num_tokens; j++){
        bytes[j] = key_gen(0,0);                // initialize the key as the default - an integer
        if((s[j] != NULL) && (s[j][0] != '\0') ){
            for(i = 0; i < strlen(s[j]); i++){
                if(!isdigit(s[j][i])){
                    if(i == 0){
                        bytes[j] = key_gen(-1,0);                       // input is a string (input[0] is not a digit)
                        break;
                    } else if((s[j][i] == 'k') || (s[j][i] == 'K') ){             // input is a size (Kb). Mark the size (1024) and the index where the number ends (i)
                        bytes[j] = key_gen(1024,i);
                        break;
                    } else if((s[j][i] == 'm') || (s[j][i] == 'M') ){             // input is a size (Mb). See above ^
                        bytes[j] = key_gen(1025,i);
                        break;
                    } else if((s[j][i] == 'g') || (s[j][i] == 'G') ){             // input is a size (Gb). See above ^
                        bytes[j] = key_gen(1026,i);
                        break;
                    } else if((s[j][i] == 't') || (s[j][i] == 'T') ){             // input is a size (Tb). See above ^
                        bytes[j] = key_gen(1027,i);
                        break;
                    } else if((s[j][i] == 'p') || (s[j][i] == 'P') ){             // ^
                        bytes[j] = key_gen(1028,i);
                        break;
                    } else if((s[j][i] == 'e') || (s[j][i] == 'E') ){             // ^
                        bytes[j] = key_gen(1029,i);
                        break;
                    } else if(s[j][i] == '.'){                    // input is a double
                        bytes[j] = key_gen(1,0);
                        // break; - marks as double then continues to check if we have suffix
                    }
                }
            }
        } else {
            bytes[j] = key_gen(-100,0);
        }
    }

    // Originally these were two separate functions, leading to the necessity of the keys. Obsolete?

    data *sizes = (data *) malloc(sizeof(data));
    int string,in,dbl;
    string = in = dbl = 0;
    for(i = 0; i < num_tokens; i++){ // Count the number of ints, doubles, and strings. Also, separate out the keys.
        int *val = key_conv(bytes[i]);
        if(val[0] == -1){
            string++;
        }
        if(val[0] == 0){
            in++;
        }
        if((val[0] > 100) || (val[0] == 1) ){
            dbl++;
        }
        free(val);
    }

    // The above section could probably be combined with the previous loop..

    sizes->i = (int *) malloc(in * sizeof(int));
    sizes->c = (char **) malloc(string * sizeof(char *));
    sizes->d = (double *) malloc(dbl * sizeof(double));
    string = in = dbl = 0;
    for(i = 0; i < num_tokens; i++){
        int *val = key_conv(bytes[i]);
        if(val[0] == -1){        // string, so just copy it
            sizes->c[string++] = strdup(s[i]);
        } else if(val[0] == 1){        // double, so just convert it
            sizes->d[dbl++] = atof(s[i]);
        } else if(val[0] > 100){
            /*      if(val[0] == 1001) val[0]=(val[0]-1)*1024;  // 1,048,576 = mb
                    else if(val[0] == 1002) val[0]=(val[0]-2)*1024*1024;  // 1,073,741,824 = gb
                    else if(val[0] == 1003) val[0]=(val[0]-3)*1024*1024*1024;  // 1,099,511,627,776 = tb
             */
            // Reasoning for the loop below is found above ;)
            for(j = val[0] - 1024; val[0] % 1024; ){
                val[0] -= j;
                val[0] *= 1024;
                val[0] += --j;
            }

            s[i][ val[1] ] = '\0';             // Truncate after the size; cut off the k,m,...

            sizes->d[dbl] = atof(s[i]);            // Used because some of the higher sizes exceed the int max_value
            sizes->d[dbl] *= val[0];
            //sizes->d[dbl] = sizes->d[dbl]/sizeof(double);
            dbl++;
        } else if(s[i][0] != '\0'){        // integer, so just convert it
            sizes->i[in++] = atoi(s[i]);
        }
        free(val);
    }
    sizes->isize = in;
    sizes->csize = string;
    sizes->dsize = dbl;
    free(line_copy);
    return sizes;
} /* get_sizes */

/**
 * \brief Splits the plan line string into tokens, ready for conversion.
 * \param [in] line The input string to be parsed.
 * \param [out] tokens The output array of tokens, each its own string.
 * \param [out] count The output number of tokens found in the input line.
 */
void tokenize_line(char *line, char ***tokens, int *count){
    char *token;
    char **token_list;
    int  num_tokens = -3;       // Start at the 4th token (the first 3 are "PLAN # NAME")
    int  num_guess = 5;         // Guess the number of tokens we'll have

    // Guess the number of useful tokens to store (then realloc if more are needed)
    token_list = (char **)malloc(num_guess * sizeof(char *));
    assert(token_list);

    // Tokenize the copied line
    token = strtok(line, " \t\n");
    while(token != NULL){
//		printf("token #%d : %s\n", num_tokens, token);
        if(num_tokens >= 0){
            // If we run out of space, allocate more
            if(num_tokens >= num_guess){
                num_guess += 5;
                token_list = (char **)realloc(token_list, num_guess * sizeof(char *));
                assert(token_list);
            }
            // Add the new token to the list.
            token_list[num_tokens] = token;
        }
        token = strtok(NULL, " \t\n");
        num_tokens++;
    }

    // Assign outputs
    *tokens = token_list;
    *count = num_tokens;
} /* tokenize_line */

