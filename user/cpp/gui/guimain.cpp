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
#include <esc/fileio.h>
#include <esc/dir.h>
#include <string.h>
#include <messages.h>

static void startDriver(const char *name,const char *wait);

int main(void) {
	// check for duplicate gui-start
	if(open("/dev/vesa",IO_READ) >= 0) {
		printe("GUI seems to be running. Stopping here");
		return EXIT_FAILURE;
	}

	// disable readline, stop reading from keyboard and stop date-refresh
	send(STDOUT_FILENO,MSG_VT_DIS_RDLINE,NULL,0);
	send(STDOUT_FILENO,MSG_VT_DIS_RDKB,NULL,0);
	send(STDOUT_FILENO,MSG_VT_DIS_DATE,NULL,0);

	// start gui drivers
	startDriver("vesa","/dev/vesa");
	startDriver("mouse","/dev/mouse");
	startDriver("winmanager","/dev/winmanager");

	// start gui-test-program
	if(fork() == 0) {
		exec("/bin/gtest",NULL);
		printe("Unable to start gui-test");
		exit(EXIT_FAILURE);
	}

	// wait here
	while(true)
		wait(EV_NOEVENT);
	return EXIT_SUCCESS;
}

static void startDriver(const char *name,const char *wait) {
	char path[MAX_PATH_LEN + 1] = "/sbin/";
	strcat(path,name);
	if(fork() == 0) {
		exec(path,NULL);
		printe("Exec with '%s' failed",path);
		exit(EXIT_FAILURE);
	}

	tFD fd;
	do {
		fd = open(wait,IO_READ);
		if(fd < 0)
			yield();
	}
	while(fd < 0);
}
