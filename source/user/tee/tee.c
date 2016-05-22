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
#include <sys/cmdargs.h>
#include <stdio.h>
#include <stdlib.h>

static char buffer[512];

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [<FILE>...]\n",name);
	fprintf(stderr,"  Copies stdin to stdout and every given file.\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	if(isHelpCmd(argc,argv))
		usage(argv[0]);

	FILE *files[argc];
	files[0] = stdout;
	for(int i = 1; i < argc; ++i) {
		files[i] = fopen(argv[i],"w");
		if(files[i] == NULL)
			error("Unable to open '%s'",argv[i]);
	}

	size_t count;
	while((count = fread(buffer,1,sizeof(buffer),stdin)) > 0) {
		for(int i = 0; i < argc; ++i) {
			if(fwrite(buffer,1,count,files[i]) != count)
				error("Write to '%s' failed",i == 0 ? "<stdout>" : argv[i]);
		}
	}

	for(int i = 1; i < argc; ++i)
		fclose(files[i]);
	return EXIT_SUCCESS;
}
