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
#include <esc/cmdargs.h>
#include <esc/dir.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_LINE_LEN		512

static bool matches(const char *line,const char *pattern);
static void strtolower(char *s);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s <pattern> [<file>]\n",name);
	fprintf(stderr,"	<pattern> will be treated case-insensitive and is NOT a\n");
	fprintf(stderr,"	regular expression because we have no regexp-library yet ;)\n");
	exit(EXIT_FAILURE);
}

static char buffer[MAX_LINE_LEN];
static char lbuffer[MAX_LINE_LEN];

int main(int argc,const char *argv[]) {
	FILE *in = stdin;
	char *pattern = NULL;
	const char **args;

	int res = ca_parse(argc,argv,CA_MAX1_FREE,"=s*",&pattern);
	if(res < 0) {
		fprintf(stderr,"Invalid arguments: %s\n",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	args = ca_getfree();
	if(args[0]) {
		in = fopen(args[0],"r");
		if(!in)
			error("Unable to open '%s'",args[0]);
	}

	strtolower(pattern);
	while(fgetl(buffer,MAX_LINE_LEN,in)) {
		if(matches(buffer,pattern))
			puts(buffer);
	}
	if(ferror(in))
		error("Read failed");

	if(args[0])
		fclose(in);
	return EXIT_SUCCESS;
}

static bool matches(const char *line,const char *pattern) {
	strcpy(lbuffer,line);
	strtolower(lbuffer);
	return strstr(lbuffer,pattern) != NULL;
}

static void strtolower(char *s) {
	while(*s) {
		*s = tolower(*s);
		s++;
	}
}
