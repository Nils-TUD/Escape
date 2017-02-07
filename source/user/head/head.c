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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-n <lines>] [<file>]\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,char *argv[]) {
	int n = 5;

	// parse params
	int opt;
	while((opt = getopt(argc,argv,"n:")) != -1) {
		switch(opt) {
			case 'n': n = atoi(optarg); break;
			default:
				usage(argv[0]);
		}
	}

	/* open file, if any */
	FILE *in = stdin;
	if(optind < argc) {
		in = fopen(argv[optind],"r");
		if(!in)
			error("Unable to open '%s'",argv[optind]);
	}

	/* read the first n lines*/
	int c,line = 0;
	while(!ferror(stdout) && line < n && (c = fgetc(in)) != EOF) {
		putchar(c);
		if(c == '\n')
			line++;
	}
	if(ferror(in))
		error("Read failed");
	if(ferror(stdout))
		error("Write failed");

	/* clean up */
	if(optind < argc)
		fclose(in);
	return EXIT_SUCCESS;
}
