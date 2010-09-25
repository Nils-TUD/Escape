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
#include <esc/proc.h>
#include <esc/thread.h>
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include "iparser.h"
#include "idriver.h"

#define PRINT_DRIVERS	0

int main(void) {
	if(getpid() != 0)
		error("It's not good to start init twice ;)");

	// wait for fs; we need it for exec
	tFD fd;
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
		// read driver-spec from file
		ostringstream buf;
		ifstream ifs("/etc/drivers");
		buf << ifs.rdbuf();

		// parse them and load them
		vector<driver*> drivers = parseDrivers(buf.str());

#if PRINT_DRIVERS
		cout << "Drivers to load:" << endl;
		for(vector<driver*>::iterator it = drivers.begin(); it != drivers.end(); ++it)
			cout << **it;
		cout << endl;
#endif

		for(vector<driver*>::iterator it = drivers.begin(); it != drivers.end(); ++it)
			(*it)->load();
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
	for(int i = 0; i < VTERM_COUNT; i++) {
		ostringstream vtermName;
		vtermName << "vterm" << i;
		string name = vtermName.str();
		s32 child = fork();
		if(child == 0) {
			const char *args[] = {"/bin/shell",NULL,NULL};
			args[1] = name.c_str();
			exec(args[0],args);
			error("Exec of '%s' failed",args[0]);
		}
		else if(child < 0)
			error("Fork of '%s %s' failed","/bin/shell",name.c_str());
	}

	/* loop and wait forever */
	while(1)
		waitChild(NULL);
	return EXIT_SUCCESS;
}
