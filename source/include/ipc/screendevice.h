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
#include <esc/rect.h>
#include <ipc/ipcstream.h>
#include <ipc/clientdevice.h>
#include <ipc/proto/screen.h>

#define DIFF(a,b)				((a) > (b) ? ((a) - (b)) : ((b) - (a)))

namespace ipc {

class ScreenClient : public Client {
public:
	explicit ScreenClient(int fd) : Client(fd), mode(), fb() {
	}

	int type() const {
		return fb ? fb->mode().type : -1;
	}

	Screen::Mode *mode;
	FrameBuffer *fb;
};

template<typename C = ScreenClient>
class ScreenDevice : public ClientDevice<C> {
	static const size_t MAX_REQC	= 30;

	struct NewCursor {
		gpos_t x,y;
		int cursor;
		ScreenClient *client;
	};
	struct Rectangle {
		sRectangle r;
		ScreenClient *client;
	};

public:
	explicit ScreenDevice(const std::vector<Screen::Mode> &modes,const char *name,mode_t mode)
		: ClientDevice<C>(name,mode,DEV_TYPE_BLOCK,DEV_OPEN | DEV_CLOSE),
		  _modes(modes), _rects(), _newCursor(), _reqc() {
		this->set(MSG_SCR_GETMODE,std::make_memfun(this,&ScreenDevice::getMode));
		this->set(MSG_SCR_SETMODE,std::make_memfun(this,&ScreenDevice::setMode));
		this->set(MSG_SCR_GETMODES,std::make_memfun(this,&ScreenDevice::getModes));
		this->set(MSG_SCR_SETCURSOR,std::make_memfun(this,&ScreenDevice::setCursor),false);
		this->set(MSG_SCR_UPDATE,std::make_memfun(this,&ScreenDevice::update),false);
		this->set(MSG_DEV_CLOSE,std::make_memfun(this,&ScreenDevice::close),false);
	}

	virtual void setScreenMode(C *c,const char *shm,Screen::Mode *mode,int type,bool sw) = 0;
	virtual void setScreenCursor(C *c,gpos_t x,gpos_t y,int cursor) = 0;
	virtual void updateScreen(C *c,gpos_t x,gpos_t y,gsize_t width,gsize_t height) = 0;

	void loop() {
		char buf[IPC_DEF_SIZE];
		while(!this->isStopped()) {
			msgid_t mid;
			int fd = getwork(this->id(),&mid,buf,sizeof(buf),isDirty() ? GW_NOBLOCK : 0);
			if(EXPECT_FALSE(fd < 0 || _reqc >= MAX_REQC)) {
				/* just log that it failed. maybe a client has sent a message that was too big */
				if(fd != -EINTR && fd != -ENOCLIENT)
					printe("getwork failed");
				_reqc = 0;
				performUpdate();
				continue;
			}

			IPCStream is(fd,buf,sizeof(buf));
			this->handleMsg(mid,is);
		}
	}

private:
	void getMode(IPCStream &is) {
		C *c = this->get(is.fd());

		int res = c->mode ? 0 : -EINVAL;
		is << res;
		if(res == 0)
			is << *c->mode;
		is << Send(MSG_DEF_RESPONSE);
	}

	void setMode(IPCStream &is) {
		C *c = this->get(is.fd());
		int modeno,type;
		bool switchMode;
		CStringBuf<MAX_PATH_LEN> path;
		is >> modeno >> type >> switchMode >> path;

		for(auto it = _modes.begin(); it != _modes.end(); ++it) {
			if(modeno == it->id) {
				setScreenMode(c,path.str(),&*it,type,switchMode);
				break;
			}
		}
		is << 0 << Send(MSG_DEF_RESPONSE);
	}

	void getModes(IPCStream &is) {
		size_t count;
		is >> count;

		if(count == 0)
			is << _modes.size() << Send(MSG_DEF_RESPONSE);
		else if(count > _modes.size())
			is << static_cast<ssize_t>(-EINVAL) << Send(MSG_DEF_RESPONSE);
		else {
			is << count << Send(MSG_DEF_RESPONSE);
			is << SendData(MSG_DEF_RESPONSE,_modes.begin(),count * sizeof(Screen::Mode));
		}
	}

	void setCursor(IPCStream &is) {
		C *c = this->get(is.fd());
		if(c->mode) {
			gpos_t x,y;
			int cursor;
			is >> x >> y >> cursor;
			addCursor(c,x,y,cursor);
		}
	}

	void update(IPCStream &is) {
		C *c = this->get(is.fd());
		gpos_t x,y;
		gsize_t width,height;
		if(c->mode) {
			is >> x >> y >> width >> height;
			addUpdate(c,x,y,width,height);
		}
	}

	void close(IPCStream &is) {
		C *c = this->get(is.fd());
		/* better perform outstanding updates to not access a deleted client */
		performUpdate();
		setScreenMode(c,"",NULL,ipc::Screen::MODE_TYPE_TUI,false);
		ClientDevice<C>::close(is);
	}

	bool isDirty() const {
		return _rects.size() > 0 || _newCursor.client;
	}

	void addUpdate(C *cli,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		static size_t mergeTolerance[] = {
			/* 0 = unused */	0,
			/* 1 = tui */		10,
			/* 2 = gui */		40
		};

		/* first check if we can merge this rectangle with another one.
		 * maybe we have even got exactly this rectangle... */
		bool present = false;
		size_t tolerance = mergeTolerance[cli->type()];
		for(auto r = _rects.begin(); r != _rects.end(); ++r) {
			if(r->client == cli &&
				(size_t)DIFF(r->r.x,x) < tolerance && (size_t)DIFF(r->r.y,y) < tolerance &&
				DIFF(r->r.width,width) < tolerance && DIFF(r->r.height,height) < tolerance) {
				/* mergable, so do it */
				rectAddTo(&r->r,x,y,width,height);
				present = true;
				break;
			}
		}
		/* if not present yet, add it */
		if(!present) {
			Rectangle r;
			r.r.x = x;
			r.r.y = y;
			r.r.width = width;
			r.r.height = height;
			r.client = cli;
			_rects.push_back(r);
		}
	}

	void addCursor(C *cli,gpos_t x,gpos_t y,int cursor) {
		if(_newCursor.client == cli) {
			_newCursor.x = x;
			_newCursor.y = y;
			_newCursor.cursor = cursor;
		}
		else
			setScreenCursor(cli,x,y,cursor);
	}

	void performUpdate() {
		for(auto r = _rects.begin(); r != _rects.end(); ++r) {
			try {
				updateScreen(static_cast<C*>(r->client),r->r.x,r->r.y,r->r.width,r->r.height);
			}
			catch(const std::exception &e) {
				printe("%s",e.what());
			}
		}
		_rects.clear();

		if(_newCursor.client) {
			try {
				setScreenCursor(static_cast<C*>(_newCursor.client),
					_newCursor.x,_newCursor.y,_newCursor.cursor);
			}
			catch(const std::exception &e) {
				printe("%s",e.what());
			}
			_newCursor.client = NULL;
		}
	}

	std::vector<Screen::Mode> _modes;
	std::vector<Rectangle> _rects;
	NewCursor _newCursor;
	uint _reqc;
};

}

#undef DIFF
