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

#include <sys/common.h>
#include <sys/io.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <dirent.h>
#include <getopt.h>
#include <stdlib.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [--ms <ms>] [-p <perms>] <fsdev> <path>\n",name);
	fprintf(stderr,"    Opens <fsdev> and makes it appear at <path>.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"    -p <perms>: set the permissions to <perms>, which is a combination\n");
	fprintf(stderr,"                of the letters r, w and x (rwx by default).\n");
	fprintf(stderr,"    --ms <ms>:  By default, the current mountspace (/sys/pid/self/ms)\n");
	fprintf(stderr,"                will be used. This can be overwritten by specifying\n");
	fprintf(stderr,"                --ms <ms>.\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	const char *mspath = "/sys/pid/self/ms";
	const char *perms = "rwx";

	int opt;
	const struct option longopts[] = {
		{"ms",		required_argument,	0,	'm'},
		{0, 0, 0, 0},
	};
	while((opt = getopt_long(argc,argv,"p:",longopts,NULL)) != -1) {
		switch(opt) {
			case 'm': mspath = optarg; break;
			case 'p': perms = optarg; break;
			default:
				usage(argv[0]);
		}
	}
	if(optind + 2 > argc)
		usage(argv[0]);

	const char *fsdev = argv[optind];
	const char *path = argv[optind + 1];

	uint flags = 0;
	for(size_t i = 0; perms[i]; ++i) {
		if(perms[i] == 'r')
			flags |= O_RDONLY;
		else if(perms[i] == 'w')
			flags |= O_WRONLY;
		else if(perms[i] == 'x')
			flags |= O_EXEC;
	}

	int fd = open(fsdev,flags);
	if(fd < 0)
		error("Unable to open '%s'",fsdev);

	int ms = open(mspath,O_WRITE);
	if(ms < 0)
		error("Unable to open '%s' for writing",mspath);
	if(mount(ms,fd,path) < 0)
		error("Unable to mount '%s' @ '%s' in mountspace '%s'",fsdev,path,mspath);

	close(ms);
	close(fd);
	return EXIT_SUCCESS;
}
