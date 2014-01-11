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

#pragma once

#include <esc/common.h>
#include <esc/sync.h>
#include <vector>
#include "process.h"
#include "../progress.h"
#include "../initerror.h"

class ProcessManager {
public:
	static const int TASK_REBOOT		= 1;
	static const int TASK_SHUTDOWN		= 2;

private:
	static const size_t KERNEL_PERCENT	= 40;

public:
	ProcessManager() : _sem(), _downProg(nullptr), _procs() {
		if(usemcrt(&_sem,1) < 0)
			throw init_error("Unable to create process-manager lock");
	}
	~ProcessManager() {
	}

	void start();
	void restart(pid_t pid);
	void shutdown();
	void setAlive(pid_t pid);
	void died(pid_t pid);
	void finalize(int task);

private:
	// no cloning
	ProcessManager(const ProcessManager& p);
	ProcessManager &operator=(const ProcessManager& p);

	Process *getByPid(pid_t pid);
	void addRunning();
	void waitForFS();
	size_t getBootModCount() const;

private:
	tUserSem _sem;
	Progress *_downProg;
	std::vector<Process*> _procs;
};
