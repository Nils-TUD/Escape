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
#include <esc/dir.h>
#include <esc/io.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cd.h"

int shell_cmdCd(int argc,char **argv) {
	char path[MAX_PATH_LEN];
	sFileInfo info;

	if(argc != 2 || isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: cd <dir>\n");
		return EXIT_FAILURE;
	}

	abspath(path,sizeof(path),argv[1]);

	/* retrieve file-info */
	if(stat(path,&info) < 0) {
		printe("Unable to get file-info for '%s'",path);
		return EXIT_FAILURE;
	}

	/* check if it is a directory */
	if(!S_ISDIR(info.mode)) {
		fprintf(stderr,"%s is no directory\n",path);
		return EXIT_FAILURE;
	}

	/* finally change dir */
	setenv("CWD",path);
	return EXIT_SUCCESS;
}
