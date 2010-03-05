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
#include <esc/heap.h>
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/fileio.h>
#include <esc/cmdargs.h>
#include <esc/proc.h>
#include <string.h>
#include <ctype.h>

#define WC_BYTES	1
#define WC_WORDS	2
#define WC_LINES	4

static void usage(char *name) {
	fprintf(stderr,"Usage: %s [-clw] [<file>]\n",name);
	fprintf(stderr,"	-c: Print byte-count\n");
	fprintf(stderr,"	-l: Print line-count\n");
	fprintf(stderr,"	-w: Print word-count\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	tFile *file = stdin;
	bool gotFlag = false;
	u8 flags = WC_BYTES | WC_WORDS | WC_LINES;
	char *arg,*path = NULL;
	u32 lines,bytes,words,bufPos;
	s32 i,ch;

	if(isHelpCmd(argc,argv))
		usage(argv[0]);

	for(i = 1; i < argc; i++) {
		arg = argv[i];
		if(*arg == '-') {
			arg++;
			while(*arg) {
				if(!gotFlag) {
					flags = 0;
					gotFlag = true;
				}
				switch(*arg) {
					case 'c':
						flags |= WC_BYTES;
						break;
					case 'l':
						flags |= WC_LINES;
						break;
					case 'w':
						flags |= WC_WORDS;
						break;
					default:
						usage(argv[0]);
						break;
				}
				arg++;
			}
		}
		else {
			if(path)
				usage(argv[0]);
			path = argv[i];
		}
	}

	/* open file, if specified */
	if(path) {
		sFileInfo info;
		char *rpath = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
		if(rpath == NULL)
			error("Unable to allocate mem for path");

		abspath(rpath,MAX_PATH_LEN + 1,path);

		/* check if it's a directory */
		if(stat(rpath,&info) < 0)
			error("Unable to get info about '%s'",rpath);
		if(MODE_IS_DIR(info.mode))
			error("'%s' is a directory!",rpath);

		file = fopen(rpath,"r");
		if(file == NULL)
			error("Unable to open '%s'",rpath);

		free(rpath);
	}

	lines = 0;
	words = 0;
	bytes = 0;
	bufPos = 0;

	while((ch = fscanc(file)) > 0) {
		if(isspace(ch)) {
			if(bufPos > 0) {
				if(ch == '\n')
					lines++;
				words++;
				bufPos = 0;
			}
		}
		else
			bufPos++;
		bytes++;
	}

	/* last word */
	if(bufPos > 0) {
		words++;
		lines++;
	}

	if(flags == WC_BYTES)
		printf("%u\n",bytes);
	else if(flags == WC_WORDS)
		printf("%u\n",words);
	else if(flags == WC_LINES)
		printf("%u\n",lines);
	else {
		if(flags & WC_LINES)
			printf("%7u",lines);
		if(flags & WC_WORDS)
			printf("%7u",words);
		if(flags & WC_BYTES)
			printf("%7u",bytes);
		printf("\n");
	}

	if(path)
		fclose(file);
	return EXIT_SUCCESS;
}
