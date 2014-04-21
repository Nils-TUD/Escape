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
#include <esc/sync.h>
#include <ipc/clientdevice.h>
#include <mutex>

#include "clients.h"
#include "header.h"
#include "keymap.h"
#include "screens.h"

class UIMngDevice : public ipc::ClientDevice<UIClient> {
public:
	explicit UIMngDevice(const char *name,mode_t mode,std::mutex &mutex)
		: ClientDevice(name,mode,DEV_TYPE_SERVICE,DEV_OPEN | DEV_CREATSIBL | DEV_CLOSE), _mutex(mutex) {
		set(MSG_FILE_OPEN,std::make_memfun(this,&UIMngDevice::open));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&UIMngDevice::close),false);
		set(MSG_DEV_CREATSIBL,std::make_memfun(this,&UIMngDevice::creatsibl));
		set(MSG_UIM_GETKEYMAP,std::make_memfun(this,&UIMngDevice::getKeymap));
		set(MSG_UIM_SETKEYMAP,std::make_memfun(this,&UIMngDevice::setKeymap));
		set(MSG_SCR_GETMODES,std::make_memfun(this,&UIMngDevice::getModes));
		set(MSG_SCR_GETMODE,std::make_memfun(this,&UIMngDevice::getMode));
		set(MSG_SCR_SETMODE,std::make_memfun(this,&UIMngDevice::setMode));
		set(MSG_SCR_SETCURSOR,std::make_memfun(this,&UIMngDevice::setCursor),false);
		set(MSG_SCR_UPDATE,std::make_memfun(this,&UIMngDevice::update),false);
	}

	void open(ipc::IPCStream &is) {
		std::lock_guard<std::mutex> guard(_mutex);
		ipc::ClientDevice<UIClient>::open(is);
		UIClient *c = get(is.fd());
		c->keymap(km_getDefault());
	}

	void close(ipc::IPCStream &is) {
		std::lock_guard<std::mutex> guard(_mutex);
		UIClient *c = get(is.fd());
		c->remove();
		ipc::ClientDevice<UIClient>::close(is);
	}

	void creatsibl(ipc::IPCStream &is) {
		UIClient *c = get(is.fd());
		ipc::FileCreatSibl::Request r;
		is >> r;

		std::lock_guard<std::mutex> guard(_mutex);
		c->attach(r.nfd);

		/* update header */
		gsize_t width,height;
		if(header_update(c,&width,&height))
			c->screen()->update(0,0,width,height);

		is << ipc::FileCreatSibl::Response(0) << ipc::Reply();
	}

	void getKeymap(ipc::IPCStream &is) {
		UIClient *c = get(is.fd());
		int res = c->keymap() != NULL ? 0 : -ENOENT;
		is << res << ipc::CString(c->keymap()->path) << ipc::Reply();
	}

	void setKeymap(ipc::IPCStream &is) {
		UIClient *c = get(is.fd());
		ipc::CStringBuf<MAX_PATH_LEN> path;
		is >> path;

		int res = -EINVAL;
		sKeymap *newMap = km_request(path.str());
		if(newMap) {
			/* we don't need to lock this, because the client is only removed if this
			 * device is closed. since this device is only handled by one thread, it
			 * can't happen now */
			c->keymap(newMap);
			res = 0;
		}
		is << res << ipc::Reply();
	}

	void getModes(ipc::IPCStream &is) {
		size_t n;
		is >> n;

		ipc::Screen::Mode *modes = n == 0 ? NULL : new ipc::Screen::Mode[n];
		ssize_t res = ScreenMng::getModes(modes,n);
		is << res << ipc::Reply();
		if(n != 0) {
			is << ipc::ReplyData(modes,res > 0 ? res * sizeof(*modes) : 0);
			delete[] modes;
		}
	}

	void getMode(ipc::IPCStream &is) {
		UIClient *c = get(is.fd());
		if(c->fb()) {
			ipc::Screen::Mode mode = c->fb()->mode();
			ScreenMng::adjustMode(&mode);
			is << 0 << mode << ipc::Reply();
		}
		else
			is << -EINVAL << ipc::Reply();
	}

	void setMode(ipc::IPCStream &is) {
		UIClient *c = get(is.fd());
		bool swmode;
		int modeid,type;
		ipc::CStringBuf<MAX_PATH_LEN> path;
		is >> modeid >> type >> swmode >> path;

		/* lock that to prevent that we interfere with e.g. the debugger keystroke */
		std::lock_guard<std::mutex> guard(_mutex);
		ipc::Screen::Mode mode;
		ipc::Screen *scr;
		if(ScreenMng::find(modeid,&mode,&scr)) {
			/* only set this mode if it's the active client */
			c->setMode(type,mode,scr,path.str(),c->isActive());
			/* update header */
			if(c->isActive()) {
				gsize_t width,height;
				if(header_update(c,&width,&height))
					c->screen()->update(0,0,width,height);
			}
		}
		is << 0 << ipc::Reply();
	}

	void setCursor(ipc::IPCStream &is) {
		UIClient *c = get(is.fd());
		gpos_t x,y;
		int cursor;
		is >> x >> y >> cursor;

		if(c->isActive())
			c->setCursor(x,y,cursor);
	}

	void update(ipc::IPCStream &is) {
		UIClient *c = get(is.fd());
		gpos_t x,y;
		gsize_t w,h;
		is >> x >> y >> w >> h;

		if(c->isActive()) {
			y += header_getHeight(c->type());
			c->screen()->update(x,y,w,h);
		}
	}

private:
	std::mutex &_mutex;
};
