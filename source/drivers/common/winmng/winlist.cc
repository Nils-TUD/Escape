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
#include <memory>
#include <stdlib.h>
#include <time.h>

#include "input.h"
#include "stack.h"
#include "winlist.h"

WinList *WinList::_inst;

WinList::WinList(int sid,esc::UI *uiobj,gsize_t width,gsize_t height,gcoldepth_t bpp)
	: ui(uiobj), drvId(sid), theme("default"), mode(), fb(), activeWindow(WINID_UNUSED),
	  topWindow(WINID_UNUSED), windows() {
	srand(time(NULL));

	setMode(width,height,bpp);
}

void WinList::setTheme(const char *name) {
	print("Setting theme '%s'",name);
	theme = name;

	resetAll();
}

int WinList::setMode(gsize_t width,gsize_t height,gcoldepth_t bpp) {
	print("Getting video mode for %zux%zux%u",width,height,bpp);

	esc::Screen::Mode newmode = ui->findGraphicsMode(width,height,bpp);

	/* first destroy the old one because we use the same shm-name again */
	delete fb;

	try {
		print("Creating new framebuffer");
		std::unique_ptr<esc::FrameBuffer> newfb(
			new esc::FrameBuffer(newmode,esc::Screen::MODE_TYPE_GUI));
		print("Setting mode %d: %zux%zux%u",newmode.id,newmode.width,newmode.height,newmode.bitsPerPixel);
		ui->setMode(esc::Screen::MODE_TYPE_GUI,newmode.id,newfb->fd(),true);

		mode = newmode;
		fb = newfb.release();

		/* recreate window buffers */
		resetAll();
	}
	catch(const std::exception &e) {
		printe("%s",e.what());
		print("Restoring old framebuffer and mode");
		fb = new esc::FrameBuffer(mode,esc::Screen::MODE_TYPE_GUI);
		ui->setMode(esc::Screen::MODE_TYPE_GUI,mode.id,fb->fd(),true);
		/* we have to repaint everything */
		for(size_t i = 0; i < WINDOW_COUNT; i++) {
			if(windows[i])
				update(windows[i],0,0,windows[i]->width(),windows[i]->height());
		}
		throw;
	}
	return mode.id;
}

gwinid_t WinList::add(gpos_t x,gpos_t y,gsize_t width,gsize_t height,int owner,uint style,
		gsize_t titleBarHeight,const char *title) {
	gpos_t z;
	if(style == Window::STYLE_DESKTOP)
		z = 0;
	else {
		Window *top = getTop();
		if(top)
			z = top->z + 1;
		else
			z = 1;
	}

	for(gwinid_t i = 0; i < WINDOW_COUNT; i++) {
		if(!windows[i]) {
			windows[i] = new Window(i,x,y,z,width,height,owner,style,titleBarHeight,title);
			return i;
		}
	}
	return -ENOSPC;
}

void WinList::remove(Window *win) {
	gwinid_t winid = win->id;
	/* remove window */
	windows[winid] = NULL;

	/* repaint window-area */
	repaint(*win,NULL,-1);

	/* delete window */
	delete win;

	/* set highest window active */
	if(activeWindow == winid || topWindow == winid) {
		if(activeWindow == winid)
			activeWindow = WINID_UNUSED;
		Window *top = getTop();
		if(top)
			setActive(top,false,Input::get().getMouseX(),Input::get().getMouseY(),true);
	}
}

void WinList::detachAll(int fd) {
	for(size_t i = 0; i < WINDOW_COUNT; ++i) {
		if(windows[i] && windows[i]->evfd == fd)
			windows[i]->evfd = -1;
	}
}

void WinList::resetAll() {
	for(size_t i = 0; i < WINDOW_COUNT; i++) {
		if(windows[i]) {
			windows[i]->destroybuf();

			/* let the window reset everything, i.e. create to new buffer, repaint, ... */
			esc::WinMngEvents::Event ev;
			ev.type = esc::WinMngEvents::Event::TYPE_RESET;
			ev.wid = i;
			send(windows[i]->evfd,MSG_WIN_EVENT,&ev,sizeof(ev));
		}
	}
}

void WinList::destroyWinsOf(int cid) {
	gwinid_t id;
	for(id = 0; id < WINDOW_COUNT; id++) {
		if(windows[id] && windows[id]->owner == cid)
			remove(windows[id]);
	}
}

Window *WinList::getAt(gpos_t x,gpos_t y) {
	gpos_t maxz = -1;
	gwinid_t winId = WINDOW_COUNT;
	for(gwinid_t i = 0; i < WINDOW_COUNT; i++) {
		Window *w = windows[i];
		if(w && w->z > maxz && w->contains(x,y)) {
			winId = i;
			maxz = w->z;
		}
	}
	if(winId != WINDOW_COUNT)
		return windows[winId];
	return NULL;
}

Window *WinList::getTop() {
	gpos_t maxz = -1;
	gwinid_t winId = WINID_UNUSED;
	for(gwinid_t i = 0; i < WINDOW_COUNT; i++) {
		Window *w = windows[i];
		if(w && w->z > maxz) {
			winId = i;
			maxz = w->z;
		}
	}
	if(winId != WINDOW_COUNT)
		return windows[winId];
	return NULL;
}

