/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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

#include <esc/common.h>
#include <esc/ports.h>
#include <esc/fileio.h>
#include <esc/io.h>
#include <esc/service.h>
#include <messages.h>
#include <esc/debug.h>
#include <esc/mem.h>
#include <esc/rect.h>
#include <stdlib.h>
#include <errors.h>

#include <vbe/vbe.h>

#define RESOLUTION_X					1024
#define RESOLUTION_Y					768
#define BITS_PER_PIXEL					24

#define CURSOR_LEN						2
#define CURSOR_COLOR					0xFFFFFF
#define CURSOR_SIZE						(CURSOR_LEN * 2 + 1)

typedef u16 tSize;
typedef s16 tCoord;
typedef u32 tColor;

static s32 vesa_setMode(void);
static s32 vesa_init(void);
static void vesa_update(tCoord x,tCoord y,tSize width,tSize height);
static void vesa_setCursor(tCoord x,tCoord y);
static void vesa_drawCross(tCoord x,tCoord y);
static tColor vesa_getVisibleFGColor(tColor bg);
static void vesa_copyRegion(u8 *src,u8 *dst,tSize width,tSize height,tCoord x1,tCoord y1,
		tCoord x2,tCoord y2,tSize w1,tSize w2);

static sVbeModeInfo *minfo = NULL;
static void *video;
static void *shmem;
static u8 *cursorCopy;
static tCoord lastX = 0;
static tCoord lastY = 0;
static sMsg msg;

int main(void) {
	tServ id,client;
	tMsgId mid;

	vbe_init();
	if(vesa_setMode() < 0)
		error("Unable to set mode");

	id = regService("vesa",SERV_DEFAULT);
	if(id < 0)
		error("Unable to register service 'vesa'");

	while(1) {
		tFD fd = getClient(&id,1,&client);
		if(fd < 0)
			wait(EV_CLIENT);
		else {
			while(receive(fd,&mid,&msg,sizeof(msg)) > 0) {
				switch(mid) {
					case MSG_VESA_UPDATE: {
						tCoord x = (tCoord)msg.args.arg1;
						tCoord y = (tCoord)msg.args.arg2;
						tSize width = (tSize)msg.args.arg3;
						tSize height = (tSize)msg.args.arg4;
						if(x >= 0 && y >= 0 && x < minfo->xResolution && y < minfo->yResolution &&
							x + width <= minfo->xResolution && y + height <= minfo->yResolution &&
							/* check for overflow */
							x + width > x && y + height > y) {
							vesa_update(x,y,width,height);
						}
					}
					break;

					case MSG_VESA_CURSOR: {
						tCoord x = (tCoord)msg.args.arg1;
						tCoord y = (tCoord)msg.args.arg2;
						vesa_setCursor(x,y);
					}
					break;

					case MSG_VESA_GETMODE_REQ: {
						msg.args.arg1 = minfo->xResolution;
						msg.args.arg2 = minfo->yResolution;
						msg.args.arg3 = minfo->bitsPerPixel;
						send(fd,MSG_VESA_GETMODE_RESP,&msg,sizeof(msg.args));
					}
					break;
				}
			}
			close(fd);
		}
	}

	unregService(id);
	free(cursorCopy);
	destroySharedMem("vesa");
	return EXIT_SUCCESS;
}

static s32 vesa_setMode(void) {
	u16 mode = vbe_findMode(RESOLUTION_X,RESOLUTION_Y,BITS_PER_PIXEL);
	if(mode != 0) {
		minfo = vbe_getModeInfo(mode);
		if(minfo) {
			video = mapPhysical(minfo->physBasePtr,minfo->xResolution *
					minfo->yResolution * (minfo->bitsPerPixel / 8));
			if(video == NULL)
				return errno;
			if(vbe_setMode(mode))
				return vesa_init();
			minfo = NULL;
			return ERR_VESA_SETMODE_FAILED;
		}
	}
	return ERR_VESA_MODE_NOT_FOUND;
}

static s32 vesa_init(void) {
	shmem = createSharedMem("vesa",minfo->xResolution *
			minfo->yResolution * (minfo->bitsPerPixel / 8));
	if(shmem == NULL)
		return ERR_NOT_ENOUGH_MEM;

	cursorCopy = (u8*)malloc(CURSOR_SIZE * CURSOR_SIZE * (minfo->bitsPerPixel / 8));
	if(cursorCopy == NULL)
		return ERR_NOT_ENOUGH_MEM;
	return 0;
}

