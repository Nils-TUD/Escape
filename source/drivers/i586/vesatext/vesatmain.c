/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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
#include <esc/messages.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <vbe/vbe.h>
#include "font.h"

#define CURSOR_LEN						FONT_WIDTH
#define CURSOR_COLOR					0xFFFFFF
#define CURSOR_SIZE						(CURSOR_LEN * 2 + 1)

#define PAD								0
#define PIXEL_SET(c,x,y)				\
	((font8x16)[(c) * FONT_HEIGHT + (y)] & (1 << (FONT_WIDTH - (x) - 1)))

typedef uint32_t tColor;

/* the colors */
typedef enum {
	/* 0 */ BLACK,
	/* 1 */ BLUE,
	/* 2 */ GREEN,
	/* 3 */ CYAN,
	/* 4 */ RED,
	/* 5 */ MARGENTA,
	/* 6 */ ORANGE,
	/* 7 */ WHITE,
	/* 8 */ GRAY,
	/* 9 */ LIGHTBLUE,
	/* 10 */ LIGHTGREEN,
	/* 11 */ LIGHTCYAN,
	/* 12 */ LIGHTRED,
	/* 13 */ LIGHTMARGENTA,
	/* 14 */ LIGHTORANGE,
	/* 15 */ LIGHTWHITE
} eColor;

typedef uint8_t *(*fSetPixel)(uint8_t *vidwork,uint8_t r,uint8_t g,uint8_t b);

static int vesa_setMode(int mode);
static int vesa_init(void);
static void vesa_drawChars(gpos_t col,gpos_t row,const uint8_t *str,size_t len);
static void vesa_drawChar(gpos_t col,gpos_t row,uint8_t c,uint8_t color);
static void vesa_drawCharLoop16(uint8_t *vid,uint8_t c,uint8_t cf1,uint8_t cf2,
		uint8_t cf3,uint8_t cb1,uint8_t cb2,uint8_t cb3);
static void vesa_drawCharLoop24(uint8_t *vid,uint8_t c,uint8_t cf1,uint8_t cf2,
		uint8_t cf3,uint8_t cb1,uint8_t cb2,uint8_t cb3);
static void vesa_drawCharLoop32(uint8_t *vid,uint8_t c,uint8_t cf1,uint8_t cf2,
		uint8_t cf3,uint8_t cb1,uint8_t cb2,uint8_t cb3);
static uint8_t *vesa_setPixel16(uint8_t *vidwork,uint8_t r,uint8_t g,uint8_t b);
static uint8_t *vesa_setPixel24(uint8_t *vidwork,uint8_t r,uint8_t g,uint8_t b);
static uint8_t *vesa_setPixel32(uint8_t *vidwork,uint8_t r,uint8_t g,uint8_t b);
static void vesa_setCursor(gpos_t col,gpos_t row);
static void vesa_drawCursor(gpos_t col,gpos_t row,uint8_t color);

static uint8_t colors[][3] = {
	/* BLACK   */ {0x00,0x00,0x00},
	/* BLUE    */ {0x00,0x00,0xA8},
	/* GREEN   */ {0x00,0xA8,0x00},
	/* CYAN    */ {0x00,0xA8,0xA8},
	/* RED     */ {0xA8,0x00,0x00},
	/* MARGENT */ {0xA8,0x00,0xA8},
	/* ORANGE  */ {0xA8,0x57,0x00},
	/* WHITE   */ {0xA8,0xA8,0xA8},
	/* GRAY    */ {0x57,0x57,0x57},
	/* LIBLUE  */ {0x57,0x57,0xFF},
	/* LIGREEN */ {0x57,0xFF,0x57},
	/* LICYAN  */ {0x57,0xFF,0xFF},
	/* LIRED   */ {0xFF,0x57,0x57},
	/* LIMARGE */ {0xFF,0x57,0xFF},
	/* LIORANG */ {0xFF,0xFF,0x57},
	/* LIWHITE */ {0xFF,0xFF,0xFF},
};

static sVbeModeInfo *minfo = NULL;
static uint8_t *whOnBlCache = NULL;
static uint8_t *content = NULL;
static uint8_t *video = NULL;

