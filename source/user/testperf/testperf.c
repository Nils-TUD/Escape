/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <esc/common.h>
#include <esc/cmdargs.h>
#include <stdlib.h>
#include <stdio.h>

#include "modules.h"

#define NAME_LEN 16

typedef int (*fTest)(int argc,char *argv[]);
typedef struct {
	char name[NAME_LEN];
	fTest func;
} sTestModule;

static sTestModule modules[] = {
	{"getpid",		mod_getpid},
	{"yield",		mod_yield},
	{"fork",		mod_fork},
	{"startthread",	mod_startthread},
	{"file",		mod_file},
	{"mmap",		mod_mmap},
	{"sendrecv",	mod_sendrecv},
	{"pingpong",	mod_pingpong},
	{"pipe",		mod_pipe},
	{"reading",		mod_reading},
	{"writenull",	mod_writenull},
	{"memops",		mod_memops},
	{"locks",		mod_locks},
	{"chgsize",		mod_chgsize},
	{"pagefault",	mod_pagefault},
};

int main(int argc,char *argv[]) {
	size_t i;
	if(isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s [<module>] [...]\n",argv[0]);
		fprintf(stderr,"	Available modules:\n");
		for(i = 0; i < ARRAY_SIZE(modules); i++)
			fprintf(stderr,"		%s\n",modules[i].name);
		return EXIT_FAILURE;
	}

	if(argc > 1) {
		for(i = 0; i < ARRAY_SIZE(modules); i++) {
			if(strcmp(argv[1],modules[i].name) == 0)
				return modules[i].func(argc,argv);
		}
		fprintf(stderr,"Module '%s' does not exist\n",argv[1]);
	}
	else {
		printf("=============== PERFORMANCE TEST START ===============\n");
		for(i = 0; i < ARRAY_SIZE(modules); i++) {
			printf("Module '%s'...\n",modules[i].name);
			modules[i].func(argc,argv);
			printf("\n");
			fflush(stdout);
		}
		printf("=============== PERFORMANCE TEST END ===============\n");
	}
	return EXIT_FAILURE;
}
