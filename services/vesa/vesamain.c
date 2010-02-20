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

#include "vesa.h"
#include "bmp.h"
#include <vbe/vbe.h>

#define CURSOR_FILE						"/etc/cursor.bmp"

#define RESOLUTION_X					800
#define RESOLUTION_Y					600
#define BITS_PER_PIXEL					16

#define CURSOR_LEN						2
#define CURSOR_COLOR					0xFFFFFF
#define CURSOR_SIZE						(CURSOR_LEN * 2 + 1)

static s32 vesa_setMode(void);
static s32 vesa_init(void);
static void vesa_update(tCoord x,tCoord y,tSize width,tSize height);
static void vesa_setPixel16(tCoord x,tCoord y,tColor col);
static void vesa_setPixel24(tCoord x,tCoord y,tColor col);
static void vesa_setPixel32(tCoord x,tCoord y,tColor col);
static void vesa_setCursor(tCoord x,tCoord y);
static void vesa_copyRegion(u8 *src,u8 *dst,tSize width,tSize height,tCoord x1,tCoord y1,
		tCoord x2,tCoord y2,tSize w1,tSize w2);

static sVbeModeInfo *minfo = NULL;
static void *video;
static void *shmem;
static u8 *cursorCopy;
static tCoord lastX = 0;
static tCoord lastY = 0;
static sMsg msg;
static sBitmap *bmp;
static fSetPixel setPixel[] = {
	/* 0 bpp */		NULL,
	/* 8 bpp */		NULL,
	/* 16 bpp */	vesa_setPixel16,
	/* 24 bpp */	vesa_setPixel24,
	/* 32 bpp */	vesa_setPixel32
};

int main(void) {
	tServ id,client;
	tMsgId mid;

	bmp = bmp_loadFromFile(CURSOR_FILE);
	if(!bmp)
		error("Unable to load bitmap from %s",CURSOR_FILE);

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
						sVESAInfo *info = (sVESAInfo*)malloc(sizeof(sVESAInfo));
						msg.data.arg1 = ERR_NOT_ENOUGH_MEM;
						if(info) {
							msg.data.arg1 = 0;
							info->width = minfo->xResolution;
							info->height = minfo->yResolution;
							info->bitsPerPixel = minfo->bitsPerPixel;
							info->redFieldPosition = minfo->redFieldPosition;
							info->redMaskSize = minfo->redMaskSize;
							info->greenFieldPosition = minfo->greenFieldPosition;
							info->greenMaskSize = minfo->greenMaskSize;
							info->blueFieldPosition = minfo->blueFieldPosition;
							info->blueMaskSize = minfo->blueMaskSize;
							memcpy(msg.data.d,info,sizeof(sVESAInfo));
						}
						send(fd,MSG_VESA_GETMODE_RESP,&msg,sizeof(msg.data));
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
			debugf("Setting (%x) %4d x %4d x %2d\n",mode,
					minfo->xResolution,minfo->yResolution,minfo->bitsPerPixel);
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
	tSize curWidth = bmp->infoHeader->width;
	tSize curHeight = bmp->infoHeader->height;
	shmem = createSharedMem("vesa",minfo->xResolution *
			minfo->yResolution * (minfo->bitsPerPixel / 8));
	if(shmem == NULL)
		return ERR_NOT_ENOUGH_MEM;

	cursorCopy = (u8*)malloc(curWidth * curHeight * (minfo->bitsPerPixel / 8));
	if(cursorCopy == NULL)
		return ERR_NOT_ENOUGH_MEM;
	return 0;
}

