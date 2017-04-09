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

#include <sys/common.h>
#include <sys/io.h>
#include <sys/stat.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../cmds.h"

int shell_cmdCd(int argc,char **argv) {
	char path[MAX_PATH_LEN];
	if(argc != 2 || getopt_ishelp(argc,argv)) {
		fprintf(stderr,"Usage: cd <dir>\n");
		return EXIT_FAILURE;
	}

	ssize_t len;
	if((len = canonpath(path,sizeof(path),argv[1])) < 0) {
		fprintf(stderr,"canonpath for '%s' failed: %s\n",argv[1],strerror(len));
		return EXIT_FAILURE;
	}

	/* check if it is a directory */
	if(!isdir(path)) {
		fprintf(stderr,"%s is no directory\n",path);
		return EXIT_FAILURE;
	}

	/* finally change dir */
	setenv("CWD",path);
	return EXIT_SUCCESS;
}
