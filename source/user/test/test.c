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

#include <sys/cmdargs.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <stdio.h>
#include <string.h>

#include "modules.h"

#define NAME_LEN 32

typedef int (*fTest)(int argc,char *argv[]);
typedef struct {
	char name[NAME_LEN];
	fTest func;
} sTestModule;

static sTestModule modules[] = {
	{"driver",mod_driver},
	{"drivercancel",mod_drivercancel},
	{"driverdelob",mod_driverdelob},
	{"debug",mod_debug},
	{"fault",mod_fault},
	{"thread",mod_thread},
	{"forkbomb",mod_forkbomb},
	{"procswarm",mod_procswarm},
	{"mem",mod_mem},
	{"stack",mod_stack},
	{"tls",mod_tls},
	{"pipe",mod_pipe},
	{"sigclone",mod_sigclone},
	{"fsreads",mod_fsreads},
	{"ooc",mod_ooc},
	{"highcpu",mod_highcpu},
	{"highcput",mod_highcput},
	{"parallel",mod_drvparallel},
	{"maxthreads",mod_maxthreads},
	{"pagefaults",mod_pagefaults},
	{"matmult",mod_matmult},
	{"rwlock",mod_rwlock},
	{"mutex",mod_mutex},
};

int main(int argc,char *argv[]) {
	size_t i;
	if(argc < 2 || isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s <module> [...]\n",argv[0]);
		fprintf(stderr,"	Available modules:\n");
		for(i = 0; i < ARRAY_SIZE(modules); i++)
			fprintf(stderr,"		%s\n",modules[i].name);
		return EXIT_FAILURE;
	}

	for(i = 0; i < ARRAY_SIZE(modules); i++) {
		if(strcmp(argv[1],modules[i].name) == 0)
			return modules[i].func(argc,argv);
	}

	fprintf(stderr,"Module '%s' does not exist\n",argv[1]);
	return EXIT_FAILURE;
}
