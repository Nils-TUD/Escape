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
#include <stdio.h>
#include <esc/proc.h>
#include <messages.h>
#include <string.h>

#define POWER_DRV		"/dev/powermng"

static sMsg msg;

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s -r|-s\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	tFD fd;
	if(argc < 2 || isHelpCmd(argc,argv))
		usage(argv[0]);

	fd = open(POWER_DRV,IO_READ | IO_WRITE);
	if(fd < 0)
		error("Unable to open '%s'",POWER_DRV);

	if(strcmp(argv[1],"-r") == 0)
		send(fd,MSG_POWER_REBOOT,&msg,sizeof(msg.args));
	else if(strcmp(argv[1],"-s") == 0)
		send(fd,MSG_POWER_SHUTDOWN,&msg,sizeof(msg.args));
	else
		usage(argv[0]);

	close(fd);
	return EXIT_SUCCESS;
}
