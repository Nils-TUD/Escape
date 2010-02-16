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
#include <esc/vm86.h>
#include <stdlib.h>
#include <errors.h>
#include <assert.h>

#include <vbe/vbe.h>
#include "font.h"

#define RESOLUTION_X					1024
#define RESOLUTION_Y					768
#define BITS_PER_PIXEL					16

#define CURSOR_LEN						FONT_WIDTH
#define CURSOR_COLOR					0xFFFFFF
#define CURSOR_SIZE						(CURSOR_LEN * 2 + 1)

#define PAD								0
#define PIXEL_SET(c,x,y)				\
	((font8x16)[(c) * FONT_HEIGHT + (y)] & (1 << (FONT_WIDTH - (x) - 1)))

typedef u16 tSize;
typedef u16 tCoord;
typedef u32 tColor;

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
	/* 15 */ LIGHTWHITE,
} eColor;

typedef u8 *(*fSetPixel)(u8 *vidwork,u8 r,u8 g,u8 b);

static s32 vesa_setMode(void);
static s32 vesa_init(void);
static void vesa_drawStr(tCoord col,tCoord row,const char *str,u32 len);
static void vesa_drawChar(tCoord col,tCoord row,u8 c,u8 color);
static void vesa_drawCharLoop16(u8 *vid,u8 c,u8 cf1,u8 cf2,u8 cf3,u8 cb1,u8 cb2,u8 cb3);
static void vesa_drawCharLoop24(u8 *vid,u8 c,u8 cf1,u8 cf2,u8 cf3,u8 cb1,u8 cb2,u8 cb3);
static void vesa_drawCharLoop32(u8 *vid,u8 c,u8 cf1,u8 cf2,u8 cf3,u8 cb1,u8 cb2,u8 cb3);
static u8 *vesa_setPixel16(u8 *vidwork,u8 r,u8 g,u8 b);
static u8 *vesa_setPixel24(u8 *vidwork,u8 r,u8 g,u8 b);
static u8 *vesa_setPixel32(u8 *vidwork,u8 r,u8 g,u8 b);
static void vesa_setCursor(tCoord col,tCoord row);
static void vesa_drawCursor(tCoord col,tCoord row,u8 color);

static u8 colors[][3] = {
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
static u8 *whOnBlCache = NULL;
static u8 *content = NULL;
static u8 *video = NULL;

static u8 cols;
static u8 rows;
static u8 lastCol;
static u8 lastRow;

static sMsg msg;

static fSetPixel setPixel[] = {
	/*  0 bpp */	NULL,
	/*  8 bpp */	NULL,
	/* 16 bpp */	vesa_setPixel16,
	/* 24 bpp */	vesa_setPixel24,
	/* 32 bpp */	vesa_setPixel32
};

int main(void) {
	tServ id,client;
	tMsgId mid;

	/* load available modes etc. */
	vbe_init();

	id = regService("vesatext",SERV_DRIVER);
	if(id < 0)
		error("Unable to register service 'vesatext'");

	/*sVM86Regs regs;
	memset(&regs,0,sizeof(regs));
	regs.ax = 0x0002;
	if(vm86int(0x10,&regs,NULL,0) < 0)
		printe("Switch to text-mode failed");*/

	while(1) {
		tFD fd = getClient(&id,1,&client);
		if(fd < 0)
			wait(EV_CLIENT);
		else {
			while(receive(fd,&mid,&msg,sizeof(msg)) > 0) {
				switch(mid) {
					case MSG_DRV_OPEN:
						msg.args.arg1 = vesa_setMode();
						send(fd,MSG_DRV_OPEN_RESP,&msg,sizeof(msg.args));
						break;

					case MSG_DRV_READ:
						msg.data.arg1 = ERR_UNSUPPORTED_OP;
						msg.data.arg2 = true;
						send(fd,MSG_DRV_READ_RESP,&msg,sizeof(msg.data));
						break;

					case MSG_DRV_WRITE: {
						u32 offset = msg.args.arg1;
						u32 count = msg.args.arg2;
						msg.args.arg1 = 0;
						if(minfo == NULL)
							msg.args.arg1 = ERR_UNSUPPORTED_OP;
						else if(offset + count <= (u32)(rows * cols * 2) && offset + count > offset) {
							char *str = (char*)malloc(count);
							vassert(str,"Unable to alloc mem");
							msg.args.arg1 = 0;
							if(receive(fd,&mid,str,count) >= 0) {
								vesa_drawStr((offset / 2) % cols,(offset / 2) / cols,str,count / 2);
								msg.args.arg1 = count;
							}
							free(str);
						}
						send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
					}
					break;

					case MSG_DRV_IOCTL: {
						if(minfo == NULL)
							msg.data.arg1 = ERR_UNSUPPORTED_OP;
						else {
							switch(msg.data.arg1) {
								case IOCTL_VID_SETCURSOR: {
									sIoCtlPos *pos = (sIoCtlPos*)msg.data.d;
									pos->col = MIN(pos->col,cols);
									pos->row = MIN(pos->row,rows);
									vesa_setCursor(pos->col,pos->row);
									msg.data.arg1 = 0;
								}
								break;

								case IOCTL_VID_GETSIZE: {
									sIoCtlSize *size = (sIoCtlSize*)msg.data.d;
									size->width = cols;
									size->height = rows;
									msg.data.arg1 = sizeof(sIoCtlSize);
								}
								break;

								default:
									msg.data.arg1 = ERR_UNSUPPORTED_OP;
									break;
							}
						}
						send(fd,MSG_DRV_IOCTL_RESP,&msg,sizeof(msg.data));
					}
					break;

					case MSG_DRV_CLOSE:
						/* ignore */
						break;
				}
			}
			close(fd);
		}
	}

	unregService(id);
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
			cols = minfo->xResolution / (FONT_WIDTH + PAD * 2);
			rows = minfo->yResolution / (FONT_HEIGHT + PAD * 2);
			if(vbe_setMode(mode))
				return vesa_init();
			minfo = NULL;
			return ERR_VESA_SETMODE_FAILED;
		}
	}
	return ERR_VESA_MODE_NOT_FOUND;
}

