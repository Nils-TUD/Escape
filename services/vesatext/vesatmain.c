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

#include "font.h"
#include "vbe.h"

#define VBE_DISPI_IOPORT_INDEX          0x01CE
#define VBE_DISPI_IOPORT_DATA           0x01CF
#define VBE_DISPI_INDEX_ID              0x0
#define VBE_DISPI_INDEX_XRES            0x1
#define VBE_DISPI_INDEX_YRES            0x2
#define VBE_DISPI_INDEX_BPP             0x3
#define VBE_DISPI_INDEX_ENABLE          0x4
#define VBE_DISPI_INDEX_BANK            0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH      0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT     0x7
#define VBE_DISPI_INDEX_X_OFFSET        0x8
#define VBE_DISPI_INDEX_Y_OFFSET        0x9

#define VBE_DISPI_DISABLED              0x00
#define VBE_DISPI_ENABLED               0x01
#define VBE_DISPI_GETCAPS               0x02
#define VBE_DISPI_8BIT_DAC              0x20
#define VBE_DISPI_LFB_ENABLED           0x40
#define VBE_DISPI_NOCLEARMEM            0x80

#define RESOLUTION_X					1024
#define RESOLUTION_Y					768
#define BITS_PER_PIXEL					16
#define PIXEL_SIZE						(BITS_PER_PIXEL / 8)

#define CURSOR_LEN						FONT_WIDTH
#define CURSOR_COLOR					0xFFFFFF
#define CURSOR_SIZE						(CURSOR_LEN * 2 + 1)

#define PAD								0
#define COLS							(minfo->xResolution / (FONT_WIDTH + PAD * 2))
#define ROWS							(minfo->yResolution / (FONT_HEIGHT + PAD * 2))

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

static void vesa_write(u16 index,u16 value);
static void vesa_setMode(tSize xres,tSize yres,u16 bpp);
static void vesa_drawStr(tCoord col,tCoord row,const char *str,u32 len);
static void vesa_update(tCoord startCol,tCoord col,tCoord row);
static void vesa_drawChar(tCoord col,tCoord row,u8 c,u8 color);

static void vesa_drawCharLoop16(u8 *vid,u8 c,u8 cf1,u8 cf2,u8 cf3,u8 cb1,u8 cb2,u8 cb3);
static void vesa_drawCharLoop24(u8 *vid,u8 c,u8 cf1,u8 cf2,u8 cf3,u8 cb1,u8 cb2,u8 cb3);
static void vesa_drawCharLoop32(u8 *vid,u8 c,u8 cf1,u8 cf2,u8 cf3,u8 cb1,u8 cb2,u8 cb3);
static u8 *vesa_setPixel16(u8 *vidwork,u8 r,u8 g,u8 b);
static u8 *vesa_setPixel24(u8 *vidwork,u8 r,u8 g,u8 b);
static u8 *vesa_setPixel32(u8 *vidwork,u8 r,u8 g,u8 b);

static u8 *vesa_setPixel(u8 *vidwork,u8 r,u8 g,u8 b);
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

static u8 *whOnBlCache;

