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

#include <esc/common.h>
#include <esc/proc.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/cmdargs.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define MAX_LINE_LEN	255

static void printFields(char *line,const char *delim,int first,int last);
static void parseFields(const char *fields,int *first,int *last);

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

int main(int argc,const char **argv) {
	char line[MAX_LINE_LEN];
	int first = 1,last = -1;
	char *fields = NULL;
	char *delim = (char*)"\t";
	const char **args;

	int res = ca_parse(argc,argv,0,"f=s* d=s",&fields,&delim);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	parseFields(fields,&first,&last);

	args = ca_getFree();
	if(args[0] == NULL) {
		while(fgetl(line,sizeof(line),stdin))
			printFields(line,delim,first,last);
	}
	else {
		FILE *f;
		while(*args) {
			if(isdir(*args)) {
				printe("'%s' is a directory!",*args);
				continue;
			}
			f = fopen(*args,"r");
			if(!f) {
				printe("Unable to open '%s'",*args);
				continue;
			}
			while(fgetl(line,sizeof(line),f))
				printFields(line,delim,first,last);
			fclose(f);
			args++;
		}
	}
	return EXIT_SUCCESS;
}

static void printFields(char *line,const char *delim,int first,int last) {
	if(first == 0 && last == -1)
		puts(line);
	else {
		int i = 1;
		char *tok = strtok(line,delim);
		while(tok != NULL) {
			if(i >= first && (last == -1 || i <= last))
				printf("%s ",tok);
			tok = strtok(NULL,delim);
			i++;
		}
		putchar('\n');
	}
}

static void parseFields(const char *fields,int *first,int *last) {
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
