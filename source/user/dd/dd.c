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
#include <sys/cmdargs.h>
#include <sys/time.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static bool run = true;

static void usage(const char *name) {
	fprintf(stderr,"Usage: %s [if=<file>] [of=<file>] [bs=N] [count=N]\n",name);
	fprintf(stderr,"    You can use the suffixes K, M and G to specify N\n");
	exit(EXIT_FAILURE);
}
static void interrupted(A_UNUSED int sig) {
	run = false;
}

int main(int argc,const char *argv[]) {
	size_t bs = 4096;
	size_t count = 0;
	ullong total = 0;
	char *inFile = NULL;
	char *outFile = NULL;
	FILE *in = stdin;
	FILE *out = stdout;

	int res = ca_parse(argc,argv,CA_NO_DASHES | CA_NO_FREE | CA_REQ_EQ,
			"if=s of=s bs=k count=k",&inFile,&outFile,&bs,&count);
	if(res < 0) {
		printe("Invalid arguments: %s",ca_error(res));
		usage(argv[0]);
	}
	if(ca_hasHelp())
		usage(argv[0]);

	if(signal(SIGINT,interrupted) == SIG_ERR)
		error("Unable to set sig-handler for SIGINT");

	if(inFile) {
		in = fopen(inFile,"r");
		if(in == NULL)
			error("Unable to open '%s'",inFile);
	}
	if(outFile) {
		out = fopen(outFile,"w");
		if(out == NULL)
			error("Unable to open '%s'",outFile);
	}

	uint64_t start = rdtsc(), end;
	{
		ulong shname;
		uchar *shmem;
		if(sharebuf(fileno(in),bs,(void**)&shmem,&shname,0) < 0) {
			if(shmem == NULL)
				error("Unable to mmap buffer");
		}

		size_t result;
		ullong limit = (ullong)count * bs;
		while(run && (!count || total < limit)) {
			if((result = fread(shmem,1,bs,in)) == 0)
				break;
			if(fwrite(shmem,1,bs,out) == 0)
				break;
			total += result;
		}

		if(ferror(in))
			error("Read failed");
		if(ferror(out))
			error("Write failed");
		destroybuf(shmem,shname);
	}
	end = rdtsc();

	uint64_t usecs = tsctotime(end - start);
	printf("%zu records in\n",count);
	printf("%zu records out\n",count);
	printf("%Lu bytes (%.1lf MB) copied, %.1lf s, %.1lf MB/s\n",
		total,total / 1000000.0,usecs / 1000000.0,total / (double)usecs);

	if(inFile)
		fclose(in);
	if(outFile)
		fclose(out);
	return EXIT_SUCCESS;
}