static uint8_t cols;
static uint8_t rows;
static uint8_t lastCol;
static uint8_t lastRow;

static sMsg msg;

static fSetPixel setPixel[] = {
	/*  0 bpp */	NULL,
	/*  8 bpp */	NULL,
	/* 16 bpp */	vesa_setPixel16,
	/* 24 bpp */	vesa_setPixel24,
	/* 32 bpp */	vesa_setPixel32
};

int main(void) {
	int id;
	msgid_t mid;

	/* load available modes etc. */
	vbe_init();

	id = createdev("/dev/vesatext",DEV_TYPE_BLOCK,DEV_WRITE);
	if(id < 0)
		error("Unable to register device 'vesatext'");

	while(1) {
		int fd = getwork(id,NULL,&mid,&msg,sizeof(msg),0);
		if(fd < 0)
			printe("[VESATEXT] Unable to get work");
		else {
			switch(mid) {
				case MSG_DEV_WRITE: {
					uint offset = msg.args.arg1;
					size_t count = msg.args.arg2;
					msg.args.arg1 = -EINVAL;
					if(video == NULL || minfo == NULL)
						msg.args.arg1 = -ENOTSUP;
					else if(offset + count <= (size_t)(rows * cols * 2) && offset + count > offset) {
						uint8_t *str = (uint8_t*)malloc(count);
						vassert(str,"Unable to alloc mem");
						msg.args.arg1 = 0;
						if(IGNSIGS(receive(fd,&mid,str,count)) >= 0) {
							vesa_drawChars((offset / 2) % cols,(offset / 2) / cols,str,count / 2);
							msg.args.arg1 = count;
						}
						free(str);
					}
					/* skip the data if there was an error */
					if((int)msg.args.arg1 < 0)
						IGNSIGS(receive(fd,NULL,NULL,0));
					send(fd,MSG_DEV_WRITE_RESP,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_GETMODE: {
					msg.args.arg1 = minfo ? minfo->modeNo : -EINVAL;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_SETMODE: {
					sVbeModeInfo *modeinfo = vbe_getModeInfo(msg.args.arg1);
					if(modeinfo == NULL)
						msg.args.arg1 = -EINVAL;
					else {
						msg.args.arg1 = vesa_setMode(modeinfo->modeNo);
						if((int)msg.args.arg1 == 0)
							msg.args.arg1 = vbe_setMode(modeinfo->modeNo);
						if((int)msg.args.arg1 == 0)
							minfo = modeinfo;
					}
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
				}
				break;

				case MSG_VID_GETMODES: {
					size_t i, count = 0;
					sVTMode *modes = vbe_collectModes(msg.args.arg1,&count);
					if(modes) {
						for(i = 0; i < count; i++) {
							modes[i].width /= FONT_WIDTH + PAD * 2;
							modes[i].height = (modes[i].height - 1) / (FONT_HEIGHT + PAD * 2);
						}
						send(fd,MSG_DEF_RESPONSE,modes,sizeof(sVTMode) * count);
						vbe_freeModes(modes);
					}
					else {
						msg.args.arg1 = count;
						send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					}
				}
				break;

				case MSG_VID_SETCURSOR: {
					sVTPos *pos = (sVTPos*)msg.data.d;
					pos->col = MIN(pos->col,(uint)(cols - 1));
					pos->row = MIN(pos->row,(uint)(rows - 1));
					if(minfo)
						vesa_setCursor(pos->col,pos->row);
				}
				break;

				case MSG_VID_GETSIZE: {
					sVTSize *size = (sVTSize*)msg.data.d;
					size->width = cols;
					size->height = rows;
					msg.data.arg1 = sizeof(sVTSize);
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.data));
				}
				break;

				default:
					msg.args.arg1 = -ENOTSUP;
					send(fd,MSG_DEF_RESPONSE,&msg,sizeof(msg.args));
					break;
			}
			close(fd);
		}
	}

	munmap(video);
	close(id);
	return EXIT_SUCCESS;
}

static int vesa_setMode(int mode) {
	minfo = vbe_getModeInfo(mode);
	if(minfo) {
		uintptr_t phys;
		if(video)
			munmap(video);
		phys = minfo->physBasePtr;
		video = regaddphys(&phys,minfo->xResolution * minfo->yResolution * (minfo->bitsPerPixel / 8),0);
		if(video == NULL) {
			minfo = NULL;
			return -errno;
		}
		cols = minfo->xResolution / (FONT_WIDTH + PAD * 2);
		/* leave at least one pixel free for the cursor */
		rows = (minfo->yResolution - 1) / (FONT_HEIGHT + PAD * 2);
		return vesa_init();
	}
	return -ENOENT;
}

static int vesa_init(void) {
	int x,y;
	if(content)
		free(content);

	content = (uint8_t*)malloc(cols * rows * 2);
	if(content == NULL)
		return -ENOMEM;
	for(y = 0; y < rows; y++) {
		for(x = 0; x < cols; x++) {
			content[y * cols * 2 + x * 2] = ' ';
			content[y * cols * 2 + x * 2 + 1] = 0x07;
		}
	}

	/* init cache for white-on-black-chars */
	if(whOnBlCache)
		free(whOnBlCache);

	{
		uint8_t *cc;
		size_t i;
		whOnBlCache = (uint8_t*)malloc((FONT_WIDTH + PAD * 2) * (FONT_HEIGHT + PAD * 2) *
				(minfo->bitsPerPixel / 8) * FONT_COUNT);
		if(whOnBlCache == NULL)
			return -ENOMEM;
		cc = whOnBlCache;
		for(i = 0; i < FONT_COUNT; i++) {
			for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
				for(x = 0; x < FONT_WIDTH + PAD * 2; x++) {
					if(y >= PAD && y < FONT_HEIGHT + PAD && x >= PAD && x < FONT_WIDTH + PAD &&
							PIXEL_SET(i,x - PAD,y - PAD)) {
						cc = setPixel[minfo->bitsPerPixel / 8](cc,colors[WHITE][2],
								colors[WHITE][1],colors[WHITE][0]);
					}
					else {
						cc = setPixel[minfo->bitsPerPixel / 8](cc,colors[BLACK][2],
								colors[BLACK][1],colors[BLACK][0]);
					}
				}
			}
		}
	}

	lastCol = cols;
	lastRow = rows;
	return 0;
}

