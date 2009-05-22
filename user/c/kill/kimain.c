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
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/signals.h>
#include <stdlib.h>
#include <string.h>

int main(int argc,char **argv) {
	tPid pid;
	if(argc != 2) {
		fprintf(stderr,"Usage: %s <pid>\n",argv[0]);
		return EXIT_FAILURE;
	}

	pid = atoi(argv[1]);
	if(pid > 0) {
		if(sendSignalTo(pid,SIG_KILL,0) < 0) {
			printe("Unable to send signal %d to %d",SIG_KILL,pid);
			return EXIT_FAILURE;
		}
	}
	else if(strcmp(argv[1],"0") != 0)
		fprintf(stderr,"Unable to kill process with pid '%s'\n",argv[1]);
	else
		fprintf(stderr,"You can't kill 'init'\n");

	return EXIT_SUCCESS;
}
