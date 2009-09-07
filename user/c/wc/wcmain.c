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
#include <esc/fileio.h>
#include <esc/cmdargs.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void usage(char *name) {
	fprintf(stderr,"Usage: %s [-p]\n",name);
	fprintf(stderr,"	-p: Print words\n");
	exit(EXIT_FAILURE);
}

int main(int argc,char **argv) {
	s32 ch;
	u32 count,bufSize,bufPos;
	char *buffer = NULL;
	bool print = false;

	if(isHelpCmd(argc,argv))
		usage(argv[0]);
	if(argc > 1) {
		if(strcmp(argv[1],"-p") == 0)
			print = true;
		else
			usage(argv[0]);
	}

	count = 0;
	bufPos = 0;
	if(print) {
		bufSize = 20;
		buffer = (char*)malloc(bufSize + sizeof(char));
		if(buffer == NULL)
			error("Unable to allocate memory");
	}

	while((ch = scanc()) > 0) {
		if(isspace(ch)) {
			if(bufPos > 0) {
				if(print) {
					buffer[bufPos] = '\0';
					printf("Word %d: '%s'\n",count + 1,buffer);
				}
				bufPos = 0;
				count++;
			}
		}
		else {
			if(print) {
				if(bufPos >= bufSize) {
					bufSize *= 2;
					buffer = (char*)realloc(buffer,bufSize * sizeof(char));
					if(buffer == NULL)
						error("Unable to allocate memory");
				}
				buffer[bufPos] = ch;
			}
			bufPos++;
		}
	}

	/* last word */
	if(bufPos > 0) {
		if(print) {
			buffer[bufPos] = '\0';
			printf("Word %d: '%s'\n",count + 1,buffer);
		}
		bufPos = 0;
		count++;
	}

	if(print)
		free(buffer);
	printf("Total: %d\n",count);
	return EXIT_SUCCESS;
}
