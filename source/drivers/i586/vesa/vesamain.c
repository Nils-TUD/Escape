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
#include <esc/arch/i586/ports.h>
#include <esc/io.h>
#include <esc/driver.h>
#include <esc/debug.h>
#include <esc/mem.h>
#include <esc/rect.h>
#include <esc/proc.h>
#include <esc/thread.h>
#include <esc/messages.h>
#include <esc/sllist.h>
#include <stdio.h>
#include <errors.h>
#include <string.h>
#include <stdlib.h>

#include "vesa.h"
#include "bmp.h"
#include <vbe/vbe.h>

#define CURSOR_DEFAULT_FILE				"/etc/cursor_def.bmp"
#define CURSOR_RESIZE_L_FILE			"/etc/cursor_resl.bmp"
#define CURSOR_RESIZE_BR_FILE			"/etc/cursor_resbr.bmp"
#define CURSOR_RESIZE_VERT_FILE			"/etc/cursor_resvert.bmp"
#define CURSOR_RESIZE_BL_FILE			"/etc/cursor_resbl.bmp"
#define CURSOR_RESIZE_R_FILE			"/etc/cursor_resr.bmp"

#define RESOLUTION_X					1024
#define RESOLUTION_Y					768
#define BITS_PER_PIXEL					24

#define MERGE_TOLERANCE					40
#define MAX_REQC						30

#define ABS(x)							((x) > 0 ? (x) : -(x))

static void vesa_doUpdate(void);
static int vesa_setMode(void);
static int vesa_init(void);
static void vesa_update(tCoord x,tCoord y,tSize width,tSize height);
static void vesa_setPixel16(tCoord x,tCoord y,tColor col);
static void vesa_setPixel24(tCoord x,tCoord y,tColor col);
static void vesa_setPixel32(tCoord x,tCoord y,tColor col);
static void vesa_setCursor(tCoord x,tCoord y);
static void vesa_copyRegion(uint8_t *src,uint8_t *dst,tSize width,tSize height,tCoord x1,tCoord y1,
		tCoord x2,tCoord y2,tSize w1,tSize w2,tSize h1);

static sSLList *dirtyRects;
static tCoord newCurX,newCurY;
static bool updCursor = false;

static sVbeModeInfo *minfo = NULL;
static void *video;
static void *shmem;
static uint8_t *cursorCopy;
static tCoord lastX = 0;
static tCoord lastY = 0;
static sMsg msg;
static uint8_t curCursor = CURSOR_DEFAULT;
static sBitmap *cursor[6];
static fSetPixel setPixel[] = {
	/* 0 bpp */		NULL,
	/* 8 bpp */		NULL,
	/* 16 bpp */	vesa_setPixel16,
	/* 24 bpp */	vesa_setPixel24,
	/* 32 bpp */	vesa_setPixel32
};

