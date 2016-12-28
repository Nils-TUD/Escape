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

Window::Window(gwinid_t id,gpos_t x,gpos_t y,gpos_t z,gsize_t width,gsize_t height,int owner,uint style,
	gsize_t titleBarHeight,const char *title)
	: WinRect(gui::Rectangle(gui::Pos(x,y),gui::Size(width,height))), z(z), owner(owner),
	  evfd(-1), style(style), titleBarHeight(titleBarHeight), winbuf(), ready(false) {
	this->id = id;

	if(style == STYLE_DEFAULT)
		Stack::add(id);

	print("Created window %d: %s @ (%d,%d,%d) with size %zux%zu",id,title,x,y,z,width,height);
	notifyWinCreate(title);
}

Window::~Window() {
	/* destroy shm area */
	destroybuf();

	if(style == STYLE_DEFAULT)
		Stack::remove(id);

	print("Destroyed window %d @ (%d,%d,%d) with size %zux%zu",id,x(),y(),z,width(),height());
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

void Window::resize(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	/* exchange buffer */
	destroybuf();

	if(x != this->x() || y != this->y())
		moveTo(x,y,width,height);
	else {
		gsize_t oldWidth = this->width();
		gsize_t oldHeight = this->height();
		setSize(width,height);

		WinList::get().removePreview();

		if(width < oldWidth) {
			gui::Rectangle nrect(this->x() + width,this->y(),oldWidth - width,oldHeight);
			WinList::get().repaint(nrect,NULL,-1);
		}
		if(height < oldHeight) {
			gui::Rectangle nrect(this->x(),this->y() + height,oldWidth,oldHeight - height);
			WinList::get().repaint(nrect,NULL,-1);
		}
	}

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_RESIZE;
	ev.wid = id;
	send(evfd,MSG_WIN_EVENT,&ev,sizeof(ev));
}

void Window::moveTo(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	WinList::get().removePreview();

	/* save old position */
	gui::Rectangle orect(*this);
	gui::Rectangle nrect(x,y,width,height);
	std::vector<gui::Rectangle> rects = gui::substraction(orect,nrect);

	setPos(nrect.getPos());
	setSize(nrect.getSize());

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
	WinList::get().repaint(nrect,this,z);
}

void Window::sendActive(bool isActive,gpos_t mouseX,gpos_t mouseY) {
	if(evfd == -1)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_SET_ACTIVE;
	ev.wid = id;
	ev.d.setactive.active = isActive;
	ev.d.setactive.mouseX = mouseX;
	ev.d.setactive.mouseY = mouseY;
	send(evfd,MSG_WIN_EVENT,&ev,sizeof(ev));
}

void Window::notifyWinCreate(const char *title) {
	if(style == STYLE_POPUP || style == STYLE_DESKTOP)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_CREATED;
	ev.wid = id;
	strnzcpy(ev.d.created.title,title,sizeof(ev.d.created.title));
	Listener::get().notify(&ev);
}

void Window::notifyWinActive() {
	if(style == STYLE_POPUP)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_ACTIVE;
	ev.wid = id;
	Listener::get().notify(&ev);
}

void Window::notifyWinDestroy() {
	if(style == STYLE_POPUP || style == STYLE_DESKTOP)
		return;

	esc::WinMngEvents::Event ev;
	ev.type = esc::WinMngEvents::Event::TYPE_DESTROYED;
	ev.wid = id;
	Listener::get().notify(&ev);
}
