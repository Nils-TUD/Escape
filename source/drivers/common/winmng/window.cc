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

#include <esc/proto/winmng.h>
#include <gui/graphics/rectangle.h>
#include <sys/common.h>
#include <sys/debug.h>
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/proc.h>
#include <assert.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "input.h"
#include "listener.h"
#include "preview.h"
#include "window.h"

static void win_createBuf(Window *win,gwinid_t id,gsize_t width,gsize_t height,const char *winmng);
static void win_destroyBuf(Window *win);
static gwinid_t win_getTop(void);
static bool win_validateRect(gui::Rectangle &r);
static void win_repaint(const gui::Rectangle &r,Window *win,gpos_t z);
static void win_sendActive(gwinid_t id,bool isActive,gpos_t mouseX,gpos_t mouseY);
static void win_getRepaintRegions(std::vector<WinRect> &list,gwinid_t id,Window *win,gpos_t z,
	const gui::Rectangle &r);
static void win_clearRegion(char *mem,const gui::Rectangle &r);
static void win_copyRegion(char *mem,const gui::Rectangle &r,gwinid_t id);
static void win_notifyWinCreate(gwinid_t id,const char *title);
static void win_notifyWinActive(gwinid_t id);
static void win_notifyWinDestroy(gwinid_t id);

static esc::UI *ui;
static int drvId;

static esc::Screen::Mode mode;
static esc::FrameBuffer *fb;

static size_t activeWindow = WINDOW_COUNT;
static size_t topWindow = WINDOW_COUNT;
static Window windows[WINDOW_COUNT];

static size_t pixelSize() {
	return mode.bitsPerPixel / 8;
}

int win_init(int sid,esc::UI *uiobj,gsize_t width,gsize_t height,gcoldepth_t bpp,const char *shmname) {
	drvId = sid;
	ui = uiobj;
	srand(time(NULL));

	return win_setMode(width,height,bpp,shmname);
}

const esc::Screen::Mode *win_getMode(void) {
	return &mode;
}

void win_setCursor(gpos_t x,gpos_t y,uint cursor) {
	ui->setCursor(x,y,cursor);
}

static void win_createBuf(Window *win,gwinid_t id,gsize_t width,gsize_t height,const char *winmng) {
	char name[32];
	snprintf(name,sizeof(name),"%s-win%d",winmng,id);
	esc::Screen::Mode winMode = mode;
	winMode.guiHeaderSize = winMode.tuiHeaderSize = 0;
	winMode.width = width;
	winMode.height = height;
	win->fb = new esc::FrameBuffer(winMode,name,esc::Screen::MODE_TYPE_GUI,0666);
	memclear(win->fb->addr(),width * height * (mode.bitsPerPixel / 8));
}

static void win_destroyBuf(Window *win) {
	delete win->fb;
	win->fb = NULL;
}

int win_setMode(gsize_t width,gsize_t height,gcoldepth_t bpp,const char *shmname) {
	print("Getting video mode for %zux%zux%u",width,height,bpp);

	esc::Screen::Mode newmode = ui->findGraphicsMode(width,height,bpp);

	/* first destroy the old one because we use the same shm-name again */
	delete fb;

	try {
		print("Creating new framebuffer named %s",shmname);
		std::unique_ptr<esc::FrameBuffer> newfb(
			new esc::FrameBuffer(newmode,shmname,esc::Screen::MODE_TYPE_GUI,0644));
		print("Setting mode %d: %zux%zux%u",newmode.id,newmode.width,newmode.height,newmode.bitsPerPixel);
		ui->setMode(esc::Screen::MODE_TYPE_GUI,newmode.id,shmname,true);

		mode = newmode;
		fb = newfb.release();

		/* recreate window buffers */
		for(size_t i = 0; i < WINDOW_COUNT; i++) {
			if(windows[i].id != WINID_UNUSED) {
				win_destroyBuf(windows + i);
				win_createBuf(windows + i,i,windows[i].width(),windows[i].height(),shmname);

				/* let the window reset everything, i.e. connect to new buffer, repaint, ... */
				esc::WinMngEvents::Event ev;
				ev.type = esc::WinMngEvents::Event::TYPE_RESET;
				ev.wid = windows[i].id;
				send(windows[i].evfd,MSG_WIN_EVENT,&ev,sizeof(ev));
			}
		}
	}
	catch(const std::exception &e) {
		printe("%s",e.what());
		print("Restoring old framebuffer and mode");
		fb = new esc::FrameBuffer(mode,shmname,esc::Screen::MODE_TYPE_GUI,0644);
		ui->setMode(esc::Screen::MODE_TYPE_GUI,mode.id,shmname,true);
		/* we have to repaint everything */
		for(size_t i = 0; i < WINDOW_COUNT; i++) {
			if(windows[i].id != WINID_UNUSED)
				win_update(i,0,0,windows[i].width(),windows[i].height());
		}
		throw;
	}
	return mode.id;
}

