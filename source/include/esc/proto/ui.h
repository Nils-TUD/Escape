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
#include <sys/stat.h>
#include <esc/proto/screen.h>
#include <esc/proto/input.h>
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
	 * @param f the file-descriptor
	 */
	explicit UI(int f) : Screen(f) {
	}

	/**
	 * No copying
	 */
	UI(const UI&) = delete;
	UI &operator=(const UI&) = delete;

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
			TYPE_MOUSE,
			TYPE_UI_ACTIVE,
			TYPE_UI_INACTIVE
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
	 * Attaches this event-channel to the given UI-connection
	 *
	 * @param path the path to the device
	 * @param ui the UI-connection
	 * @throws if the open failed
	 */
	explicit UIEvents(UI &ui) : _is(creatsibl(ui.fd(),0)) {
		if(_is.fd() < 0)
			VTHROWE("creatsibl(" << ui.fd() << ",0)",_is.fd());
	}

	/**
	 * No copying
	 */
	UIEvents(const UIEvents&) = delete;
	UIEvents &operator=(const UIEvents&) = delete;

	/**
	 * @return the IPC stream
	 */
	IPCStream &is() {
		return _is;
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