static u8 lastCol;
static u8 lastRow;
static u8 *video;
static sMsg msg;
static sVbeModeInfo *minfo;
static u8 *content;
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
	u32 i,x,y;
	u8 *cc;

	/* request ports; note that we read/write words to them, so we have to request 3 ports */
	if(requestIOPorts(VBE_DISPI_IOPORT_INDEX,3) < 0) {
		error("Unable to request io-ports %d..%d",VBE_DISPI_IOPORT_INDEX,
				VBE_DISPI_IOPORT_INDEX + 2);
	}

	id = regService("vesatext",SERV_DRIVER);
	if(id < 0)
		error("Unable to register service 'vesatext'");

	vbe_init();
	u16 mode = vbe_findMode(RESOLUTION_X,RESOLUTION_Y,BITS_PER_PIXEL);
	if(mode != 0) {
		minfo = vbe_getModeInfo(mode);
		if(minfo) {
			debugf("	%4d x %4d %d bpp, %d planes, %s, %s, %s (@%x), %d banks\n",minfo->xResolution,minfo->yResolution,
				minfo->bitsPerPixel,minfo->numberOfPlanes,
				(minfo->modeAttributes & MODE_GRAPHICS_MODE) ? "graphics" : "text",
				(minfo->modeAttributes & MODE_COLOR_MODE) ? "color" : "monochrome",
				minfo->memoryModel == memPL ? "plane" :
				minfo->memoryModel == memPK ? "packed" :
				minfo->memoryModel == memRGB ? "direct RGB" :
				minfo->memoryModel == memYUV ? "direct YUV" : "unknown",
				minfo->physBasePtr,minfo->numberOfBanks);
			debugf("	rsize=%d, rlsb=%d, gsize=%d, glsb=%d, bsize=%d, blsb=%d\n",
					minfo->redMaskSize,minfo->redFieldPosition,minfo->greenMaskSize,
					minfo->greenFieldPosition,minfo->blueMaskSize,minfo->blueFieldPosition);
			video = mapPhysical(minfo->physBasePtr,minfo->xResolution *
					minfo->yResolution * (minfo->bitsPerPixel / 8));
			if(video == NULL)
				error("Unable to request physical memory at 0x%x",minfo->physBasePtr);
			assert(vbe_setMode(mode));
		}
	}

	/*sVM86Regs regs;
	memset(&regs,0,sizeof(regs));
	regs.ax = 0x0002;
	if(vm86int(0x10,&regs,NULL,0) < 0)
		printe("Switch to text-mode failed");*/

	content = (u8*)malloc(COLS * ROWS * 2);
	if(content == NULL)
		error("Unable to alloc mem for content");
	for(y = 0; y < ROWS; y++) {
		for(x = 0; x < COLS; x++) {
			content[y * COLS * 2 + x * 2] = ' ';
			content[y * COLS * 2 + x * 2 + 1] = 0x07;
		}
	}

	/* init cache for white-on-black-chars */
	whOnBlCache = (u8*)malloc((FONT_WIDTH + PAD * 2) * (FONT_HEIGHT + PAD * 2) *
			(minfo->bitsPerPixel / 8) * FONT_COUNT);
	if(whOnBlCache == NULL)
		error("Unable to alloc mem for white-on-black-cache");
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

	lastCol = COLS;
	lastRow = ROWS;

	while(1) {
		tFD fd = getClient(&id,1,&client);
		if(fd < 0)
			wait(EV_CLIENT);
		else {
			while(receive(fd,&mid,&msg,sizeof(msg)) > 0) {
				switch(mid) {
					case MSG_DRV_OPEN:
						msg.args.arg1 = 0;
						/*vesa_setMode(RESOLUTION_X,RESOLUTION_Y,BITS_PER_PIXEL);*/
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
						if(offset + count <= ROWS * COLS * 2 && offset + count > offset) {
							char *str = (char*)malloc(count);
							vassert(str,"Unable to alloc mem");
							msg.args.arg1 = 0;
							if(receive(fd,&mid,str,count) >= 0) {
								vesa_drawStr((offset / 2) % COLS,(offset / 2) / COLS,str,count / 2);
								msg.args.arg1 = count;
							}
							free(str);
						}
						send(fd,MSG_DRV_WRITE_RESP,&msg,sizeof(msg.args));
					}
					break;

					case MSG_DRV_IOCTL: {
						switch(msg.data.arg1) {
							case IOCTL_VID_SETCURSOR: {
								sIoCtlPos *pos = (sIoCtlPos*)msg.data.d;
								pos->col = MIN(pos->col,COLS);
								pos->row = MIN(pos->row,ROWS);
								vesa_setCursor(pos->col,pos->row);
								msg.data.arg1 = 0;
							}
							break;

							case IOCTL_VID_GETSIZE: {
								sIoCtlSize *size = (sIoCtlSize*)msg.data.d;
								size->width = COLS;
								size->height = ROWS;
								msg.data.arg1 = sizeof(sIoCtlSize);
							}
							break;

							default:
								msg.data.arg1 = ERR_UNSUPPORTED_OP;
								break;
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
	releaseIOPorts(VBE_DISPI_IOPORT_INDEX,3);
	return EXIT_SUCCESS;
}

static void vesa_write(u16 index,u16 value) {
	outWord(VBE_DISPI_IOPORT_INDEX,index);
	outWord(VBE_DISPI_IOPORT_DATA,value);
}

static void vesa_setMode(tSize xres,tSize yres,u16 bpp) {
    vesa_write(VBE_DISPI_INDEX_ENABLE,VBE_DISPI_DISABLED);
    vesa_write(VBE_DISPI_INDEX_XRES,xres);
    vesa_write(VBE_DISPI_INDEX_YRES,yres);
    vesa_write(VBE_DISPI_INDEX_BPP,bpp);
    vesa_write(VBE_DISPI_INDEX_ENABLE,VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
}

static void vesa_drawStr(tCoord col,tCoord row,const char *str,u32 len) {
	while(len-- > 0) {
		char c = *str++;
		char color = *str++;
		vesa_drawChar(col,row,c,color);
		if(col >= COLS - 1) {
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
	if(content[row * COLS * 2 + col * 2] == c && content[row * COLS * 2 + col * 2 + 1] == color)
		return;

	content[row * COLS * 2 + col * 2] = c;
	content[row * COLS * 2 + col * 2 + 1] = color;
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
	u32 x,y;
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
	u32 x,y;
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
	u32 x,y;
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
	if(lastCol < COLS && lastRow < ROWS)
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
		u8 *col = colors[color];
		vidwork = pxFunc(vidwork,col[2],col[1],col[0]);
	}
}
