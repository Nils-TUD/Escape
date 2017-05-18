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
#include <esc/proto/initui.h>
#include <esc/stream/std.h>
#include <sys/common.h>
#include <sys/driver.h>
#include <sys/messages.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/thread.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>

#include "process/driverprocess.h"
#include "process/processmanager.h"
#include "process/uimanager.h"

class PowerDevice;

enum {
	STATE_RUN,
	STATE_REBOOT,
	STATE_SHUTDOWN,
};

static void sigAlarm(int sig);
static int driverThread(void *arg);
static int uiThread(void *arg);

static const int SHUTDOWN_TIMEOUT = 3; /* seconds */

static std::mutex mutex;
static ProcessManager pm;
static UIManager uim;
static int state = STATE_RUN;
static PowerDevice *dev;

static const char *uitypes[] = {
	"/etc/init/tui",
	"/etc/init/gui",
};

class PowerDevice : public esc::Device {
public:
	explicit PowerDevice(const char *path,mode_t mode) : esc::Device(path,mode,DEV_TYPE_SERVICE,0) {
		set(MSG_POWER_REBOOT,std::make_memfun(this,&PowerDevice::reboot),false);
		set(MSG_POWER_SHUTDOWN,std::make_memfun(this,&PowerDevice::shutdown),false);
	}

	void reboot(esc::IPCStream &) {
		std::lock_guard<std::mutex> guard(mutex);
		if(state == STATE_RUN) {
			state = STATE_REBOOT;
			if(alarm(SHUTDOWN_TIMEOUT) < 0)
				printe("Unable to set alarm");
			pm.shutdown();
		}
	}

	void shutdown(esc::IPCStream &) {
		std::lock_guard<std::mutex> guard(mutex);
		if(state == STATE_RUN) {
			state = STATE_SHUTDOWN;
			if(alarm(SHUTDOWN_TIMEOUT) < 0)
				printe("Unable to set alarm");
			pm.shutdown();
		}
	}
};

class InitUIDevice : public esc::Device {
public:
	explicit InitUIDevice(const char *path,mode_t mode) : esc::Device(path,mode,DEV_TYPE_SERVICE,0) {
		set(MSG_INITUI_START,std::make_memfun(this,&InitUIDevice::start));
	}

	void start(esc::IPCStream &is) {
		esc::InitUI::Type type;
		esc::CStringBuf<16> name;
		is >> type >> name;

		if((size_t)type >= ARRAY_SIZE(uitypes) || !isalnumstr(name.str()) || state != STATE_RUN) {
			is << -EINVAL << esc::Reply();
			return;
		}

		std::string uipath(std::string("/dev/") + name.str());

		esc::FStream ifs(uitypes[type],"r");
		if(!ifs)
			throw esc::default_error(std::string("Unable to open ") + uitypes[type]);

		std::lock_guard<std::mutex> guard(mutex);
		std::vector<Process*> ui;
		while(ifs.good()) {
			DriverProcess *drv = new DriverProcess();
			ifs >> *drv;
			drv->replace("$ui",uipath);
			drv->load();
			ui.push_back(drv);
		}
		uim.add(ui);

		is << 0 << esc::Reply();
	}
};

int main(void) {
	if(getpid() > 1)
		exitmsg("It's not good to start init twice ;)");

	if(startthread(driverThread,nullptr) < 0)
		exitmsg("Unable to start driver-thread");
	if(startthread(uiThread,nullptr) < 0)
		exitmsg("Unable to start ui-thread");

	try {
		std::lock_guard<std::mutex> guard(mutex);
		pm.start();
	}
	catch(const std::exception& e) {
		exitmsg("Unable to init system: " << e.what());
	}

	// loop and wait forever
	while(1) {
		sExitState st;
		if(waitchild(&st,-1,0) == 0) {
			std::lock_guard<std::mutex> guard(mutex);
			try {
				if(state != STATE_RUN)
					pm.died(st.pid);
				else {
					if(st.exitCode != 0 || st.signal != SIG_COUNT)
						pm.restart(st.pid);
					uim.died(st.pid);
				}
			}
			catch(const std::exception& e) {
				errmsg("Unable to react on child-death: " << e.what());
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
		exitmsg("Unable to set alarm-handler");

	try {
		dev = new PowerDevice("/dev/power",0110);
		int gid = usergroup_nameToId(GROUPS_PATH,"power");
		if(fchown(dev->id(),-1,gid) < 0)
			error("Unable to chown power device");
		dev->loop();
	}
	catch(const std::exception &e) {
		errmsg("Handling power device failed: " << e.what());
	}

	std::lock_guard<std::mutex> guard(mutex);
	if(state == STATE_REBOOT)
		pm.finalize(ProcessManager::TASK_REBOOT);
	else
		pm.finalize(ProcessManager::TASK_SHUTDOWN);
	return 0;
}

static int uiThread(A_UNUSED void *arg) {
	InitUIDevice dev("/dev/initui",0110);
	int gid = usergroup_nameToId(GROUPS_PATH,"ui");
	if(fchown(dev.id(),-1,gid) < 0)
		error("Unable to chown initui device");
	dev.loop();
	return 0;
}
