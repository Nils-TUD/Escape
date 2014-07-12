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

#pragma once

#include <sys/common.h>
#include <sys/messages.h>
#include <sys/proc.h>
#include <ipc/ipcstream.h>

namespace ipc {

/**
 * The IPC-interface for the init device. Allows you to reboot, shutdown etc..
 */
class Init {
public:
	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @throws if the operation failed
	 */
	explicit Init(const char *path) : _is(path) {
	}

	/**
	 * No copying
	 */
	Init(const Init&) = delete;
	Init &operator=(const Init&) = delete;

	/**
	 * Reboots the machine
	 *
	 * @throws if the operation failed
	 */
	void reboot() {
		_is << Send(MSG_INIT_REBOOT);
	}

	/**
	 * Shuts the machine down.
	 *
	 * @throws if the operation failed
	 */
	void shutdown() {
		_is << Send(MSG_INIT_SHUTDOWN);
	}

	/**
	 * Sends the I-am-alive signal to init, so that it gives you more time to terminate on reboot/
	 * shutdown.
	 *
	 * @throws if the operation failed
	 */
	void iamalive() {
		pid_t pid = getpid();
		_is << pid << Send(MSG_INIT_IAMALIVE);
	}

private:
	IPCStream _is;
};

}
