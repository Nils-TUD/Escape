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
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/sync.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../modules.h"

typedef struct State {
	volatile long sysc;
	volatile size_t arg1;
	volatile size_t arg2;
	volatile size_t arg3;
	volatile size_t arg4;
} State;

static State *state;

static ulong vals[] = {
	0,
	1,
	(ulong)-1,
	0xDEADBEEF,
};

static bool is_blacklisted(long sysc) {
	static long blacklist[] = {
		/* these syscalls are complicated to handle with this scheme */
		SYSCALL_FORK,
		SYSCALL_STARTTHREAD,
		SYSCALL_EXEC,
		SYSCALL_ACKSIG,
		/* we don't want to exit, sleep or block */
		SYSCALL_EXIT,
		SYSCALL_SLEEP,
		SYSCALL_RECEIVE,
		SYSCALL_SENDRECV,
		/* we don't want to set an fd to unblocking or so */
		SYSCALL_FCNTL,
#if defined(__eco32__) || defined(__mmix__)
		/* don't enter kernel debugger */
		SYSCALL_DEBUG,
#endif
	};
	for(size_t i = 0; i < ARRAY_SIZE(blacklist); ++i) {
		if(blacklist[i] == sysc)
			return true;
	}
	return false;
}

static void check_syscall(void) {
	if(is_blacklisted(state->sysc))
		return;

	for(; state->arg1 < ARRAY_SIZE(vals); ++state->arg1) {
		for(; state->arg2 < ARRAY_SIZE(vals); ++state->arg2) {
			for(; state->arg3 < ARRAY_SIZE(vals); ++state->arg3) {
				for(; state->arg4 < ARRAY_SIZE(vals); ++state->arg4) {
					long res = syscall4(
						state->sysc,
						vals[state->arg1],
						vals[state->arg2],
						vals[state->arg3],
						vals[state->arg4]
					);

					print("syscall(%ld,%#lx,%#lx,%#lx,%#lx) -> %d",
						state->sysc,
						vals[state->arg1],vals[state->arg2],
						vals[state->arg3],vals[state->arg4],
						res);
				}
				state->arg4 = 0;
			}
			state->arg3 = 0;
		}
		state->arg2 = 0;
	}
	state->arg1 = 0;
}

static void check_syscalls(void) {
	for(; state->sysc < SYSCALL_COUNT; ++state->sysc)
		check_syscall();
}

int mod_syscalls(A_UNUSED int argc,A_UNUSED char *argv[]) {
	FILE *tmp = tmpfile();
	ftruncate(fileno(tmp),sizeof(State));
	state = (State*)mmap(NULL,sizeof(State),sizeof(State),PROT_READ | PROT_WRITE,MAP_SHARED,fileno(tmp),0);
	if(state == NULL)
		error("Unable to mmap state");

	memclear(state,sizeof(*state));

	while(state->sysc < SYSCALL_COUNT) {
		int pid = fork();
		if(pid < 0)
			error("fork failed");
		if(pid == 0) {
			check_syscalls();
			exit(0);
		}
		else {
			sExitState estate;
			waitchild(&estate,-1,0);
			if(estate.exitCode != 0) {
				print("syscall(%ld,%#lx,%#lx,%#lx,%#lx) -> died with signal %d",
					state->sysc,
					vals[state->arg1],vals[state->arg2],
					vals[state->arg3],vals[state->arg4],
					estate.signal);
				state->arg4++;
			}
		}
	}

	munmap(state);
	fclose(tmp);
	return EXIT_SUCCESS;
}
