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
#include <stdio.h>
#include <stdlib.h>

#define LOG_PATH		"/system/log"
#define DESKTOP_PROG	"desktop"

int main(void) {
	int fd;

	/* open stdin */
	if((fd = open(LOG_PATH,IO_READ)) != STDIN_FILENO)
		error("Unable to open '%s' for STDIN: Got fd %d",LOG_PATH,fd);

	/* open stdout */
	if((fd = open(LOG_PATH,IO_WRITE)) != STDOUT_FILENO)
		error("Unable to open '%s' for STDOUT: Got fd %d",LOG_PATH,fd);

	/* dup stdout to stderr */
	if((fd = dup(fd)) != STDERR_FILENO)
		error("Unable to duplicate STDOUT to STDERR: Got fd %d",fd);

	/* TODO later we should provide a login screen here */
	const char *args[] = {DESKTOP_PROG,NULL};
	execp(DESKTOP_PROG,args);
	return EXIT_FAILURE;
}
