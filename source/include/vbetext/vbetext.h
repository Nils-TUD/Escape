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

#pragma once

#include <esc/common.h>
#include <esc/messages.h>

#define FONT_WIDTH	8
#define FONT_HEIGHT	16
#define PAD			0
#define FONT_COUNT	256

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

uint8_t *vbet_getColor(tColor col);
void vbet_drawChar(sScreenMode *mode,uint8_t *frmbuf,gpos_t col,gpos_t row,uint8_t c,uint8_t color);

extern const uchar font8x16[];
