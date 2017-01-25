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

#include <esc/ipc/clientdevice.h>
#include <keymap/keymap.h>
#include <sys/common.h>
#include <sys/sync.h>
#include <mutex>

#include "clients.h"
#include "header.h"
#include "keystrokes.h"
#include "screens.h"

class UIMngDevice : public esc::ClientDevice<UIClient> {
public:
	explicit UIMngDevice(const char *name,mode_t mode,std::mutex &mutex)
		: ClientDevice(name,mode,DEV_TYPE_SERVICE,DEV_OPEN | DEV_OBTAIN | DEV_DELEGATE | DEV_CLOSE),
		  _mutex(mutex) {
		set(MSG_FILE_OPEN,std::make_memfun(this,&UIMngDevice::open));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&UIMngDevice::close),false);
		set(MSG_DEV_OBTAIN,std::make_memfun(this,&UIMngDevice::obtain));
		set(MSG_DEV_DELEGATE,std::make_memfun(this,&UIMngDevice::delegate));
		set(MSG_UIM_GETKEYMAP,std::make_memfun(this,&UIMngDevice::getKeymap));
		set(MSG_UIM_SETKEYMAP,std::make_memfun(this,&UIMngDevice::setKeymap));
		set(MSG_SCR_GETMODES,std::make_memfun(this,&UIMngDevice::getModes));
		set(MSG_SCR_GETMODE,std::make_memfun(this,&UIMngDevice::getMode));
		set(MSG_SCR_SETMODE,std::make_memfun(this,&UIMngDevice::setMode));
		set(MSG_SCR_SETCURSOR,std::make_memfun(this,&UIMngDevice::setCursor),false);
		set(MSG_SCR_UPDATE,std::make_memfun(this,&UIMngDevice::update),false);
	}

	void open(esc::IPCStream &is) {
		std::lock_guard<std::mutex> guard(_mutex);
		esc::ClientDevice<UIClient>::open(is);
	}

	void close(esc::IPCStream &is) {
		std::lock_guard<std::mutex> guard(_mutex);
		UIClient *c = get(is.fd());
		c->remove();
		esc::ClientDevice<UIClient>::close(is);
		if(client_count() == 0)
			Keystrokes::createTextConsole();
	}

	void obtain(esc::IPCStream &is) {
		UIClient *c = get(is.fd());
		esc::DevObtain::Request r;
		is >> r;

		std::lock_guard<std::mutex> guard(_mutex);
		int nfd = createchan(id(),O_MSGS);
		if(nfd < 0)
			VTHROW("createchan() failed");
		c->attach(nfd);

		/* update header */
		gsize_t width,height;
		if(Header::update(c,&width,&height))
			c->update(0,0,width,height);

		is << esc::DevObtain::Response::success(nfd,O_MSGS) << esc::Reply();
	}

	void getKeymap(esc::IPCStream &is) {
		UIClient *c = get(is.fd());
		const Keymap *km = c->keymap() ? c->keymap() : Keymap::getDefault();
		is << errcode_t(0) << esc::CString(km->file().c_str()) << esc::Reply();
	}

	void setKeymap(esc::IPCStream &is) {
		UIClient *c = get(is.fd());
		esc::CStringBuf<MAX_PATH_LEN> path;
		is >> path;

		errcode_t res = -EINVAL;
		Keymap *newMap = Keymap::request(path.str());
		if(newMap) {
			/* we don't need to lock this, because the client is only removed if this
			 * device is closed. since this device is only handled by one thread, it
			 * can't happen now */
			c->keymap(newMap);
			res = 0;
		}
		is << res << esc::Reply();
	}

	void getModes(esc::IPCStream &is) {
		size_t n;
		is >> n;

		esc::Screen::Mode *modes = n == 0 ? NULL : new esc::Screen::Mode[n];
		ssize_t res = ScreenMng::getModes(modes,n);
		is << esc::ValueResponse<size_t>::result(res) << esc::Reply();
		if(n != 0) {
			is << esc::ReplyData(modes,res > 0 ? res * sizeof(*modes) : 0);
			delete[] modes;
		}
	}

	void getMode(esc::IPCStream &is) {
		UIClient *c = get(is.fd());
		if(c->fb()) {
			esc::Screen::Mode mode = c->fb()->mode();
			ScreenMng::adjustMode(&mode);
			is << esc::ValueResponse<esc::Screen::Mode>::success(mode) << esc::Reply();
		}
		else
			is << esc::ValueResponse<esc::Screen::Mode>::error(-EINVAL) << esc::Reply();
	}

	void delegate(esc::IPCStream &is) {
		UIClient *c = get(is.fd());
		esc::DevDelegate::Request r;
		is >> r;

		int res = c->recvFb(r.nfd);
		is << esc::DevDelegate::Response(res) << esc::Reply();
	}

	void setMode(esc::IPCStream &is) {
		UIClient *c = get(is.fd());
		bool swmode;
		int modeid,type;
		is >> modeid >> type >> swmode;

		/* lock that to prevent that we interfere with e.g. the debugger keystroke */
		std::lock_guard<std::mutex> guard(_mutex);
		esc::Screen::Mode mode;
		esc::Screen *scr;
		if(ScreenMng::find(modeid,&mode,&scr)) {
			/* only set this mode if it's the active client */
			c->setMode(type,mode,scr,c->isActive());
			/* update header */
			if(c->isActive()) {
				gsize_t width,height;
				if(Header::update(c,&width,&height))
					c->update(0,0,width,height);
			}
		}
		is << errcode_t(0) << esc::Reply();
	}

	void setCursor(esc::IPCStream &is) {
		UIClient *c = get(is.fd());
		gpos_t x,y;
		int cursor;
		is >> x >> y >> cursor;

		if(c->isActive())
			c->setCursor(x,y,cursor);
	}

	void update(esc::IPCStream &is) {
		UIClient *c = get(is.fd());
		gpos_t x,y;
		gsize_t w,h;
		is >> x >> y >> w >> h;

		if(c->isActive()) {
			y += Header::getHeight(c->type());
			c->update(x,y,w,h);
		}
	}

private:
	std::mutex &_mutex;
};