gwinid_t win_create(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int owner,uint style,
		gsize_t titleBarHeight,const char *title,const char *winmng) {
	gwinid_t i;
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(windows[i].id == WINID_UNUSED) {
			win_createBuf(windows + i,i,width,height,winmng);

			windows[i].id = i;
			windows[i].setPos(x,y);
			windows[i].setSize(width,height);
			if(style == WIN_STYLE_DESKTOP)
				windows[i].z = 0;
			else {
				gwinid_t top = win_getTop();
				if(top != WINID_UNUSED)
					windows[i].z = windows[top].z + 1;
				else
					windows[i].z = 1;
			}
			windows[i].owner = owner;
			windows[i].evfd = -1;
			windows[i].style = style;
			windows[i].titleBarHeight = titleBarHeight;
			windows[i].ready = false;
			print("Created window %d: %s @ (%d,%d,%d) with size %zux%zu",
				i,title,x,y,windows[i].z,width,height);
			win_notifyWinCreate(i,title);
			return i;
		}
	}
	return WINID_UNUSED;
}

void win_attach(gwinid_t winid,int fd) {
	if(win_exists(winid) && windows[winid].evfd == -1) {
		windows[winid].evfd = fd;
		if(activeWindow == WINDOW_COUNT || windows[winid].style != WIN_STYLE_DESKTOP)
			win_setActive(winid,false,input_getMouseX(),input_getMouseY());
	}
}

void win_detachAll(int fd) {
	for(size_t i = 0; i < WINDOW_COUNT; ++i) {
		if(windows[i].evfd == fd)
			windows[i].evfd = -1;
	}
}

void win_destroyWinsOf(int cid,gpos_t mouseX,gpos_t mouseY) {
	gwinid_t id;
	for(id = 0; id < WINDOW_COUNT; id++) {
		if(windows[id].id != WINID_UNUSED && windows[id].owner == cid)
			win_destroy(id,mouseX,mouseY);
	}
}

void win_destroy(gwinid_t id,gpos_t mouseX,gpos_t mouseY) {
	/* destroy shm area */
	win_destroyBuf(windows + id);

	/* mark unused */
	windows[id].id = WINID_UNUSED;
	win_notifyWinDestroy(id);

	print("Destroyed window %d @ (%d,%d,%d) with size %zux%zu",
		id,windows[id].x(),windows[id].y(),windows[id].z,windows[id].width(),windows[id].height());

	/* repaint window-area */
	win_repaint(windows[id],NULL,-1);

	/* set highest window active */
	if(activeWindow == id || topWindow == id) {
		gwinid_t winId = win_getTop();
		if(winId != WINID_UNUSED)
			win_setActive(winId,false,mouseX,mouseY);
	}
}

Window *win_get(gwinid_t id) {
	if(id >= WINDOW_COUNT || windows[id].id == WINID_UNUSED)
		return NULL;
	return windows + id;
}

bool win_exists(gwinid_t id) {
	return id < WINDOW_COUNT && windows[id].id != WINID_UNUSED;
}

Window *win_getAt(gpos_t x,gpos_t y) {
	gwinid_t i;
	gpos_t maxz = -1;
	gwinid_t winId = WINDOW_COUNT;
	Window *w = windows;
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(w->id != WINID_UNUSED && w->z > maxz && w->contains(x,y)) {
			winId = i;
			maxz = w->z;
		}
		w++;
	}
	if(winId != WINDOW_COUNT)
		return windows + winId;
	return NULL;
}

Window *win_getActive(void) {
	if(activeWindow != WINDOW_COUNT)
		return windows + activeWindow;
	return NULL;
}

