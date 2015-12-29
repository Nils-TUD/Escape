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

#include <esc/ipc/vtermdevice.h>
#include <esc/proto/file.h>
#include <esc/proto/ui.h>
#include <esc/proto/vterm.h>
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/keycodes.h>
#include <sys/messages.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <sys/thread.h>
#include <usergroup/group.h>
#include <vterm/vtctrl.h>
#include <vterm/vtin.h>
#include <vterm/vtout.h>
#include <assert.h>
#include <errno.h>
#include <memory>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace esc;

class TUIVTermDevice;

static int vtermThread(void *arg);
static int uimInputThread(void *arg);
static int vtInit(int id,const char *name,uint cols,uint rows);
static void vtSetVideoMode(int mode);
static void vtUpdate(void);
static void vtSetCursor(sVTerm *vt);

static std::vector<Screen::Mode> modes;
static FrameBuffer *fb = NULL;
static sVTerm vterm;
static bool run = true;
static TUIVTermDevice *vtdev;

class TUIVTermDevice : public VTermDevice {
public:
	explicit TUIVTermDevice(const char *name,mode_t mode,sVTerm *vt) : VTermDevice(name,mode,vt) {
		set(MSG_SCR_GETMODE,std::make_memfun(this,&TUIVTermDevice::getMode));
		set(MSG_SCR_GETMODES,std::make_memfun(this,&TUIVTermDevice::getModes));
		set(MSG_UIM_GETKEYMAP,std::make_memfun(this,&TUIVTermDevice::getKeymap));
		set(MSG_UIM_SETKEYMAP,std::make_memfun(this,&TUIVTermDevice::setKeymap));
	}

	virtual void setVideoMode(int mode) {
		vtSetVideoMode(mode);
	}

	virtual void update() {
		vtUpdate();
	}

	void getMode(IPCStream &is) {
		Screen::Mode mode = vterm.ui->getMode();
		is << ValueResponse<Screen::Mode>::success(mode) << Reply();
	}

	void getModes(IPCStream &is) {
		size_t n;
		is >> n;

		is << ValueResponse<size_t>::success(modes.size()) << Reply();
		if(n)
			is << ReplyData(modes.begin(),sizeof(Screen::Mode) * modes.size());
	}

	void getKeymap(IPCStream &is) {
		std::string keymap = vterm.ui->getKeymap();
		is << errcode_t(0) << CString(keymap.c_str(),keymap.length()) << Reply();
	}

	void setKeymap(IPCStream &is) {
		CStringBuf<MAX_PATH_LEN> path;
		is >> path;
		vterm.ui->setKeymap(std::string(path.str()));
		is << errcode_t(0) << Reply();
	}
};

static void sigterm(int) {
	run = false;
	vtdev->stop();
}

int main(int argc,char **argv) {
	if(argc < 4) {
		fprintf(stderr,"Usage: %s <cols> <rows> <name>\n",argv[0]);
		return EXIT_FAILURE;
	}

	if(signal(SIGTERM,sigterm) == SIG_ERR)
		error("Unable to set SIGTERM handler");

	char path[MAX_PATH_LEN];
	snprintf(path,sizeof(path),"/dev/%s",argv[3]);

	print("Creating vterm at %s",path);

	/* create device */
	vtdev = new TUIVTermDevice(path,0770,&vterm);

	/* we want to give only users that are in the ui-group access to this vterm */
	size_t gcount;
	sGroup *groups = group_parseFromFile(GROUPS_PATH,&gcount);
	sGroup *uigrp = group_getByName(groups,argv[3]);
	if(uigrp != NULL) {
		if(chown(path,ROOT_UID,uigrp->gid) < 0)
			printe("Unable to add ui-group to group-list");
	}
	else
		printe("Unable to find ui-group '%s'",argv[3]);
	group_free(groups);

	print("Getting video mode for %s columns, %s rows",argv[1],argv[2]);

	/* init vterms */
	int modeid = vtInit(vtdev->id(),argv[3],atoi(argv[1]),atoi(argv[2]));
	if(modeid < 0)
		error("Unable to init vterms");

	/* start thread to read input-events from uimanager */
	int tid;
	if((tid = startthread(uimInputThread,&modeid)) < 0)
		error("Unable to start thread for vterm %s",path);

	/* handle device here */
	vtermThread(vtdev);

	/* clean up */
	join(tid);
	delete fb;
	delete vterm.ui;
	delete vterm.speaker;
	delete vtdev;
	vtctrl_destroy(&vterm);
	return EXIT_SUCCESS;
}

