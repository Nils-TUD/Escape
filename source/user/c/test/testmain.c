/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <stdio.h>
#include <string.h>
#include <esc/messages.h>

#include "modules/speed.h"
#include "modules/tdriver.h"
#include "modules/debug.h"
#include "modules/fault.h"
#include "modules/thread.h"
#include "modules/fork.h"
#include "modules/procswarm.h"
#include "modules/mem.h"
#include "modules/stack.h"
#include "modules/forkbomb.h"
#include "modules/tls.h"
#include "modules/pipe.h"
#include "modules/shmclone.h"
#include "modules/sigclone.h"
#include "modules/fsreads.h"
#include "modules/ooc.h"
#include "modules/highcpu.h"
#include "modules/drvparallel.h"
#include "modules/pingpong.h"

#define NAME_LEN 10

typedef int (*fTest)(int argc,char *argv[]);
typedef struct {
	char name[NAME_LEN];
	fTest func;
} sTestModule;

static sTestModule modules[] = {
	{"speed",mod_speed},
	{"driver",mod_driver},
	{"debug",mod_debug},
	{"fault",mod_fault},
	{"thread",mod_thread},
	{"fork",mod_fork},
	{"forkbomb",mod_forkbomb},
	{"procswarm",mod_procswarm},
	{"mem",mod_mem},
	{"stack",mod_stack},
	{"tls",mod_tls},
	{"pipe",mod_pipe},
	{"shmclone",mod_shmclone},
	{"sigclone",mod_sigclone},
	{"fsreads",mod_fsreads},
	{"ooc",mod_ooc},
	{"highcpu",mod_highcpu},
	{"parallel",mod_drvparallel},
	{"pingpong",mod_pingpong},
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
