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
#include <esc/proto/screen.h>
#include <esc/ipc/clientdevice.h>
#include <stdlib.h>
#include <assert.h>

#include "keymap.h"

/**
 * Represents a client of the uimng. Note that not all clients are active, but only the ones that
 * have an attached event-channel. Additionally, some clients might not have a screen/framebuffer
 * set yet and are thus not considered when using next()/prev() etc.
 */
class UIClient : public esc::Client {
public:
	/* we have 8 ui groups atm */
	static const size_t MAX_CLIENTS	= 8;

	/**
	 * @return true if the client with given index exists
	 */
	static bool exists(size_t idx) {
		return _clients[idx] != NULL;
	}
	/**
	 * @return the active client
	 */
	static UIClient *getActive() {
		return _active != MAX_CLIENTS ? _clients[_active] : NULL;
	}
	/**
	 * @return the client at index <idx>
	 */
	static UIClient *getByIdx(size_t idx) {
		return _clients[idx];
	}
	/**
	 * @return true if client <idx> is active
	 */
	static bool isActive(size_t idx) {
		return idx == _active;
	}
	/**
	 * @return the current number of clients
	 */
	static size_t count() {
		return _clientCount;
	}

	/**
	 * Sends the given message to the active client
	 */
	static void send(const void *msg,size_t size);

	/**
	 * Reactivates the screen for the new client
	 *
	 * @param cli the new client
	 * @param old the old client
	 * @param oldMode our previous mode
	 */
	static void reactivate(UIClient *cli,UIClient *old,int oldMode);

	/**
	 * Switches to the next client
	 */
	static void next() {
		switchClient(+1);
	}
	/**
	 * Switches to the previous client
	 */
	static void prev() {
		switchClient(-1);
	}
	/**
	 * Switches to client with index <idx>
	 */
	static void switchTo(size_t idx) {
		assert(idx < MAX_CLIENTS);
		if(idx != _active && exists(idx))
			reactivate(_clients[idx],getActive(),getOldMode());
	}

	/**
	 * Creates this client
	 */
	explicit UIClient(int f);
	/**
	 * Destroys the client
	 */
	~UIClient();

	/**
	 * @return true if active
	 */
	bool isActive() const {
		return _idx == _active;
	}

	/**
	 * @return the keymap (might be NULL)
	 */
	const Keymap *keymap() const {
		return _map;
	}
	/**
	 * Sets the given keymap and releases the old one, if any.
	 *
	 * @param map the new keymap
	 */
	void keymap(Keymap *map) {
		if(_map)
			Keymap::release(_map);
		_map = map;
	}

	/**
	 * @return the screen instance for this client (might be NULL)
	 */
	esc::Screen *screen() {
		return _screen;
	}

	/**
	 * @return the framebuffer for this client (might be NULL)
	 */
	esc::FrameBuffer *fb() {
		return _fb;
	}
	const esc::FrameBuffer *fb() const {
		return _fb;
	}
	/**
	 * @return the type of UI
	 */
	int type() const {
		return _fb ? _fb->mode().type : -1;
	}

	/**
	 * @return the header line
	 */
	char *header() {
		return _header;
	}

	/**
	 * Attaches the given event-channel to this client
	 *
	 * @param evfd the event-channel
	 */
	int attach(int evfd);

	/**
	 * Sets the given mode.
	 *
	 * @param type the type of mode
	 * @param mode the mode information
	 * @param scr the screen instance to use
	 * @param file the file for the framebuffer
	 * @param set whether to set the mode via the screen
	 */
	void setMode(int type,const esc::Screen::Mode &mode,esc::Screen *scr,const char *file,bool set);

	/**
	 * Sets the cursor to given position
	 */
	void setCursor(gpos_t x,gpos_t y,int cursor);

	/**
	 * Removes this client from the active-client-list
	 */
	void remove();

private:
	static void switchClient(int incr);
	static int getOldMode() {
		UIClient *active = getActive();
		return active ? active->_fb->mode().id : -1;
	}

	void setActive(bool active);
	int modeid() const {
		return _fb ? _fb->mode().id : -1;
	}

	size_t _idx;
	int _evfd;
	Keymap *_map;
	esc::Screen *_screen;
	esc::FrameBuffer *_fb;
	char *_header;
	struct {
		gpos_t x;
		gpos_t y;
		int cursor;
	} _cursor;
	int _type;

	// the attached clients
	static size_t _active;
	static UIClient *_clients[MAX_CLIENTS];
	static UIClient *_allclients[MAX_CLIENTS];
	static size_t _clientCount;
	static std::vector<UIClient*> _clientStack;
};
