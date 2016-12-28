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

gwinid_t WinList::nextId;
WinList *WinList::_inst;

WinList::WinList(int sid,esc::UI *uiobj,const gui::Size &size,gcoldepth_t bpp)
	: ui(uiobj), drvId(sid), theme("default"), mode(), fb(), activeWindow(WINID_UNUSED),
	  topWindow(WINID_UNUSED), windows() {
	srand(time(NULL));

	setMode(size,bpp);
}

void WinList::setTheme(const char *name) {
	::print("Setting theme '%s'",name);
	theme = name;

	resetAll();
}

int WinList::setMode(const gui::Size &size,gcoldepth_t bpp) {
	::print("Getting video mode for %zux%zux%u",size.width,size.height,bpp);

	esc::Screen::Mode newmode = ui->findGraphicsMode(size.width,size.height,bpp);

	/* first destroy the old one because we use the same shm-name again */
	delete fb;

	try {
		::print("Creating new framebuffer");
		std::unique_ptr<esc::FrameBuffer> newfb(
			new esc::FrameBuffer(newmode,esc::Screen::MODE_TYPE_GUI));
		::print("Setting mode %d: %zux%zux%u",newmode.id,newmode.width,newmode.height,newmode.bitsPerPixel);
		ui->setMode(esc::Screen::MODE_TYPE_GUI,newmode.id,newfb->fd(),true);

		mode = newmode;
		fb = newfb.release();

		/* recreate window buffers */
		resetAll();
	}
	catch(const std::exception &e) {
		::printe("%s",e.what());
		::print("Restoring old framebuffer and mode");
		fb = new esc::FrameBuffer(mode,esc::Screen::MODE_TYPE_GUI);
		ui->setMode(esc::Screen::MODE_TYPE_GUI,mode.id,fb->fd(),true);
		/* we have to repaint everything */
		for(auto w = windows.begin(); w != windows.end(); ++w)
			update(&*w,gui::Rectangle(gui::Pos(0,0),w->getSize()));
		throw;
	}
	return mode.id;
}

gwinid_t WinList::add(const gui::Rectangle &r,int owner,uint style,
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

	windows.insert(new Window(nextId++,r,z,owner,style,titleBarHeight,title));
	return nextId - 1;
}

void WinList::remove(Window *win) {
	gwinid_t winid = win->id();
	/* remove window */
	windows.remove(win);

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
			setActive(top,false,Input::get().getMouse(),true);
	}
}

void WinList::detachAll(int fd) {
	for(auto w = windows.begin(); w != windows.end(); ++w) {
		if(w->evfd == fd)
			w->evfd = -1;
	}
}

void WinList::resetAll() {
	for(auto w = windows.begin(); w != windows.end(); ++w) {
		w->destroybuf();

		/* let the window reset everything, i.e. create to new buffer, repaint, ... */
		esc::WinMngEvents::Event ev;
		ev.type = esc::WinMngEvents::Event::TYPE_RESET;
		ev.wid = w->id();
		send(w->evfd,MSG_WIN_EVENT,&ev,sizeof(ev));
	}
}

void WinList::destroyWinsOf(int cid) {
	for(auto w = windows.begin(); w != windows.end(); ) {
		auto old = w++;
		if(old->owner == cid)
			remove(&*old);
	}
}

Window *WinList::getAt(const gui::Pos &pos) {
	gpos_t maxz = -1;
	gwinid_t winId = WINID_UNUSED;
	for(auto w = windows.begin(); w != windows.end(); ++w) {
		if(w->z > maxz && w->contains(pos.x,pos.y)) {
			winId = w->id();
			maxz = w->z;
		}
	}
	if(winId != WINID_UNUSED)
		return windows.find(winId);
	return NULL;
}

Window *WinList::getTop() {
	gpos_t maxz = -1;
	gwinid_t winId = WINID_UNUSED;
	for(auto w = windows.begin(); w != windows.end(); ++w) {
		if(w->z > maxz) {
			winId = w->id();
			maxz = w->z;
		}
	}
	if(winId != WINID_UNUSED)
		return windows.find(winId);
	return NULL;
}