static void vesa_drawChars(gpos_t col,gpos_t row,const uint8_t *str,size_t len) {
	while(len-- > 0) {
		uint8_t c = *str++;
		uint8_t color = *str++;
		vesa_drawChar(col,row,c,color);
		if(col >= cols - 1) {
			row++;
			col = 0;
		}
		else
			col++;
	}
}

static void vesa_drawChar(gpos_t col,gpos_t row,uint8_t c,uint8_t color) {
	gpos_t y;
	gsize_t rx = minfo->xResolution;
	size_t pxSize = minfo->bitsPerPixel / 8;
	uint8_t *vid;
	/* don't print the same again ;) */
	if(content[row * cols * 2 + col * 2] == c && content[row * cols * 2 + col * 2 + 1] == color)
		return;

	content[row * cols * 2 + col * 2] = c;
	content[row * cols * 2 + col * 2 + 1] = color;
	vid = video + row * (FONT_HEIGHT + PAD * 2) * rx * pxSize + col *
			(FONT_WIDTH + PAD * 2) * pxSize;
	if(color == ((BLACK << 4) | WHITE)) {
		const uint8_t *cache = whOnBlCache + c * (FONT_HEIGHT + PAD * 2) *
				(FONT_WIDTH + PAD * 2) * pxSize;
		for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
			memcpy(vid,cache,(FONT_WIDTH + PAD * 2) * pxSize);
			cache += (FONT_WIDTH + PAD * 2) * pxSize;
			vid += rx * pxSize;
		}
	}
	else {
		uint8_t colFront1 = colors[color & 0xf][2];
		uint8_t colFront2 = colors[color & 0xf][1];
		uint8_t colFront3 = colors[color & 0xf][0];
		uint8_t colBack1 = colors[color >> 4][2];
		uint8_t colBack2 = colors[color >> 4][1];
		uint8_t colBack3 = colors[color >> 4][0];
		switch(minfo->bitsPerPixel) {
			case 16:
				vesa_drawCharLoop16(vid,c,colFront1,colFront2,colFront3,colBack1,colBack2,colBack3);
				break;
			case 24:
				vesa_drawCharLoop24(vid,c,colFront1,colFront2,colFront3,colBack1,colBack2,colBack3);
				break;
			case 32:
				vesa_drawCharLoop32(vid,c,colFront1,colFront2,colFront3,colBack1,colBack2,colBack3);
				break;
		}
	}
}

