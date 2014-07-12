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
#include <sys/rect.h>
#include <string.h>
#include <stdlib.h>

#include "window.h"
#include "preview.h"

static void preview_handleIntersec(char *shmem,sRectangle *curRec,sRectangle *intersec,size_t i,
		gsize_t xres,gsize_t yres);
static void preview_clearRegion(char *shmem,gpos_t x,gpos_t y,gsize_t w,gsize_t h);
static void preview_copyRegion(char *src,char *dst,gsize_t width,gsize_t height,gpos_t x1,gpos_t y1,
		gpos_t x2,gpos_t y2,gsize_t w1,gsize_t w2,gsize_t h1);

static sRectangle previewRect;
static gsize_t previewThickness;
static char *previewRectCopies[4];

void preview_updateRect(char *shmem,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
	gsize_t xres = win_getMode()->width;
	gsize_t yres = win_getMode()->height;
	sRectangle upRec,curRec,intersec;
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
			preview_handleIntersec(shmem,&curRec,&intersec,0,xres,yres);
		/* bottom */
		curRec.y = previewRect.y + previewRect.height - previewThickness;
		if(rectIntersect(&curRec,&upRec,&intersec))
			preview_handleIntersec(shmem,&curRec,&intersec,2,xres,yres);
		/* left */
		curRec.y = previewRect.y;
		curRec.width = previewThickness;
		curRec.height = previewRect.height;
		if(rectIntersect(&curRec,&upRec,&intersec))
			preview_handleIntersec(shmem,&curRec,&intersec,3,xres,yres);
		/* right */
		curRec.x = previewRect.x + previewRect.width - previewThickness;
		if(rectIntersect(&curRec,&upRec,&intersec))
			preview_handleIntersec(shmem,&curRec,&intersec,1,xres,yres);
	}
}

void preview_set(char *shmem,gpos_t x,gpos_t y,gsize_t width,gsize_t height,gsize_t thickness) {
	gsize_t xres = win_getMode()->width;
	gsize_t yres = win_getMode()->height;
	gsize_t pxSize = win_getMode()->bitsPerPixel / 8;

	if(previewRectCopies[0]) {
		/* copy old content back */
		/* top */
		preview_copyRegion(previewRectCopies[0],shmem,
				previewRect.width,previewThickness,0,0,previewRect.x,previewRect.y,
				previewRect.width,xres,previewThickness);
		win_notifyUimng(previewRect.x,previewRect.y,previewRect.width,previewThickness);
		/* right */
		preview_copyRegion(previewRectCopies[1],shmem,
				previewThickness,previewRect.height,0,0,
				previewRect.x + previewRect.width - previewThickness,previewRect.y,
				previewThickness,xres,previewRect.height);
		win_notifyUimng(previewRect.x + previewRect.width - previewThickness,previewRect.y,
			previewThickness,previewRect.height);
		/* bottom */
		preview_copyRegion(previewRectCopies[2],shmem,
				previewRect.width,previewThickness,0,0,previewRect.x,
				previewRect.y + previewRect.height - previewThickness,
				previewRect.width,xres,previewThickness);
		win_notifyUimng(previewRect.x,previewRect.y + previewRect.height - previewThickness,
			previewRect.width,previewThickness);
		/* left */
		preview_copyRegion(previewRectCopies[3],shmem,
				previewThickness,previewRect.height,0,0,previewRect.x,previewRect.y,
				previewThickness,xres,previewRect.height);
		win_notifyUimng(previewRect.x,previewRect.y,previewThickness,previewRect.height);

		if(thickness != previewThickness || width != previewRect.width ||
				height != previewRect.height) {
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
		if(thickness != previewThickness || width != previewRect.width ||
				height != previewRect.height) {
			previewRectCopies[0] = (char*)malloc(width * thickness * pxSize);
			previewRectCopies[1] = (char*)malloc(height * thickness * pxSize);
			previewRectCopies[2] = (char*)malloc(width * thickness * pxSize);
			previewRectCopies[3] = (char*)malloc(height * thickness * pxSize);
		}

		/* store preview rect */
		if(x < 0) {
			width = MAX(0,(int)(width + x));
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
		previewRect.x = x;
		previewRect.y = y;
		previewRect.width = width;
		previewRect.height = height;

		/* save content */
		/* top */
		preview_copyRegion(shmem,previewRectCopies[0],
				width,thickness,x,y,0,0,
				xres,width,yres);
		/* right */
		preview_copyRegion(shmem,previewRectCopies[1],
				thickness,height,x + width - thickness,y,0,0,
				xres,thickness,yres);
		/* bottom */
		preview_copyRegion(shmem,previewRectCopies[2],
				width,thickness,x,y + height - thickness,0,0,
				xres,width,yres);
		/* left */
		preview_copyRegion(shmem,previewRectCopies[3],
				thickness,height,x,y,0,0,
				xres,thickness,yres);

		/* draw rect */
		/* top */
		preview_clearRegion(shmem,x,y,width,thickness);
		win_notifyUimng(x,y,width,thickness);
		/* right */
		preview_clearRegion(shmem,x + width - thickness,y,thickness,height);
		win_notifyUimng(x + width - thickness,y,thickness,height);
		/* bottom */
		preview_clearRegion(shmem,x,y + height - thickness,width,thickness);
		win_notifyUimng(x,y + height - thickness,width,thickness);
		/* left */
		preview_clearRegion(shmem,x,y,thickness,height);
		win_notifyUimng(x,y,thickness,height);
	}

	previewThickness = thickness;
}

static void preview_handleIntersec(char *shmem,sRectangle *curRec,sRectangle *intersec,size_t i,
		gsize_t xres,gsize_t yres) {
	preview_copyRegion(shmem,previewRectCopies[i],intersec->width,intersec->height,
		intersec->x,intersec->y,intersec->x - curRec->x,intersec->y - curRec->y,
		xres,curRec->width,yres);
	preview_clearRegion(shmem,curRec->x,curRec->y,curRec->width,curRec->height);
}

static void preview_clearRegion(char *shmem,gpos_t x,gpos_t y,gsize_t w,gsize_t h) {
	gsize_t xres = win_getMode()->width;
	gsize_t yres = win_getMode()->height;
	gpos_t maxy = MIN(yres,y + h);
	gsize_t pxSize = win_getMode()->bitsPerPixel / 8;
	size_t count = MIN(xres - x,w) * pxSize;
	size_t dstInc = xres * pxSize;
	uint8_t *dst = (uint8_t*)shmem + (y * xres + x) * pxSize;
	while(y < maxy) {
		memclear(dst,count);
		dst += dstInc;
		y++;
 	}
}

static void preview_copyRegion(char *src,char *dst,gsize_t width,gsize_t height,gpos_t x1,gpos_t y1,
		gpos_t x2,gpos_t y2,gsize_t w1,gsize_t w2,gsize_t h1) {
	gpos_t maxy = MIN(h1,y1 + height);
	gsize_t pxSize = win_getMode()->bitsPerPixel / 8;
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