void WinList::setActive(Window *win,bool repaint,const gui::Pos &mouse,bool updateWinStack) {
	gpos_t curz = win->z;
	gpos_t maxz = 0;
	if(win->id() != WINID_UNUSED && win->style != Window::STYLE_DESKTOP) {
		for(auto w = windows.begin(); w != windows.end(); ++w) {
			if(w->z > curz && w->style != Window::STYLE_POPUP) {
				if(w->z > maxz)
					maxz = w->z;
				w->z--;
			}
		}
		if(maxz > 0)
			win->z = maxz;
	}

	if(win->id() != activeWindow) {
		Window *active = getActive();
		if(active)
			active->sendActive(false,mouse);

		if(updateWinStack && win->style == Window::STYLE_DEFAULT)
			Stack::activate(win->id());

		activeWindow = win->id();
		if(win->style != Window::STYLE_DESKTOP)
			topWindow = win->id();

		win->sendActive(true,mouse);
		win->notifyWinActive();

		if(repaint && win->style != Window::STYLE_DESKTOP)
			this->repaint(*win,win,win->z);
	}
}

void WinList::update(Window *win,const gui::Rectangle &r) {
	win->ready = true;

	gui::Rectangle rect(win->x() + r.x(),win->y() + r.y(),r.width(),r.height());
	if(validateRect(rect)) {
		if(topWindow == win->id())
			copyRegion(fb->addr(),rect,win->id());
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
	getRepaintRegions(rects,windows.begin(),win,z,r);
	for(auto rect = rects.begin(); rect != rects.end(); ++rect) {
		/* validate rect */
		if(!validateRect(*rect))
			continue;

		/* if it doesn't belong to a window, we have to clear it */
		if(rect->id() == WINID_UNUSED)
			clearRegion(fb->addr(),*rect);
		/* otherwise copy from the window buffer */
		else
			copyRegion(fb->addr(),*rect,rect->id());
	}
}

void WinList::getRepaintRegions(std::vector<WinRect> &list,esc::DListTreap<Window>::iterator w,
		Window *win,gpos_t z,const gui::Rectangle &r) {
	for(; w != windows.end(); ) {
		auto next = w;
		++next;

		/* skip unused, ourself and rects behind ourself */
		if((win && w->id() == win->id()) || w->z < z || !w->ready) {
			w = next;
			continue;
		}

		/* substract the window-rectangle from r */
		std::vector<gui::Rectangle> rects = gui::substraction(r,*w);

		gui::Rectangle inter = gui::intersection(*w,r);
		if(!inter.empty())
			getRepaintRegions(list,next,&*w,w->z,inter);

		if(!rects.empty()) {
			/* split all by all other windows */
			for(auto rect = rects.begin(); rect != rects.end(); ++rect)
				getRepaintRegions(list,next,win,z,*rect);
		}

		/* if we made a recursive call we can leave here */
		if(!rects.empty() || !inter.empty())
			return;

		w = next;
	}

	/* no split, so its on top */
	WinRect final(r);
	if(win)
		final.key(win->id());
	else
		final.key(WINID_UNUSED);
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

	Preview::get().updateRect(mem,r);
	notifyUimng(r);
}

void WinList::copyRegion(char *mem,const gui::Rectangle &r,gwinid_t id) {
	Window *w = get(id);
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

	gui::Rectangle rect(gui::Pos(w->x() + x,w->y() + (endy - r.height())),r.getSize());
	Preview::get().updateRect(mem,rect);
	notifyUimng(rect);
}

void WinList::notifyUimng(const gui::Rectangle &r) {
	gpos_t x = r.x();
	gsize_t width = r.width();
	if(x < 0) {
		width += x;
		x = 0;
	}
	ui->update(x,r.y(),esc::Util::min((gsize_t)mode.width - x,width),
		esc::Util::min((gsize_t)mode.height - r.y(),r.height()));
}

void WinList::print(esc::OStream &os) {
	for(auto w = windows.begin(); w != windows.end(); ++w) {
		os << "Window " << w->id() << "\n";
		os << "\tOwner: " << w->owner << "\n";
		os << "\tPosition: " << w->x() << "," << w->y() << "," << w->z << "\n";
		os << "\tSize: " << w->width() << " x " << w->height() << "\n";
		os << "\tStyle: 0x" << esc::fmt(w->style,"x") << "\n";
	}
}