static void vesa_drawCharLoop16(uint8_t *vid,uint8_t c,uint8_t cf1,uint8_t cf2,uint8_t cf3,
		uint8_t cb1,uint8_t cb2,uint8_t cb3) {
	int x,y;
	uint8_t *vidwork;
	gsize_t rx = minfo->xResolution;
	uint8_t rms = minfo->redMaskSize;
	uint8_t gms = minfo->greenMaskSize;
	uint8_t bms = minfo->blueMaskSize;
	uint8_t rfp = minfo->redFieldPosition;
	uint8_t gfp = minfo->greenFieldPosition;
	uint8_t bfp = minfo->blueFieldPosition;

#define DRAWPIXEL16(r,g,b) \
		uint8_t ___red = (r) >> (8 - rms); \
		uint8_t ___green = (g) >> (8 - gms); \
		uint8_t ___blue = (b) >> (8 - bms); \
		*((uint16_t*)(vidwork)) = (___red << rfp) | \
				(___green << gfp) | \
				(___blue << bfp); \
		(vidwork) += 2;

	for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
		vidwork = vid + y * rx * 2;
		for(x = 0; x < FONT_WIDTH + PAD * 2; x++) {
			if(y >= PAD && x < FONT_WIDTH + PAD && x >= PAD) {
				if(y < FONT_HEIGHT + PAD && PIXEL_SET(c,x - PAD,y - PAD)) {
					DRAWPIXEL16(cf3,cf2,cf1);
				}
				else {
					DRAWPIXEL16(cb3,cb2,cb1);
				}
			}
			else {
				DRAWPIXEL16(cb3,cb2,cb1);
			}
		}
	}
}

static void vesa_drawCharLoop24(uint8_t *vid,uint8_t c,uint8_t cf1,uint8_t cf2,uint8_t cf3,
		uint8_t cb1,uint8_t cb2,uint8_t cb3) {
	int x,y;
	uint8_t *vidwork;
	gsize_t rx = minfo->xResolution;
	uint8_t rms = minfo->redMaskSize;
	uint8_t gms = minfo->greenMaskSize;
	uint8_t bms = minfo->blueMaskSize;
	uint8_t rfp = minfo->redFieldPosition;
	uint8_t gfp = minfo->greenFieldPosition;
	uint8_t bfp = minfo->blueFieldPosition;

#define DRAWPIXEL24(r,g,b) \
		uint8_t ___red = (r) >> (8 - rms); \
		uint8_t ___green = (g) >> (8 - gms); \
		uint8_t ___blue = (b) >> (8 - bms); \
		uint32_t ___val = (___red << rfp) | \
			(___green << gfp) | \
			(___blue << bfp); \
		*vidwork++ = ___val & 0xFF; \
		*vidwork++ = ___val >> 8; \
		*vidwork++ = ___val >> 16;

	for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
		vidwork = vid + y * rx * 3;
		for(x = 0; x < FONT_WIDTH + PAD * 2; x++) {
			if(y >= PAD && x < FONT_WIDTH + PAD && x >= PAD) {
				if(y < FONT_HEIGHT + PAD && PIXEL_SET(c,x - PAD,y - PAD)) {
					DRAWPIXEL24(cf3,cf2,cf1)
				}
				else {
					DRAWPIXEL24(cb3,cb2,cb1);
				}
			}
			else {
				DRAWPIXEL24(cb3,cb2,cb1);
			}
		}
	}
}