void win_setActive(gwinid_t id,bool repaint,gpos_t mouseX,gpos_t mouseY) {
	gwinid_t i;
	gpos_t curz = windows[id].z;
	gpos_t maxz = 0;
	Window *w = windows;
	if(id != WINDOW_COUNT && windows[id].style != WIN_STYLE_DESKTOP) {
		for(i = 0; i < WINDOW_COUNT; i++) {
			if(w->id != WINID_UNUSED && w->z > curz && w->style != WIN_STYLE_POPUP) {
				if(w->z > maxz)
					maxz = w->z;
				w->z--;
			}
			w++;
		}
		if(maxz > 0)
			windows[id].z = maxz;
	}

	if(id != activeWindow) {
		if(activeWindow != WINDOW_COUNT)
			win_sendActive(activeWindow,false,mouseX,mouseY);

		activeWindow = id;
		if(windows[activeWindow].style != WIN_STYLE_DESKTOP)
			topWindow = id;
		if(activeWindow != WINDOW_COUNT) {
			win_sendActive(activeWindow,true,mouseX,mouseY);
			win_notifyWinActive(activeWindow);

			if(repaint && windows[activeWindow].style != WIN_STYLE_DESKTOP)
				win_repaint(windows[activeWindow],windows + activeWindow,windows[activeWindow].z);
		}
	}
}

void win_previewResize(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	preview_set(fb->addr(),x,y,width,height,2);
}

void win_previewMove(gwinid_t window,gpos_t x,gpos_t y) {
	Window *w = windows + window;
	preview_set(fb->addr(),x,y,w->width(),w->height(),2);
}

void win_resize(gwinid_t window,gpos_t x,gpos_t y,gsize_t width,gsize_t height,const char *winmng) {
	Window *w = windows + window;

	/* exchange buffer */
	win_destroyBuf(w);
	win_createBuf(w,window,width,height,winmng);

	if(x != w->x() || y != w->y())
		win_moveTo(window,x,y,width,height);
	else {
		gsize_t oldWidth = w->width();
		gsize_t oldHeight = w->height();
		w->setSize(width,height);

		/* remove preview */
		preview_set(fb->addr(),0,0,0,0,0);

		if(width < oldWidth) {
			gui::Rectangle nrect(w->x() + width,w->y(),oldWidth - width,oldHeight);
			win_repaint(nrect,NULL,-1);
		}
		if(height < oldHeight) {
			gui::Rectangle nrect(w->x(),w->y() + height,oldWidth,oldHeight - height);
			win_repaint(nrect,NULL,-1);
		}
	}
}

void win_moveTo(gwinid_t window,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	Window *w = windows + window;

	/* remove preview */
	preview_set(fb->addr(),0,0,0,0,0);

	/* save old position */
	gui::Rectangle orect(*w);
	gui::Rectangle nrect(x,y,width,height);
	std::vector<gui::Rectangle> rects = gui::substraction(orect,nrect);

	w->setPos(nrect.getPos());
	w->setSize(nrect.getSize());

	/* clear old position */
	if(!rects.empty()) {
		/* if there is an intersection, use the splitted parts */
		for(auto rect = rects.begin(); rect != rects.end(); ++rect)
			win_repaint(*rect,NULL,-1);
	}
	else {
		/* no intersection, so use the whole old rectangle */
		win_repaint(orect,NULL,-1);
	}

	/* repaint new position */
	win_repaint(nrect,w,w->z);
}

void win_update(gwinid_t window,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	Window *w = windows + window;
	w->ready = true;

	gui::Rectangle rect(w->x() + x,w->y() + y,width,height);
	if(win_validateRect(rect)) {
		if(topWindow == window)
			win_copyRegion(fb->addr(),rect,window);
		else
			win_repaint(rect,w,w->z);
	}
}

static gwinid_t win_getTop(void) {
	gwinid_t i,winId = WINID_UNUSED;
	int maxz = -1;
	Window *w = windows;
	for(i = 0; i < WINDOW_COUNT; i++) {
		if(w->id != WINID_UNUSED && w->z > maxz) {
			winId = i;
			maxz = w->z;
		}
		w++;
	}
	return winId;
}

static bool win_validateRect(gui::Rectangle &r) {
	if(r.x() < 0) {
		if(-r.x() > (gpos_t)r.width())
			return false;

		r.setSize(r.width() + r.x(),r.height());
		r.setPos(0,r.y());
	}

	if(r.x() >= (gpos_t)mode.width || r.y() >= (gpos_t)mode.height)
		return false;

	r.setSize(MIN(mode.width - r.x(),r.width()),MIN(mode.height - r.y(),r.height()));
	return true;
}

