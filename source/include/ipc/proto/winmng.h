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
 * The IPC-interface for the window-manager-device. You can also use (most) of the UI/Screen
 * interface (the others are hidden).
 */
class WinMng : public UI {
public:
	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @throws if the open failed
	 */
	explicit WinMng(const char *path) : UI(path) {
	}

	gwinid_t create(gpos_t x,gpos_t y,gsize_t w,gsize_t h,uint style,gsize_t titleHeight,const char *title) {
		gwinid_t res;
		_is << x << y << w << h << style << titleHeight << CString(title);
		_is << ipc::SendReceive(MSG_WIN_CREATE) >> res;
		if(res < 0)
			VTHROWE("create(" << x << "," << y << "," << w << "," << h << ")",res);
		return res;
	}

	void setActive(gwinid_t wid) {
		_is << wid << ipc::Send(MSG_WIN_SET_ACTIVE);
	}

	void destroy(gwinid_t wid) {
		_is << wid << ipc::Send(MSG_WIN_DESTROY);
	}

	void move(gwinid_t wid,gpos_t x,gpos_t y,bool finished) {
		_is << wid << x << y << finished << ipc::Send(MSG_WIN_MOVE);
	}

	void resize(gwinid_t wid,gpos_t x,gpos_t y,gsize_t width,gsize_t height,bool finished) {
		_is << wid << x << y << width << height << finished << ipc::Send(MSG_WIN_RESIZE);
	}

	void update(gwinid_t wid,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		int res;
		_is << wid << x << y << width << height << ipc::SendReceive(MSG_WIN_UPDATE) >> res;
		if(res < 0)
			VTHROWE("update(" << x << "," << y << "," << width << "," << height << ")",res);
	}

	void setMode(gsize_t width,gsize_t height,gcoldepth_t bpp) {
		int res;
		_is << width << height << bpp << ipc::SendReceive(MSG_WIN_SETMODE) >> res;
		if(res < 0)
			VTHROWE("setMode(" << width << "," << height << "," << bpp << ")",res);
	}

private:
	/* hide these members since they are not supported */
	using UI::setCursor;
	using UI::getId;
};

/**
 * The IPC-interface for the window-manager-event-device. It is supposed to be used in conjunction
 * with WinMng.
 */
class WinMngEvents {
public:
	static const size_t MAX_WINTITLE_LEN	= 64;
	struct Event {
		enum Type {
			TYPE_KEYBOARD,
			TYPE_MOUSE,
			TYPE_RESIZE,
			TYPE_SET_ACTIVE,
			TYPE_RESET,
			/* subscribable events */
			TYPE_CREATED,
			TYPE_ACTIVE,
			TYPE_DESTROYED,
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
			struct {
				/* current position */
				gpos_t x;
				gpos_t y;
				/* movement */
				short movedX;
				short movedY;
				short movedZ;
				/* pressed buttons */
				uchar buttons;
			} mouse;
			struct {
				bool active;
				gpos_t mouseX;
				gpos_t mouseY;
			} setactive;
			struct {
				char title[MAX_WINTITLE_LEN];
			} created;
		} d;
		/* window id */
		gwinid_t wid;
	};

	/**
	 * Opens the given device
	 *
	 * @param path the path to the device
	 * @throws if the open failed
	 */
	explicit WinMngEvents(const char *path) : _is(path) {
	}

	/**
	 * @return the file-descriptor
	 */
	int fd() const {
		return _is.fd();
	}

	/**
	 * Attaches this event-channel to receive events for the given window.
	 *
	 * @param wid the window-id (default: all)
	 * @throws if the operation failed
	 */
	void attach(gwinid_t wid = -1) {
		int res;
		_is << wid << SendReceive(MSG_WIN_ATTACH) >> res;
		if(res < 0)
			VTHROWE("attach(" << wid << ")",res);
	}

	/**
	 * Subscribes the given event-type (only possible for TYPE_{CREATED,ACTIVE,DESTROYED}).
	 *
	 * @param type the event-type
	 * @throws if the send failed
	 */
	void subscribe(Event::Type type) {
		_is << type << ipc::Send(MSG_WIN_ADDLISTENER);
	}

	/**
	 * Unsubscribes the given event-type (only possible for TYPE_{CREATED,ACTIVE,DESTROYED}).
	 *
	 * @param type the event-type
	 * @throws if the send failed
	 */
	void unsubscribe(Event::Type type) {
		_is << type << ipc::Send(MSG_WIN_REMLISTENER);
	}

	/**
	 * Reads the next event from this channel.
	 *
	 * @throws if the receive failed
	 */
	WinMngEvents &operator>>(Event &ev) {
		_is >> ipc::ReceiveData(&ev,sizeof(ev));
		return *this;
	}

private:
	IPCStream _is;
};

std::ostream &operator<<(std::ostream &os,const WinMngEvents::Event &ev);

}
