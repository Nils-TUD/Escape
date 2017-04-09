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
#include <sys/stat.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-f] <path> ...\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	bool follow = false;

	int opt;
	while((opt = getopt(argc,argv,"f")) != -1) {
		switch(opt) {
			case 'f': follow = true; break;
			default:
				usage(argv[0]);
		}
	}
	if(optind >= argc)
		usage(argv[0]);

	for(int i = optind; i < argc; ++i) {
		char tmp[MAX_PATH_LEN];
		if(follow) {
			if(canonpath(tmp,sizeof(tmp),argv[i]) < 0) {
				printe("readlink for '%s' failed",argv[i]);
				continue;
			}
			puts(tmp);
		}
		else {
			if(!islink(argv[i]))
				printe("'%s' is no symbolic link",argv[i]);
			else if(readlink(argv[i],tmp,sizeof(tmp)) < 0)
				printe("readlink for '%s' failed",argv[i]);
			else
				puts(tmp);
		}
	}
	return EXIT_SUCCESS;
}
