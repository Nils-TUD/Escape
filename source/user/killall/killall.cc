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

#include <info/process.h>
#include <sys/common.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* the signal the user can send */
static struct {
	const char *name;
	int signal;
} const signals[] = {
	{"SIGKILL",SIGKILL},
	{"SIGTERM",SIGTERM},
	{"SIGINT",SIGINT},
	{"KILL",SIGKILL},
	{"TERM",SIGTERM},
	{"INT",SIGINT},
};

static int sig = SIGTERM;
static bool verbose = false;

static void killProc(const info::process &p) {
	if(kill(p.pid(),sig) < 0)
		printe("Unable to send signal %d to process %d:%s",sig,p.pid(),p.command().c_str());
	else if(verbose)
		printf("Sent signal %d to process %d:%s\n",sig,p.pid(),p.command().c_str());
}

static int usage(const char *name) {
	fprintf(stderr,"Usage: %s [-l] [-v] [-s <signal>] <name>...\n",name);
	fprintf(stderr,"    -l: list available signals\n");
	fprintf(stderr,"    -s: the signal to send (SIGTERM by default)\n");
	fprintf(stderr,"    -v: be more verbose\n");
	return EXIT_FAILURE;
}

int main(int argc,char **argv) {
	char *ssig = NULL;
	bool list = false;
	size_t i;

	// parse params
	int opt;
	while((opt = getopt(argc,argv,"lvs:")) != -1) {
		switch(opt) {
			case 'l': list = true; break;
			case 'v': verbose = true; break;
			case 's': ssig = optarg; break;
			default:
				return usage(argv[0]);
		}
	}
	if((!list && optind >= argc))
		return usage(argv[0]);

	/* translate signal-name to signal-number */
	if(ssig) {
		for(i = 0; i < ARRAY_SIZE(signals); i++) {
			if(strcmp(ssig,signals[i].name) == 0 ||
				strcmp(ssig,signals[i].name + 3) == 0) {
				sig = signals[i].signal;
				break;
			}
		}
	}

	/* print signals */
	if(list) {
		for(i = 0; i < ARRAY_SIZE(signals); i++)
			printf("%10s - %d\n",signals[i].name,signals[i].signal);
	}
	else {
		/* kill processes */
		std::vector<info::process*> list = info::process::get_list();
		for(int i = optind; i < argc; ++i) {
			for(auto &p : list) {
				if(p->command().find(argv[i]) != std::string::npos)
					killProc(*p);
			}
		}
	}
	return EXIT_SUCCESS;
}
