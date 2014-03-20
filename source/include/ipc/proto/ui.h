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
#include <ipc/proto/screen.h>
#include <ipc/proto/input.h>
#include <vector>
#include <string>

namespace ipc {

/**
 * The IPC-interface for the uimng-device. It inherits from Screen since it supports all operations
 * a Screen supports plus a few additional ones.
 */
class UI : public Screen {
public:
	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @throws if the open failed
	 */
	explicit UI(const char *path) : Screen(path) {
	}
	/**
	 * Attaches this object to the given file-descriptor
	 *
	 * @param fd
	 */
	explicit UI(int fd) : Screen(fd) {
	}

	/**
	 * @return the random id for this UI that can be used to attach the event-channel.
	 * @throws if the operation failed
	 */
	int getId() {
		int res;
		_is << SendReceive(MSG_UIM_GETID) >> res;
		if(res < 0)
			VTHROWE("getId()",res);
		return res;
	}

	/**
	 * @return the currently set keymap file
	 * @throws if the operation failed
	 */
	std::string getKeymap() {
		CStringBuf<MAX_PATH_LEN> path;
		int res;
		_is << SendReceive(MSG_UIM_GETKEYMAP) >> res >> path;
		if(res < 0)
			VTHROWE("getKeymap()",res);
		return path.str();
	}

	/**
	 * Sets the given keymap for this UI.
	 *
	 * @param map the new keymap file
	 * @throws if the operation failed
	 */
	void setKeymap(const std::string &map) {
		int res;
		_is << CString(map.c_str(),map.length()) << SendReceive(MSG_UIM_SETKEYMAP) >> res;
		if(res < 0)
			VTHROWE("setKeymap(" << map << ")",res);
	}
};

/**
 * The IPC-interface for the uimng-event-device. It is supposed to be used in conjunction with UI.
 */
class UIEvents {
public:
	struct Event {
		enum {
			TYPE_KEYBOARD,
			TYPE_MOUSE
		} type;
		union {
			struct {
				/* the keycode (see keycodes.h) */
				uchar keycode;
				/* modifiers (STATE_CTRL, STATE_SHIFT, STATE_ALT, STATE_BREAK) */
				uchar modifier;
				/* the character, translated by the current keymap */
				char character;
			} keyb;
			Mouse::Event mouse;
		} d;
	};

	/**
	 * Opens the given device and attaches to the given UI-connection
	 *
	 * @param path the path to the device
	 * @param ui the UI-connection
	 * @throws if the open failed
	 */
	explicit UIEvents(const char *path,UI &ui) : _is(path) {
		int res;
		_is << ui.getId() << SendReceive(MSG_UIM_ATTACH) >> res;
		if(res < 0)
			VTHROWE("attach()",res);
	}

	/**
	 * Reads the next event from this channel.
	 */
	UIEvents &operator>>(Event &ev) {
		_is >> ipc::ReceiveData(&ev,sizeof(ev));
		return *this;
	}

private:
	IPCStream _is;
};

std::ostream &operator<<(std::ostream &os,const UIEvents::Event &ev);

}
