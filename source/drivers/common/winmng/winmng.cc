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
#include "window.h"

using namespace esc;

static const int DEF_BPP	= 24;

static sInputThread inputData;

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
		set(MSG_FILE_CLOSE,std::make_memfun(this,&WinMngDevice::close),false);
		set(MSG_DEV_DELEGATE,std::make_memfun(this,&WinMngDevice::delegate),false);
	}

	void create(IPCStream &is) {
		gpos_t x,y;
		gsize_t width,height,titleBarHeight;
		uint style;
		CStringBuf<WinMngEvents::MAX_WINTITLE_LEN> title;
		is >> x >> y >> width >> height >> style >> titleBarHeight >> title;

		errcode_t res = win_create(x,y,width,height,is.fd(),style,titleBarHeight,title.str());

		is << ValueResponse<gwinid_t>::result(res) << Reply();
	}

	void delegate(IPCStream &is) {
		esc::DevDelegate::Request r;
		is >> r;

		int res = win_joinbuf(r.arg,r.nfd);
		is << esc::DevDelegate::Response(res) << esc::Reply();
	}

	void setActive(IPCStream &is) {
		gwinid_t wid;
		is >> wid;

		Window *win = win_get(wid);
		if(win)
			win_setActive(wid,true,input_getMouseX(),input_getMouseY());
	}

	void destroy(IPCStream &is) {
		gwinid_t wid;
		is >> wid;

		if(win_exists(wid))
			win_destroy(wid,input_getMouseX(),input_getMouseY());
	}

	void move(IPCStream &is) {
		gwinid_t wid;
		gpos_t x,y;
		bool finished;
		is >> wid >> x >> y >> finished;

		Window *win = win_get(wid);
		if(win && x < (gpos_t)win_getMode()->width && y < (gpos_t)win_getMode()->height) {
			if(finished)
				win_moveTo(wid,x,y,win->width(),win->height());
			else
				win_previewMove(wid,x,y);
		}
	}

	void resize(IPCStream &is) {
		gwinid_t wid;
		gpos_t x,y;
		gsize_t width,height;
		bool finished;
		is >> wid >> x >> y >> width >> height >> finished;

		if(win_exists(wid)) {
			int evid = win_get(wid)->evfd;
			if(finished) {
				win_resize(wid,x,y,width,height);

				WinMngEvents::Event ev;
				ev.type = WinMngEvents::Event::TYPE_RESIZE;
				ev.wid = wid;
				send(evid,MSG_WIN_EVENT,&ev,sizeof(ev));
			}
			else
				win_previewResize(x,y,width,height);
		}
	}

	void update(IPCStream &is) {
		gwinid_t wid;
		gpos_t x,y;
		gsize_t width,height;
		is >> wid >> x >> y >> width >> height;

		Window *win = win_get(wid);
		if(win != NULL) {
			if((gpos_t)(x + width) > x &&
					(gpos_t)(y + height) > y && x + width <= win->width() &&
					y + height <= win->height()) {
				win_update(wid,x,y,width,height);
			}
			else
				wid = -EINVAL;
		}

		is << ValueResponse<gwinid_t>::result(wid) << Reply();
	}

	void getModes(IPCStream &is) {
		size_t n;
		is >> n;

		std::vector<Screen::Mode> modes = inputData.ui->getModes();
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
		is << ValueResponse<Screen::Mode>::success(*win_getMode()) << Reply();
	}

	void setMode(IPCStream &is) {
		gsize_t width,height;
		gcoldepth_t bpp;
		is >> width >> height >> bpp;

		errcode_t res = win_setMode(width,height,bpp);
		is << res << Reply();
	}

	void close(IPCStream &is) {
		win_destroyWinsOf(is.fd(),input_getMouseX(),input_getMouseY());
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

		win_attach(winid,is.fd());
	}

	void addListener(IPCStream &is) {
		ev_type type;
		is >> type;

		listener_add(is.fd(),type);
	}

	void remListener(IPCStream &is) {
		ev_type type;
		is >> type;

		listener_remove(is.fd(),type);
	}

	void close(IPCStream &is) {
		listener_removeAll(is.fd());
		win_detachAll(is.fd());
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
	inputData.ui = new UI("/dev/uimng");

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
	inputData.uiev = new UIEvents(*inputData.ui);

	/* init window stuff */
	inputData.winmng = path;
	inputData.mode = win_init(windev.id(),inputData.ui,atoi(argv[1]),atoi(argv[2]),DEF_BPP);
	if(inputData.mode < 0)
		error("Setting mode %zu%zu@%d failed",atoi(argv[1]),atoi(argv[2]),DEF_BPP);

	/* start helper threads */
	listener_init(windev.id());
	if(startthread(input_thread,&inputData) < 0)
		error("Unable to start thread for mouse-handler");
	if(startthread(infodev_thread,argv[3]) < 0)
		error("Unable to start thread for the infodev");
	if(startthread(eventThread,&evdev) < 0)
		error("Unable to start thread for the event-channel");

	windev.loop();
	return EXIT_SUCCESS;
}