static void vesa_update(tCoord x,tCoord y,tSize width,tSize height) {
	sRectangle upRec,curRec,intersec;
	tCoord y1,y2;
	tSize xres = minfo->xResolution;
	tSize pxSize = minfo->bitsPerPixel / 8;
	tSize curWidth = bmp->infoHeader->width;
	tSize curHeight = bmp->infoHeader->height;
	u32 count;
	u8 *src,*dst;
	y1 = y;
	y2 = y + height;
	count = width * pxSize;

	/* copy from shared-mem to video-mem */
	dst = (u8*)video + (y1 * xres + x) * pxSize;
	src = (u8*)shmem + (y1 * xres + x) * pxSize;
	while(y1 < y2) {
		memcpy(dst,src,count);
		src += xres * pxSize;
		dst += xres * pxSize;
		y1++;
	}

	/* look if we have to update the cursor-copy */
	upRec.x = x;
	upRec.y = y;
	upRec.width = width;
	upRec.height = height;
	curRec.x = lastX;
	curRec.y = lastY;
	curRec.width = curWidth;
	curRec.height = curHeight;
	if(rectIntersect(&curRec,&upRec,&intersec)) {
		vesa_copyRegion(video,cursorCopy,intersec.width,intersec.height,
			intersec.x,intersec.y,intersec.x - lastX,intersec.y - lastY,xres,curWidth);
		bmp_draw(bmp,lastX,lastY,setPixel[pxSize]);
	}
}

static void vesa_setPixel16(tCoord x,tCoord y,tColor col) {
	tSize xres = minfo->xResolution;
	if(x >= xres)
		return;
	u16 *data = (u16*)((u8*)video + (y * xres + x) * 2);
	u8 red = (col >> 16) >> (8 - minfo->redMaskSize);
	u8 green = (col >> 8) >> (8 - minfo->greenMaskSize);
	u8 blue = (col & 0xFF) >> (8 - minfo->blueMaskSize);
	u16 val = (red << minfo->redFieldPosition) |
			(green << minfo->greenFieldPosition) |
			(blue << minfo->blueFieldPosition);
	*data = val;
}

static void vesa_setPixel24(tCoord x,tCoord y,tColor col) {
	tSize xres = minfo->xResolution;
	if(x >= xres)
		return;
	u8 *data = (u8*)video + (y * xres + x) * 3;
	u8 red = (col >> 16) >> (8 - minfo->redMaskSize);
	u8 green = (col >> 8) >> (8 - minfo->greenMaskSize);
	u8 blue = (col & 0xFF) >> (8 - minfo->blueMaskSize);
	u32 val = (red << minfo->redFieldPosition) |
			(green << minfo->greenFieldPosition) |
			(blue << minfo->blueFieldPosition);
	data[2] = val >> 16;
	data[1] = val >> 8;
	data[0] = val & 0xFF;
}

static void vesa_setPixel32(tCoord x,tCoord y,tColor col) {
	tSize xres = minfo->xResolution;
	if(x >= xres)
		return;
	u8 *data = (u8*)video + (y * xres + x) * 4;
	u8 red = (col >> 16) >> (8 - minfo->redMaskSize);
	u8 green = (col >> 8) >> (8 - minfo->greenMaskSize);
	u8 blue = (col & 0xFF) >> (8 - minfo->blueMaskSize);
	u32 val = (red << minfo->redFieldPosition) |
			(green << minfo->greenFieldPosition) |
			(blue << minfo->blueFieldPosition);
	*((u32*)data) = val;
}

static void vesa_setCursor(tCoord x,tCoord y) {
	tSize curWidth = bmp->infoHeader->width;
	tSize curHeight = bmp->infoHeader->height;
	tSize xres = minfo->xResolution;
	tSize yres = minfo->yResolution;
	/* validate position */
	x = MIN(x,xres - 1);
	y = MIN(y,yres - 1);

	if(lastX != x || lastY != y) {
		/* copy old content back */
		vesa_copyRegion(cursorCopy,video,curWidth,curHeight,0,0,lastX,lastY,curWidth,xres);
		/* save content */
		vesa_copyRegion(video,cursorCopy,curWidth,curHeight,x,y,0,0,xres,curWidth);
	}

	bmp_draw(bmp,x,y,setPixel[minfo->bitsPerPixel / 8]);
	lastX = x;
	lastY = y;
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
