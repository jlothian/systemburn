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
#ifndef __PLAN_YOUR_NAME_H
#define __PLAN_YOUR_NAME_H

extern void * makeYOUR_NAMEPlan(data *i);			// <- Change YOUR_NAME to your module's name.
extern int    initYOUR_NAMEPlan(void *p);			// <- Change YOUR_NAME to your module's name.
extern int    execYOUR_NAMEPlan(void *p);			// <- Change YOUR_NAME to your module's name.
extern void * killYOUR_NAMEPlan(void *p);			// <- Change YOUR_NAME to your module's name.
extern int    parseYOUR_NAMEPlan(char *line, LoadPlan *output);	// <- Change YOUR_NAME to your module's name.
extern int    perfYOUR_NAMEPlan(void *p);
extern plan_info YOUR_NAME_info;
//If there are other functions to be used by your module, declare the prototypes here. Examples in plan_dstream.h and plan_istream.h

//Below is a sample data structure. This is optional, but your module will probably need some form of input to function.

typedef struct {
	int YOUR_MEMBER;  // <- Replace YOUR_MEMBER with your member's name. This int field is necessary to be compatable with systemburn.
	OTHER_MEMBERS;    // <- Place your other members needed to run here
} YOUR_DATA_STRUCTURE_NAME; // <- Replace YOUR_DATA_STRUCTURE_NAME with the desired name of your data structure.

#endif /* __PLAN_YOUR_NAME_H */
