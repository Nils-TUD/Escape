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
#include <esc/messages.h>
#include <esc/cmdargs.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define POWER_DRV		"/dev/powermng"

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s -r|-s\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	bool reboot = false;
	bool shutdown = false;
	sMsg msg;
	int fd;

	int res = ca_parse(argc,argv,CA_NO_FREE,"r s",&reboot,&shutdown);
	if(res < 0 || (!reboot && !shutdown) || (reboot && shutdown)) {
		fprintf(stderr,"Invalid arguments: %s\n",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	fd = open(POWER_DRV,IO_MSGS);
	if(fd < 0)
		error("Unable to open '%s'",POWER_DRV);
	if(reboot)
		send(fd,MSG_POWER_REBOOT,&msg,sizeof(msg.args));
	else if(shutdown)
		send(fd,MSG_POWER_SHUTDOWN,&msg,sizeof(msg.args));
	close(fd);
	return EXIT_SUCCESS;
}
