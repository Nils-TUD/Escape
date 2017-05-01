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
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

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
		: Device(path,mode,DEV_TYPE_SERVICE,DEV_READ | DEV_DELEGATE | DEV_CLOSE) {
		set(MSG_FILE_READ,std::make_memfun(this,&WinMngDevice::read));
		set(MSG_WIN_CREATE,std::make_memfun(this,&WinMngDevice::create));
		set(MSG_WIN_SET_ACTIVE,std::make_memfun(this,&WinMngDevice::setActive),false);
		set(MSG_WIN_DESTROY,std::make_memfun(this,&WinMngDevice::destroy));
		set(MSG_WIN_MOVE,std::make_memfun(this,&WinMngDevice::move),false);
		set(MSG_WIN_RESIZE,std::make_memfun(this,&WinMngDevice::resize),false);
		set(MSG_WIN_UPDATE,std::make_memfun(this,&WinMngDevice::update));
		set(MSG_SCR_GETMODES,std::make_memfun(this,&WinMngDevice::getModes));
		set(MSG_SCR_GETMODE,std::make_memfun(this,&WinMngDevice::getMode));
		set(MSG_UIM_SETMODE,std::make_memfun(this,&WinMngDevice::setMode));
		set(MSG_WIN_GETTHEME,std::make_memfun(this,&WinMngDevice::getTheme));
		set(MSG_WIN_SETTHEME,std::make_memfun(this,&WinMngDevice::setTheme));
		set(MSG_UIM_GETKEYMAP,std::make_memfun(this,&WinMngDevice::getKeymap));
		set(MSG_UIM_SETKEYMAP,std::make_memfun(this,&WinMngDevice::setKeymap));
		set(MSG_FILE_CLOSE,std::make_memfun(this,&WinMngDevice::close),false);
		set(MSG_DEV_DELEGATE,std::make_memfun(this,&WinMngDevice::delegate),false);
	}

	void read(IPCStream &is) {
		esc::FileRead::Request r;
		is >> r;
		if(r.offset + (off_t)r.count < r.offset)
			VTHROWE("Invalid offset/count (" << r.offset << "," << r.count << ")",-EINVAL);

		esc::OStringStream os;
		WinList::get().print(os);

		ssize_t res = 0;
		if(r.offset >= os.str().length())
			res = 0;
		else if(r.offset + r.count > os.str().length())
			res = os.str().length() - r.offset;
		else
			res = r.count;

		is << FileRead::Response::result(res) << Reply();
		if(res)
			is << ReplyData(os.str().c_str(),res);
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
		int mode;
		is >> mode;

		errcode_t res = WinList::get().setMode(mode);
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

	void getKeymap(IPCStream &is) {
		std::string keymap = ui->getKeymap();
		is << errcode_t(0) << CString(keymap.c_str(),keymap.length()) << Reply();
	}

	void setKeymap(IPCStream &is) {
		CStringBuf<MAX_PATH_LEN> path;
		is >> path;
		ui->setKeymap(std::string(path.str()));
		is << errcode_t(0) << Reply();
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

static WinMngEventDevice *evdev;
static WinMngDevice *windev;
static volatile bool run = true;

static void sighdl(int) {
	evdev->stop();
	windev->stop();
	run = false;
	signal(SIGINT,sighdl);
}

static int eventThread(void *) {
	if(signal(SIGINT,sighdl) == SIG_ERR)
		error("Unable to set signal handler");

	evdev->bindto(gettid());
	evdev->loop();
	return 0;
}

static int inputThread(void *) {
	if(signal(SIGINT,sighdl) == SIG_ERR)
		error("Unable to set signal handler");

	/* read from uimanager and handle the keys */
	try {
		Input &in = Input::get();
		while(run) {
			esc::UIEvents::Event ev;
			in.events() >> ev;

			switch(ev.type) {
				case esc::UIEvents::Event::TYPE_KEYBOARD:
					in.handleKbMessage(&ev);
					break;

				case esc::UIEvents::Event::TYPE_MOUSE:
					in.handleMouseMessage(&ev);
					break;

				default:
					break;
			}
		}
	}
	catch(const std::exception &e) {
		fprintf(stderr,"Input thread failed: %s\n",e.what());
		kill(getpid(),SIGINT);
	}
	return 0;
}

int main(int argc,char *argv[]) {
	char path[MAX_PATH_LEN];

	if(argc != 4) {
		fprintf(stderr,"Usage: %s <width> <height> <path>\n",argv[0]);
		return 1;
	}

	/* connect to ui-manager */
	ui = new UI("/dev/uimng");

	/* create event-device */
	snprintf(path,sizeof(path),"%s-events",argv[3]);
	print("Creating window-manager-events at %s",path);
	evdev = new WinMngEventDevice(path,0110);

	/* create device */
	print("Creating window-manager at %s",argv[3]);
	windev = new WinMngDevice(argv[3],0550);

	if(signal(SIGINT,sighdl) == SIG_ERR || signal(SIGTERM,sighdl) == SIG_ERR)
		error("Unable to set signal handler");

	/* open input device and attach */
	UIEvents *uiev = new UIEvents(*ui);

	esc::Screen::Mode mode = ui->findGraphicsMode(atoi(argv[1]),atoi(argv[2]),DEF_BPP);
	WinList::create(windev->id(),ui,mode.id);

	/* start helper modules */
	Listener::create(windev->id());
	Input::create(uiev);
	Preview::create();

	/* start threads */
	if(startthread(inputThread,NULL) < 0)
		error("Unable to start input thread");
	if(startthread(eventThread,&evdev) < 0)
		error("Unable to start thread for the event-channel");

	windev->loop();

	/* stop other threads */
	kill(getpid(),SIGINT);
	join(0);

	delete uiev;
	delete windev;
	delete evdev;
	delete ui;
	return EXIT_SUCCESS;
}