static void vesa_update(tCoord x,tCoord y,tSize width,tSize height) {
	static sRectangle upRec,curRec,intersect;
	tCoord y1,y2;
	tSize xres = minfo->xResolution;
	tSize pxSize = minfo->bitsPerPixel / 8;
	u32 count;
	u8 *src,*dst;
	y1= y;
	y2 = y + height;
	count = width * pxSize;

	/* look if we have to update the cursor-copy */
	upRec.x = x;
	upRec.y = y;
	upRec.width = width;
	upRec.height = height;
	curRec.x = MAX(0,lastX - CURSOR_LEN);
	curRec.y = MAX(0,lastY - CURSOR_LEN);
	curRec.width = CURSOR_SIZE;
	curRec.height = CURSOR_SIZE;
	if(rectIntersect(&upRec,&curRec,&intersect)) {
		vesa_copyRegion(shmem,cursorCopy,intersect.width,intersect.height,intersect.x,intersect.y,
				intersect.x - curRec.x,intersect.y - curRec.y,xres,CURSOR_SIZE);
	}

	/* copy from shared-mem to video-mem */
	dst = (u8*)video + (y1 * xres + x) * pxSize;
	src = (u8*)shmem + (y1 * xres + x) * pxSize;
	while(y1 < y2) {
		memcpy(dst,src,count);
		src += xres * pxSize;
		dst += xres * pxSize;
		y1++;
	}
}

static void vesa_setCursor(tCoord x,tCoord y) {
	tCoord cx,cy;
	tSize xres = minfo->xResolution;
	tSize yres = minfo->yResolution;
	/* validate position */
	x = MIN(x,xres - 1);
	y = MIN(y,yres - 1);

	if(lastX != x || lastY != y) {
		cx = MAX(0,lastX - CURSOR_LEN);
		cy = MAX(0,lastY - CURSOR_LEN);
		/* copy old content back */
		vesa_copyRegion(cursorCopy,video,CURSOR_SIZE,CURSOR_SIZE,0,0,cx,cy,CURSOR_SIZE,xres);
	}
	/* save content */
	cx = MAX(0,x - CURSOR_LEN);
	cy = MAX(0,y - CURSOR_LEN);
	vesa_copyRegion(video,cursorCopy,CURSOR_SIZE,CURSOR_SIZE,cx,cy,0,0,xres,CURSOR_SIZE);

	vesa_drawCross(x,y);
	lastX = x;
	lastY = y;
}

static void vesa_drawCross(tCoord x,tCoord y) {
	tColor color = CURSOR_COLOR;
	tSize xres = minfo->xResolution;
	tSize yres = minfo->yResolution;
	tSize pxSize = minfo->bitsPerPixel / 8;
	u8 *mid = (u8*)video + (y * xres + x) * pxSize;
	u8 *ccopyMid = cursorCopy + (CURSOR_LEN * CURSOR_SIZE + CURSOR_LEN) * pxSize;

	/* draw pixel at cursor */
	if(x < xres && y < yres) {
		memcpy(&color,ccopyMid,pxSize);
		color = vesa_getVisibleFGColor(color);
		memcpy(mid,&color,pxSize);
	}
	/* draw left */
	if(x >= CURSOR_LEN) {
		memcpy(&color,ccopyMid - CURSOR_LEN * pxSize,pxSize);
		color = vesa_getVisibleFGColor(color);
		memcpy(mid - CURSOR_LEN * pxSize,&color,pxSize);
	}
	/* draw top */
	if(y >= CURSOR_LEN) {
		memcpy(&color,ccopyMid - CURSOR_LEN * CURSOR_SIZE * pxSize,pxSize);
		color = vesa_getVisibleFGColor(color);
		memcpy(mid - CURSOR_LEN * xres * pxSize,&color,pxSize);
	}
	/* draw right */
	if(x < xres - CURSOR_LEN) {
		memcpy(&color,ccopyMid + CURSOR_LEN * pxSize,pxSize);
		color = vesa_getVisibleFGColor(color);
		memcpy(mid + CURSOR_LEN * pxSize,&color,pxSize);
	}
	/* draw bottom */
	if(y < yres - CURSOR_LEN) {
		memcpy(&color,ccopyMid + CURSOR_LEN * CURSOR_SIZE * pxSize,pxSize);
		color = vesa_getVisibleFGColor(color);
		memcpy(mid + CURSOR_LEN * xres * pxSize,&color,pxSize);
	}
}

static tColor vesa_getVisibleFGColor(tColor bg) {
	/* NOTE: THIS ASSUMES 24BIT MODE! */
	u32 red = (u8)(bg >> 16);
	u32 green = (u8)(bg >> 8);
	u32 blue = (u8)(bg & 0xFF);
	/*
	 * formular of W3C for the brightness of a color:
	 * 	((Red value X 299) + (Green value X 587) + (Blue value X 114)) / 1000
	 */
	if((red * 299 + green * 587 + blue * 114) > 128000)
		return 0;
	return 0xFFFFFFFF;
}

static void vesa_copyRegion(u8 *src,u8 *dst,tSize width,tSize height,tCoord x1,tCoord y1,
		tCoord x2,tCoord y2,tSize w1,tSize w2) {
	tCoord maxy = y1 + height;
	tSize pxSize = minfo->bitsPerPixel / 8;
	u32 count = width * pxSize;
	u32 srcInc = w1 * pxSize;
	u32 dstInc = w2 * pxSize;
	src += (y1 * w1 + x1) * pxSize;
	dst += (y2 * w2 + x2) * pxSize;
	while(y1 < maxy) {
		memcpy(dst,src,count);
		src += srcInc;
		dst += dstInc;
		y1++;
	}
}