static void vesa_drawCharLoop32(uint8_t *vid,uint8_t c,uint8_t cf1,uint8_t cf2,uint8_t cf3,
		uint8_t cb1,uint8_t cb2,uint8_t cb3) {
	int x,y;
	uint8_t *vidwork;
	gsize_t rx = minfo->xResolution;
	uint8_t rms = minfo->redMaskSize;
	uint8_t gms = minfo->greenMaskSize;
	uint8_t bms = minfo->blueMaskSize;
	uint8_t rfp = minfo->redFieldPosition;
	uint8_t gfp = minfo->greenFieldPosition;
	uint8_t bfp = minfo->blueFieldPosition;

#define DRAWPIXEL32(r,g,b) \
		uint8_t ___red = (r) >> (8 - rms); \
		uint8_t ___green = (g) >> (8 - gms); \
		uint8_t ___blue = (b) >> (8 - bms); \
		*((uint32_t*)vidwork) = (___red << rfp) | \
			(___green << gfp) | \
			(___blue << bfp); \
		vidwork += 4;

	for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
		vidwork = vid + y * rx * 4;
		for(x = 0; x < FONT_WIDTH + PAD * 2; x++) {
			if(y >= PAD && x < FONT_WIDTH + PAD && x >= PAD) {
				if(y < FONT_HEIGHT + PAD && PIXEL_SET(c,x - PAD,y - PAD)) {
					DRAWPIXEL32(cf3,cf2,cf1);
				}
				else {
					DRAWPIXEL32(cb3,cb2,cb1);
				}
			}
			else {
				DRAWPIXEL32(cb3,cb2,cb1);
			}
		}
	}
}

static uint8_t *vesa_setPixel16(uint8_t *vidwork,uint8_t r,uint8_t g,uint8_t b) {
	uint8_t red = r >> (8 - minfo->redMaskSize);
	uint8_t green = g >> (8 - minfo->greenMaskSize);
	uint8_t blue = b >> (8 - minfo->blueMaskSize);
	uint16_t val = (red << minfo->redFieldPosition) |
			(green << minfo->greenFieldPosition) |
			(blue << minfo->blueFieldPosition);
	*((uint16_t*)vidwork) = val;
	return vidwork + 2;
}

static uint8_t *vesa_setPixel24(uint8_t *vidwork,uint8_t r,uint8_t g,uint8_t b) {
	uint8_t red = r >> (8 - minfo->redMaskSize);
	uint8_t green = g >> (8 - minfo->greenMaskSize);
	uint8_t blue = b >> (8 - minfo->blueMaskSize);
	uint32_t val = (red << minfo->redFieldPosition) |
			(green << minfo->greenFieldPosition) |
			(blue << minfo->blueFieldPosition);
	vidwork[2] = val >> 16;
	vidwork[1] = val >> 8;
	vidwork[0] = val & 0xFF;
	return vidwork + 3;
}

static uint8_t *vesa_setPixel32(uint8_t *vidwork,uint8_t r,uint8_t g,uint8_t b) {
	uint8_t red = r >> (8 - minfo->redMaskSize);
	uint8_t green = g >> (8 - minfo->greenMaskSize);
	uint8_t blue = b >> (8 - minfo->blueMaskSize);
	uint32_t val = (red << minfo->redFieldPosition) |
			(green << minfo->greenFieldPosition) |
			(blue << minfo->blueFieldPosition);
	*((uint32_t*)vidwork) = val;
	return vidwork + 4;
}

static void vesa_setCursor(gpos_t col,gpos_t row) {
	if(lastCol < cols && lastRow < rows)
		vesa_drawCursor(lastCol,lastRow,BLACK);
	vesa_drawCursor(col,row,WHITE);
	lastCol = col;
	lastRow = row;
}

static void vesa_drawCursor(gpos_t col,gpos_t row,uint8_t color) {
	gsize_t xres = minfo->xResolution;
	gsize_t pxSize = minfo->bitsPerPixel / 8;
	fSetPixel pxFunc = setPixel[pxSize];
	gpos_t x,y = FONT_HEIGHT + PAD;
	uint8_t *vid = video +
			row * (FONT_HEIGHT + PAD * 2) * xres * pxSize +
			col * (FONT_WIDTH + PAD * 2) * pxSize;
	uint8_t *vidwork = vid + y * xres * pxSize;
	for(x = 0; x < CURSOR_LEN; x++) {
		uint8_t *colPtr = colors[color];
		vidwork = pxFunc(vidwork,colPtr[2],colPtr[1],colPtr[0]);
	}
}
