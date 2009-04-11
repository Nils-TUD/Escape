/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/env.h>
#include <esc/dir.h>
#include <esc/io.h>
#include <esc/fileio.h>
#include <string.h>
#include <stdlib.h>
#include "cd.h"

s32 shell_cmdCd(u32 argc,char **argv) {
	char *path;
	sFileInfo info;

	if(argc != 2) {
		printf("Usage: cd <directory>\n");
		return EXIT_FAILURE;
	}

	path = abspath(argv[1]);

	/* ensure that the user remains in file: */
	if(strncmp(path,"file:",5) != 0) {
		printf("Unable to change to '%s'\n",path);
		return 1;
	}

	/* retrieve file-info */
	if(getFileInfo(path,&info) < 0) {
		printe("Unable to get file-info for '%s'",path);
		return EXIT_FAILURE;
	}

	/* check if it is a directory */
	if(!MODE_IS_DIR(info.mode)) {
		fprintf(stderr,"%s is no directory\n",path);
		return EXIT_FAILURE;
	}

	/* finally change dir */
	setEnv("CWD",path);
	return EXIT_SUCCESS;
}
