/**
 * $Id: vesamain.c 312 2009-09-07 12:55:10Z nasmussen $
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
#include <assert.h>

#include "font.h"

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

#define VESA_MEMORY						0xE0000000
#define RESOLUTION_X					800
#define RESOLUTION_Y					600
#define BITS_PER_PIXEL					24
#define PIXEL_SIZE						(BITS_PER_PIXEL / 8)
#define VESA_MEM_SIZE					(RESOLUTION_X * RESOLUTION_Y * PIXEL_SIZE)

#define CURSOR_LEN						FONT_WIDTH
#define CURSOR_COLOR					0xFFFFFF
#define CURSOR_SIZE						(CURSOR_LEN * 2 + 1)

#define COLS							(RESOLUTION_X / (FONT_WIDTH + 2))
#define ROWS							(RESOLUTION_Y / (FONT_HEIGHT + 2))

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

static void vbe_write(u16 index,u16 value);
static void vbe_setMode(tSize xres,tSize yres,u16 bpp);
static void vbe_drawStr(tCoord col,tCoord row,const char *str,u32 len);
static void vbe_drawChar(tCoord col,tCoord row,char c,u8 color);
static void vbe_setCursor(tCoord col,tCoord row);
static void vbe_drawCursor(tCoord col,tCoord row,u8 color);

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

static u8 lastCol = COLS;
static u8 lastRow = ROWS;
static u8 *video;
static sMsg msg;

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

	video = mapPhysical(VESA_MEMORY,VESA_MEM_SIZE);
	if(video == NULL)
		error("Unable to request physical memory at 0x%x",VESA_MEMORY);

	/* init cache for white-on-black-chars */
	whOnBlCache = (u8*)malloc((FONT_WIDTH + 2) * (FONT_HEIGHT * 2) * PIXEL_SIZE * FONT_COUNT);
	if(whOnBlCache == NULL)
		error("Unable to alloc mem for white-on-black-cache");
	cc = whOnBlCache;
	for(i = 0; i < FONT_COUNT; i++) {
		const char *pixel = font6x8[i];
		for(y = 0; y < FONT_HEIGHT + 2; y++) {
			for(x = 0; x < FONT_WIDTH + 2; x++) {
				if(y > 0 && y < FONT_HEIGHT + 1 && x > 0 && x < FONT_WIDTH + 1 &&
						pixel[(y - 1) * FONT_WIDTH + (x - 1)]) {
					*cc++ = colors[WHITE][2];
					*cc++ = colors[WHITE][1];
					*cc++ = colors[WHITE][0];
				}
				else {
					*cc++ = colors[BLACK][2];
					*cc++ = colors[BLACK][1];
					*cc++ = colors[BLACK][0];
				}
			}
		}
	}

	id = regService("vesatext",SERV_DRIVER);
	if(id < 0)
		error("Unable to register service 'vesatext'");

	while(1) {
		tFD fd = getClient(&id,1,&client);
		if(fd < 0)
			wait(EV_CLIENT);
		else {
			while(receive(fd,&mid,&msg,sizeof(msg)) > 0) {
				switch(mid) {
					case MSG_DRV_OPEN:
						msg.args.arg1 = 0;
						vbe_setMode(RESOLUTION_X,RESOLUTION_Y,BITS_PER_PIXEL);
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
							receive(fd,&mid,str,count);
							vbe_drawStr((offset / 2) % COLS,(offset / 2) / COLS,str,count / 2);
							msg.args.arg1 = count;
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
								vbe_setCursor(pos->col,pos->row);
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

static void vbe_write(u16 index,u16 value) {
	outWord(VBE_DISPI_IOPORT_INDEX,index);
	outWord(VBE_DISPI_IOPORT_DATA,value);
}

static void vbe_setMode(tSize xres,tSize yres,u16 bpp) {
    vbe_write(VBE_DISPI_INDEX_ENABLE,VBE_DISPI_DISABLED);
    vbe_write(VBE_DISPI_INDEX_XRES,xres);
    vbe_write(VBE_DISPI_INDEX_YRES,yres);
    vbe_write(VBE_DISPI_INDEX_BPP,bpp);
    vbe_write(VBE_DISPI_INDEX_ENABLE,VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
}

static void vbe_drawStr(tCoord col,tCoord row,const char *str,u32 len) {
	while(len-- > 0) {
		char c = *str++;
		char color = *str++;
		vbe_drawChar(col,row,c,color);
		if(col >= COLS - 1) {
			row++;
			col = 0;
		}
		else
			col++;
	}
}

static void vbe_drawChar(tCoord col,tCoord row,char c,u8 color) {
	u32 x,y;
	u8 *vid = video +
			row * (FONT_HEIGHT + 2) * RESOLUTION_X * PIXEL_SIZE +
			col * (FONT_WIDTH + 2) * PIXEL_SIZE;
	if(c < 32)
		return;
	if(color == ((BLACK << 4) | WHITE)) {
		const u8 *cache = whOnBlCache + (c - 32) * (FONT_HEIGHT + 2) * (FONT_WIDTH + 2) * PIXEL_SIZE;
		for(y = 0; y < FONT_HEIGHT + 2; y++) {
			memcpy(vid,cache,(FONT_WIDTH + 2) * PIXEL_SIZE);
			cache += (FONT_WIDTH + 2) * PIXEL_SIZE;
			vid += RESOLUTION_X * PIXEL_SIZE;
		}
	}
	else {
		u8 *vidwork = vid;
		u8 colFront1 = colors[color & 0xf][2];
		u8 colFront2 = colors[color & 0xf][1];
		u8 colFront3 = colors[color & 0xf][0];
		u8 colBack1 = colors[color >> 4][2];
		u8 colBack2 = colors[color >> 4][1];
		u8 colBack3 = colors[color >> 4][0];
		const char *pixel = font6x8[(s32)c - 32];
		for(y = 0; y < FONT_HEIGHT + 2; y++) {
			vidwork = vid + y * RESOLUTION_X * PIXEL_SIZE;
			for(x = 0; x < FONT_WIDTH + 2; x++) {
				if(y > 0 && x < FONT_WIDTH + 1 && x > 0) {
					if(y < FONT_HEIGHT + 1 && *pixel) {
						*vidwork++ = colFront1;
						*vidwork++ = colFront2;
						*vidwork++ = colFront3;
					}
					else {
						*vidwork++ = colBack1;
						*vidwork++ = colBack2;
						*vidwork++ = colBack3;
					}
					pixel++;
				}
				else {
					*vidwork++ = colBack1;
					*vidwork++ = colBack2;
					*vidwork++ = colBack3;
				}
			}
		}
	}
}

static void vbe_setCursor(tCoord col,tCoord row) {
	if(lastCol < COLS && lastRow < ROWS)
		vbe_drawCursor(lastCol,lastRow,BLACK);
	vbe_drawCursor(col,row,WHITE);
	lastCol = col;
	lastRow = row;
}

static void vbe_drawCursor(tCoord col,tCoord row,u8 color) {
	u32 x,y = FONT_HEIGHT + 1;
	u8 *vid = video +
			row * (FONT_HEIGHT + 2) * RESOLUTION_X * PIXEL_SIZE +
			col * (FONT_WIDTH + 2) * PIXEL_SIZE;
	u8 *vidwork = vid + y * RESOLUTION_X * PIXEL_SIZE;
	for(x = 0; x < CURSOR_LEN; x++) {
		*vidwork++ = colors[color][2];
		*vidwork++ = colors[color][1];
		*vidwork++ = colors[color][0];
	}
}
