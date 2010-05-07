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
#include <esc/cmdargs.h>
#include <esc/io/console.h>
#include <esc/dir.h>
#include <esc/io.h>

int main(int argc,char *argv[]) {
	s32 i;
	char path[MAX_PATH_LEN];
	if(argc < 2 || isHelpCmd(argc,argv)) {
		cerr->writef(cerr,"Usage: %s <path> ...\n",argv[0]);
		return EXIT_FAILURE;
	}

	for(i = 1; i < argc; i++) {
		abspath(path,MAX_PATH_LEN,argv[i]);
		if(rmdir(path) < 0)
			error("Unable to remove directory '%s'",path);
	}

	return EXIT_SUCCESS;
}
