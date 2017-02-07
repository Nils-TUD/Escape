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
#include <sys/proc.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

static const size_t MAX_LINE_LEN = 255;

static void handleFile(FILE *f,const char *delim,int first,int last);
static void printFields(char *line,const char *delim,int first,int last);
static void parseFields(const char *fields,int *first,int *last);

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-f <fields>] [-d <delim>] [<file>...]\n",name);
	fprintf(stderr,"    -f: <fields> may be:\n");
	fprintf(stderr,"        N        N'th field, counted from 1\n");
	fprintf(stderr,"        N-       from N'th field, to end of line\n");
	fprintf(stderr,"        N-M      from N'th to M'th (included) field\n");
	fprintf(stderr,"        -M       from first to M'th (included) field\n");
	fprintf(stderr,"    -d: use <delim> as delimiter instead of TAB\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	const char *fields = NULL;
	const char *delim = "\t";

	int opt;
	while((opt = getopt(argc,argv,"f:d:")) != -1) {
		switch(opt) {
			case 'f': fields = optarg; break;
			case 'd': delim = optarg; break;
			default:
				usage(argv[0]);
		}
	}

	int first = 0,last = -1;
	if(fields)
		parseFields(fields,&first,&last);

	if(optind >= argc)
		handleFile(stdin,delim,first,last);
	else {
		for(int i = optind; i < argc; ++i) {
			FILE *f;
			int fd = open(argv[i],O_RDONLY);
			if(fd < 0) {
				printe("Unable to open '%s'",argv[i]);
				continue;
			}
			if(fisdir(fd)) {
				printe("'%s' is a directory!",argv[i]);
				goto errFd;
			}
			f = fattach(fd,"r");
			if(!f) {
				printe("Unable to attach FILE to fd for %s",argv[i]);
				goto errFd;
			}
			handleFile(f,delim,first,last);

			fclose(f);
		errFd:
			close(fd);
		}
	}
	return EXIT_SUCCESS;
}

static void handleFile(FILE *f,const char *delim,int first,int last) {
	char line[MAX_LINE_LEN];
	while(fgetl(line,sizeof(line),f)) {
		printFields(line,delim,first,last);
		if(ferror(stdout)) {
			printe("Write failed");
			break;
		}
	}
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
