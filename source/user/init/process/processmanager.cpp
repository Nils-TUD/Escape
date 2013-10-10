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
#include <esc/messages.h>
#include <esc/io.h>
#include <esc/thread.h>
#include <esc/debug.h>
#include <vterm/vtctrl.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <algorithm>
#include <functional>
#include <sstream>
#include <fstream>
#include <file.h>

#include "processmanager.h"
#include "driverprocess.h"
#include "loginprocess.h"
#include "../machine.h"
#include "../initerror.h"
#include "../progress.h"

using namespace std;

void ProcessManager::start() {
	// TODO this is not exception-safe
	locku(&_lock);
	waitForFS();

	// set basic env-vars
	if(setenv("CWD","/") < 0)
		throw init_error("Unable to set CWD");
	if(setenv("PATH","/bin/") < 0)
		throw init_error("Unable to set PATH");

	// read drivers from file
	ifstream ifs("/etc/drivers");
	if(!ifs)
		throw init_error("Unable to open /etc/drivers");
	while(!ifs.eof()) {
		DriverProcess *drv = new DriverProcess();
		ifs >> *drv;
		_procs.push_back(drv);
	}

	// load processes
	Progress pg(KERNEL_PERCENT,0,_procs.size());
	for(auto  it = _procs.begin(); it != _procs.end(); ++it) {
		pg.itemStarting(string("Loading ") + (*it)->name() + "...");
		(*it)->load();
		pg.itemLoaded();
	}

	// finally enable vterm, which exchanges the loading-screen with its own stuff
	/* TODO setVTermEnabled(true); */
	unlocku(&_lock);
}

void ProcessManager::restart(pid_t pid) {
	Process *p;
	locku(&_lock);
	p = getByPid(pid);
	if(p)
		p->load();
	unlocku(&_lock);
}

void ProcessManager::setAlive(pid_t pid) {
	Process *p;
	locku(&_lock);
	p = getByPid(pid);
	if(p) {
		p->setAlive();
		cout << "Process " << pid << " is alive and promised to terminate ASAP" << endl;
	}
	unlocku(&_lock);
}

void ProcessManager::died(pid_t pid) {
	Process *p;
	locku(&_lock);
	p = getByPid(pid);
	if(p) {
		p->setDead();
		_downProg->itemTerminated();
	}
	unlocku(&_lock);
}

void ProcessManager::shutdown() {
	locku(&_lock);
	/* TODO setVTermEnabled(false); */
	addRunning();
	_downProg = new Progress(0,_procs.size(),_procs.size());
	_downProg->paintBar();
	_downProg->itemStarting("Terminating all processes...");
	for(auto  it = _procs.rbegin(); it != _procs.rend(); ++it) {
		Process *p = *it;
		if(!p->isDead() && p->isKillable()) {
			cout << "Sending SIG_TERM to " << p->pid() << endl;
			if(kill(p->pid(),SIG_TERM) < 0)
				printe("[INIT] Unable to send the term-signal to %d",p->pid());
		}
	}
	unlocku(&_lock);
}

void ProcessManager::finalize(int task) {
	locku(&_lock);
	_downProg->itemStarting("Timout reached, killing remaining processes...");

	// remove all except video (we don't get notified about all terminated processes; just about
	// our childs)
	for(auto  it = _procs.begin(); it != _procs.end(); ) {
		if((*it)->isKillable()) {
			delete *it;
			_procs.erase(it);
		}
		else
			++it;
	}

	// now add all still running processes and kill them
	addRunning();
	for(auto  it = _procs.rbegin(); it != _procs.rend(); ++it) {
		Process *p = *it;
		if(!p->isAlive() && !p->isDead() && p->isKillable()) {
			cout << "Sending SIG_KILL to " << p->pid() << endl;
			if(kill(p->pid(),SIG_KILL) < 0)
				printe("[INIT] Unable to send the kill-signal to %d",p->pid());
		}
	}
	_downProg->allTerminated();

	// ask the machine to reboot/shutdown
	Machine *m = Machine::createInstance();
	if(task == TASK_REBOOT)
		m->reboot(*_downProg);
	else
		m->shutdown(*_downProg);
	unlocku(&_lock);
}

void ProcessManager::addRunning() {
	file procDir("/system/processes");
	vector<sDirEntry> procs = procDir.list_files(false);
	for(auto  it = procs.begin(); it != procs.end(); ++it) {
		int pid = atoi(it->name);
		if(pid != 0 && getByPid(pid) == nullptr)
			_procs.push_back(new Process(pid));
	}
	sort(_procs.begin(),_procs.end());
}

Process *ProcessManager::getByPid(pid_t pid) {
	for(auto it = _procs.begin(); it != _procs.end(); ++it) {
		if((*it)->pid() == pid)
			return *it;
	}
	return nullptr;
}

void ProcessManager::waitForFS() {
	// wait for fs; we need it for exec
	int fd;
	int retries = 0;
	do {
		fd = open("/dev/fs",IO_READ | IO_WRITE);
		if(fd < 0)
			yield();
		retries++;
	}
	while(fd < 0 && retries < DriverProcess::MAX_WAIT_RETRIES);
	if(fd < 0)
		throw init_error("Timeout reached: unable to open /dev/fs");
	close(fd);
}
