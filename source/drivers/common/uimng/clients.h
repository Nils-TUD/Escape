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

#include <esc/common.h>
#include <esc/messages.h>
#include <ipc/proto/screen.h>
#include <ipc/clientdevice.h>
#include <stdlib.h>
#include <assert.h>

#include "keymap.h"

#define MAX_CLIENTS						8	/* we have 8 ui groups atm */

class UIClient : public ipc::Client {
public:
	static bool exists(size_t idx) {
		return _clients[idx] != NULL;
	}
	static UIClient *getActive() {
		return _active != MAX_CLIENTS ? _clients[_active] : NULL;
	}
	static UIClient *getByIdx(size_t idx) {
		return _clients[idx];
	}
	static bool isActive(size_t idx) {
		return idx == _active;
	}
	static size_t count() {
		return _clientCount;
	}

	static void send(const void *msg,size_t size);
	static void reactivate(UIClient *cli,UIClient *old,int oldMode);
	static void next() {
		switchClient(+1);
	}
	static void prev() {
		switchClient(-1);
	}
	static void switchTo(size_t idx) {
		assert(idx < MAX_CLIENTS);
		if(idx != _active && exists(idx))
			reactivate(_clients[idx],getActive(),getOldMode());
	}
	static size_t attach(int randId,int evfd);
	static void detach(int evfd);

	explicit UIClient(int f);
	~UIClient();

	bool isActive() const {
		return _idx == _active;
	}
	int randId() const {
		return _randid;
	}

	const sKeymap *keymap() const {
		return _map;
	}
	void keymap(sKeymap *map) {
		if(_map)
			km_release(_map);
		_map = map;
	}

	ipc::Screen *screen() {
		return _screen;
	}

	ipc::FrameBuffer *fb() {
		return _fb;
	}
	const ipc::FrameBuffer *fb() const {
		return _fb;
	}
	int type() const {
		return _fb ? _fb->mode().type : -1;
	}

	char *header() {
		return _header;
	}

	void setMode(int type,const ipc::Screen::Mode &mode,ipc::Screen *scr,const char *file,bool set);
	void setCursor(gpos_t x,gpos_t y,int cursor);
	void remove();

private:
	static int getByRandId(int randid,UIClient **cli);
	static void switchClient(int incr);
	static int getOldMode() {
		UIClient *active = getActive();
		return active ? active->_fb->mode().id : -1;
	}

	int modeid() const {
		return _fb ? _fb->mode().id : -1;
	}

	size_t _idx;
	int _evfd;
	int _randid;
	sKeymap *_map;
	ipc::Screen *_screen;
	ipc::FrameBuffer *_fb;
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
};
