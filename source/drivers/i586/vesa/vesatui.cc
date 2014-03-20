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

#include "vesatui.h"

#define CURSOR_LEN						FONT_WIDTH
#define CURSOR_COLOR					0xFFFFFF
#define CURSOR_SIZE						(CURSOR_LEN * 2 + 1)

static void vesatui_drawChar(sVESAScreen *scr,gpos_t col,gpos_t row,uint8_t c,uint8_t color);
static void vesatui_drawCursor(sVESAScreen *scr,gpos_t col,gpos_t row,uint8_t color);

uint8_t *vesatui_setPixel(sVESAScreen *scr,uint8_t *vid,uint8_t *color) {
	return vbe_setPixelAt(*scr->mode,vid,color);
}

void vesatui_drawChars(sVESAScreen *scr,gpos_t col,gpos_t row,const uint8_t *str,size_t len) {
	while(len-- > 0) {
		uint8_t c = *str++;
		uint8_t color = *str++;
		vesatui_drawChar(scr,col,row,c,color);
		if(col >= scr->cols - 1) {
			row++;
			col = 0;
		}
		else
			col++;
	}
}

void vesatui_setCursor(sVESAScreen *scr,gpos_t col,gpos_t row) {
	if(scr->lastCol < scr->cols && scr->lastRow < scr->rows)
		vesatui_drawCursor(scr,scr->lastCol,scr->lastRow,BLACK);
	if(col < scr->cols && row < scr->rows)
		vesatui_drawCursor(scr,col,row,WHITE);
	scr->lastCol = col;
	scr->lastRow = row;
}

static void vesatui_drawChar(sVESAScreen *scr,gpos_t col,gpos_t row,uint8_t c,uint8_t color) {
	gpos_t y;
	gsize_t rx = scr->mode->width;
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
	else
		vbet_drawChar(*scr->mode,scr->frmbuf,col,row,c,color);
}

static void vesatui_drawCursor(sVESAScreen *scr,gpos_t col,gpos_t row,uint8_t color) {
	gsize_t xres = scr->mode->width;
	gsize_t pxSize = scr->mode->bitsPerPixel / 8;
	fSetPixel pxFunc = vbe_getPixelFunc(*scr->mode);
	gpos_t x,y = FONT_HEIGHT + PAD;
	uint8_t *vid = scr->frmbuf +
			row * (FONT_HEIGHT + PAD * 2) * xres * pxSize +
			col * (FONT_WIDTH + PAD * 2) * pxSize;
	uint8_t *vidwork = vid + y * xres * pxSize;
	for(x = 0; x < CURSOR_LEN; x++) {
		uint8_t *colPtr = vbet_getColor(color);
		vidwork = pxFunc(*scr->mode,vidwork,colPtr);
	}
}
