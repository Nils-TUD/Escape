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

#include <esc/ipc/clientdevice.h>
#include <esc/proto/ui.h>
#include <esc/proto/winmng.h>
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/keycodes.h>
#include <sys/messages.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <usergroup/usergroup.h>
#include <stdio.h>
#include <stdlib.h>

#include "infodev.h"
#include "input.h"
#include "listener.h"
#include "preview.h"
#include "winlist.h"

using namespace esc;

static const int DEF_BPP	= 24;

static UI *ui;

class WinMngDevice : public Device {
public:
	explicit WinMngDevice(const char *path,mode_t mode)
		: Device(path,mode,DEV_TYPE_SERVICE,DEV_DELEGATE | DEV_CLOSE) {
		set(MSG_WIN_CREATE,std::make_memfun(this,&WinMngDevice::create));
		set(MSG_WIN_SET_ACTIVE,std::make_memfun(this,&WinMngDevice::setActive),false);
		set(MSG_WIN_DESTROY,std::make_memfun(this,&WinMngDevice::destroy));
		set(MSG_WIN_MOVE,std::make_memfun(this,&WinMngDevice::move),false);
		set(MSG_WIN_RESIZE,std::make_memfun(this,&WinMngDevice::resize),false);
		set(MSG_WIN_UPDATE,std::make_memfun(this,&WinMngDevice::update));
		set(MSG_SCR_GETMODES,std::make_memfun(this,&WinMngDevice::getModes));
		set(MSG_SCR_GETMODE,std::make_memfun(this,&WinMngDevice::getMode));
		set(MSG_WIN_SETMODE,std::make_memfun(this,&WinMngDevice::setMode));
		set(MSG_WIN_GETTHEME,std::make_memfun(this,&WinMngDevice::getTheme));
		set(MSG_WIN_SETTHEME,std::make_memfun(this,&WinMngDevice::setTheme));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&WinMngDevice::close),false);
		set(MSG_DEV_DELEGATE,std::make_memfun(this,&WinMngDevice::delegate),false);
	}

	void create(IPCStream &is) {
		gpos_t x,y;
		gsize_t width,height,titleBarHeight;
		uint style;
		CStringBuf<WinMngEvents::MAX_WINTITLE_LEN> title;
		is >> x >> y >> width >> height >> style >> titleBarHeight >> title;

		gui::Rectangle r(x,y,width,height);
		gwinid_t id = WinList::get().add(r,is.fd(),style,titleBarHeight,title.str());

		is << ValueResponse<gwinid_t>::result(id) << Reply();
	}

	void delegate(IPCStream &is) {
		esc::DevDelegate::Request r;
		is >> r;

		int res = WinList::get().joinbuf(r.arg,r.nfd);
		is << esc::DevDelegate::Response(res) << esc::Reply();
	}

	void setActive(IPCStream &is) {
		gwinid_t wid;
		is >> wid;

		WinList::get().setActive(wid,true,true);
	}

	void destroy(IPCStream &is) {
		gwinid_t wid;
		is >> wid;

		WinList::get().remove(wid);
	}

	void move(IPCStream &is) {
		gwinid_t wid;
		gpos_t x,y;
		bool finished;
		is >> wid >> x >> y >> finished;

		WinList::get().moveTo(wid,gui::Pos(x,y),finished);
	}

	void resize(IPCStream &is) {
		gwinid_t wid;
		gpos_t x,y;
		gsize_t width,height;
		bool finished;
		is >> wid >> x >> y >> width >> height >> finished;

		gui::Rectangle r(x,y,width,height);
		WinList::get().resize(wid,r,finished);
	}

	void update(IPCStream &is) {
		gwinid_t wid;
		gpos_t x,y;
		gsize_t width,height;
		is >> wid >> x >> y >> width >> height;

		if((gpos_t)(x + width) > x && (gpos_t)(y + height) > y)
			wid = WinList::get().update(wid,gui::Rectangle(x,y,width,height));
		else
			wid = -EINVAL;

		is << ValueResponse<gwinid_t>::result(wid) << Reply();
	}

	void getModes(IPCStream &is) {
		size_t n;
		is >> n;

		std::vector<Screen::Mode> modes = ui->getModes();
		if(n == 0) {
			size_t count = 0;
			for(auto m = modes.begin(); m != modes.end(); ++m) {
				if(m->type & Screen::MODE_TYPE_GUI)
					count++;
			}
			is << ValueResponse<size_t>::success(count) << Reply();
		}
		else {
			size_t pos = 0;
			Screen::Mode *marray = new Screen::Mode[n];
			for(auto m = modes.begin(); pos < n && m != modes.end(); ++m) {
				if(m->type & Screen::MODE_TYPE_GUI)
					marray[pos++] = *m;
			}
			is << ValueResponse<size_t>::success(pos) << Reply();
			is << ReplyData(marray,pos * sizeof(*marray));
			delete[] marray;
		}
	}

	void getMode(IPCStream &is) {
		is << ValueResponse<Screen::Mode>::success(WinList::get().getMode()) << Reply();
	}

	void setMode(IPCStream &is) {
		gsize_t width,height;
		gcoldepth_t bpp;
		is >> width >> height >> bpp;

		errcode_t res = WinList::get().setMode(gui::Size(width,height),bpp);
		is << res << Reply();
	}

	void getTheme(IPCStream &is) {
		const std::string &nameobj = WinList::get().getTheme();
		is << CString(nameobj.c_str(),nameobj.length()) << Reply();
	}

	void setTheme(IPCStream &is) {
		CStringBuf<64> name;
		is >> name;

		WinList::get().setTheme(name.str());
		is << 0 << Reply();
	}

	void close(IPCStream &is) {
		WinList::get().destroyWinsOf(is.fd());
		Device::close(is);
	}
};

