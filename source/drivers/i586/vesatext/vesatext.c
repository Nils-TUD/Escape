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
#include <string.h>

#include "vesatext.h"
#include "font.h"

#define CURSOR_LEN						FONT_WIDTH
#define CURSOR_COLOR					0xFFFFFF
#define CURSOR_SIZE						(CURSOR_LEN * 2 + 1)

typedef uint8_t *(*fSetPixel)(sVESAScreen *scr,uint8_t *vidwork,uint8_t *color);

static void vesat_drawChar(sVESAScreen *scr,gpos_t col,gpos_t row,uint8_t c,uint8_t color);
static void vesat_drawCharLoop16(sVESAScreen *scr,uint8_t *vid,uint8_t c,uint8_t *fg,uint8_t *bg);
static void vesat_drawCharLoop24(sVESAScreen *scr,uint8_t *vid,uint8_t c,uint8_t *fg,uint8_t *bg);
static void vesat_drawCharLoop32(sVESAScreen *scr,uint8_t *vid,uint8_t c,uint8_t *fg,uint8_t *bg);
static uint8_t *vesat_setPixel16(sVESAScreen *scr,uint8_t *vidwork,uint8_t *color);
static uint8_t *vesat_setPixel24(sVESAScreen *scr,uint8_t *vidwork,uint8_t *color);
static uint8_t *vesat_setPixel32(sVESAScreen *scr,uint8_t *vidwork,uint8_t *color);
static void vesat_drawCursor(sVESAScreen *scr,gpos_t col,gpos_t row,uint8_t color);

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

static fSetPixel setPixel[] = {
	/*  0 bpp */	NULL,
	/*  8 bpp */	NULL,
	/* 16 bpp */	vesat_setPixel16,
	/* 24 bpp */	vesat_setPixel24,
	/* 32 bpp */	vesat_setPixel32
};

uint8_t *vesat_setPixel(sVESAScreen *scr,uint8_t *vid,uint8_t *color) {
	return setPixel[scr->mode->bitsPerPixel / 8](scr,vid,color);
}

uint8_t *vesat_getColor(tColor col) {
	return colors[col];
}

void vesat_drawChars(sVESAScreen *scr,gpos_t col,gpos_t row,const uint8_t *str,size_t len) {
	while(len-- > 0) {
		uint8_t c = *str++;
		uint8_t color = *str++;
		vesat_drawChar(scr,col,row,c,color);
		if(col >= scr->cols - 1) {
			row++;
			col = 0;
		}
		else
			col++;
	}
}

void vesat_setCursor(sVESAScreen *scr,gpos_t col,gpos_t row) {
	if(scr->lastCol < scr->cols && scr->lastRow < scr->rows)
		vesat_drawCursor(scr,scr->lastCol,scr->lastRow,BLACK);
	if(col < scr->cols && row < scr->rows)
		vesat_drawCursor(scr,col,row,WHITE);
	scr->lastCol = col;
	scr->lastRow = row;
}

static void vesat_drawChar(sVESAScreen *scr,gpos_t col,gpos_t row,uint8_t c,uint8_t color) {
	gpos_t y;
	gsize_t rx = scr->mode->xResolution;
	size_t pxSize = scr->mode->bitsPerPixel / 8;
	uint8_t *vid;
	/* don't print the same again ;) */
	if(scr->content[row * scr->cols * 2 + col * 2] == c &&
		scr->content[row * scr->cols * 2 + col * 2 + 1] == color)
		return;

	scr->content[row * scr->cols * 2 + col * 2] = c;
	scr->content[row * scr->cols * 2 + col * 2 + 1] = color;
	vid = scr->frmbuf + row * (FONT_HEIGHT + PAD * 2) * rx * pxSize + col *
			(FONT_WIDTH + PAD * 2) * pxSize;
	if(color == ((BLACK << 4) | WHITE)) {
		const uint8_t *cache = scr->whOnBlCache + c * (FONT_HEIGHT + PAD * 2) *
				(FONT_WIDTH + PAD * 2) * pxSize;
		for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
			memcpy(vid,cache,(FONT_WIDTH + PAD * 2) * pxSize);
			cache += (FONT_WIDTH + PAD * 2) * pxSize;
			vid += rx * pxSize;
		}
	}
	else {
		uint8_t *fg = vesat_getColor(color & 0xf);
		uint8_t *bg = vesat_getColor(color >> 4);
		switch(scr->mode->bitsPerPixel) {
			case 16:
				vesat_drawCharLoop16(scr,vid,c,fg,bg);
				break;
			case 24:
				vesat_drawCharLoop24(scr,vid,c,fg,bg);
				break;
			case 32:
				vesat_drawCharLoop32(scr,vid,c,fg,bg);
				break;
		}
	}
}

