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
#include <esc/cmdargs.h>
#include <esc/io.h>
#include <stdlib.h>
#include <stdio.h>

#define BUF_SIZE		4096

typedef struct sReadBuf {
	struct sReadBuf *next;
	size_t bytes;
	size_t lines;
	char buffer[BUF_SIZE];
} sReadBuf;

static size_t countLines(const char *b,size_t count);
static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [-n <lines>] [<file>]\n",name);
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	const char **args;
	FILE *in = stdin;
	size_t i,lines,n = 5;
	int c;
	fpos_t pos;
	sReadBuf *first,*buf,*prev;

	/* parse args */
	int res = ca_parse(argc,argv,CA_MAX1_FREE,"n=d",&n);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	/* open file, if any */
	args = ca_getFree();
	if(args[0]) {
		in = fopen(args[0],"r");
		if(!in)
			error("Unable to open '%s'",args[0]);
	}

	/* check whether we can seek */
	if(fseek(in,0,SEEK_END) < 0) {
		/* remove error that fseek() has set */
		clearerr(in);

		/* no, ok, then the idea is the following:
		 * we don't need to keep all the memory and do a lot of malloc and realloc. it is sufficient
		 * to read the stuff block by block and keep only the last few blocks that contain at least
		 * <n> lines. therefore, we introduce a single-linked-list and reuse the first one if all
		 * others still contain at least <n> lines. this way, we save a lot of memory and time
		 * because we don't need so many heap-allocs/reallocs. */
		lines = 0;
		prev = NULL;
		first = NULL;
		do {
			if(first && (lines - first->lines) > n) {
				buf = first;
				lines -= buf->lines;
				first = first->next;
			}
			else {
				buf = (sReadBuf*)malloc(sizeof(sReadBuf));
				if(!buf)
					error("Unable to allocate more memory");
			}

			buf->bytes = fread(buf->buffer,1,BUF_SIZE,in);
			buf->lines = countLines(buf->buffer,buf->bytes);
			lines += buf->lines;
			buf->next = NULL;
			if(prev)
				prev->next = buf;
			else if(first)
				first->next = buf;
			else
				first = buf;
			prev = buf;
		}
		while(buf->bytes > 0);

		/* throw away the first blocks if they are not needed */
		while((lines - first->lines) > n) {
			lines -= first->lines;
			first = first->next;
		}

		/* print out the lines */
		buf = first;
		while(!ferror(stdout) && buf != NULL) {
			for(i = 0; i < buf->bytes; i++) {
				c = buf->buffer[i];
				if(lines <= n)
					putchar(c);
				if(c == '\n')
					lines--;
			}
			buf = buf->next;
		}
	}
	else {
		/* we can seek, therefore we can do it better. we don't need to search through the whole
		 * file, but can seek to the end and stepwise backwards until we've found enough lines. */
		if(fgetpos(in,&pos) < 0)
			error("Unable to get file-position");

		/* search backwards until we've found the number of requested lines */
		lines = 0;
		while(!ferror(in) && pos > 0 && lines < n) {
			size_t amount = MIN(BUF_SIZE,pos);
			pos -= amount;
			fseek(in,pos,SEEK_SET);
			for(i = 0; i < amount; i++) {
				c = fgetc(in);
				if(c == '\n')
					lines++;
			}
		}
		if(ferror(in))
			error("Read failed");

		/* print the lines */
		fseek(in,pos,SEEK_SET);
		while((c = fgetc(in)) != EOF) {
			if(lines <= n)
				putchar(c);
			if(c == '\n') {
				if(ferror(stdout))
					break;
				lines--;
			}
		}
	}
	if(ferror(in))
		error("Read failed");
	if(ferror(stdout))
		error("Write failed");

	/* clean up */
	if(args[0])
		fclose(in);
	return EXIT_SUCCESS;
}

static size_t countLines(const char *b,size_t count) {
	size_t i,lines = 0;
	for(i = 0; i < count; i++) {
		if(b[i] == '\n')
			lines++;
	}
	return lines;
}
