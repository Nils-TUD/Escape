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
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	WC_BYTES	= 0x1,
	WC_WORDS	= 0x2,
	WC_LINES	= 0x4,
};

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

int main(int argc,char *argv[]) {
	uint flags = 0;

	int opt;
	while((opt = getopt(argc,argv,"clw")) != -1) {
		switch(opt) {
			case 'c': flags |= WC_BYTES; break;
			case 'l': flags |= WC_LINES; break;
			case 'w': flags |= WC_WORDS; break;
			default:
				usage(argv[0]);
		}
	}

	if(!flags)
		flags = WC_BYTES | WC_WORDS | WC_LINES;

	if(optind >= argc)
		countFile(stdin);
	else {
		for(int i = optind; i < argc; ++i) {
			if(isdir(argv[i]))
				printe("'%s' is a directory!",argv[i]);
			else {
				FILE *f = fopen(argv[i],"r");
				if(!f)
					printe("Unable to open '%s'",argv[i]);
				else {
					countFile(f);
					fclose(f);
				}
			}
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
