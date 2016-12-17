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
#include <sys/driver.h>
#include <sys/io.h>
#include <sys/messages.h>
#include <sys/mman.h>
#include <vbe/vbe.h>
#include <stdlib.h>
#include <string.h>

#include "image.h"
#include "vesagui.h"
#include "vesascreen.h"

static const size_t CURSOR_LEFT_OFF				= 10;
static const char *CURSOR_DEFAULT_FILE			= "/etc/cursor_def.png";
static const char *CURSOR_RESIZE_L_FILE			= "/etc/cursor_resl.png";
static const char *CURSOR_RESIZE_BR_FILE		= "/etc/cursor_resbr.png";
static const char *CURSOR_RESIZE_VERT_FILE		= "/etc/cursor_resvert.png";
static const char *CURSOR_RESIZE_BL_FILE		= "/etc/cursor_resbl.png";
static const char *CURSOR_RESIZE_R_FILE			= "/etc/cursor_resr.png";

VESAGUI::VESAGUI()
		: _cursorCopy(), _lastX(), _lastY(),_curCursor(esc::Screen::CURSOR_DEFAULT), _cursor() {
	_cursor[0] = new VESAImage(CURSOR_DEFAULT_FILE);
	if(_cursor[0] == NULL)
		error("Unable to load bitmap from %s",CURSOR_DEFAULT_FILE);
	_cursor[1] = new VESAImage(CURSOR_RESIZE_L_FILE);
	if(_cursor[1] == NULL)
		error("Unable to load bitmap from %s",CURSOR_RESIZE_L_FILE);
	_cursor[2] = new VESAImage(CURSOR_RESIZE_BR_FILE);
	if(_cursor[2] == NULL)
		error("Unable to load bitmap from %s",CURSOR_RESIZE_BR_FILE);
	_cursor[3] = new VESAImage(CURSOR_RESIZE_VERT_FILE);
	if(_cursor[3] == NULL)
		error("Unable to load bitmap from %s",CURSOR_RESIZE_VERT_FILE);
	_cursor[4] = new VESAImage(CURSOR_RESIZE_BL_FILE);
	if(_cursor[4] == NULL)
		error("Unable to load bitmap from %s",CURSOR_RESIZE_BL_FILE);
	_cursor[5] = new VESAImage(CURSOR_RESIZE_R_FILE);
	if(_cursor[5] == NULL)
		error("Unable to load bitmap from %s",CURSOR_RESIZE_R_FILE);

	gsize_t curWidth,curHeight;
	_cursor[_curCursor]->getSize(&curWidth,&curHeight);
	_cursorCopy = (uint8_t*)malloc(curWidth * curHeight * 4);
	if(_cursorCopy == NULL)
		error("Unable to create cursor copy");
}

void VESAGUI::setCursor(VESAScreen *scr,void *shmem,gpos_t newCurX,gpos_t newCurY,int newCursor) {
	if(newCursor == esc::Screen::CURSOR_RESIZE_L || newCursor == esc::Screen::CURSOR_RESIZE_BL)
		newCurX -= CURSOR_LEFT_OFF;
	doSetCursor(scr,shmem,newCurX,newCurY,newCursor);
}

void VESAGUI::update(VESAScreen *scr,void *shmem,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	esc::Screen::Mode *minfo = scr->mode;
	gpos_t y1,y2;
	gsize_t xres = minfo->width;
	gsize_t yres = minfo->height;
	gsize_t pxSize = minfo->bitsPerPixel / 8;
	gsize_t curWidth,curHeight;
	_cursor[_curCursor]->getSize(&curWidth,&curHeight);
	size_t count;
	uint8_t *src,*dst;
	y1 = y;
	y2 = esc::Util::min(yres,y + height);
	width = esc::Util::min(xres - x,width);
	count = width * pxSize;

	/* copy from shared-mem to video-mem */
	dst = (uint8_t*)scr->frmbuf + (y1 * xres + x) * pxSize;
	src = (uint8_t*)shmem + (y1 * xres + x) * pxSize;
	while(y1 < y2) {
		memcpy(dst,src,count);
		src += xres * pxSize;
		dst += xres * pxSize;
		y1++;
	}

	gui::Rectangle upRec(x,y,width,height);
	gui::Rectangle curRec(_lastX,_lastY,curWidth,curHeight);

	/* look if we have to update the cursor-copy */
	gui::Rectangle intersec = gui::intersection(curRec,upRec);
	if(!intersec.empty()) {
		copyRegion(scr,(uint8_t*)shmem,_cursorCopy,intersec.width(),intersec.height(),
			intersec.x(),intersec.y(),intersec.x() - _lastX,intersec.y() - _lastY,xres,curWidth,yres);
		_cursor[_curCursor]->paint(scr,_lastX,_lastY);
	}
}

void VESAGUI::doSetCursor(VESAScreen *scr,void *shmem,gpos_t x,gpos_t y,int newCursor) {
	if(newCursor < 0 || (size_t)newCursor >= ARRAY_SIZE(_cursor))
		return;

	gsize_t xres = scr->mode->width;
	gsize_t yres = scr->mode->height;
	/* validate position */
	x = esc::Util::min(x,(gpos_t)(xres - 1));
	y = esc::Util::min(y,(gpos_t)(yres - 1));

	if(_lastX != x || _lastY != y || newCursor != _curCursor) {
		/* copy old content back */
		gsize_t oldWidth,oldHeight;
		_cursor[_curCursor]->getSize(&oldWidth,&oldHeight);
		gsize_t upHeight = esc::Util::min(oldHeight,yres - _lastY);
		copyRegion(scr,_cursorCopy,scr->frmbuf,oldWidth,upHeight,0,0,_lastX,_lastY,oldWidth,xres,oldHeight);

		/* save content */
		gsize_t curWidth,curHeight;
		_cursor[newCursor]->getSize(&curWidth,&curHeight);
		copyRegion(scr,(uint8_t*)shmem,_cursorCopy,curWidth,curHeight,x,y,0,0,xres,curWidth,yres);
	}

	_cursor[newCursor]->paint(scr,x,y);
	_lastX = x;
	_lastY = y;
	_curCursor = newCursor;
}

void VESAGUI::copyRegion(VESAScreen *scr,uint8_t *src,uint8_t *dst,gsize_t width,gsize_t height,
		gpos_t x1,gpos_t y1,gpos_t x2,gpos_t y2,gsize_t w1,gsize_t w2,gsize_t h1) {
	gpos_t maxy = esc::Util::min(h1,y1 + height);
	gsize_t pxSize = scr->mode->bitsPerPixel / 8;
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
