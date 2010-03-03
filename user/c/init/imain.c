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
#include <esc/proc.h>
#include <esc/heap.h>
#include <esc/debug.h>
#include <stdlib.h>
#include <sllist.h>
#include <string.h>
#include <width.h>
#include "parser.h"
#include "service.h"

/**
 * Loads the service-dependency-file
 * @return the file-content
 */
static char *getServices(void);

int main(void) {
	tFD fd;
	s32 child;
	u32 i,retries = 0;
	u32 vtLen;
	char *vtermName;
	char *servDefs;
	sServiceLoad **services;

	if(getpid() != 0)
		error("It's not good to start init twice ;)\n");

	/* wait for fs; we need it for exec */
	do {
		fd = open("/services/fs",IO_READ | IO_WRITE);
		if(fd < 0)
			yield();
		retries++;
	}
	while(fd < 0 && retries < MAX_WAIT_RETRIES);
	if(fd < 0)
		error("Unable to open /services/fs after %d retries",retries);
	close(fd);

	/* now read the services we should load */
	servDefs = getServices();
	if(servDefs == NULL)
		error("Unable to read service-file");

	/* parse them */
	services = parseServices(servDefs);
	if(services == NULL)
		error("Unable to parse service-file");

	/* finally load them */
	if(!loadServices(services))
		error("Unable to load services");

	/* remove stdin, stdout and stderr. the shell wants to provide them */
	close(STDERR_FILENO);
	close(STDOUT_FILENO);
	close(STDIN_FILENO);

	/* now load the shells */
	vtLen = SSTRLEN("vterm") + getnwidth(VTERM_COUNT) + 1;
	vtermName = (char*)malloc(vtLen);
	if(vtermName == NULL)
		error("Unable to allocate mem for vterm-name");

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
	free(vtermName);

	/* loop and wait forever */
	while(1)
		waitChild(NULL);
	return EXIT_SUCCESS;
}

static char *getServices(void) {
	const u32 stepSize = 128 * sizeof(u8);
	tFD fd;
	u32 c,pos = 0,bufSize = stepSize;
	char *buffer;

	/* open file */
	fd = open("/etc/services",IO_READ);
	if(fd < 0)
		return NULL;

	/* create buffer */
	buffer = (char*)malloc(stepSize);
	if(buffer == NULL) {
		close(fd);
		return NULL;
	}

	/* read file */
	while((c = read(fd,buffer + pos,stepSize - 1)) > 0) {
		bufSize += stepSize;
		buffer = (char*)realloc(buffer,bufSize);
		if(buffer == NULL) {
			close(fd);
			return NULL;
		}

		pos += c;
	}

	/* terminate */
	*(buffer + pos) = '\0';

	close(fd);
	return buffer;
}
