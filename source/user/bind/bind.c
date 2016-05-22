/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/cmdargs.h>
#include <sys/common.h>
#include <sys/io.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [--ms <ms>] <fs-device> <path>\n",name);
	fprintf(stderr,"    Opens <fs-device> and makes it appear at <path>.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"    By default, the current mountspace (/sys/proc/self/ms) will\n");
	fprintf(stderr,"    be used. This can be overwritten by specifying --ms <ms>.\n");
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	char *mspath = (char*)"/sys/proc/self/ms";
	char *fsdev = NULL;
	char *path = NULL;

	int res = ca_parse(argc,argv,CA_NO_FREE,"ms=s =s* =s*",&mspath,&fsdev,&path);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	int fd = open(fsdev,O_MSGS);
	if(fd < 0)
		error("Unable to open '%s'",fsdev);

	/* now mount it */
	int ms = open(mspath,O_WRITE);
	if(ms < 0)
		error("Unable to open '%s' for writing",mspath);
	if(mount(ms,fd,path) < 0)
		error("Unable to mount '%s' @ '%s' in mountspace '%s'",fsdev,path,mspath);

	close(ms);
	close(fd);
	return EXIT_SUCCESS;
}
