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
#include <esc/dir.h>
#include <esc/fileio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LEN		512

static char buffer[MAX_LINE_LEN + 1];

static bool matches(char *line,char *pattern);
static void strtolower(char *s);

int main(int argc,char *argv[]) {
	tFile *file;
	char *pattern;
	u32 count;

	if(argc <= 1 || argc > 3 || isHelpCmd(argc,argv)) {
		fprintf(stderr,"Usage: %s <pattern> [<file>]\n",argv[0]);
		fprintf(stderr,"	<pattern> will be treated case-insensitive and is NOT a\n");
		fprintf(stderr,"	regular expression because we have no regexp-library yet ;)\n");
		return EXIT_FAILURE;
	}

	pattern = argv[1];
	file = stdin;
	if(argc > 2) {
		char *rpath = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
		if(rpath == NULL)
			error("Unable to allocate mem for path");

		abspath(rpath,MAX_PATH_LEN + 1,argv[2]);
		file = fopen(rpath,"r");
		if(file == NULL)
			error("Unable to open '%s'",rpath);

		free(rpath);
	}

	strtolower(pattern);
	while((count = fscanl(file,buffer,MAX_LINE_LEN)) > 0) {
		*(buffer + count) = '\0';
		if(matches(buffer,pattern))
			printf("%s\n",buffer);
	}

	if(argc == 3)
		fclose(file);

	return EXIT_SUCCESS;
}

static bool matches(char *line,char *pattern) {
	strtolower(line);
	return strstr(line,pattern) != NULL;
}

static void strtolower(char *s) {
	while(*s) {
		if(isupper(*s))
			*s = tolower(*s);
		s++;
	}
}