static void processKeyEvent(const UIEvents::Event &ev) {
	if((ev.d.keyb.modifier & (STATE_BREAK | STATE_CTRL | STATE_SHIFT)) == (STATE_CTRL | STATE_SHIFT)) {
		if(ev.d.keyb.keycode == VK_C) {
			Clipboard::Stream s = Clipboard::writer();
			std::lock_guard<std::mutex> guard(*vterm.mutex);
			vtctrl_getSelection(&vterm,s);
			return;
		}
		else if(ev.d.keyb.keycode == VK_V) {
			Clipboard::Stream s = Clipboard::reader();
			std::lock_guard<std::mutex> guard(*vterm.mutex);
			while(!s.eof()) {
				char buf[256];
				size_t len = s.read(buf,sizeof(buf) - 1);
				buf[len] = '\0';
				vtin_input(&vterm,buf,len);
			}
			return;
		}
	}

	std::lock_guard<std::mutex> guard(*vterm.mutex);
	vtin_handleKey(&vterm,ev.d.keyb.keycode,ev.d.keyb.modifier,ev.d.keyb.character);
	vtUpdate();
}

static void processMouseEvent(const UIEvents::Event &ev) {
	static int mx = 0, my = 0;
	mx += ev.d.mouse.x;
	my -= ev.d.mouse.y;

	int px_per_col = fb->mode().width / vterm.cols;
	int px_per_row = fb->mode().height / vterm.rows;

	// make sure it's within the bounds
	mx = MAX(0,MIN(mx,(int)fb->mode().width - px_per_col - 1));
	my = MAX(0,MIN(my,(int)fb->mode().height - px_per_row - 1));

	// set cursor
	std::lock_guard<std::mutex> guard(*vterm.mutex);
	vtin_handleMouse(&vterm,mx / px_per_col,my / px_per_row,
		WHEEL_SCROLL_FACTOR * ev.d.mouse.z,ev.d.mouse.buttons & 1);
	vtUpdate();
}

static int uimInputThread(void *arg) {
	int modeid = *(int*)arg;
	/* open uimng's input device */
	UIEvents uiev(*vterm.ui);

	if(signal(SIGTERM,sigterm) == SIG_ERR)
		error("Unable to set SIGTERM handler");

	{
		std::lock_guard<std::mutex> guard(*vterm.mutex);
		/* set video mode */
		vtSetVideoMode(modeid);
		/* now we're the active client. update screen */
		vtctrl_markScrDirty(&vterm);
		vtUpdate();
	}

	/* read from uimanager and handle the keys */
	while(run) {
		UIEvents::Event ev;
		uiev.is() >> ReceiveData(&ev,sizeof(ev),false);
		if(!run)
			break;

		if(ev.type == UIEvents::Event::TYPE_KEYBOARD)
			processKeyEvent(ev);
		else if(ev.type == UIEvents::Event::TYPE_MOUSE)
			processMouseEvent(ev);
		vtdev->checkPending();
	}
	return 0;
}

static int vtermThread(void *arg) {
	TUIVTermDevice *dev = (TUIVTermDevice*)arg;
	dev->loop();
	return 0;
}

static int vtInit(int id,const char *name,uint cols,uint rows) {
	vterm.ui = new UI("/dev/uimng");
	modes = vterm.ui->getModes();

	/* find a suitable mode */
	Screen::Mode mode = vterm.ui->findTextModeIn(modes,cols,rows);

	/* open speaker */
	struct stat info;
	if(stat("/dev/speaker",&info) >= 0) {
		try {
			vterm.speaker = new Speaker("/dev/speaker");
		}
		catch(const std::exception &e) {
			/* ignore errors here. in this case we simply don't use it */
			printe("%s",e.what());
		}
	}

	vterm.index = 0;
	vterm.sid = id;
	vterm.defForeground = LIGHTGRAY;
	vterm.defBackground = BLACK;
	vterm.setCursor = vtSetCursor;
	memcpy(vterm.name,name,MAX_VT_NAME_LEN + 1);
	if(!vtctrl_init(&vterm,&mode))
		return -ENOMEM;
	return mode.id;
}

