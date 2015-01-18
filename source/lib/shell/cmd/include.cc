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

#include <shell/shell.h>
#include <sys/common.h>
#include <stdio.h>

#include "../exec/env.h"
#include "../cmds.h"

extern sEnv *curEnv;

int shell_cmdInclude(int argc,char **argv) {
	if(argc < 2) {
		fprintf(stderr,"Usage: %s <file> [<args>...]\n",argv[0]);
		return EXIT_FAILURE;
	}

	sEnv *old = curEnv;
	curEnv = env_create(old);
	env_addArgs(curEnv,argc - 1,(const char**)argv + 1);
	int res = shell_executeCmd(argv[1],true);
	env_destroy(curEnv);
	curEnv = old;
	return res;
}
