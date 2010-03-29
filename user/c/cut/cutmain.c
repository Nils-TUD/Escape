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
#include <esc/fileio.h>
#include <esc/dir.h>
#include <esc/cmdargs.h>
#include <esc/proc.h>
#include <string.h>
#include <ctype.h>
#define MAX_LINE_LEN	255

static void printFields(char *line,const char *delim,s32 first,s32 last);
static void parseFields(const char *fields,s32 *first,s32 *last);

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s -f <fields> [-d <delim>] [<file>]\n",name);
	fprintf(stderr,"	-f: <fields> may be:\n");
	fprintf(stderr,"		N		N'th field, counted from 1\n");
	fprintf(stderr,"		N-		from N'th field, to end of line\n");
	fprintf(stderr,"		N-M		from N'th to M'th (included) field\n");
	fprintf(stderr,"		-M		from first to M'th (included) field\n");
	fprintf(stderr,"	-d: use <delim> as delimiter instead of TAB\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	char line[MAX_LINE_LEN];
	s32 i;
	s32 first = 1,last = -1;
	char *delim = (char*)"\t";
	char *filename = NULL;
	tFile *f = stdin;

	if(argc < 3 || isHelpCmd(argc,argv))
		usage(argv[0]);

	for(i = 1; i < argc; i++) {
		if(argv[i][0] == '-') {
			switch(argv[i][1]) {
				case 'f':
					if(i >= argc - 1)
						usage(argv[0]);
					parseFields(argv[i + 1],&first,&last);
					i++;
					break;
				case 'd':
					if(i >= argc - 1)
						usage(argv[0]);
					delim = argv[i + 1];
					i++;
					break;
			}
		}
		else
			filename = argv[i];
	}

	if(filename != NULL) {
		char apath[MAX_PATH_LEN];
		abspath(apath,sizeof(apath),filename);
		f = fopen(apath,"r");
		if(!f)
			error("Unable to open '%s'",apath);
	}

	while(fscanl(f,line,sizeof(line)) > 0) {
		printFields(line,delim,first,last);
	}

	if(filename != NULL)
		fclose(f);
	return EXIT_SUCCESS;
}

static void printFields(char *line,const char *delim,s32 first,s32 last) {
	if(first == 0 && last == -1)
		printf("%s\n",line);
	else {
		s32 i = 1;
		char *tok = strtok(line,delim);
		while(tok != NULL) {
			if(i >= first && (last == -1 || i <= last))
				printf("%s ",tok);
			tok = strtok(NULL,delim);
			i++;
		}
		printf("\n");
	}
}

static void parseFields(const char *fields,s32 *first,s32 *last) {
	if(*fields == '-')
		*first = 1;
	else {
		sscanf(fields,"%d",first);
		while(isdigit(*fields))
			fields++;
	}
	if(*fields == '-') {
		fields++;
		if(*fields != '\0')
			sscanf(fields,"%d",last);
		else
			*last = -1;
	}
	else
		*last = *first;
}
