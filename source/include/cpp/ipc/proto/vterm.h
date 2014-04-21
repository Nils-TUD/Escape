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

#pragma once

#include <esc/common.h>
#include <esc/messages.h>
#include <ipc/ipcstream.h>
#include <ipc/proto/ui.h>
#include <vector>
#include <string>
#include <vthrow.h>

namespace ipc {

/**
 * The IPC-interface for the vterm-device. You can also use (most) of the UI/Screen interface (the
 * others are hidden).
 */
class VTerm : public UI {
public:
	enum Flag {
		FL_ECHO,
		FL_READLINE,
		FL_NAVI
	};

	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @throws if the open failed
	 */
	explicit VTerm(const char *path) : UI(path) {
	}
	/**
	 * Attaches this object to the given file-descriptor
	 *
	 * @param f the file-descriptor
	 */
	explicit VTerm(int f) : UI(f) {
	}

	/**
	 * @param flag the flag
	 * @return the value of the given flag
	 * @throws if the operation failed
	 */
	bool getFlag(Flag flag) {
		int res;
		_is << flag << SendReceive(MSG_VT_GETFLAG) >> res;
		if(res < 0)
			VTHROWE("getFlag(" << flag << ")",res);
		return res == 1;
	}

	/**
	 * Sets the given flag to <val>.
	 *
	 * @param flag the flag
	 * @param val the new value
	 * @throws if the operation failed
	 */
	void setFlag(Flag flag,bool val) {
		int res;
		_is << flag << val << SendReceive(MSG_VT_SETFLAG) >> res;
		if(res < 0)
			VTHROWE("setFlag(" << flag << "," << val << ")",res);
	}

	/**
	 * Creates a backup of the current screen, which can be restored with restore() later.
	 *
	 * @throws if the operation failed
	 */
	void backup() {
		int res;
		_is << SendReceive(MSG_VT_BACKUP) >> res;
		if(res < 0)
			VTHROWE("backup()",res);
	}

	/**
	 * Restores the latest backup created by backup().
	 *
	 * @throws if the operation failed
	 */
	void restore() {
		int res;
		_is << SendReceive(MSG_VT_RESTORE) >> res;
		if(res < 0)
			VTHROWE("restore()",res);
	}

	/**
	 * Publishes the pid of the shell to the vterm.
	 *
	 * @param pid the shell-pid
	 * @throws if the operation failed
	 */
	void setShellPid(pid_t pid) {
		int res;
		_is << pid << SendReceive(MSG_VT_SHELLPID) >> res;
		if(res < 0)
			VTHROWE("setShellPid(" << pid << ")",res);
	}

	/**
	 * Sets the given video-mode.
	 *
	 * @param mode the video-mode
	 * @throws if the operation failed
	 */
	void setMode(int mode) {
		int res;
		_is << mode << SendReceive(MSG_VT_SETMODE) >> res;
		if(res < 0)
			VTHROWE("setMode(" << mode << ")",res);
	}

private:
	/* hide these members since they are not supported */
	using UI::setCursor;
	using UI::update;
};

}
