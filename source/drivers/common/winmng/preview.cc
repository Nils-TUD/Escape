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

#include <esc/util.h>
#include <gui/graphics/rectangle.h>
#include <sys/common.h>
#include <stdlib.h>
#include <string.h>

#include "preview.h"
#include "winlist.h"

Preview *Preview::_inst;

void Preview::updateRect(char *shmem,const gui::Rectangle &r) {
	const esc::Screen::Mode &mode = WinList::get().getMode();
	gsize_t xres = mode.width;
	gsize_t yres = mode.height;
	/* look if we have to update the preview-rectangle */
	if(_rectCopies[0]) {
		gui::Rectangle curRec(_rect.x(),_rect.y(),_rect.width(),_thickness);
		gui::Rectangle intersec;

		/* top */
		intersec = gui::intersection(curRec,r);
		if(!intersec.empty())
			handleIntersec(shmem,curRec,intersec,0,xres,yres);

		/* bottom */
		curRec.setPos(curRec.x(),_rect.y() + _rect.height() - _thickness);
		intersec = gui::intersection(curRec,r);
		if(!intersec.empty())
			handleIntersec(shmem,curRec,intersec,2,xres,yres);

		/* left */
		curRec.setPos(curRec.x(),_rect.y());
		curRec.setSize(_thickness,_rect.height());
		intersec = gui::intersection(curRec,r);
		if(!intersec.empty())
			handleIntersec(shmem,curRec,intersec,3,xres,yres);

		/* right */
		curRec.setPos(_rect.x() + _rect.width() - _thickness,curRec.y());
		intersec = gui::intersection(curRec,r);
		if(!intersec.empty())
			handleIntersec(shmem,curRec,intersec,1,xres,yres);
	}
}

void Preview::set(char *shmem,const gui::Rectangle &r,gsize_t thickness) {
	const esc::Screen::Mode &mode = WinList::get().getMode();
	gsize_t xres = mode.width;
	gsize_t yres = mode.height;
	gsize_t pxSize = mode.bitsPerPixel / 8;

	if(_rectCopies[0]) {
		/* copy old content back */
		/* top */
		copyRegion(_rectCopies[0],shmem,
				_rect.width(),_thickness,0,0,_rect.x(),_rect.y(),
				_rect.width(),xres,_thickness);
		WinList::get().notifyUimng(gui::Rectangle(_rect.x(),_rect.y(),_rect.width(),_thickness));
		/* right */
		copyRegion(_rectCopies[1],shmem,
				_thickness,_rect.height(),0,0,
				_rect.x() + _rect.width() - _thickness,_rect.y(),
				_thickness,xres,_rect.height());
		WinList::get().notifyUimng(gui::Rectangle(_rect.x() + _rect.width() - _thickness,_rect.y(),
			_thickness,_rect.height()));
		/* bottom */
		copyRegion(_rectCopies[2],shmem,
				_rect.width(),_thickness,0,0,_rect.x(),
				_rect.y() + _rect.height() - _thickness,
				_rect.width(),xres,_thickness);
		WinList::get().notifyUimng(gui::Rectangle(_rect.x(),_rect.y() + _rect.height() - _thickness,
			_rect.width(),_thickness));
		/* left */
		copyRegion(_rectCopies[3],shmem,
				_thickness,_rect.height(),0,0,_rect.x(),_rect.y(),
				_thickness,xres,_rect.height());
		WinList::get().notifyUimng(gui::Rectangle(_rect.x(),_rect.y(),_thickness,_rect.height()));

		if(thickness != _thickness || r.width() != _rect.width() ||
				r.height() != _rect.height()) {
			free(_rectCopies[0]);
			_rectCopies[0] = NULL;
			free(_rectCopies[1]);
			_rectCopies[1] = NULL;
			free(_rectCopies[2]);
			_rectCopies[2] = NULL;
			free(_rectCopies[3]);
			_rectCopies[3] = NULL;
		}
	}

	if(thickness > 0) {
		if(thickness != _thickness || r.width() != _rect.width() ||
				r.height() != _rect.height()) {
			_rectCopies[0] = (char*)malloc(r.width() * thickness * pxSize);
			_rectCopies[1] = (char*)malloc(r.height() * thickness * pxSize);
			_rectCopies[2] = (char*)malloc(r.width() * thickness * pxSize);
			_rectCopies[3] = (char*)malloc(r.height() * thickness * pxSize);
		}

		/* store preview rect */
		gpos_t x = r.x();
		gpos_t y = r.y();
		gsize_t width = r.width();
		gsize_t height = r.height();
		if(x < 0) {
			width = esc::Util::max(0,(int)(width + x));
			x = 0;
		}
		if(x > (int)xres)
			x = xres;
		if(x + width >= xres)
			width = xres - x;
		if(y > (int)yres)
			y = yres;
		if(y + height >= yres)
			height = yres - y;
		_rect.setPos(x,y);
		_rect.setSize(width,height);

		/* save content */
		/* top */
		copyRegion(shmem,_rectCopies[0],width,thickness,x,y,0,0,xres,width,yres);
		/* right */
		copyRegion(shmem,_rectCopies[1],thickness,height,x + width - thickness,y,0,0,xres,thickness,yres);
		/* bottom */
		copyRegion(shmem,_rectCopies[2],width,thickness,x,y + height - thickness,0,0,xres,width,yres);
		/* left */
		copyRegion(shmem,_rectCopies[3],thickness,height,x,y,0,0,xres,thickness,yres);

		/* draw rect */
		/* top */
		clearRegion(shmem,x,y,width,thickness);
		WinList::get().notifyUimng(gui::Rectangle(x,y,width,thickness));
		/* right */
		clearRegion(shmem,x + width - thickness,y,thickness,height);
		WinList::get().notifyUimng(gui::Rectangle(x + width - thickness,y,thickness,height));
		/* bottom */
		clearRegion(shmem,x,y + height - thickness,width,thickness);
		WinList::get().notifyUimng(gui::Rectangle(x,y + height - thickness,width,thickness));
		/* left */
		clearRegion(shmem,x,y,thickness,height);
		WinList::get().notifyUimng(gui::Rectangle(x,y,thickness,height));
	}

	_thickness = thickness;
}

