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

#include <esc/ipc/ipcstream.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <sys/proc.h>

namespace esc {

/**
 * The IPC-interface for the power device. Allows you to reboot, shutdown etc..
 */
class Power {
public:
	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @throws if the operation failed
	 */
	explicit Power(const char *path) : _is(path) {
	}

	/**
	 * No copying
	 */
	Power(const Power&) = delete;
	Power &operator=(const Power&) = delete;

	/**
	 * Reboots the machine
	 *
	 * @throws if the operation failed
	 */
	void reboot() {
		_is << Send(MSG_POWER_REBOOT);
	}

	/**
	 * Shuts the machine down.
	 *
	 * @throws if the operation failed
	 */
	void shutdown() {
		_is << Send(MSG_POWER_SHUTDOWN);
	}

private:
	IPCStream _is;
};

}
