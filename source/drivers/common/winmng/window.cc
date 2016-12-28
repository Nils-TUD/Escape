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
#include <esc/util.h>
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
#include "stack.h"
#include "window.h"
#include "winlist.h"

Window::Window(gwinid_t id,const gui::Rectangle &r,gpos_t z,int owner,uint style,
	gsize_t titleBarHeight,const char *title)
	: WinRect(r,id), z(z), owner(owner),
	  evfd(-1), style(style), titleBarHeight(titleBarHeight), winbuf(), ready(false) {

	if(style == STYLE_DEFAULT)
		Stack::add(id);

	::print("Created window %d: %s @ (%d,%d,%d) with size %zux%zu",
		this->id(),title,r.x(),r.y(),z,r.width(),r.height());
	notifyWinCreate(title);
}

Window::~Window() {
	/* destroy shm area */
	destroybuf();

	if(style == STYLE_DEFAULT)
		Stack::remove(id());

	::print("Destroyed window %d @ (%d,%d,%d) with size %zux%zu",id(),x(),y(),z,width(),height());
	notifyWinDestroy();
}

void Window::destroybuf() {
	delete winbuf;
	winbuf = NULL;
}

int Window::joinbuf(int fd) {
	destroybuf();

	esc::Screen::Mode winMode = WinList::get().getMode();
	winMode.guiHeaderSize = winMode.tuiHeaderSize = 0;
	winMode.width = width();
	winMode.height = height();
	winbuf = new esc::FrameBuffer(winMode,fd,esc::Screen::MODE_TYPE_GUI);

	memclear(winbuf->addr(),width() * height() * (winMode.bitsPerPixel / 8));
	return 0;
}

void Window::attach(int fd) {
	if(evfd == -1) {
		evfd = fd;
		if(!WinList::get().getActive() || style != STYLE_DESKTOP)
			WinList::get().setActive(this,false,true);
	}
}

void Window::resize(const gui::Rectangle &r) {
	/* exchange buffer */
	destroybuf();

	if(r.x() != this->x() || r.y() != this->y())
		moveTo(r);
	else {
		gsize_t oldWidth = this->width();
		gsize_t oldHeight = this->height();
		setSize(r.getSize());

		WinList::get().removePreview();

		if(r.width() < oldWidth) {
			gui::Rectangle nrect(this->x() + r.width(),this->y(),oldWidth - r.width(),oldHeight);
			WinList::get().repaint(nrect,NULL,-1);
		}
		if(r.height() < oldHeight) {
			gui::Rectangle nrect(this->x(),this->y() + r.height(),oldWidth,oldHeight - r.height());
			WinList::get().repaint(nrect,NULL,-1);
		}
	}

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_RESIZE;
	ev.wid = id();
	send(evfd,MSG_WIN_EVENT,&ev,sizeof(ev));
}

void Window::moveTo(const gui::Rectangle &r) {
	WinList::get().removePreview();

	/* save old position */
	gui::Rectangle orect(*this);
	std::vector<gui::Rectangle> rects = gui::substraction(orect,r);

	setPos(r.getPos());
	setSize(r.getSize());

	/* clear old position */
	if(!rects.empty()) {
		/* if there is an intersection, use the splitted parts */
		for(auto rect = rects.begin(); rect != rects.end(); ++rect)
			WinList::get().repaint(*rect,NULL,-1);
	}
	else {
		/* no intersection, so use the whole old rectangle */
		WinList::get().repaint(orect,NULL,-1);
	}

	/* repaint new position */
	WinList::get().repaint(r,this,z);
}

void Window::sendActive(bool isActive,const gui::Pos &mouse) {
	if(evfd == -1)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_SET_ACTIVE;
	ev.wid = id();
	ev.d.setactive.active = isActive;
	ev.d.setactive.mouseX = mouse.x;
	ev.d.setactive.mouseY = mouse.y;
	send(evfd,MSG_WIN_EVENT,&ev,sizeof(ev));
}

void Window::notifyWinCreate(const char *title) {
	if(style == STYLE_POPUP || style == STYLE_DESKTOP)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_CREATED;
	ev.wid = id();
	strnzcpy(ev.d.created.title,title,sizeof(ev.d.created.title));
	Listener::get().notify(&ev);
}

void Window::notifyWinActive() {
	if(style == STYLE_POPUP)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_ACTIVE;
	ev.wid = id();
	Listener::get().notify(&ev);
}

void Window::notifyWinDestroy() {
	if(style == STYLE_POPUP || style == STYLE_DESKTOP)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_DESTROYED;
	ev.wid = id();
	Listener::get().notify(&ev);
}