void WinList::setActive(Window *win,bool repaint,gpos_t mouseX,gpos_t mouseY,bool updateWinStack) {
	gpos_t curz = win->z;
	gpos_t maxz = 0;
	if(win->id != WINDOW_COUNT && win->style != Window::STYLE_DESKTOP) {
		for(gwinid_t i = 0; i < WINDOW_COUNT; i++) {
			Window *w = windows[i];
			if(w && w->z > curz && w->style != Window::STYLE_POPUP) {
				if(w->z > maxz)
					maxz = w->z;
				w->z--;
			}
		}
		if(maxz > 0)
			win->z = maxz;
	}

	if(win->id != activeWindow) {
		if(activeWindow != WINDOW_COUNT)
			windows[activeWindow]->sendActive(false,mouseX,mouseY);

		if(updateWinStack && win->style == Window::STYLE_DEFAULT)
			Stack::activate(win->id);

		activeWindow = win->id;
		if(windows[activeWindow]->style != Window::STYLE_DESKTOP)
			topWindow = win->id;
		if(activeWindow != WINDOW_COUNT) {
			windows[activeWindow]->sendActive(true,mouseX,mouseY);
			windows[activeWindow]->notifyWinActive();

			if(repaint && windows[activeWindow]->style != Window::STYLE_DESKTOP)
				this->repaint(*windows[activeWindow],windows[activeWindow],windows[activeWindow]->z);
		}
	}
}

void WinList::update(Window *win,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	win->ready = true;

	gui::Rectangle rect(win->x() + x,win->y() + y,width,height);
	if(validateRect(rect)) {
		if(topWindow == win->id)
			copyRegion(fb->addr(),rect,win->id);
		else
			repaint(rect,win,win->z);
	}
}

bool WinList::validateRect(gui::Rectangle &r) {
	if(r.x() < 0) {
		if(-r.x() > (gpos_t)r.width())
			return false;

		r.setSize(r.width() + r.x(),r.height());
		r.setPos(0,r.y());
	}

	if(r.x() >= (gpos_t)mode.width || r.y() >= (gpos_t)mode.height)
		return false;

	gsize_t width = esc::Util::min((gsize_t)mode.width - r.x(),r.width());
	gsize_t height = esc::Util::min((gsize_t)mode.height - r.y(),r.height());
	r.setSize(width,height);
	return true;
}

void WinList::repaint(const gui::Rectangle &r,Window *win,gpos_t z) {
	std::vector<WinRect> rects;
	getRepaintRegions(rects,0,win,z,r);
	for(auto rect = rects.begin(); rect != rects.end(); ++rect) {
		/* validate rect */
		if(!validateRect(*rect))
			continue;

		/* if it doesn't belong to a window, we have to clear it */
		if(rect->id == WINDOW_COUNT)
			clearRegion(fb->addr(),*rect);
		/* otherwise copy from the window buffer */
		else
			copyRegion(fb->addr(),*rect,rect->id);
	}
}

void WinList::getRepaintRegions(std::vector<WinRect> &list,gwinid_t id,Window *win,gpos_t z,
		const gui::Rectangle &r) {
	for(; id < WINDOW_COUNT; id++) {
		Window *w = windows[id];
		/* skip unused, ourself and rects behind ourself */
		if(!w || (win && w->id == win->id) || w->z < z || !w->ready)
			continue;

		/* substract the window-rectangle from r */
		std::vector<gui::Rectangle> rects = gui::substraction(r,*w);

		gui::Rectangle inter = gui::intersection(*w,r);
		if(!inter.empty())
			getRepaintRegions(list,id + 1,w,w->z,inter);

		if(!rects.empty()) {
			/* split all by all other windows */
			for(auto rect = rects.begin(); rect != rects.end(); ++rect)
				getRepaintRegions(list,id + 1,win,z,*rect);
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

void WinList::clearRegion(char *mem,const gui::Rectangle &r) {
	gpos_t y = r.y();
	size_t pxsize = mode.bitsPerPixel / 8;
	size_t count = r.width() * pxsize;
	gpos_t maxy = y + r.height();
	mem += (y * mode.width + r.x()) * pxsize;
	while(y < maxy) {
		memclear(mem,count);
		mem += mode.width * pxsize;
		y++;
	}

	Preview::get().updateRect(mem,r.x(),r.y(),r.width(),r.height());
	notifyUimng(r.x(),r.y(),r.width(),r.height());
}

void WinList::copyRegion(char *mem,const gui::Rectangle &r,gwinid_t id) {
	Window *w = windows[id];
	/* ignore that if we don't have a framebuffer yet */
	esc::FrameBuffer *winbuf = w->getBuffer();
	if(!winbuf)
		return;

	gpos_t x = r.x() - w->x();
	gpos_t y = r.y() - w->y();

	gpos_t endy = y + r.height();
	size_t pxsize = mode.bitsPerPixel / 8;
	size_t count = r.width() * pxsize;
	size_t srcAdd = w->width() * pxsize;
	size_t dstAdd = mode.width * pxsize;
	char *src = winbuf->addr() + (y * w->width() + x) * pxsize;
	char *dst = mem + ((w->y() + y) * mode.width + (w->x() + x)) * pxsize;

	while(y < endy) {
		memcpy(dst,src,count);
		src += srcAdd;
		dst += dstAdd;
		y++;
	}

	Preview::get().updateRect(mem,w->x() + x,w->y() + (endy - r.height()),r.width(),r.height());
	notifyUimng(w->x() + x,w->y() + (endy - r.height()),r.width(),r.height());
}

void WinList::notifyUimng(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	if(x < 0) {
		width += x;
		x = 0;
	}
	ui->update(x,y,esc::Util::min((gsize_t)mode.width - x,width),
		esc::Util::min((gsize_t)mode.height - y,height));
}
