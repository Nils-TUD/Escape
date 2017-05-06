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
#include <esc/proto/richui.h>
#include <esc/vthrow.h>
#include <sys/common.h>
#include <sys/messages.h>
#include <string>
#include <utility>
#include <vector>

namespace esc {

/**
 * The IPC-interface for the vterm-device. You can also use (most) of the UI/Screen interface (the
 * others are hidden).
 */
class VTerm : public RichUI {
public:
	enum Flag {
		FL_ECHO,
		FL_READLINE,
		FL_NAVI
	};

	/**
	 * Determines the console size for given vterm device. If not available, it defaults to 80x30.
	 *
	 * @param path the vterm device
	 * @return the size (cols,rows)
	 */
	static std::pair<uint,uint> getSize(const char *path) {
		std::pair<uint,uint> size;
		try {
			esc::VTerm vterm(path);
			esc::Screen::Mode mode = vterm.getMode();
			size = std::make_pair(mode.cols,mode.rows);
		}
		catch(...) {
			size = std::make_pair(80,30);
		}
		return size;
	}

	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @throws if the open failed
	 */
	explicit VTerm(const char *path) : RichUI(path) {
	}
	/**
	 * Attaches this object to the given file-descriptor
	 *
	 * @param f the file-descriptor
	 */
	explicit VTerm(int f) : RichUI(f) {
	}

	/**
	 * @param flag the flag
	 * @return the value of the given flag
	 * @throws if the operation failed
	 */
	bool getFlag(Flag flag) {
		ValueResponse<bool> r;
		_is << flag << SendReceive(MSG_VT_GETFLAG) >> r;
		if(r.err < 0)
			VTHROWE("getFlag(" << flag << ")",r.err);
		return r.res;
	}

	/**
	 * Sets the given flag to <val>.
	 *
	 * @param flag the flag
	 * @param val the new value
	 * @throws if the operation failed
	 */
	void setFlag(Flag flag,bool val) {
		errcode_t res;
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
		errcode_t res;
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
		errcode_t res;
		_is << SendReceive(MSG_VT_RESTORE) >> res;
		if(res < 0)
			VTHROWE("restore()",res);
	}

	/**
	 * Allows the vterm to send signals to this process.
	 *
	 * @throws if the operation failed
	 */
	void enableSignals() {
		int fd = open("/sys/pid/self",O_WRONLY);
		if(fd < 0)
			VTHROWE("enableSignals()",fd);
		int res = delegate(_is.fd(),fd,O_WRONLY,0);
		close(fd);
		if(res < 0)
			VTHROWE("enableSignals()",res);
	}

private:
	/* hide these members since they are not supported */
	using UI::setCursor;
	using UI::update;
};

}
