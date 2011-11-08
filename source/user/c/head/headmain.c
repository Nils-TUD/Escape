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
#include <esc/cmdargs.h>
#include <esc/io.h>
#include <stdlib.h>
#include <stdio.h>

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-n <lines>] [<file>]\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	const char **args;
	FILE *in = stdin;
	int c,line,n = 5;

	/* parse args */
	int res = ca_parse(argc,argv,CA_MAX1_FREE,"n=d",&n);
	if(res < 0) {
		fprintf(stderr,"Invalid arguments: %s\n",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	/* open file, if any */
	args = ca_getFree();
	if(args[0]) {
		in = fopen(args[0],"r");
		if(!in)
			error("Unable to open '%s'",args[0]);
	}

	/* read the first n lines*/
	line = 0;
	while(line < n && (c = fgetc(in)) != EOF) {
		putchar(c);
		if(c == '\n')
			line++;
	}
	if(ferror(in))
		error("Read failed");

	/* clean up */
	if(args[0])
		fclose(in);
	return EXIT_SUCCESS;
}
