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
#include <esc/heap.h>
#include <sllist.h>
#include <assert.h>
#include "running.h"

static tCmdId nextId = 1;
static sSLList *running = NULL;

void run_init(void) {
	running = sll_create();
	vassert(running != NULL,"Unable to create linked list for running processes");
}

tCmdId run_requestId(void) {
	return nextId++;
}

bool run_addProc(tCmdId cmdId,tPid pid,tFD *pipe) {
	sRunningProc *run = (sRunningProc*)malloc(sizeof(sRunningProc));
	if(run) {
		run->cmdId = cmdId;
		run->pid = pid;
		run->pipe[0] = pipe[0];
		run->pipe[1] = pipe[1];
		run->nextRunning = false;
		run->terminated = false;
		if(sll_append(running,run))
			return true;
		free(run);
	}
	return false;
}

sRunningProc *run_findProc(tCmdId cmdId,tPid pid) {
	sSLNode *n;
	sRunningProc *p;
	for(n = sll_begin(running); n != NULL; n = n->next) {
		p = (sRunningProc*)n->data;
		if(p->pid == pid && (cmdId == CMD_ID_ALL || p->cmdId == cmdId))
			return p;
	}
	return NULL;
}

void run_remProc(tPid pid) {
	sRunningProc *run = run_findProc(CMD_ID_ALL,pid);
	if(run) {
		sll_removeFirst(running,run);
		free(run);
	}
}
