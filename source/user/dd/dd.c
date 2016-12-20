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
#include <sys/proc.h>
#include <sys/time.h>
#include <dirent.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

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
	int infd = STDIN_FILENO;
	int outfd = STDOUT_FILENO;

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
		infd = open(inFile,O_RDONLY);
		if(infd < 0)
			error("Unable to open '%s'",inFile);
	}
	if(outFile) {
		outfd = open(outFile,O_WRONLY);
		if(outfd < 0)
			error("Unable to open '%s'",outFile);
	}

	uint64_t start = rdtsc(), end;
	{
		int shmfd;
		uchar *shmem;
		if((shmfd = sharebuf(infd,bs,(void**)&shmem,0)) < 0) {
			if(shmem == NULL)
				error("Unable to mmap buffer");
		}
		if(shmem) {
			if(delegate(outfd,shmfd,O_RDONLY,DEL_ARG_SHFILE) < 0) {}
		}

		ssize_t result;
		ullong limit = (ullong)count * bs;
		while(run && (!count || total < limit)) {
			if((result = read(infd,shmem,bs)) <= 0) {
				if(result < 0)
					error("Read failed");
				break;
			}

			if(write(outfd,shmem,result) < 0)
				error("Write failed");

			total += result;
		}

		destroybuf(shmem,shmfd);
	}
	end = rdtsc();

	uint64_t usecs = tsctotime(end - start);
	printf("%zu records in\n",count);
	printf("%zu records out\n",count);
	printf("%Lu bytes (%.1lf MB) copied, %.1lf s, %.1lf MB/s\n",
		total,total / 1000000.0,usecs / 1000000.0,total / (double)usecs);

	if(inFile)
		close(infd);
	if(outFile)
		close(outfd);
	return EXIT_SUCCESS;
}
