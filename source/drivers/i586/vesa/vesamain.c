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
static void vesa_update(gpos_t x,gpos_t y,gsize_t width,gsize_t height);
static void vesa_handlePreviewIntersec(sRectangle *curRec,sRectangle *intersec,size_t i,
		gsize_t xres,gsize_t yres);
static void vesa_setPixel16(gpos_t x,gpos_t y,tColor col);
static void vesa_setPixel24(gpos_t x,gpos_t y,tColor col);
static void vesa_setPixel32(gpos_t x,gpos_t y,tColor col);
static void vesa_setPreview(sRectangle *rect,gsize_t thickness);
static void vesa_setCursor(gpos_t x,gpos_t y);
static void vesa_clearRegion(gpos_t x,gpos_t y,gsize_t w,gsize_t h);
static void vesa_copyRegion(uint8_t *src,uint8_t *dst,gsize_t width,gsize_t height,gpos_t x1,gpos_t y1,
		gpos_t x2,gpos_t y2,gsize_t w1,gsize_t w2,gsize_t h1);

static sSLList *dirtyRects;
static gpos_t newCurX,newCurY;
static bool updCursor = false;
static bool setPreview;
static sRectangle newPreview;
static gsize_t newThickness;

static sVbeModeInfo *minfo = NULL;
static void *video;
static void *shmem;
static uint8_t *cursorCopy;
static sRectangle previewRect;
static gsize_t previewThickness;
static uint8_t *previewRectCopies[4];
static gpos_t lastX = 0;
static gpos_t lastY = 0;
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
					gpos_t x = (gpos_t)msg.args.arg1;
					gpos_t y = (gpos_t)msg.args.arg2;
					gsize_t width = (gsize_t)msg.args.arg3;
					gsize_t height = (gsize_t)msg.args.arg4;
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

				case MSG_VESA_PREVIEWRECT: {
					if(enabled) {
						setPreview = true;
						newPreview.x = (gpos_t)msg.args.arg1;
						newPreview.y = (gpos_t)msg.args.arg2;
						newPreview.width = (gsize_t)msg.args.arg3;
						newPreview.height = (gsize_t)msg.args.arg4;
						newThickness = (gsize_t)msg.args.arg5;
					}
				}
				break;

				case MSG_VESA_SETMODE: {
					if(minfo)
						vbe_setMode(minfo->modeNo);
				}
				break;

				case MSG_VESA_CURSOR: {
					newCurX = (gpos_t)msg.args.arg1;
					newCurY = (gpos_t)msg.args.arg2;
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

	if(setPreview) {
		vesa_setPreview(&newPreview,newThickness);
		setPreview = false;
	}

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
	gsize_t curWidth = cursor[curCursor]->infoHeader->width;
	gsize_t curHeight = cursor[curCursor]->infoHeader->height;
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

static void vesa_update(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	sRectangle upRec,curRec,intersec;
	gpos_t y1,y2;
	gsize_t xres = minfo->xResolution;
	gsize_t yres = minfo->yResolution;
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
	dst = (uint8_t*)video + (y1 * xres + x) * pxSize;
	src = (uint8_t*)shmem + (y1 * xres + x) * pxSize;
	while(y1 < y2) {
		memcpy(dst,src,count);
		src += xres * pxSize;
		dst += xres * pxSize;
		y1++;
	}

	upRec.x = x;
	upRec.y = y;
	upRec.width = width;
	upRec.height = height;

	/* look if we have to update the preview-rectangle */
	if(previewRectCopies[0]) {
		curRec.x = previewRect.x;
		curRec.y = previewRect.y;
		curRec.width = previewRect.width;
		curRec.height = previewThickness;
		/* top */
		if(rectIntersect(&curRec,&upRec,&intersec))
			vesa_handlePreviewIntersec(&curRec,&intersec,0,xres,yres);
		/* bottom */
		curRec.y = previewRect.y + previewRect.height - previewThickness;
		if(rectIntersect(&curRec,&upRec,&intersec))
			vesa_handlePreviewIntersec(&curRec,&intersec,2,xres,yres);
		/* left */
		curRec.y = previewRect.y;
		curRec.width = previewThickness;
		curRec.height = previewRect.height;
		if(rectIntersect(&curRec,&upRec,&intersec))
			vesa_handlePreviewIntersec(&curRec,&intersec,3,xres,yres);
		/* right */
		curRec.x = previewRect.x + previewRect.width - previewThickness;
		if(rectIntersect(&curRec,&upRec,&intersec))
			vesa_handlePreviewIntersec(&curRec,&intersec,1,xres,yres);
	}

	/* look if we have to update the cursor-copy */
	curRec.x = lastX;
	curRec.y = lastY;
	curRec.width = curWidth;
	curRec.height = curHeight;
	if(rectIntersect(&curRec,&upRec,&intersec)) {
		vesa_copyRegion(shmem,cursorCopy,intersec.width,intersec.height,
			intersec.x,intersec.y,intersec.x - lastX,intersec.y - lastY,xres,curWidth,yres);
		bmp_draw(cursor[curCursor],lastX,lastY,setPixel[pxSize]);
	}
}

static void vesa_handlePreviewIntersec(sRectangle *curRec,sRectangle *intersec,size_t i,
		gsize_t xres,gsize_t yres) {
	vesa_copyRegion(shmem,previewRectCopies[i],intersec->width,intersec->height,
		intersec->x,intersec->y,intersec->x - curRec->x,intersec->y - curRec->y,
		xres,curRec->width,yres);
	vesa_clearRegion(curRec->x,curRec->y,curRec->width,curRec->height);
}

static void vesa_setPixel16(gpos_t x,gpos_t y,tColor col) {
	gsize_t xres = minfo->xResolution;
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

static void vesa_setPixel24(gpos_t x,gpos_t y,tColor col) {
	gsize_t xres = minfo->xResolution;
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

static void vesa_setPixel32(gpos_t x,gpos_t y,tColor col) {
	gsize_t xres = minfo->xResolution;
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

static void vesa_setPreview(sRectangle *rect,gsize_t thickness) {
	gsize_t xres = minfo->xResolution;
	gsize_t yres = minfo->yResolution;
	gsize_t pxSize = minfo->bitsPerPixel / 8;

	if(previewRectCopies[0]) {
		/* copy old content back */
		/* top */
		vesa_copyRegion(previewRectCopies[0],video,
				previewRect.width,previewThickness,0,0,previewRect.x,previewRect.y,
				previewRect.width,xres,previewThickness);
		/* right */
		vesa_copyRegion(previewRectCopies[1],video,
				previewThickness,previewRect.height,0,0,
				previewRect.x + previewRect.width - previewThickness,previewRect.y,
				previewThickness,xres,previewRect.height);
		/* bottom */
		vesa_copyRegion(previewRectCopies[2],video,
				previewRect.width,previewThickness,0,0,previewRect.x,
				previewRect.y + previewRect.height - previewThickness,
				previewRect.width,xres,previewThickness);
		/* left */
		vesa_copyRegion(previewRectCopies[3],video,
				previewThickness,previewRect.height,0,0,previewRect.x,previewRect.y,
				previewThickness,xres,previewRect.height);

		if(thickness != previewThickness || rect->width != previewRect.width ||
				rect->height != previewRect.height) {
			free(previewRectCopies[0]);
			previewRectCopies[0] = NULL;
			free(previewRectCopies[1]);
			previewRectCopies[1] = NULL;
			free(previewRectCopies[2]);
			previewRectCopies[2] = NULL;
			free(previewRectCopies[3]);
			previewRectCopies[3] = NULL;
		}
	}

	if(thickness > 0) {
		if(thickness != previewThickness || rect->width != previewRect.width ||
				rect->height != previewRect.height) {
			previewRectCopies[0] = (uint8_t*)malloc(rect->width * thickness * pxSize);
			previewRectCopies[1] = (uint8_t*)malloc(rect->height * thickness * pxSize);
			previewRectCopies[2] = (uint8_t*)malloc(rect->width * thickness * pxSize);
			previewRectCopies[3] = (uint8_t*)malloc(rect->height * thickness * pxSize);
		}

		if(rect->x < 0) {
			rect->width = MAX(0,rect->width + rect->x);
			rect->x = 0;
		}
		if(rect->x > xres)
			rect->x = xres;
		if(rect->x + rect->width >= xres)
			rect->width = xres - rect->x;
		if(rect->y > yres)
			rect->y = yres;
		if(rect->y + rect->height >= yres)
			rect->height = yres - rect->y;
		memcpy(&previewRect,rect,sizeof(sRectangle));

		/* save content */
		/* top */
		vesa_copyRegion(shmem,previewRectCopies[0],
				rect->width,thickness,rect->x,rect->y,0,0,
				xres,rect->width,yres);
		/* right */
		vesa_copyRegion(shmem,previewRectCopies[1],
				thickness,rect->height,rect->x + rect->width - thickness,rect->y,0,0,
				xres,thickness,yres);
		/* bottom */
		vesa_copyRegion(shmem,previewRectCopies[2],
				rect->width,thickness,rect->x,rect->y + rect->height - thickness,0,0,
				xres,rect->width,yres);
		/* left */
		vesa_copyRegion(shmem,previewRectCopies[3],
				thickness,rect->height,rect->x,rect->y,0,0,
				xres,thickness,yres);

		/* draw rect */
		/* top */
		vesa_clearRegion(rect->x,rect->y,rect->width,thickness);
		/* right */
		vesa_clearRegion(rect->x + rect->width - thickness,rect->y,thickness,rect->height);
		/* bottom */
		vesa_clearRegion(rect->x,rect->y + rect->height - thickness,rect->width,thickness);
		/* left */
		vesa_clearRegion(rect->x,rect->y,thickness,rect->height);
	}
	previewThickness = thickness;
}

static void vesa_setCursor(gpos_t x,gpos_t y) {
	gsize_t curWidth = cursor[curCursor]->infoHeader->width;
	gsize_t curHeight = cursor[curCursor]->infoHeader->height;
	gsize_t xres = minfo->xResolution;
	gsize_t yres = minfo->yResolution;
	/* validate position */
	x = MIN(x,xres - 1);
	y = MIN(y,yres - 1);

	if(lastX != x || lastY != y) {
		gsize_t upHeight = MIN(curHeight,yres - lastY);
		/* copy old content back */
		vesa_copyRegion(cursorCopy,video,curWidth,upHeight,0,0,lastX,lastY,curWidth,xres,curHeight);
		/* save content */
		vesa_copyRegion(shmem,cursorCopy,curWidth,curHeight,x,y,0,0,xres,curWidth,yres);
	}

	bmp_draw(cursor[curCursor],x,y,setPixel[minfo->bitsPerPixel / 8]);
	lastX = x;
	lastY = y;
}

static void vesa_clearRegion(gpos_t x,gpos_t y,gsize_t w,gsize_t h) {
	gsize_t xres = minfo->xResolution;
	gsize_t yres = minfo->yResolution;
	gpos_t maxy = MIN(yres,y + h);
	gsize_t pxSize = minfo->bitsPerPixel / 8;
	size_t count = MIN(xres - x,w) * pxSize;
	size_t dstInc = xres * pxSize;
	uint8_t *dst = (uint8_t*)video + (y * xres + x) * pxSize;
	while(y < maxy) {
		memclear(dst,count);
		dst += dstInc;
		y++;
	}
}

static void vesa_copyRegion(uint8_t *src,uint8_t *dst,gsize_t width,gsize_t height,gpos_t x1,gpos_t y1,
		gpos_t x2,gpos_t y2,gsize_t w1,gsize_t w2,gsize_t h1) {
	gpos_t maxy = MIN(h1,y1 + height);
	gsize_t pxSize = minfo->bitsPerPixel / 8;
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