static s32 vesa_init(void) {
	s32 x,y;
	if(content == NULL) {
		content = (u8*)malloc(cols * rows * 2);
		if(content == NULL)
			return ERR_NOT_ENOUGH_MEM;
		for(y = 0; y < rows; y++) {
			for(x = 0; x < cols; x++) {
				content[y * cols * 2 + x * 2] = ' ';
				content[y * cols * 2 + x * 2 + 1] = 0x07;
			}
		}
	}

	/* init cache for white-on-black-chars */
	if(whOnBlCache == NULL) {
		u8 *cc;
		u32 i;
		whOnBlCache = (u8*)malloc((FONT_WIDTH + PAD * 2) * (FONT_HEIGHT + PAD * 2) *
				(minfo->bitsPerPixel / 8) * FONT_COUNT);
		if(whOnBlCache == NULL)
			return ERR_NOT_ENOUGH_MEM;
		cc = whOnBlCache;
		for(i = 0; i < FONT_COUNT; i++) {
			for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
				for(x = 0; x < FONT_WIDTH + PAD * 2; x++) {
					if(y >= PAD && y < FONT_HEIGHT + PAD && x >= PAD && x < FONT_WIDTH + PAD &&
							PIXEL_SET(i,x - PAD,y - PAD)) {
						cc = setPixel[minfo->bitsPerPixel / 8](cc,colors[WHITE][2],colors[WHITE][1],colors[WHITE][0]);
					}
					else {
						cc = setPixel[minfo->bitsPerPixel / 8](cc,colors[BLACK][2],colors[BLACK][1],colors[BLACK][0]);
					}
				}
			}
		}
	}

	lastCol = cols;
	lastRow = rows;
	return 0;
}

static void vesa_drawStr(tCoord col,tCoord row,const char *str,u32 len) {
	while(len-- > 0) {
		char c = *str++;
		char color = *str++;
		vesa_drawChar(col,row,c,color);
		if(col >= cols - 1) {
			row++;
			col = 0;
		}
		else
			col++;
	}
}

