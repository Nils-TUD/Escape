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
#include <sys/proc.h>
#include <sys/width.h>
#include <sys/cmdargs.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define BUF_SIZE		8192
#define NPRINT_CHAR		'.'
#define OUT_FORMAT_OCT	'o'
#define OUT_FORMAT_DEC	'd'
#define OUT_FORMAT_HEX	'h'
#define MAX_BASE		16

static char ascii[MAX_BASE];

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-n <bytes>] [-f o|h|d] [<file>]\n",name);
	fprintf(stderr,"    -n <bytes> : Read the first <bytes> bytes\n");
	fprintf(stderr,"    -f o|h|d   : The base to print the bytes in:\n");
	fprintf(stderr,"                 o = octal\n");
	fprintf(stderr,"                 h = hexadecimal\n");
	fprintf(stderr,"                 d = decimal\n");
	exit(EXIT_FAILURE);
}

static void printAscii(uint base,int pos) {
	uint j;
	if(pos > 0) {
		while(pos % base != 0) {
			ascii[pos % base] = ' ';
			printf("%*s ",getuwidth(0xFF,base)," ");
			pos++;
		}
		putchar('|');
		for(j = 0; j < base; j++)
			putchar(ascii[j]);
		putchar('|');
	}
	putchar('\n');
}

int main(int argc,const char *argv[]) {
	FILE *in = stdin;
	uint base = 16;
	char format = OUT_FORMAT_HEX;
	int i,count = -1;
	const char **args;

	int res = ca_parse(argc,argv,CA_MAX1_FREE,"n=d f=c",&count,&format);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	/* TODO perhaps cmdargs should provide a possibility to restrict the values of an option */
	/* like 'arg=[ohd]' */
	if(format != OUT_FORMAT_DEC && format != OUT_FORMAT_HEX &&
			format != OUT_FORMAT_OCT)
		usage(argv[0]);

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

	args = ca_getFree();
	if(args[0]) {
		in = fopen(args[0],"r");
		if(!in)
			error("Unable to open '%s'",args[0]);
	}

	ulong shname;
	uchar *shmem;
	if(sharebuf(fileno(in),BUF_SIZE,(void**)&shmem,&shname,0) < 0) {
		if(shmem == NULL)
			error("Unable to mmap buffer");
	}

	i = 0;
	while(!ferror(stdout) && (count < 0 || count > 0)) {
		size_t x,c;
		c = count >= 0 ? MIN(count,BUF_SIZE) : BUF_SIZE;
		c = fread(shmem,sizeof(char),c,in);
		if(c == 0)
			break;

		for(x = 0; x < c; x++, i++) {
			if(i % base == 0) {
				if(i > 0)
					printAscii(base,i);
				printf("%08x: ",i);
			}

			if(isprint(shmem[x]) && shmem[x] < 0x80 && !isspace(shmem[x]))
				ascii[i % base] = shmem[x];
			else
				ascii[i % base] = NPRINT_CHAR;
			switch(format) {
				case OUT_FORMAT_DEC:
					printf("%03d ",shmem[x]);
					break;
				case OUT_FORMAT_HEX:
					printf("%02x ",shmem[x]);
					break;
				case OUT_FORMAT_OCT:
					printf("%03o ",shmem[x]);
					break;
			}
		}

		if(count >= 0)
			count -= c;
	}
	if(ferror(in))
		error("Read failed");
	if(ferror(stdout))
		error("Write failed");

	printAscii(base,i);

	destroybuf(shmem,shname);
	if(args[0])
		fclose(in);
	return EXIT_SUCCESS;
}