static void vesat_drawCharLoop16(sVESAScreen *scr,uint8_t *vid,uint8_t c,uint8_t *fg,uint8_t *bg) {
	int x,y;
	uint8_t *vidwork;
	gsize_t rx = scr->mode->xResolution;
	uint8_t rms = scr->mode->redMaskSize;
	uint8_t gms = scr->mode->greenMaskSize;
	uint8_t bms = scr->mode->blueMaskSize;
	uint8_t rfp = scr->mode->redFieldPosition;
	uint8_t gfp = scr->mode->greenFieldPosition;
	uint8_t bfp = scr->mode->blueFieldPosition;

#define DRAWPIXEL16(col) \
		uint8_t ___red = (col[0]) >> (8 - rms); \
		uint8_t ___green = (col[1]) >> (8 - gms); \
		uint8_t ___blue = (col[2]) >> (8 - bms); \
		*((uint16_t*)(vidwork)) = (___red << rfp) | \
				(___green << gfp) | \
				(___blue << bfp); \
		(vidwork) += 2;

	for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
		vidwork = vid + y * rx * 2;
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

static void vesat_drawCharLoop24(sVESAScreen *scr,uint8_t *vid,uint8_t c,uint8_t *fg,uint8_t *bg) {
	int x,y;
	uint8_t *vidwork;
	gsize_t rx = scr->mode->xResolution;
	uint8_t rms = scr->mode->redMaskSize;
	uint8_t gms = scr->mode->greenMaskSize;
	uint8_t bms = scr->mode->blueMaskSize;
	uint8_t rfp = scr->mode->redFieldPosition;
	uint8_t gfp = scr->mode->greenFieldPosition;
	uint8_t bfp = scr->mode->blueFieldPosition;

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
		vidwork = vid + y * rx * 3;
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

static void vesat_drawCharLoop32(sVESAScreen *scr,uint8_t *vid,uint8_t c,uint8_t *fg,uint8_t *bg) {
	int x,y;
	uint8_t *vidwork;
	gsize_t rx = scr->mode->xResolution;
	uint8_t rms = scr->mode->redMaskSize;
	uint8_t gms = scr->mode->greenMaskSize;
	uint8_t bms = scr->mode->blueMaskSize;
	uint8_t rfp = scr->mode->redFieldPosition;
	uint8_t gfp = scr->mode->greenFieldPosition;
	uint8_t bfp = scr->mode->blueFieldPosition;

#define DRAWPIXEL32(col) \
		uint8_t ___red = (col[0]) >> (8 - rms); \
		uint8_t ___green = (col[1]) >> (8 - gms); \
		uint8_t ___blue = (col[2]) >> (8 - bms); \
		*((uint32_t*)vidwork) = (___red << rfp) | \
			(___green << gfp) | \
			(___blue << bfp); \
		vidwork += 4;

	for(y = 0; y < FONT_HEIGHT + PAD * 2; y++) {
		vidwork = vid + y * rx * 4;
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

static uint8_t *vesat_setPixel16(sVESAScreen *scr,uint8_t *vidwork,uint8_t *color) {
	uint8_t red = color[0] >> (8 - scr->mode->redMaskSize);
	uint8_t green = color[1] >> (8 - scr->mode->greenMaskSize);
	uint8_t blue = color[2] >> (8 - scr->mode->blueMaskSize);
	uint16_t val = (red << scr->mode->redFieldPosition) |
			(green << scr->mode->greenFieldPosition) |
			(blue << scr->mode->blueFieldPosition);
	*((uint16_t*)vidwork) = val;
	return vidwork + 2;
}

static uint8_t *vesat_setPixel24(sVESAScreen *scr,uint8_t *vidwork,uint8_t *color) {
	uint8_t red = color[0] >> (8 - scr->mode->redMaskSize);
	uint8_t green = color[1] >> (8 - scr->mode->greenMaskSize);
	uint8_t blue = color[2] >> (8 - scr->mode->blueMaskSize);
	uint32_t val = (red << scr->mode->redFieldPosition) |
			(green << scr->mode->greenFieldPosition) |
			(blue << scr->mode->blueFieldPosition);
	vidwork[2] = val >> 16;
	vidwork[1] = val >> 8;
	vidwork[0] = val & 0xFF;
	return vidwork + 3;
}

static uint8_t *vesat_setPixel32(sVESAScreen *scr,uint8_t *vidwork,uint8_t *color) {
	uint8_t red = color[0] >> (8 - scr->mode->redMaskSize);
	uint8_t green = color[1] >> (8 - scr->mode->greenMaskSize);
	uint8_t blue = color[2] >> (8 - scr->mode->blueMaskSize);
	uint32_t val = (red << scr->mode->redFieldPosition) |
			(green << scr->mode->greenFieldPosition) |
			(blue << scr->mode->blueFieldPosition);
	*((uint32_t*)vidwork) = val;
	return vidwork + 4;
}

static void vesat_drawCursor(sVESAScreen *scr,gpos_t col,gpos_t row,uint8_t color) {
	gsize_t xres = scr->mode->xResolution;
	gsize_t pxSize = scr->mode->bitsPerPixel / 8;
	fSetPixel pxFunc = setPixel[pxSize];
	gpos_t x,y = FONT_HEIGHT + PAD;
	uint8_t *vid = scr->frmbuf +
			row * (FONT_HEIGHT + PAD * 2) * xres * pxSize +
			col * (FONT_WIDTH + PAD * 2) * pxSize;
	uint8_t *vidwork = vid + y * xres * pxSize;
	for(x = 0; x < CURSOR_LEN; x++) {
		uint8_t *colPtr = colors[color];
		vidwork = pxFunc(scr,vidwork,colPtr);
	}
}