class WinMngEventDevice : public Device {
public:
	explicit WinMngEventDevice(const char *path,mode_t mode)
		: Device(path,mode,DEV_TYPE_SERVICE,DEV_CLOSE) {
		set(MSG_WIN_ATTACH,std::make_memfun(this,&WinMngEventDevice::attach),false);
		set(MSG_WIN_ADDLISTENER,std::make_memfun(this,&WinMngEventDevice::addListener),false);
		set(MSG_WIN_REMLISTENER,std::make_memfun(this,&WinMngEventDevice::remListener),false);
		set(MSG_FILE_CLOSE,std::make_memfun(this,&WinMngEventDevice::close),false);
	}

	void attach(IPCStream &is) {
		gwinid_t winid;
		is >> winid;

		WinList::get().attach(winid,is.fd());
	}

	void addListener(IPCStream &is) {
		Listener::ev_type type;
		is >> type;

		Listener::get().add(is.fd(),type);
	}

	void remListener(IPCStream &is) {
		Listener::ev_type type;
		is >> type;

		Listener::get().remove(is.fd(),type);
	}

	void close(IPCStream &is) {
		Listener::get().removeAll(is.fd());
		WinList::get().detachAll(is.fd());
		Device::close(is);
	}
};

static int eventThread(void *arg) {
	WinMngEventDevice *evdev = (WinMngEventDevice*)arg;
	evdev->bindto(gettid());
	evdev->loop();
	return 0;
}

int main(int argc,char *argv[]) {
	char path[MAX_PATH_LEN];

	if(argc != 4) {
		fprintf(stderr,"Usage: %s <width> <height> <name>\n",argv[0]);
		return 1;
	}

	/* connect to ui-manager */
	ui = new UI("/dev/uimng");

	/* we want to give only users that are in the ui-group access to this window manager */
	int gid = usergroup_nameToId(GROUPS_PATH,argv[3]);
	if(gid < 0)
		printe("Unable to find ui-group '%s'",argv[3]);

	/* create event-device */
	snprintf(path,sizeof(path),"/dev/%s-events",argv[3]);
	print("Creating window-manager-events at %s",path);
	WinMngEventDevice evdev(path,0110);
	if(chown(path,ROOT_UID,gid) < 0)
		printe("Unable to add ui-group to group-list");

	/* create device */
	snprintf(path,sizeof(path),"/dev/%s",argv[3]);
	print("Creating window-manager at %s",path);
	WinMngDevice windev(path,0110);
	if(chown(path,ROOT_UID,gid) < 0)
		printe("Unable to add ui-group to group-list");

	/* open input device and attach */
	UIEvents *uiev = new UIEvents(*ui);

	WinList::create(windev.id(),ui,gui::Size(atoi(argv[1]),atoi(argv[2])),DEF_BPP);

	/* start helper modules */
	Listener::create(windev.id());
	Input::create(uiev);
	InfoDev::create(argv[3]);
	Preview::create();

	if(startthread(eventThread,&evdev) < 0)
		error("Unable to start thread for the event-channel");

	windev.loop();
	return EXIT_SUCCESS;
}
