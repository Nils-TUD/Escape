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

#include <esc/ipc/device.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/messages.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <sys/wait.h>
#include <iostream>
#include <signal.h>
#include <stdlib.h>

#include "process/processmanager.h"
#include "initerror.h"

#define SHUTDOWN_TIMEOUT		3000
#define STATE_RUN				0
#define STATE_REBOOT			1
#define STATE_SHUTDOWN			2

class InitDevice;

static void sigAlarm(int sig);
static int driverThread(void *arg);

static ProcessManager pm;
static int state = STATE_RUN;
static InitDevice *dev;

class InitDevice : public esc::Device {
public:
	explicit InitDevice(const char *path,mode_t mode) : esc::Device(path,mode,DEV_TYPE_SERVICE,0) {
		set(MSG_INIT_REBOOT,std::make_memfun(this,&InitDevice::reboot),false);
		set(MSG_INIT_SHUTDOWN,std::make_memfun(this,&InitDevice::shutdown),false);
		set(MSG_INIT_IAMALIVE,std::make_memfun(this,&InitDevice::iamalive),false);
	}

	void reboot(esc::IPCStream &) {
		if(state == STATE_RUN) {
			state = STATE_REBOOT;
			if(alarm(SHUTDOWN_TIMEOUT) < 0)
				printe("Unable to set alarm");
			pm.shutdown();
		}
	}

	void shutdown(esc::IPCStream &) {
		if(state == STATE_RUN) {
			state = STATE_SHUTDOWN;
			if(alarm(SHUTDOWN_TIMEOUT) < 0)
				printe("Unable to set alarm");
			pm.shutdown();
		}
	}

	void iamalive(esc::IPCStream &is) {
		pid_t pid;
		is >> pid;

		if(state != STATE_RUN)
			pm.setAlive(pid);
	}
};

int main(void) {
	if(getpid() != 0)
		error("It's not good to start init twice ;)");

	if(startthread(driverThread,nullptr) < 0)
		error("Unable to start driver-thread");

	try {
		pm.start();
	}
	catch(const init_error& e) {
		error("Unable to init system: %s",e.what());
	}

	// loop and wait forever
	while(1) {
		sExitState st;
		if(waitchild(&st,-1) == 0) {
			try {
				if(state != STATE_RUN)
					pm.died(st.pid);
				else
					pm.restart(st.pid);
			}
			catch(const init_error& e) {
				printe("Unable to react on child-death: %s",e.what());
			}
		}
	}
	return EXIT_SUCCESS;
}

static void sigAlarm(A_UNUSED int sig) {
	dev->stop();
}

static int driverThread(A_UNUSED void *arg) {
	if(signal(SIGALRM,sigAlarm) == SIG_ERR)
		error("Unable to set alarm-handler");

	try {
		dev = new InitDevice("/dev/init",0111);
		dev->loop();
	}
	catch(const std::exception &e) {
		printe("%s",e.what());
	}

	if(state == STATE_REBOOT)
		pm.finalize(ProcessManager::TASK_REBOOT);
	else
		pm.finalize(ProcessManager::TASK_SHUTDOWN);
	return 0;
}