static void win_repaint(const gui::Rectangle &r,Window *win,gpos_t z) {
	std::vector<WinRect> rects;
	win_getRepaintRegions(rects,0,win,z,r);
	for(auto rect = rects.begin(); rect != rects.end(); ++rect) {
		/* validate rect */
		if(!win_validateRect(*rect))
			continue;

		/* if it doesn't belong to a window, we have to clear it */
		if(rect->id == WINDOW_COUNT)
			win_clearRegion(fb->addr(),*rect);
		/* otherwise copy from the window buffer */
		else
			win_copyRegion(fb->addr(),*rect,rect->id);
	}
}

static void win_sendActive(gwinid_t id,bool isActive,gpos_t mouseX,gpos_t mouseY) {
	if(windows[id].evfd == -1)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_SET_ACTIVE;
	ev.wid = id;
	ev.d.setactive.active = isActive;
	ev.d.setactive.mouseX = mouseX;
	ev.d.setactive.mouseY = mouseY;
	send(windows[id].evfd,MSG_WIN_EVENT,&ev,sizeof(ev));
}

static void win_getRepaintRegions(std::vector<WinRect> &list,gwinid_t id,Window *win,gpos_t z,
		const gui::Rectangle &r) {
	for(; id < WINDOW_COUNT; id++) {
		Window *w = windows + id;
		/* skip unused, ourself and rects behind ourself */
		if((win && w->id == win->id) || w->id == WINID_UNUSED || w->z < z || !w->ready)
			continue;

		/* substract the window-rectangle from r */
		std::vector<gui::Rectangle> rects = gui::substraction(r,*w);

		gui::Rectangle inter = gui::intersection(*w,r);
		if(!inter.empty())
			win_getRepaintRegions(list,id + 1,w,w->z,inter);

		if(!rects.empty()) {
			/* split all by all other windows */
			for(auto rect = rects.begin(); rect != rects.end(); ++rect)
				win_getRepaintRegions(list,id + 1,win,z,*rect);
		}

		/* if we made a recursive call we can leave here */
		if(!rects.empty() || !inter.empty())
			return;
	}

	/* no split, so its on top */
	WinRect final(r);
	if(win)
		final.id = win->id;
	else
		final.id = WINDOW_COUNT;
	list.push_back(final);
}

static void win_clearRegion(char *mem,const gui::Rectangle &r) {
	gpos_t y = r.y();
	size_t count = r.width() * pixelSize();
	gpos_t maxy = y + r.height();
	mem += (y * mode.width + r.x()) * pixelSize();
	while(y < maxy) {
		memclear(mem,count);
		mem += mode.width * pixelSize();
		y++;
	}

	preview_updateRect(mem,r.x(),r.y(),r.width(),r.height());
	win_notifyUimng(r.x(),r.y(),r.width(),r.height());
}

static void win_copyRegion(char *mem,const gui::Rectangle &r,gwinid_t id) {
	Window *w = windows + id;
	gpos_t x = r.x() - w->x();
	gpos_t y = r.y() - w->y();

	char *src,*dst;
	gpos_t endy = y + r.height();
	size_t count = r.width() * pixelSize();
	size_t srcAdd = w->width() * pixelSize();
	size_t dstAdd = mode.width * pixelSize();
	src = w->fb->addr() + (y * w->width() + x) * pixelSize();
	dst = mem + ((w->y() + y) * mode.width + (w->x() + x)) * pixelSize();

	while(y < endy) {
		memcpy(dst,src,count);
		src += srcAdd;
		dst += dstAdd;
		y++;
	}

	preview_updateRect(mem,w->x() + x,w->y() + (endy - r.height()),r.width(),r.height());
	win_notifyUimng(w->x() + x,w->y() + (endy - r.height()),r.width(),r.height());
}

void win_notifyUimng(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	if(x < 0) {
		width += x;
		x = 0;
	}
	ui->update(x,y,MIN(mode.width - x,width),MIN(mode.height - y,height));
}

static void win_notifyWinCreate(gwinid_t id,const char *title) {
	if(windows[id].style == WIN_STYLE_POPUP || windows[id].style == WIN_STYLE_DESKTOP)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_CREATED;
	ev.wid = id;
	strnzcpy(ev.d.created.title,title,sizeof(ev.d.created.title));
	listener_notify(&ev);
}

static void win_notifyWinActive(gwinid_t id) {
	if(windows[id].style == WIN_STYLE_POPUP)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_ACTIVE;
	ev.wid = id;
	listener_notify(&ev);
}

static void win_notifyWinDestroy(gwinid_t id) {
	if(windows[id].style == WIN_STYLE_POPUP || windows[id].style == WIN_STYLE_DESKTOP)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_DESTROYED;
	ev.wid = id;
	listener_notify(&ev);
}
