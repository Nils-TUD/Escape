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
#include <ctype.h>
#include <dirent.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE		8192
#define NPRINT_CHAR		'.'
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

int main(int argc,char *argv[]) {
	uint base = 16;
	int count = -1;

	// parse params
	int opt;
	while((opt = getopt(argc,argv,"n:f:")) != -1) {
		switch(opt) {
			case 'n': count = atoi(optarg); break;
			case 'f':
				if(optarg[0] == 'd')
					base = 10;
				else if(optarg[0] == 'x')
					base = 16;
				else if(optarg[0] == 'o')
					base = 8;
				else
					usage(argv[0]);
				break;
			default:
				usage(argv[0]);
		}
	}

	FILE *in = stdin;
	if(optind < argc) {
		in = fopen(argv[optind],"r");
		if(!in)
			error("Unable to open '%s'",argv[optind]);
	}

	int shmfd;
	uchar *shmem;
	if((shmfd = sharebuf(fileno(in),BUF_SIZE,(void**)&shmem,0)) < 0) {
		if(shmem == NULL)
			error("Unable to mmap buffer");
	}

	int i = 0;
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
			switch(base) {
				case 10:
					printf("%03d ",shmem[x]);
					break;
				case 16:
					printf("%02x ",shmem[x]);
					break;
				case 8:
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

	destroybuf(shmem,shmfd);
	if(optind < argc)
		fclose(in);
	return EXIT_SUCCESS;
}
