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
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/sync.h>
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

static sJob *jobs_get(int id);
static void jobs_terminate(int pid);

static tUserSem usem;
static sSLList jobs;

void jobs_init(void) {
	if(usemcrt(&usem,1) < 0)
		error("Unable to create jobs lock");
	sll_init(&jobs,malloc,free);
}

void jobs_wait(void) {
	/* TODO not optimal, but we should wait at the beginning until the first child has been started,
	 * which is done in another thread */
	while(sll_length(&jobs) == 0)
		sleep(50);

	while(true) {
		sExitState state;
		int res = waitchild(&state);
		if(res == 0) {
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
}

int jobs_getId(void) {
	usemdown(&usem);
	if(sll_length(&jobs) >= MAX_CLIENTS) {
		usemup(&usem);
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
	usemup(&usem);
	return id;
}

bool jobs_exists(int id) {
	usemdown(&usem);
	sJob *job = jobs_get(id);
	bool res = job != NULL;
	usemup(&usem);
	return res;
}

void jobs_add(int id,int vtermPid) {
	usemdown(&usem);
	sJob *job = (sJob*)malloc(sizeof(sJob));
	if(!job)
		goto error;

	job->id = id;
	job->shPid = -1;
	job->vtermPid = vtermPid;
	if(!sll_append(&jobs,job))
		goto error;
	usemup(&usem);
	return;

error:
	printe("Unable to add job");
	usemup(&usem);
}

void jobs_setLoginPid(int id,int pid) {
	usemdown(&usem);
	sJob *job = jobs_get(id);
	if(job)
		job->shPid = pid;
	usemup(&usem);
}

static sJob *jobs_get(int id) {
	for(sSLNode *n = sll_begin(&jobs); n != NULL; n = n->next) {
		sJob *job = (sJob*)n->data;
		if(job->id == id)
			return job;
	}
	return NULL;
}

static void jobs_terminate(int pid) {
	usemdown(&usem);
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
	usemup(&usem);
}
