/**
 * $Id: statmain.c 453 2010-02-03 13:19:34Z nasmussen $
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
#include <esc/io.h>
#include <esc/fileio.h>
#include <esc/dir.h>
#include <esc/heap.h>
#include <esc/cmdargs.h>
#include <stdlib.h>
#include <string.h>

#define ARRAY_INC		16
#define MAX_LINE_LEN	255

static int compareStrs(const void *a,const void *b);
static char *scanLine(tFile *f);

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-r] [-i] [<file>]\n",name);
	fprintf(stderr,"	-r: reverse; i.e. descending instead of ascending\n");
	fprintf(stderr,"	-i: ignore case\n");
	exit(EXIT_FAILURE);
}

static bool ignoreCase = false;
static bool reverse = false;

int main(int argc,char *argv[]) {
	char apath[MAX_PATH_LEN];
	u32 i,j,size = ARRAY_INC;
	char **lines;
	char *filename = NULL;
	tFile *f = stdin;

	if(isHelpCmd(argc,argv))
		usage(argv[0]);

	for(i = 1; (s32)i < argc; i++) {
		if(*argv[i] == '-') {
			switch(argv[i][1]) {
				case 'r':
					reverse = true;
					break;
				case 'i':
					ignoreCase = true;
					break;
				default:
					usage(argv[0]);
					break;
			}
		}
		else
			filename = argv[i];
	}

	/* a file? */
	if(filename != NULL) {
		abspath(apath,MAX_PATH_LEN,filename);
		f = fopen(apath,"r");
		if(f == NULL)
			error("Unable to open '%s'",apath);
	}

	/* read lines */
	lines = (char**)malloc(size * sizeof(char*));
	if(!lines)
		error("Unable to allocate memory");
	i = 0;
	while(!feof(f)) {
		char *line = scanLine(f);
		if(!line)
			error("Unable to allocate memory");
		lines[i++] = line;
		if(i >= size) {
			size *= 2;
			lines = (char**)realloc(lines,size * sizeof(char*));
			if(!lines)
				error("Unable to allocate memory");
		}
	}

	/* sort */
	qsort(lines,i,sizeof(char*),compareStrs);

	/* print */
	for(j = 0; j < i; j++) {
		printf("%s\n",lines[j]);
		free(lines[j]);
	}
	free(lines);
	if(filename != NULL)
		fclose(f);
	return EXIT_SUCCESS;
}

static int compareStrs(const void *a,const void *b) {
	char *s1 = *(char**)a;
	char *s2 = *(char**)b;
	s32 res;
	if(ignoreCase)
		res = strcasecmp(s1,s2);
	else
		res = strcmp(s1,s2);
	return reverse ? -res : res;
}

static char *scanLine(tFile *f) {
	u32 i = 0;
	u32 size = ARRAY_INC;
	char c;
	char *line = (char*)malloc(size);
	if(!line)
		return NULL;
	while((c = fscanc(f)) != IO_EOF && c != '\n') {
		line[i++] = c;
		if(i >= size - 1) {
			size += ARRAY_INC;
			line = (char*)realloc(line,size);
			if(!line)
				return NULL;
		}
	}
	line[i] = '\0';
	return line;
}
