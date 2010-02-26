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
#include <esc/fileio.h>
#include <esc/dir.h>
#include <esc/heap.h>
#include <esc/cmdargs.h>
#include <esc/signals.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static void usage(char *name) {
	fprintf(stderr,"Usage: %s [if=<file>] [of=<file>] [bs=N] [count=N]\n",name);
	fprintf(stderr,"	You can use the suffixes K, M and G to specify N\n");
	exit(EXIT_FAILURE);
}

static u32 scanNumber(const char *str);
static void interrupted(tSig sig,u32 data);

static bool run = true;

int main(int argc,char *argv[]) {
	s32 i,res;
	u32 bs = 4096;
	u32 count = 0;
	u32 total = 0;
	u32 limit;
	u8 *buffer;
	char *inFile = NULL;
	char *outFile = NULL;
	tFD in = STDIN_FILENO;
	tFD out = STDOUT_FILENO;
	if(isHelpCmd(argc,argv))
		usage(argv[0]);

	if(setSigHandler(SIG_INTRPT,interrupted) < 0)
		error("Unable to set sig-handler for SIG_INTRPT");

	for(i = 1; i < argc; i++) {
		if(strncmp(argv[i],"if=",3) == 0)
			inFile = argv[i] + 3;
		else if(strncmp(argv[i],"of=",3) == 0)
			outFile = argv[i] + 3;
		else if(strncmp(argv[i],"bs=",3) == 0)
			bs = scanNumber(argv[i] + 3);
		else if(strncmp(argv[i],"count=",6) == 0)
			count = scanNumber(argv[i] + 6);
		else
			usage(argv[0]);
	}

	if(inFile) {
		char apath[MAX_PATH_LEN];
		abspath(apath,sizeof(apath),inFile);
		in = open(apath,IO_READ);
		if(in < 0)
			error("Unable to open '%s' for reading",apath);
	}
	if(outFile) {
		char apath[MAX_PATH_LEN];
		abspath(apath,sizeof(apath),outFile);
		out = open(apath,IO_WRITE | IO_TRUNCATE | IO_CREATE);
		if(out < 0)
			error("Unable to open '%s' for writing",apath);
	}

	buffer = malloc(bs);
	if(!buffer)
		error("Unable to alloc %u bytes of mem",bs);

	limit = count * bs;
	while(run && (!count || total < limit)) {
		if((res = read(in,buffer,bs)) <= 0)
			break;
		if(write(out,buffer,res) < res)
			error("Unable to write");
		total += res;
	}

	printf("Wrote %u bytes in %.3f packages, each %u bytes long\n",total,total / (float)bs,bs);

	free(buffer);
	if(inFile)
		close(in);
	if(outFile)
		close(out);
	unsetSigHandler(SIG_INTRPT);
	return EXIT_SUCCESS;
}

static u32 scanNumber(const char *str) {
	u32 val = 0;
	while(isdigit(*str))
		val = val * 10 + (*str++ - '0');
	switch(*str) {
		case 'K':
		case 'k':
			val *= K;
			break;
		case 'M':
		case 'm':
			val *= M;
			break;
		case 'G':
		case 'g':
			val *= G;
			break;
	}
	return val;
}

static void interrupted(tSig sig,u32 data) {
	UNUSED(sig);
	UNUSED(data);
	run = false;
}
