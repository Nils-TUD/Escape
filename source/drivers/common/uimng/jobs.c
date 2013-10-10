/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/sllist.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "keystrokes.h"
#include "clients.h"
#include "jobs.h"

typedef struct {
	int id;
	int shPid;
	int vtermPid;
} sJob;

static void jobs_terminate(int pid);

static tULock lck;
static sSLList jobs;

void jobs_init(void) {
	if(crtlocku(&lck) < 0)
		error("Unable to create jobs lock");
	sll_init(&jobs,malloc,free);
}

void jobs_wait(void) {
	while(true) {
		sExitState state;
		waitchild(&state);
		if(state.signal != SIG_COUNT)
			printf("[uimng] Child %d terminated because of signal %d\n",state.pid,state.signal);
		else
			printf("[uimng] Child %d terminated with exitcode %d\n",state.pid,state.exitCode);
		fflush(stdout);
		jobs_terminate(state.pid);

		/* if no jobs are left, create a new one */
		if(sll_length(&jobs) == 0)
			keys_createTextConsole();
	}
}

int jobs_getId(void) {
	locku(&lck);
	if(sll_length(&jobs) >= MAX_CLIENTS) {
		unlocku(&lck);
		return -1;
	}
	int id = 0;
	for(sSLNode *n = sll_begin(&jobs); n != NULL; ) {
		sJob *job = (sJob*)n->data;
		if(job->id == id) {
			id++;
			n = sll_begin(&jobs);
		}
		else
			n = n->next;
	}
	unlocku(&lck);
	return id;
}

void jobs_add(int id,int shPid,int vtermPid) {
	locku(&lck);
	sJob *job = (sJob*)malloc(sizeof(sJob));
	if(!job)
		goto error;

	job->id = id;
	job->shPid = shPid;
	job->vtermPid = vtermPid;
	if(!sll_append(&jobs,job))
		goto error;
	unlocku(&lck);
	return;

error:
	printe("Unable to add job");
	unlocku(&lck);
}

static void jobs_terminate(int pid) {
	locku(&lck);
	for(sSLNode *n = sll_begin(&jobs); n != NULL; n = n->next) {
		sJob *job = (sJob*)n->data;
		if(job->shPid == pid || job->vtermPid == pid) {
			if(job->shPid == pid) {
				if(job->vtermPid != -1)
					kill(job->vtermPid,SIG_KILL);
				job->shPid = -1;
			}
			else {
				if(job->shPid != -1)
					kill(job->shPid,SIG_KILL);
				job->vtermPid = -1;
			}

			if(job->shPid == -1 && job->vtermPid == -1) {
				sll_removeFirstWith(&jobs,job);
				free(job);
			}
			break;
		}
	}
	unlocku(&lck);
}
