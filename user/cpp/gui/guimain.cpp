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
#include <esc/thread.h>
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define MAX_RETRY_COUNT	500

static void startDriver(const char *name,const char *wait);
static void quit(const char *msg,...);

int main(void) {
	// check for duplicate gui-start
	if(open("/dev/vesa",IO_READ) >= 0) {
		printe("GUI seems to be running. Stopping here");
		return EXIT_FAILURE;
	}

	// disable readline, stop reading from keyboard and stop date-refresh
	sendRecvMsgData(STDOUT_FILENO,MSG_VT_DIS_RDLINE,NULL,0);
	sendRecvMsgData(STDOUT_FILENO,MSG_VT_DIS_RDKB,NULL,0);
	sendRecvMsgData(STDOUT_FILENO,MSG_VT_DISABLE,NULL,0);

	// start gui drivers
	startDriver("vesa","/dev/vesa");
	startDriver("mouse","/dev/mouse");
	startDriver("winmanager","/dev/winmanager");

	// start gui-test-program
	if(fork() == 0) {
		exec("/bin/gtest",NULL);
		quit("Unable to start gui-test");
	}

	// wait here
	while(true)
		wait(EV_NOEVENT);
	return EXIT_SUCCESS;
}

static void startDriver(const char *name,const char *wait) {
	tFD fd;
	u32 i;
	char path[MAX_PATH_LEN + 1] = "/sbin/";
	// start
	strcat(path,name);
	if(fork() == 0) {
		exec(path,NULL);
		quit("Exec with '%s' failed",path);
	}

	// wait for it
	i = 0;
	do {
		fd = open(wait,IO_READ);
		if(fd < 0)
			sleep(20);
		i++;
	}
	while(fd < 0 && i < MAX_RETRY_COUNT);
	if(fd < 0)
		quit("Haven't found '%s' after %d retries",wait,i);
}

static void quit(const char *msg,...) {
	va_list ap;
	sendRecvMsgData(STDOUT_FILENO,MSG_VT_EN_RDLINE,NULL,0);
	sendRecvMsgData(STDOUT_FILENO,MSG_VT_EN_RDKB,NULL,0);
	sendRecvMsgData(STDOUT_FILENO,MSG_VT_ENABLE,NULL,0);
	va_start(ap,msg);
	vprinte(msg,ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}
