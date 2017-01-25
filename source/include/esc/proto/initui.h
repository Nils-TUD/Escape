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
#include <esc/vthrow.h>
#include <string>

namespace esc {

/**
 * The IPC-interface for the initui device. Allows you to start new UIs.
 */
class InitUI {
public:
	enum Type {
		TUI,
		GUI,
	};

	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @throws if the operation failed
	 */
	explicit InitUI(const char *path) : _is(path) {
	}

	/**
	 * No copying
	 */
	InitUI(const InitUI&) = delete;
	InitUI &operator=(const InitUI&) = delete;

	/**
	 * Starts a new UI of given type and given name
	 *
	 * @param type the type
	 * @param name the name
	 * @throws if the operation failed
	 */
	void start(Type type,const std::string &name) {
		errcode_t res;
		_is << type << CString(name.c_str(),name.length()) << SendReceive(MSG_INITUI_START) >> res;
		if(res < 0)
			VTHROWE("start(" << type << "," << name << ")",res);
	}

private:
	IPCStream _is;
};

}
