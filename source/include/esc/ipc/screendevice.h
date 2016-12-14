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
#include <esc/ipc/ipcstream.h>
#include <esc/proto/screen.h>
#include <gui/graphics/rectangle.h>
#include <sys/common.h>

namespace esc {

/**
 * The Client for the ScreenDevice
 */
class ScreenClient : public Client {
public:
	explicit ScreenClient(int f) : Client(f), nfd(-1), mode(), fb() {
	}

	int type() const {
		return fb ? fb->mode().type : -1;
	}

	int nfd;
	Screen::Mode *mode;
	FrameBuffer *fb;
};

/**
 * The base-class for all screen-devices which handles most of the operations except the actual
 * mode setting, updating and cursor painting.
 */
template<typename C = ScreenClient>
class ScreenDevice : public ClientDevice<C> {
	static const size_t MAX_REQC	= 30;

	struct NewCursor {
		gpos_t x,y;
		int cursor;
		ScreenClient *client;
	};
	struct Rectangle {
		gui::Rectangle r;
		ScreenClient *client;
	};

public:
	/**
	 * Creates the device at given path.
	 *
	 * @param modes the supported modes
	 * @param path the path
	 * @param mode the permissions to set
	 */
	explicit ScreenDevice(const std::vector<Screen::Mode> &modes,const char *path,mode_t mode)
		: ClientDevice<C>(path,mode,DEV_TYPE_SERVICE,DEV_OPEN | DEV_DELEGATE | DEV_CLOSE),
		  _modes(modes), _rects(), _newCursor(), _reqc() {
		this->set(MSG_SCR_GETMODE,std::make_memfun(this,&ScreenDevice::getMode));
		this->set(MSG_SCR_SETMODE,std::make_memfun(this,&ScreenDevice::setMode));
		this->set(MSG_SCR_GETMODES,std::make_memfun(this,&ScreenDevice::getModes));
		this->set(MSG_SCR_SETCURSOR,std::make_memfun(this,&ScreenDevice::setCursor),false);
		this->set(MSG_SCR_UPDATE,std::make_memfun(this,&ScreenDevice::update),false);
		this->set(MSG_FILE_CLOSE,std::make_memfun(this,&ScreenDevice::close),false);
		this->set(MSG_DEV_DELEGATE,std::make_memfun(this,&ScreenDevice::delegate),false);
	}

	/**
	 * Sets the screen mode. Has to be implemented by the subclass.
	 *
	 * @param c the client
	 * @param mode the mode to set
	 * @param type the mode-type
	 * @param sw whether to switch to that mode
	 * @throws if the operation failed
	 */
	virtual void setScreenMode(C *c,Screen::Mode *mode,int type,bool sw) = 0;

	/**
	 * Sets the cursor. Has to be implemented by the subclass.
	 *
	 * @param c the client
	 * @param x the x-position
	 * @param y the y-position
	 * @param cursor the cursor shape
	 * @throws if the operation failed
	 */
	virtual void setScreenCursor(C *c,gpos_t x,gpos_t y,int cursor) = 0;

	/**
	 * Updates the given rectangle. Has to be implemented by the subclass.
	 *
	 * @param c the client
	 * @param x the x-position
	 * @param y the y-position
	 * @param width the width
	 * @param height the height
	 */
	virtual void updateScreen(C *c,gpos_t x,gpos_t y,gsize_t width,gsize_t height) = 0;

	/**
	 * Executes the device-loop.
	 */
	void loop() {
		ulong buf[IPC_DEF_SIZE / sizeof(ulong)];
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

			IPCStream is(fd,buf,sizeof(buf),mid);
			this->handleMsg(mid,is);
		}
	}

private:
	void getMode(IPCStream &is) {
		C *c = (*this)[is.fd()];

		int res = c->mode ? 0 : -EINVAL;
		if(res == 0)
			is << ValueResponse<Screen::Mode>::success(*c->mode) << Reply();
		else
			is << ValueResponse<Screen::Mode>::error(res) << Reply();
	}

	void delegate(IPCStream &is) {
		C *c = (*this)[is.fd()];
		DevDelegate::Request r;
		is >> r;

		struct stat info;
		int res;
		if((res = fstat(r.nfd,&info)) < 0) {
			is << DevDelegate::Response(res) << Reply();
			return;
		}
		if(c->nfd != -1 || !S_ISREG(info.st_mode)) {
			is << DevDelegate::Response(-EINVAL) << Reply();
			return;
		}

		c->nfd = r.nfd;
		is << DevDelegate::Response(0) << Reply();
	}

	void setMode(IPCStream &is) {
		C *c = (*this)[is.fd()];
		int modeno,type;
		bool switchMode;
		is >> modeno >> type >> switchMode;

		for(auto it = _modes.begin(); it != _modes.end(); ++it) {
			if(modeno == it->id) {
				setScreenMode(c,&*it,type,switchMode);
				c->nfd = -1;
				break;
			}
		}
		is << errcode_t(0) << Reply();
	}

	void getModes(IPCStream &is) {
		size_t count;
		is >> count;

		if(count == 0)
			is << ValueResponse<size_t>::success(_modes.size()) << Reply();
		else if(count > _modes.size())
			is << ValueResponse<size_t>::error(-EINVAL) << Reply();
		else {
			is << ValueResponse<size_t>::success(count) << Reply();
			is << ReplyData(_modes.begin(),count * sizeof(Screen::Mode));
		}
	}

	void setCursor(IPCStream &is) {
		C *c = (*this)[is.fd()];
		if(c->mode) {
			gpos_t x,y;
			int cursor;
			is >> x >> y >> cursor;
			addCursor(c,x,y,cursor);
		}
	}

	void update(IPCStream &is) {
		C *c = (*this)[is.fd()];
		gpos_t x,y;
		gsize_t width,height;
		if(c->mode) {
			is >> x >> y >> width >> height;
			addUpdate(c,x,y,width,height);
		}
	}

	void close(IPCStream &is) {
		C *c = (*this)[is.fd()];
		/* better perform outstanding updates to not access a deleted client */
		performUpdate();
		c->nfd = -1;
		setScreenMode(c,NULL,esc::Screen::MODE_TYPE_TUI,false);
		ClientDevice<C>::close(is);
	}

	bool isDirty() const {
		return _rects.size() > 0 || _newCursor.client;
	}

	template<typename T>
	static T difference(T a,T b) {
		return a > b ? a - b : b - a;
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
				difference(r->r.x(),x) < tolerance &&
				difference(r->r.y(),y) < tolerance &&
				difference(r->r.width(),width) < tolerance &&
				difference(r->r.height(),height) < tolerance) {
				/* mergable, so do it */
				r->r = gui::unify(r->r,gui::Rectangle(x,y,width,height));
				present = true;
				break;
			}
		}
		/* if not present yet, add it */
		if(!present) {
			Rectangle r;
			r.r = gui::Rectangle(x,y,width,height);
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
				updateScreen(static_cast<C*>(r->client),r->r.x(),r->r.y(),r->r.width(),r->r.height());
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
