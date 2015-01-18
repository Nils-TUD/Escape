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

#include <sys/cmdargs.h>
#include <sys/common.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WC_BYTES	1
#define WC_WORDS	2
#define WC_LINES	4

static void countFile(FILE *in);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-clw] [<file>...]\n",name);
	fprintf(stderr,"    -c: Print byte-count\n");
	fprintf(stderr,"    -l: Print line-count\n");
	fprintf(stderr,"    -w: Print word-count\n");
	exit(EXIT_FAILURE);
}

static ulong lines = 0;
static ulong bytes = 0;
static ulong words = 0;

int main(int argc,const char *argv[]) {
	uint flags = 0;
	int flines = 0,fwords = 0,fbytes = 0;
	const char **args;

	int res = ca_parse(argc,argv,0,"w c l",&fwords,&fbytes,&flines);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);
	if(flines == false && fwords == false && fbytes == false)
		flags = WC_BYTES | WC_WORDS | WC_LINES;
	else
		flags = (flines ? WC_LINES : 0) | (fwords ? WC_WORDS : 0) | (fbytes ? WC_BYTES : 0);

	args = ca_getFree();
	if(!*args)
		countFile(stdin);
	else {
		while(*args) {
			if(isdir(*args))
				printe("'%s' is a directory!",*args);
			else {
				FILE *f = fopen(*args,"r");
				if(!f)
					printe("Unable to open '%s'",*args);
				else {
					countFile(f);
					fclose(f);
				}
			}
			args++;
		}
	}

	if(flags == WC_BYTES)
		printf("%lu\n",bytes);
	else if(flags == WC_WORDS)
		printf("%lu\n",words);
	else if(flags == WC_LINES)
		printf("%lu\n",lines);
	else {
		if(flags & WC_LINES)
			printf("%7lu",lines);
		if(flags & WC_WORDS)
			printf("%7lu",words);
		if(flags & WC_BYTES)
			printf("%7lu",bytes);
		putchar('\n');
	}
	return EXIT_SUCCESS;
}

static void countFile(FILE *in) {
	uint bufPos = 0;
	char c;
	while((c = getc(in)) != EOF) {
		if(isspace(c)) {
			if(bufPos > 0) {
				if(c == '\n')
					lines++;
				words++;
				bufPos = 0;
			}
		}
		else
			bufPos++;
		bytes++;
	}
	if(ferror(in))
		printe("Read failed");

	/* last word */
	if(bufPos > 0) {
		words++;
		lines++;
	}
}
