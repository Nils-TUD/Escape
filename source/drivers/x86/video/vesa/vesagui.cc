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

#include <sys/common.h>
#include <sys/io.h>
#include <sys/driver.h>
#include <sys/mman.h>
#include <sys/messages.h>
#include <gui/graphics/rectangle.h>
#include <vbe/vbe.h>
#include <string.h>
#include <stdlib.h>

#include "vesagui.h"
#include "bmp.h"
#include "vesascreen.h"

#define CURSOR_LEFT_OFF					10
#define CURSOR_DEFAULT_FILE				"/etc/cursor_def.bmp"
#define CURSOR_RESIZE_L_FILE			"/etc/cursor_resl.bmp"
#define CURSOR_RESIZE_BR_FILE			"/etc/cursor_resbr.bmp"
#define CURSOR_RESIZE_VERT_FILE			"/etc/cursor_resvert.bmp"
#define CURSOR_RESIZE_BL_FILE			"/etc/cursor_resbl.bmp"
#define CURSOR_RESIZE_R_FILE			"/etc/cursor_resr.bmp"

static void vesagui_doSetCursor(sVESAScreen *scr,void *shmem,gpos_t x,gpos_t y,int newCursor);
static void vesagui_copyRegion(sVESAScreen *scr,uint8_t *src,uint8_t *dst,gsize_t width,gsize_t height,
		gpos_t x1,gpos_t y1,gpos_t x2,gpos_t y2,gsize_t w1,gsize_t w2,gsize_t h1);

static uint8_t *cursorCopy;
static gpos_t lastX = 0;
static gpos_t lastY = 0;
static uint8_t curCursor = esc::Screen::CURSOR_DEFAULT;
static sBitmap *cursor[6];

void vesagui_init(void) {
	cursor[0] = bmp_loadFromFile(CURSOR_DEFAULT_FILE);
	if(cursor[0] == NULL)
		error("Unable to load bitmap from %s",CURSOR_DEFAULT_FILE);
	cursor[1] = bmp_loadFromFile(CURSOR_RESIZE_L_FILE);
	if(cursor[1] == NULL)
		error("Unable to load bitmap from %s",CURSOR_RESIZE_L_FILE);
	cursor[2] = bmp_loadFromFile(CURSOR_RESIZE_BR_FILE);
	if(cursor[2] == NULL)
		error("Unable to load bitmap from %s",CURSOR_RESIZE_BR_FILE);
	cursor[3] = bmp_loadFromFile(CURSOR_RESIZE_VERT_FILE);
	if(cursor[3] == NULL)
		error("Unable to load bitmap from %s",CURSOR_RESIZE_VERT_FILE);
	cursor[4] = bmp_loadFromFile(CURSOR_RESIZE_BL_FILE);
	if(cursor[4] == NULL)
		error("Unable to load bitmap from %s",CURSOR_RESIZE_BL_FILE);
	cursor[5] = bmp_loadFromFile(CURSOR_RESIZE_R_FILE);
	if(cursor[5] == NULL)
		error("Unable to load bitmap from %s",CURSOR_RESIZE_R_FILE);

	gsize_t curWidth = cursor[curCursor]->infoHeader->width;
	gsize_t curHeight = cursor[curCursor]->infoHeader->height;
	cursorCopy = (uint8_t*)malloc(curWidth * curHeight * 4);
	if(cursorCopy == NULL)
		error("Unable to create cursor copy");
}

void vesagui_setCursor(sVESAScreen *scr,void *shmem,int newCurX,int newCurY,int newCursor) {
	if(newCursor == esc::Screen::CURSOR_RESIZE_L || newCursor == esc::Screen::CURSOR_RESIZE_BL)
		newCurX -= CURSOR_LEFT_OFF;
	vesagui_doSetCursor(scr,shmem,newCurX,newCurY,newCursor);
}

void vesagui_update(sVESAScreen *scr,void *shmem,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	esc::Screen::Mode *minfo = scr->mode;
	gpos_t y1,y2;
	gsize_t xres = minfo->width;
	gsize_t yres = minfo->height;
	gsize_t pxSize = minfo->bitsPerPixel / 8;
	gsize_t curWidth = cursor[curCursor]->infoHeader->width;
	gsize_t curHeight = cursor[curCursor]->infoHeader->height;
	size_t count;
	uint8_t *src,*dst;
	y1 = y;
	y2 = MIN(yres,y + height);
	width = MIN(xres - x,width);
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
	gui::Rectangle curRec(lastX,lastY,curWidth,curHeight);

	/* look if we have to update the cursor-copy */
	gui::Rectangle intersec = gui::intersection(curRec,upRec);
	if(!intersec.empty()) {
		vesagui_copyRegion(scr,(uint8_t*)shmem,cursorCopy,intersec.width(),intersec.height(),
			intersec.x(),intersec.y(),intersec.x() - lastX,intersec.y() - lastY,xres,curWidth,yres);
		bmp_draw(scr,cursor[curCursor],lastX,lastY,vbe_getPixelFunc(*scr->mode));
	}
}

static void vesagui_doSetCursor(sVESAScreen *scr,void *shmem,gpos_t x,gpos_t y,int newCursor) {
	if(newCursor < 0 || (size_t)newCursor >= ARRAY_SIZE(cursor))
		return;

	gsize_t curWidth = cursor[newCursor]->infoHeader->width;
	gsize_t curHeight = cursor[newCursor]->infoHeader->height;
	gsize_t xres = scr->mode->width;
	gsize_t yres = scr->mode->height;
	/* validate position */
	x = MIN(x,(int)(xres - 1));
	y = MIN(y,(int)(yres - 1));

	if(lastX != x || lastY != y) {
		gsize_t upHeight = MIN(curHeight,yres - lastY);
		/* copy old content back */
		vesagui_copyRegion(scr,cursorCopy,scr->frmbuf,curWidth,upHeight,0,0,lastX,lastY,curWidth,xres,curHeight);
		/* save content */
		vesagui_copyRegion(scr,(uint8_t*)shmem,cursorCopy,curWidth,curHeight,x,y,0,0,xres,curWidth,yres);
	}

	bmp_draw(scr,cursor[newCursor],x,y,vbe_getPixelFunc(*scr->mode));
	lastX = x;
	lastY = y;
	curCursor = newCursor;
}

static void vesagui_copyRegion(sVESAScreen *scr,uint8_t *src,uint8_t *dst,gsize_t width,gsize_t height,
		gpos_t x1,gpos_t y1,gpos_t x2,gpos_t y2,gsize_t w1,gsize_t w2,gsize_t h1) {
	gpos_t maxy = MIN(h1,y1 + height);
	gsize_t pxSize = scr->mode->bitsPerPixel / 8;
	size_t count = MIN(w2 - x2,MIN(w1 - x1,width)) * pxSize;
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