void Preview::handleIntersec(char *shmem,const gui::Rectangle &curRec,
		const gui::Rectangle &intersec,size_t i,gsize_t xres,gsize_t yres) {
	copyRegion(shmem,_rectCopies[i],intersec.width(),intersec.height(),
		intersec.x(),intersec.y(),intersec.x() - curRec.x(),intersec.y() - curRec.y(),
		xres,curRec.width(),yres);
	clearRegion(shmem,curRec.x(),curRec.y(),curRec.width(),curRec.height());
}

void Preview::clearRegion(char *shmem,gpos_t x,gpos_t y,gsize_t w,gsize_t h) {
	const esc::Screen::Mode &mode = WinList::get().getMode();
	gsize_t xres = mode.width;
	gsize_t yres = mode.height;
	gpos_t maxy = esc::Util::min(yres,y + h);
	gsize_t pxSize = mode.bitsPerPixel / 8;
	size_t count = esc::Util::min(xres - x,w) * pxSize;
	size_t dstInc = xres * pxSize;
	uint8_t *dst = (uint8_t*)shmem + (y * xres + x) * pxSize;
	while(y < maxy) {
		memclear(dst,count);
		dst += dstInc;
		y++;
 	}
}

void Preview::copyRegion(char *src,char *dst,gsize_t width,gsize_t height,gpos_t x1,gpos_t y1,
		gpos_t x2,gpos_t y2,gsize_t w1,gsize_t w2,gsize_t h1) {
	gpos_t maxy = esc::Util::min(h1,y1 + height);
	gsize_t pxSize = WinList::get().getMode().bitsPerPixel / 8;
	size_t count = esc::Util::min(w2 - x2,esc::Util::min(w1 - x1,width)) * pxSize;
	size_t srcInc = w1 * pxSize;
	size_t dstInc = w2 * pxSize;
	src += (y1 * w1 + x1) * pxSize;
	dst += (y2 * w2 + x2) * pxSize;
	while(y1 < maxy) {
		memcpy(dst,src,count);
		src += srcInc;
		dst += dstInc;
		y1++;
	}
 }
