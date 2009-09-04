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
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/fileio.h>
#include <stdlib.h>

#define BUF_SIZE 512

static char buffer[BUF_SIZE];

int main(int argc,char *argv[]) {
	char path[MAX_PATH_LEN];
	tFD fd;
	s32 c;
	if(argc != 2 || isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s <file>\n",argv[0]);
		return EXIT_FAILURE;
	}

	abspath(path,MAX_PATH_LEN,argv[1]);
	fd = open(path,IO_WRITE | IO_CREATE);
	if(fd < 0) {
		printe("Unable to open '%s'",path);
		return EXIT_FAILURE;
	}

	while((c = read(STDIN_FILENO,buffer,BUF_SIZE)) > 0) {
		if(write(fd,buffer,c) != c) {
			printe("Unable to write %d bytes\n",c);
			return EXIT_FAILURE;
		}
	}

	close(fd);

	return EXIT_SUCCESS;
}
