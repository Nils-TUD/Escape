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
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/heap.h>
#include <esc/fileio.h>
#include "pwd.h"

s32 shell_cmdPwd(u32 argc,char **argv) {
	char *path;
	UNUSED(argc);
	UNUSED(argv);

	path = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
	if(path == NULL) {
		printe("Unable to allocate mem");
		return EXIT_FAILURE;
	}

	if(!getEnv(path,MAX_PATH_LEN + 1,"CWD")) {
		printe("Unable to get CWD");
		return EXIT_FAILURE;
	}

	printf("%s\n",path);
	free(path);
	return EXIT_SUCCESS;
}