static void vesa_drawChar(tCoord col,tCoord row,u8 c,u8 color) {
	u32 y;
	u32 rx = minfo->xResolution;
	u32 pxSize = minfo->bitsPerPixel / 8;
	u8 *vid;
	/* don't print the same again ;) */
	if(content[row * cols * 2 + col * 2] == c && content[row * cols * 2 + col * 2 + 1] == color)
		return;

	content[row * cols * 2 + col * 2] = c;
	content[row * cols * 2 + col * 2 + 1] = color;
	vid = video + row * (FONT_HEIGHT + PAD * 2) * rx * pxSize + col * (FONT_WIDTH + PAD * 2) * pxSize;
	if(color == ((BLACK << 4) | WHITE)) {
		const u8 *cache = whOnBlCache + c * (FONT_HEIGHT + PAD * 2) * (FONT_WIDTH + PAD * 2) * pxSize;
		for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
			memcpy(vid,cache,(FONT_WIDTH + PAD * 2) * pxSize);
			cache += (FONT_WIDTH + PAD * 2) * pxSize;
			vid += rx * pxSize;
		}
	}
	else {
		u8 colFront1 = colors[color & 0xf][2];
		u8 colFront2 = colors[color & 0xf][1];
		u8 colFront3 = colors[color & 0xf][0];
		u8 colBack1 = colors[color >> 4][2];
		u8 colBack2 = colors[color >> 4][1];
		u8 colBack3 = colors[color >> 4][0];
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

static void vesa_drawCharLoop16(u8 *vid,u8 c,u8 cf1,u8 cf2,u8 cf3,u8 cb1,u8 cb2,u8 cb3) {
	s32 x,y;
	u8 *vidwork;
	u32 rx = minfo->xResolution;
	u8 rms = minfo->redMaskSize;
	u8 gms = minfo->greenMaskSize;
	u8 bms = minfo->blueMaskSize;
	u8 rfp = minfo->redFieldPosition;
	u8 gfp = minfo->greenFieldPosition;
	u8 bfp = minfo->blueFieldPosition;

#define DRAWPIXEL16(r,g,b) \
		u8 ___red = (r) >> (8 - rms); \
		u8 ___green = (g) >> (8 - gms); \
		u8 ___blue = (b) >> (8 - bms); \
		*((u16*)(vidwork)) = (___red << rfp) | \
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

static void vesa_drawCharLoop24(u8 *vid,u8 c,u8 cf1,u8 cf2,u8 cf3,u8 cb1,u8 cb2,u8 cb3) {
	s32 x,y;
	u8 *vidwork;
	u32 rx = minfo->xResolution;
	u8 rms = minfo->redMaskSize;
	u8 gms = minfo->greenMaskSize;
	u8 bms = minfo->blueMaskSize;
	u8 rfp = minfo->redFieldPosition;
	u8 gfp = minfo->greenFieldPosition;
	u8 bfp = minfo->blueFieldPosition;

#define DRAWPIXEL24(r,g,b) \
		u8 ___red = (r) >> (8 - rms); \
		u8 ___green = (g) >> (8 - gms); \
		u8 ___blue = (b) >> (8 - bms); \
		u32 ___val = (___red << rfp) | \
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

static void vesa_drawCharLoop32(u8 *vid,u8 c,u8 cf1,u8 cf2,u8 cf3,u8 cb1,u8 cb2,u8 cb3) {
	s32 x,y;
	u8 *vidwork;
	u32 rx = minfo->xResolution;
	u8 rms = minfo->redMaskSize;
	u8 gms = minfo->greenMaskSize;
	u8 bms = minfo->blueMaskSize;
	u8 rfp = minfo->redFieldPosition;
	u8 gfp = minfo->greenFieldPosition;
	u8 bfp = minfo->blueFieldPosition;

#define DRAWPIXEL32(r,g,b) \
		u8 ___red = (r) >> (8 - rms); \
		u8 ___green = (g) >> (8 - gms); \
		u8 ___blue = (b) >> (8 - bms); \
		*((u32*)vidwork) = (___red << rfp) | \
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

static u8 *vesa_setPixel16(u8 *vidwork,u8 r,u8 g,u8 b) {
	u8 red = r >> (8 - minfo->redMaskSize);
	u8 green = g >> (8 - minfo->greenMaskSize);
	u8 blue = b >> (8 - minfo->blueMaskSize);
	u16 val = (red << minfo->redFieldPosition) |
			(green << minfo->greenFieldPosition) |
			(blue << minfo->blueFieldPosition);
	*((u16*)vidwork) = val;
	return vidwork + 2;
}

static u8 *vesa_setPixel24(u8 *vidwork,u8 r,u8 g,u8 b) {
	u8 red = r >> (8 - minfo->redMaskSize);
	u8 green = g >> (8 - minfo->greenMaskSize);
	u8 blue = b >> (8 - minfo->blueMaskSize);
	u32 val = (red << minfo->redFieldPosition) |
			(green << minfo->greenFieldPosition) |
			(blue << minfo->blueFieldPosition);
	vidwork[2] = val >> 16;
	vidwork[1] = val >> 8;
	vidwork[0] = val & 0xFF;
	return vidwork + 3;
}

static u8 *vesa_setPixel32(u8 *vidwork,u8 r,u8 g,u8 b) {
	u8 red = r >> (8 - minfo->redMaskSize);
	u8 green = g >> (8 - minfo->greenMaskSize);
	u8 blue = b >> (8 - minfo->blueMaskSize);
	u32 val = (red << minfo->redFieldPosition) |
			(green << minfo->greenFieldPosition) |
			(blue << minfo->blueFieldPosition);
	*((u32*)vidwork) = val;
	return vidwork + 4;
}

static void vesa_setCursor(tCoord col,tCoord row) {
	if(lastCol < cols && lastRow < rows)
		vesa_drawCursor(lastCol,lastRow,BLACK);
	vesa_drawCursor(col,row,WHITE);
	lastCol = col;
	lastRow = row;
}

static void vesa_drawCursor(tCoord col,tCoord row,u8 color) {
	u32 xres = minfo->xResolution;
	u32 pxSize = minfo->bitsPerPixel / 8;
	fSetPixel pxFunc = setPixel[pxSize];
	u32 x,y = FONT_HEIGHT + PAD - 1;
	u8 *vid = video +
			row * (FONT_HEIGHT + PAD * 2) * xres * pxSize +
			col * (FONT_WIDTH + PAD * 2) * pxSize;
	u8 *vidwork = vid + y * xres * pxSize;
	for(x = 0; x < CURSOR_LEN; x++) {
		u8 *colPtr = colors[color];
		vidwork = pxFunc(vidwork,colPtr[2],colPtr[1],colPtr[0]);
	}
}
