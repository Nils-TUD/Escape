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
#include <esc/io/console.h>
#include <esc/exceptions/cmdargs.h>
#include <esc/util/cmdargs.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#define SIG_NAME_LEN		7

typedef struct {
	char name[SIG_NAME_LEN + 1];
	tSig signal;
} sSigName;

/* the signal the user can send */
static sSigName signals[] = {
	{"SIGKILL",SIG_KILL},
	{"SIGTERM",SIG_TERM},
	{"SIGINT",SIG_INTRPT}
};

static void usage(const char *name) {
	cerr->writef(cerr,"Usage: %s [-L] [-s <signal>] <pid>...\n",name);
	cerr->writef(cerr,"	<signal> may be: SIGKILL, SIGTERM, SIGINT, KILL, TERM, INT\n");
	exit(EXIT_FAILURE);
}

int main(int argc,const char *argv[]) {
	tSig sig = SIG_KILL;
	char *ssig = NULL;
	bool list = false;
	sCmdArgs *args;
	u32 i;

	TRY {
		args = cmdargs_create(argc,argv,0);
		args->parse(args,"L s=s",&list,&ssig);
		if(args->isHelp)
			usage(argv[0]);
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
	}
	CATCH(CmdArgsException,e) {
		cerr->writef(cerr,"Invalid arguments: %s\n",e->toString(e));
		usage(argv[0]);
	}
	ENDCATCH

	/* print signals */
	if(list) {
		for(i = 0; i < ARRAY_SIZE(signals); i++)
			cout->writef(cout,"%10s - %d\n",signals[i].name,signals[i].signal);
	}
	else {
		/* kill processes */
		sIterator it = args->getFreeArgs(args);
		while(it.hasNext(&it)) {
			const char *spid = (const char*)it.next(&it);
			tPid pid = atoi(spid);
			if(pid > 0) {
				if(sendSignalTo(pid,sig,0) < 0)
					cerr->writef(cerr,"Unable to send signal %d to %d",sig,pid);
			}
			else if(strcmp(spid,"0") != 0)
				cerr->writef(cerr,"Unable to kill process with pid '%s'\n",spid);
			else
				cerr->writef(cerr,"You can't kill 'init'\n");
		}
	}
	return EXIT_SUCCESS;
}