int main(void) {
	int id;
	msgid_t mid;
	size_t reqc;
	bool enabled = true;

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

	vbe_init();
	if(vesa_setMode() < 0)
		error("Unable to set mode");

	dirtyRects = sll_create();
	id = regDriver("vesa",0);
	if(id < 0)
		error("Unable to register driver 'vesa'");

	reqc = 0;
	while(1) {
		int fd = getWork(&id,1,NULL,&mid,&msg,sizeof(msg),GW_NOBLOCK);
		if(fd < 0 || reqc >= MAX_REQC) {
			reqc = 0;
			vesa_doUpdate();
		}
		if(fd < 0)
			wait(EV_CLIENT,id);
		else {
			reqc++;
			switch(mid) {
				case MSG_VESA_UPDATE: {
					tCoord x = (tCoord)msg.args.arg1;
					tCoord y = (tCoord)msg.args.arg2;
					tSize width = (tSize)msg.args.arg3;
					tSize height = (tSize)msg.args.arg4;
					if(enabled &&
							x >= 0 && y >= 0 && x < minfo->xResolution && y < minfo->yResolution &&
							x + width <= minfo->xResolution && y + height <= minfo->yResolution &&
							/* check for overflow */
							x + width > x && y + height > y) {
						/* first check if we can merge this rectangle with another one.
						 * maybe we have even got exactly this rectangle... */
						sSLNode *n;
						bool present = false;
						sRectangle *add = (sRectangle*)malloc(sizeof(sRectangle));
						add->x = x;
						add->y = y;
						add->width = width;
						add->height = height;
						for(n = sll_begin(dirtyRects); n != NULL; n = n->next) {
							sRectangle *r = (sRectangle*)n->data;
							if(ABS(r->x - x) < MERGE_TOLERANCE &&
									ABS(r->y - y) < MERGE_TOLERANCE &&
									ABS(r->width - width) < MERGE_TOLERANCE &&
									ABS(r->height - height) < MERGE_TOLERANCE) {
								/* mergable, so do it */
								rectAdd(r,add);
								free(add);
								present = true;
								break;
							}
						}
						/* if not present yet, add it */
						if(!present)
							sll_append(dirtyRects,add);
					}
				}
				break;

				case MSG_VESA_SETMODE: {
					if(minfo)
						vbe_setMode(minfo->modeNo);
				}
				break;

				case MSG_VESA_CURSOR: {
					newCurX = (tCoord)msg.args.arg1;
					newCurY = (tCoord)msg.args.arg2;
					curCursor = ((uint8_t)msg.args.arg3) % 6;
					updCursor = true;
				}
				break;

				case MSG_VESA_GETMODE: {
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

				case MSG_VESA_ENABLE:
					enabled = true;
					break;

				case MSG_VESA_DISABLE:
					enabled = false;
					break;

				default:
					msg.args.arg1 = ERR_UNSUPPORTED_OP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}

	close(id);
	free(cursorCopy);
	destroySharedMem("vesa");
	return EXIT_SUCCESS;
}

static void vesa_doUpdate(void) {
	sSLNode *n;
	for(n = sll_begin(dirtyRects); n != NULL; n = n->next) {
		sRectangle *r = (sRectangle*)n->data;
		vesa_update(r->x,r->y,r->width,r->height);
		free(r);
	}
	sll_clear(dirtyRects);

	if(updCursor) {
		vesa_setCursor(newCurX,newCurY);
		updCursor = false;
	}
}

static int vesa_setMode(void) {
	uint mode = vbe_findMode(RESOLUTION_X,RESOLUTION_Y,BITS_PER_PIXEL);
	if(mode != 0) {
		minfo = vbe_getModeInfo(mode);
		if(minfo) {
			video = mapPhysical(minfo->physBasePtr,minfo->xResolution *
					minfo->yResolution * (minfo->bitsPerPixel / 8));
			printf("[VESA] Setting (%x) %4d x %4d x %2d\n",mode,
					minfo->xResolution,minfo->yResolution,minfo->bitsPerPixel);
			fflush(stdout);
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

static int vesa_init(void) {
	tSize curWidth = cursor[curCursor]->infoHeader->width;
	tSize curHeight = cursor[curCursor]->infoHeader->height;
	shmem = createSharedMem("vesa",minfo->xResolution *
			minfo->yResolution * (minfo->bitsPerPixel / 8));
	if(shmem == NULL)
		return ERR_NOT_ENOUGH_MEM;

	cursorCopy = (uint8_t*)malloc(curWidth * curHeight * (minfo->bitsPerPixel / 8));
	if(cursorCopy == NULL)
		return ERR_NOT_ENOUGH_MEM;

	/* black screen */
	memclear(video,minfo->xResolution * minfo->yResolution * (minfo->bitsPerPixel / 8));
	return 0;
}

static void vesa_update(tCoord x,tCoord y,tSize width,tSize height) {
	sRectangle upRec,curRec,intersec;
	tCoord y1,y2;
	tSize xres = minfo->xResolution;
	tSize yres = minfo->yResolution;
	tSize pxSize = minfo->bitsPerPixel / 8;
	tSize curWidth = cursor[curCursor]->infoHeader->width;
	tSize curHeight = cursor[curCursor]->infoHeader->height;
	size_t count;
	uint8_t *src,*dst;
	y1 = y;
	y2 = MIN(yres,y + height);
	width = MIN(xres - x,width);
	count = width * pxSize;

	/* copy from shared-mem to video-mem */
	dst = (uint8_t*)video + (y1 * xres + x) * pxSize;
	src = (uint8_t*)shmem + (y1 * xres + x) * pxSize;
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
			intersec.x,intersec.y,intersec.x - lastX,intersec.y - lastY,xres,curWidth,yres);
		bmp_draw(cursor[curCursor],lastX,lastY,setPixel[pxSize]);
	}
}

static void vesa_setPixel16(tCoord x,tCoord y,tColor col) {
	tSize xres = minfo->xResolution;
	if(x >= xres || y >= minfo->yResolution)
		return;
	uint16_t *data = (uint16_t*)((uintptr_t)video + (y * xres + x) * 2);
	uint8_t red = (col >> 16) >> (8 - minfo->redMaskSize);
	uint8_t green = (col >> 8) >> (8 - minfo->greenMaskSize);
	uint8_t blue = (col & 0xFF) >> (8 - minfo->blueMaskSize);
	uint16_t val = (red << minfo->redFieldPosition) |
			(green << minfo->greenFieldPosition) |
			(blue << minfo->blueFieldPosition);
	*data = val;
}

static void vesa_setPixel24(tCoord x,tCoord y,tColor col) {
	tSize xres = minfo->xResolution;
	if(x >= xres || y >= minfo->yResolution)
		return;
	uint8_t *data = (uint8_t*)video + (y * xres + x) * 3;
	uint8_t red = (col >> 16) >> (8 - minfo->redMaskSize);
	uint8_t green = (col >> 8) >> (8 - minfo->greenMaskSize);
	uint8_t blue = (col & 0xFF) >> (8 - minfo->blueMaskSize);
	uint32_t val = (red << minfo->redFieldPosition) |
			(green << minfo->greenFieldPosition) |
			(blue << minfo->blueFieldPosition);
	data[2] = val >> 16;
	data[1] = val >> 8;
	data[0] = val & 0xFF;
}

static void vesa_setPixel32(tCoord x,tCoord y,tColor col) {
	tSize xres = minfo->xResolution;
	if(x >= xres || y >= minfo->yResolution)
		return;
	uint8_t *data = (uint8_t*)video + (y * xres + x) * 4;
	uint8_t red = (col >> 16) >> (8 - minfo->redMaskSize);
	uint8_t green = (col >> 8) >> (8 - minfo->greenMaskSize);
	uint8_t blue = (col & 0xFF) >> (8 - minfo->blueMaskSize);
	uint32_t val = (red << minfo->redFieldPosition) |
			(green << minfo->greenFieldPosition) |
			(blue << minfo->blueFieldPosition);
	*((uint32_t*)data) = val;
}

static void vesa_setCursor(tCoord x,tCoord y) {
	tSize curWidth = cursor[curCursor]->infoHeader->width;
	tSize curHeight = cursor[curCursor]->infoHeader->height;
	tSize xres = minfo->xResolution;
	tSize yres = minfo->yResolution;
	/* validate position */
	x = MIN(x,xres - 1);
	y = MIN(y,yres - 1);

	if(lastX != x || lastY != y) {
		tSize upHeight = MIN(curHeight,yres - lastY);
		/* copy old content back */
		vesa_copyRegion(cursorCopy,video,curWidth,upHeight,0,0,lastX,lastY,curWidth,xres,curHeight);
		/* save content */
		vesa_copyRegion(video,cursorCopy,curWidth,curHeight,x,y,0,0,xres,curWidth,yres);
	}

	bmp_draw(cursor[curCursor],x,y,setPixel[minfo->bitsPerPixel / 8]);
	lastX = x;
	lastY = y;
}

static void vesa_copyRegion(uint8_t *src,uint8_t *dst,tSize width,tSize height,tCoord x1,tCoord y1,
		tCoord x2,tCoord y2,tSize w1,tSize w2,tSize h1) {
	tCoord maxy = MIN(h1,y1 + height);
	tSize pxSize = minfo->bitsPerPixel / 8;
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
