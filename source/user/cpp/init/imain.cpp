/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/proc.h>
#include <esc/thread.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <fstream>
#include "idriver.h"

static void createLogin(int vterm);

static int loginShells[VTERM_COUNT];

int main(void) {
	sExitState state;
	if(getpid() != 0)
		error("It's not good to start init twice ;)");

	// wait for fs; we need it for exec
	int fd;
	int retries = 0;
	do {
		fd = open("/dev/fs",IO_READ | IO_WRITE);
		if(fd < 0)
			yield();
		retries++;
	}
	while(fd < 0 && retries < driver::MAX_WAIT_RETRIES);
	if(fd < 0)
		error("Unable to open /dev/fs after %d retries",retries);
	close(fd);

	/* set basic env-vars */
	if(setenv("CWD","/") < 0)
		error("Unable to set CWD");
	if(setenv("PATH","/bin/") < 0)
		error("Unable to set PATH");

	try {
		// read drivers from file
		vector<driver> drivers;
		ifstream ifs("/etc/drivers");
		while(!ifs.eof()) {
			driver drv;
			ifs >> drv;
			drivers.push_back(drv);
		}

		for(vector<driver>::iterator it = drivers.begin(); it != drivers.end(); ++it) {
			cout << "Loading '/sbin/" << it->name() << "'..." << endl;
			it->load();
		}
	}
	catch(const load_error& e) {
		cerr << "Unable to load drivers: " << e.what() << endl;
		return EXIT_FAILURE;
	}

	// remove stdin, stdout and stderr. the shell wants to provide them
	close(STDERR_FILENO);
	close(STDOUT_FILENO);
	close(STDIN_FILENO);

	// now load the shells
	for(int i = 0; i < VTERM_COUNT; i++)
		createLogin(i);

	/* loop and wait forever */
	while(1) {
		waitChild(&state);

		/* restart the child */
		for(int i = 0; i < VTERM_COUNT; i++) {
			if(loginShells[i] == state.pid) {
				createLogin(i);
				break;
			}
		}
	}
	return EXIT_SUCCESS;
}

static void createLogin(int vterm) {
	ostringstream vtermName;
	vtermName << "vterm" << vterm;
	string name = vtermName.str();
	loginShells[vterm] = fork();
	if(loginShells[vterm] == 0) {
		const char *args[] = {"/bin/login",NULL,NULL};
		args[1] = name.c_str();
		exec(args[0],args);
		error("Exec of '%s' failed",args[0]);
	}
}
