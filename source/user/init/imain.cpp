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
#include <esc/driver.h>
#include <esc/messages.h>
#include <signal.h>
#include <stdlib.h>
#include <iostream>

#include "process/processmanager.h"
#include "initerror.h"

using namespace std;

#define SHUTDOWN_TIMEOUT		3000
#define STATE_RUN				0
#define STATE_REBOOT			1
#define STATE_SHUTDOWN			2

static void sigAlarm(int sig);
static int driverThread(void *arg);

static ProcessManager pm;
static volatile int timeout = false;
static int state = STATE_RUN;

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
		waitchild(&st);
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
	return EXIT_SUCCESS;
}

static void sigAlarm(A_UNUSED int sig) {
	timeout = true;
}

static int driverThread(A_UNUSED void *arg) {
	int drv = createdev("/dev/init",DEV_TYPE_SERVICE,0);
	if(drv < 0)
		error("Unable to register device 'init'");
	if(signal(SIG_ALARM,sigAlarm) == SIG_ERR)
		error("Unable to set alarm-handler");

	while(!timeout) {
		msgid_t mid;
		sMsg msg;
		int fd = getwork(drv,&mid,&msg,sizeof(msg),0);
		if(fd < 0) {
			if(fd != -EINTR)
				printe("Unable to get work");
		}
		else {
			switch(mid) {
				case MSG_INIT_REBOOT:
					if(state == STATE_RUN) {
						state = STATE_REBOOT;
						if(alarm(SHUTDOWN_TIMEOUT) < 0)
							printe("Unable to set alarm");
						pm.shutdown();
					}
					break;

				case MSG_INIT_SHUTDOWN:
					if(state == STATE_RUN) {
						state = STATE_SHUTDOWN;
						if(alarm(SHUTDOWN_TIMEOUT) < 0)
							printe("Unable to set alarm");
						pm.shutdown();
					}
					break;

				case MSG_INIT_IAMALIVE:
					if(state != STATE_RUN)
						pm.setAlive((pid_t)msg.args.arg1);
					break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
		}
	}

	if(state == STATE_REBOOT)
		pm.finalize(ProcessManager::TASK_REBOOT);
	else
		pm.finalize(ProcessManager::TASK_SHUTDOWN);
	close(drv);
	return 0;
}
