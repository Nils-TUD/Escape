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
#include <sys/thread.h>
#include <sys/sync.h>
#include <sys/sllist.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "keystrokes.h"
#include "clients.h"
#include "jobmng.h"

std::mutex JobMng::_mutex;
std::vector<JobMng::Job> JobMng::_jobs;

void JobMng::wait() {
	/* TODO not optimal, but we should wait at the beginning until the first child has been started,
	 * which is done in another thread */
	while(_jobs.size() == 0)
		sleep(50);

	while(true) {
		sExitState state;
		int res = waitchild(&state,-1);
		if(res == 0) {
			if(state.signal != SIG_COUNT)
				print("Child %d terminated because of signal %d",state.pid,state.signal);
			else
				print("Child %d terminated with exitcode %d",state.pid,state.exitCode);
			fflush(stdout);
			terminate(state.pid);

			/* if no jobs are left, create a new one */
			if(_jobs.size() == 0)
				Keystrokes::createTextConsole();
		}
	}
}

int JobMng::getId() {
	std::lock_guard<std::mutex> guard(_mutex);
	if(_jobs.size() >= UIClient::MAX_CLIENTS)
		return -1;

	int id = 0;
	for(auto it = _jobs.begin(); it != _jobs.end(); ) {
		if(it->id == id) {
			id++;
			it = _jobs.begin();
		}
		else
			++it;
	}
	return id;
}

JobMng::Job *JobMng::get(int id) {
	for(auto it = _jobs.begin(); it != _jobs.end(); ++it) {
		if(it->id == id)
			return &*it;
	}
	return NULL;
}

void JobMng::terminate(int pid) {
	std::lock_guard<std::mutex> guard(_mutex);
	for(auto it = _jobs.begin(); it != _jobs.end(); ++it) {
		if(it->loginPid == pid || it->termPid == pid) {
			if(it->loginPid == pid) {
				if(it->termPid != -1)
					kill(it->termPid,SIGTERM);
				it->loginPid = -1;
			}
			else {
				if(it->loginPid != -1)
					kill(it->loginPid,SIGTERM);
				it->termPid = -1;
			}

			if(it->loginPid == -1 && it->termPid == -1)
				_jobs.erase(it);
			break;
		}
	}
}
