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
#include <stdio.h>
#include <esc/proc.h>
#include <esc/debug.h>
#include <esc/io/ifilestream.h>
#include <esc/util/string.h>
#include <esc/util/vector.h>
#include <esc/mem/heap.h>
#include <string.h>
#include <width.h>

#include "iparser.h"
#include "idriver.h"

#define DRIVERS_FILE		"/etc/drivers"

/**
 * Loads the driver-dependency-file
 * @return the file-content
 */
static char *getDrivers(void);

int main(void) {
	tFD fd;
	s32 child;
	u32 i,retries = 0;
	u32 vtLen;
	char *vtermName;
	sVector *drivers;

	if(getpid() != 0)
		error("It's not good to start init twice ;)");

	/* wait for fs; we need it for exec */
	do {
		fd = open("/dev/fs",IO_READ | IO_WRITE);
		if(fd < 0)
			yield();
		retries++;
	}
	while(fd < 0 && retries < MAX_WAIT_RETRIES);
	if(fd < 0)
		error("Unable to open /dev/fs after %d retries",retries);
	close(fd);

	/* now read the drivers we should load and parse them */
	drivers = parseDrivers(getDrivers());
	/* finally load them */
	if(!loadDrivers(drivers))
		error("Unable to load drivers");

	/* remove stdin, stdout and stderr. the shell wants to provide them */
	close(STDERR_FILENO);
	close(STDOUT_FILENO);
	close(STDIN_FILENO);

	/* now load the shells */
	vtLen = SSTRLEN("vterm") + getnwidth(VTERM_COUNT) + 1;
	vtermName = (char*)heap_alloc(vtLen);

	for(i = 0; i < VTERM_COUNT; i++) {
		snprintf(vtermName,vtLen,"vterm%d",i);
		child = fork();
		if(child == 0) {
			const char *args[] = {"/bin/shell",NULL,NULL};
			args[1] = vtermName;
			exec(args[0],args);
			error("Exec of '%s' failed",args[0]);
		}
		else if(child < 0)
			error("Fork of '%s %s' failed","/bin/shell",vtermName);
	}
	heap_free(vtermName);

	/* loop and wait forever */
	while(1)
		waitChild(NULL);
	return EXIT_SUCCESS;
}

static char *getDrivers(void) {
	sString *s = str_create();
	sIStream *is = ifstream_open(DRIVERS_FILE,IO_READ);
	is->readAll(is,s);
	is->close(is);
	return s->str;
}
