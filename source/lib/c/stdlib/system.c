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
#include <esc/proc.h>
#include <esc/io.h>
#include <errors.h>

int system(const char *cmd) {
	int child;
	sExitState state;
	/* check whether we have a shell */
	if(cmd == NULL) {
		tFD fd = open("/bin/shell",IO_READ);
		if(fd >= 0) {
			close(fd);
			return EXIT_SUCCESS;
		}
		return EXIT_FAILURE;
	}

	child = fork();
	if(child == 0) {
		const char *args[] = {"/bin/shell","-e",NULL,NULL};
		args[2] = cmd;
		exec(args[0],args);

		/* if we're here there is something wrong */
		error("Exec of '%s' failed",args[0]);
	}
	else if(child < 0)
		error("Fork failed");

	/* wait and return exit-code */
	if((child = waitChild(&state)) < 0)
		return child;
	return state.exitCode;
}