static void vtUpdate(void) {
	if(!fb)
		return;

	/* if we should scroll, mark the whole screen (without title) as dirty */
	if(vterm.upScroll != 0) {
		vterm.upCol = 0;
		vterm.upRow = MIN(vterm.upRow,0);
		vterm.upHeight = vterm.rows - vterm.upRow;
		vterm.upWidth = vterm.cols;
	}

	if(vterm.upWidth > 0) {
		/* update content */
		assert(vterm.upCol + vterm.upWidth <= vterm.cols);
		assert(vterm.upRow + vterm.upHeight <= vterm.rows);
		size_t offset = vterm.upRow * vterm.cols * 2 + vterm.upCol * 2;
		char **lines = vterm.lines + vterm.firstVisLine + vterm.upRow;
		for(size_t i = 0; i < vterm.upHeight; ++i) {
			memcpy(fb->addr() + offset + i * vterm.cols * 2,
				lines[i] + vterm.upCol * 2,vterm.upWidth * 2);
		}

		vterm.ui->update(vterm.upCol,vterm.upRow,vterm.upWidth,vterm.upHeight);
	}
	vtSetCursor(&vterm);

	/* all synchronized now */
	vterm.upCol = vterm.cols;
	vterm.upRow = vterm.rows;
	vterm.upWidth = 0;
	vterm.upHeight = 0;
	vterm.upScroll = 0;
}

static void vtSetVideoMode(int mode) {
	for(auto it = modes.begin(); it != modes.end(); ++it) {
		if(it->id == mode) {
			print("Setting video mode %d: %dx%dx%d",mode,it->width,it->height,it->bitsPerPixel);

			/* rename old shm as a backup */
			FrameBuffer *fbtmp = fb;
			std::string tmpname;
			if(fbtmp) {
				tmpname = fbtmp->filename() + "-tmp";
				print("Renaming old framebuffer to %s",tmpname.c_str());
				fbtmp->rename(tmpname);
			}

			/* try to set new mode */
			print("Creating new framebuffer named %s",vterm.name);
			try {
				std::unique_ptr<FrameBuffer> nfb(
					new FrameBuffer(*it,vterm.name,Screen::MODE_TYPE_TUI,0644));
				vterm.ui->setMode(Screen::MODE_TYPE_TUI,it->id,vterm.name,true);
				fb = nfb.release();
			}
			catch(const default_error &e) {
				fb = fbtmp;
				if(fb)
					fb->rename(vterm.name);
				throw;
			}

			/* resize vterm if necessary */
			if(vterm.cols != it->cols || vterm.rows != it->rows) {
				if(!vtctrl_resize(&vterm,it->cols,it->rows)) {
					delete fb;
					fb = fbtmp;
					if(fb) {
						fb->rename(vterm.name);
						vterm.ui->setMode(Screen::MODE_TYPE_TUI,fb->mode().id,vterm.name,true);
					}
					VTHROWE("vtctrl_resize(" << it->cols << "," << it->rows << ")",-ENOMEM);
				}
			}

			if(fbtmp) {
				print("Deleting old framebuffer");
				delete fbtmp;
			}
			return;
		}
	}
	VTHROW("Unable to find mode " << mode);
}

static void vtSetCursor(sVTerm *vt) {
	gpos_t x,y;
	if(vt->upScroll != 0 || vt->col != vt->lastCol || vt->row != vt->lastRow) {
		/* draw no cursor if it's not visible by setting it to an invalid location */
		if(vt->firstVisLine + vt->rows <= vt->currLine + vt->row) {
			x = vt->cols;
			y = vt->rows;
		}
		else {
			x = vt->col;
			y = vt->row;
		}
		vt->ui->setCursor(x,y);
		vt->lastCol = x;
		vt->lastRow = y;
	}
}
