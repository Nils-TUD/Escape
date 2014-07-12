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
#include <esc/io.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include "../cmds.h"

int shell_cmdPwd(A_UNUSED int argc,A_UNUSED char **argv) {
	char *path = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
	if(path == NULL) {
		printe("Unable to allocate mem");
		return EXIT_FAILURE;
	}

	if(getenvto(path,MAX_PATH_LEN + 1,"CWD") < 0) {
		printe("Unable to get CWD");
		return EXIT_FAILURE;
	}

	printf("%s\n",path);
	free(path);
	return EXIT_SUCCESS;
}
