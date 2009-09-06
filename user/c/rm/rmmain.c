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
#include <esc/fileio.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/cmdargs.h>
#include <stdlib.h>

int main(int argc,char *argv[]) {
	u32 i;
	char rPath[MAX_PATH_LEN];
	if(argc < 2 || isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s <path> ...\n",argv[0]);
		return EXIT_FAILURE;
	}

	for(i = 1; i < argc; i++) {
		abspath(rPath,MAX_PATH_LEN,argv[i]);
		if(unlink(rPath) < 0) {
			printe("Unable to remove '%s'",rPath);
			return EXIT_FAILURE;
		}
	}

	return EXIT_SUCCESS;
}
