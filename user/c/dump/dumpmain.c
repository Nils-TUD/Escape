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
#include <esc/io.h>
#include <esc/dir.h>
#include <esc/fileio.h>
#include <esc/cmdargs.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <width.h>

#define BUF_SIZE 512

#define NPRINT_CHAR		'.'
#define OUT_FORMAT_OCT	'o'
#define OUT_FORMAT_DEC	'd'
#define OUT_FORMAT_HEX	'h'
#define MAX_BASE		16

static char ascii[MAX_BASE];
static u8 buffer[BUF_SIZE];

static void usage(char *name) {
	fprintf(stderr,"Usage: %s [-n <bytes>] [-f o|h|d] [<file>]\n",name);
	fprintf(stderr,"	-n <bytes>	: Read the first <bytes> bytes\n");
	fprintf(stderr,"	-f o|h|d	: The base to print the bytes in:\n");
	fprintf(stderr,"					o = octal\n");
	fprintf(stderr,"					h = hexadecimal\n");
	fprintf(stderr,"					d = decimal\n");
	exit(EXIT_FAILURE);
}

static void printAscii(u8 base,s32 pos) {
	s32 j;
	if(pos > 0) {
		while(pos % base != 0) {
			ascii[pos % base] = ' ';
			printf("%*s ",getuwidth(0xFF,base)," ");
			pos++;
		}
		printc('|');
		for(j = 0; j < base; j++)
			printc(ascii[j]);
		printc('|');
	}
	printf("\n");
}

int main(int argc,char *argv[]) {
	tFile *file;
	char *s;
	char *path = NULL;
	u8 base = 16;
	char format = OUT_FORMAT_HEX;
	s32 i,x,c,count = -1;

	if(isHelpCmd(argc,argv))
		usage(argv[0]);

	for(i = 1; i < argc; i++) {
		s = argv[i];
		if(*s == '-') {
			s++;
			while(*s) {
				switch(*s) {
					case 'n':
						if(i >= argc - 1)
							usage(argv[0]);
						count = atoi(argv[i + 1]);
						if(count <= 0)
							usage(argv[0]);
						break;
					case 'f':
						if(i >= argc - 1)
							usage(argv[0]);
						format = argv[i + 1][0];
						if(format != OUT_FORMAT_DEC && format != OUT_FORMAT_HEX &&
								format != OUT_FORMAT_OCT)
							usage(argv[0]);
						break;
					default:
						usage(argv[0]);
						break;
				}
				s++;
			}
		}
		else
			path = s;
	}

	file = stdin;
	if(path != NULL) {
		char *rpath = (char*)malloc((MAX_PATH_LEN + 1) * sizeof(char));
		if(rpath == NULL)
			error("Unable to allocate mem for path");

		abspath(rpath,MAX_PATH_LEN + 1,path);
		file = fopen(rpath,"r");
		if(file == NULL)
			error("Unable to open '%s'",rpath);

		free(rpath);
	}

	switch(format) {
		case OUT_FORMAT_DEC:
			base = 10;
			break;
		case OUT_FORMAT_HEX:
			base = 16;
			break;
		case OUT_FORMAT_OCT:
			base = 8;
			break;
	}

	i = 0;
	while(count == -1 || count > 0) {
		c = count != -1 ? MIN(count,BUF_SIZE) : BUF_SIZE;
		c = fread(buffer,sizeof(u8),c,file);
		if(c == 0) {
			if(ferror(file))
				error("Unable to read");
			break;
		}

		for(x = 0; x < c; x++, i++) {
			if(i % base == 0) {
				if(i > 0)
					printAscii(base,i);
				printf("%08x: ",i);
			}

			if(isprint(buffer[x]) && buffer[x] < 0x80 && !isspace(buffer[x]))
				ascii[i % base] = buffer[x];
			else
				ascii[i % base] = NPRINT_CHAR;
			switch(format) {
				case OUT_FORMAT_DEC:
					printf("%03d ",buffer[x]);
					break;
				case OUT_FORMAT_HEX:
					printf("%02x ",buffer[x]);
					break;
				case OUT_FORMAT_OCT:
					printf("%03o ",buffer[x]);
					break;
			}
		}

		if(count != -1)
			count -= c;
	}

	printAscii(base,i);

	if(path != NULL)
		fclose(file);

	return EXIT_SUCCESS;
}
