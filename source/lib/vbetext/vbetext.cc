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

#include <esc/common.h>
#include <vbe/vbe.h>
#include <vbetext/vbetext.h>
#include <string.h>

#include "font.inc"

static void vbet_drawCharLoop16(const ipc::Screen::Mode &mode,uint8_t *pos,uint8_t c,uint8_t *fg,uint8_t *bg);
static void vbet_drawCharLoop24(const ipc::Screen::Mode &mode,uint8_t *pos,uint8_t c,uint8_t *fg,uint8_t *bg);
static void vbet_drawCharLoop32(const ipc::Screen::Mode &mode,uint8_t *pos,uint8_t c,uint8_t *fg,uint8_t *bg);

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

uint8_t *vbet_getColor(tColor col) {
	return colors[col];
}

void vbet_drawChar(const ipc::Screen::Mode &mode,uint8_t *frmbuf,gpos_t col,gpos_t row,uint8_t c,uint8_t color) {
	uint8_t *fg = vbet_getColor(color & 0xf);
	uint8_t *bg = vbet_getColor(color >> 4);
	gsize_t rx = mode.width;
	size_t pxSize = mode.bitsPerPixel / 8;
	uint8_t *pos = frmbuf + row * (FONT_HEIGHT + PAD * 2) * rx * pxSize +
		col * (FONT_WIDTH + PAD * 2) * pxSize;
	switch(mode.bitsPerPixel) {
		case 16:
			vbet_drawCharLoop16(mode,pos,c,fg,bg);
			break;
		case 24:
			vbet_drawCharLoop24(mode,pos,c,fg,bg);
			break;
		case 32:
			vbet_drawCharLoop32(mode,pos,c,fg,bg);
			break;
	}
}

static void vbet_drawCharLoop16(const ipc::Screen::Mode &mode,uint8_t *pos,uint8_t c,uint8_t *fg,uint8_t *bg) {
	int x,y;
	uint8_t *vidwork;
	gsize_t rx = mode.width;
	uint8_t rms = mode.redMaskSize;
	uint8_t gms = mode.greenMaskSize;
	uint8_t bms = mode.blueMaskSize;
	uint8_t rfp = mode.redFieldPosition;
	uint8_t gfp = mode.greenFieldPosition;
	uint8_t bfp = mode.blueFieldPosition;

#define DRAWPIXEL16(col) \
		uint8_t ___red = (col[0]) >> (8 - rms); \
		uint8_t ___green = (col[1]) >> (8 - gms); \
		uint8_t ___blue = (col[2]) >> (8 - bms); \
		*((uint16_t*)(vidwork)) = (___red << rfp) | \
				(___green << gfp) | \
				(___blue << bfp); \
		(vidwork) += 2;

	for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
		vidwork = pos + y * rx * 2;
		for(x = 0; x < FONT_WIDTH + PAD * 2; x++) {
			if(y >= PAD && x < FONT_WIDTH + PAD && x >= PAD) {
				if(y < FONT_HEIGHT + PAD && PIXEL_SET(c,x - PAD,y - PAD)) {
					DRAWPIXEL16(fg);
				}
				else {
					DRAWPIXEL16(bg);
				}
			}
			else {
				DRAWPIXEL16(bg);
			}
		}
	}
}

static void vbet_drawCharLoop24(const ipc::Screen::Mode &mode,uint8_t *pos,uint8_t c,uint8_t *fg,uint8_t *bg) {
	int x,y;
	uint8_t *vidwork;
	gsize_t rx = mode.width;
	uint8_t rms = mode.redMaskSize;
	uint8_t gms = mode.greenMaskSize;
	uint8_t bms = mode.blueMaskSize;
	uint8_t rfp = mode.redFieldPosition;
	uint8_t gfp = mode.greenFieldPosition;
	uint8_t bfp = mode.blueFieldPosition;

#define DRAWPIXEL24(col) \
		uint8_t ___red = (col[0]) >> (8 - rms); \
		uint8_t ___green = (col[1]) >> (8 - gms); \
		uint8_t ___blue = (col[2]) >> (8 - bms); \
		uint32_t ___val = (___red << rfp) | \
			(___green << gfp) | \
			(___blue << bfp); \
		*vidwork++ = ___val & 0xFF; \
		*vidwork++ = ___val >> 8; \
		*vidwork++ = ___val >> 16;

	for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
		vidwork = pos + y * rx * 3;
		for(x = 0; x < FONT_WIDTH + PAD * 2; x++) {
			if(y >= PAD && x < FONT_WIDTH + PAD && x >= PAD) {
				if(y < FONT_HEIGHT + PAD && PIXEL_SET(c,x - PAD,y - PAD)) {
					DRAWPIXEL24(fg)
				}
				else {
					DRAWPIXEL24(bg);
				}
			}
			else {
				DRAWPIXEL24(bg);
			}
		}
	}
}

static void vbet_drawCharLoop32(const ipc::Screen::Mode &mode,uint8_t *pos,uint8_t c,uint8_t *fg,uint8_t *bg) {
	int x,y;
	uint8_t *vidwork;
	gsize_t rx = mode.width;
	uint8_t rms = mode.redMaskSize;
	uint8_t gms = mode.greenMaskSize;
	uint8_t bms = mode.blueMaskSize;
	uint8_t rfp = mode.redFieldPosition;
	uint8_t gfp = mode.greenFieldPosition;
	uint8_t bfp = mode.blueFieldPosition;

#define DRAWPIXEL32(col) \
		uint8_t ___red = (col[0]) >> (8 - rms); \
		uint8_t ___green = (col[1]) >> (8 - gms); \
		uint8_t ___blue = (col[2]) >> (8 - bms); \
		*((uint32_t*)vidwork) = (___red << rfp) | \
			(___green << gfp) | \
			(___blue << bfp); \
		vidwork += 4;

	for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
		vidwork = pos + y * rx * 4;
		for(x = 0; x < FONT_WIDTH + PAD * 2; x++) {
			if(y >= PAD && x < FONT_WIDTH + PAD && x >= PAD) {
				if(y < FONT_HEIGHT + PAD && PIXEL_SET(c,x - PAD,y - PAD)) {
					DRAWPIXEL32(fg);
				}
				else {
					DRAWPIXEL32(bg);
				}
			}
			else {
				DRAWPIXEL32(bg);
			}
		}
	}
}
