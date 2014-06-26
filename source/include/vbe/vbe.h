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

#pragma once

#include <esc/common.h>
#include <ipc/proto/screen.h>

#define FONT_WIDTH	8
#define FONT_HEIGHT	16
#define PAD			0
#define FONT_COUNT	256

#define PIXEL_SET(c,x,y)				\
	((font8x16)[(c) * FONT_HEIGHT + (y)] & (1 << (FONT_WIDTH - (x) - 1)))

typedef uint8_t *(*fSetPixel)(const ipc::Screen::Mode &mode,uint8_t *vidwork,uint8_t *color);

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

/**
 * Sets a pixel with color <color> at position <x>,<y> in <frmbuf>.
 *
 * @param mode the screen mode
 * @param frmbuf the framebuffer
 * @param x the x-position
 * @param y the y-position
 * @param color the color (in the corresponding format)
 */
void vbe_setPixel(const ipc::Screen::Mode &mode,uint8_t *frmbuf,gpos_t x,gpos_t y,uint8_t *color);

/**
 * Sets a pixel with color <color> at <pos> and returns the position of the next pixel.
 *
 * @param mode the screen mode
 * @param pos the position in the framebuffer where to put the pixel
 * @param color the color (in the corresponding format)
 * @return the position of the next pixel
 */
uint8_t *vbe_setPixelAt(const ipc::Screen::Mode &mode,uint8_t *pos,uint8_t *color);

/**
 * @param mode the screen mode
 * @return the function to put a pixel in the given mode
 */
fSetPixel vbe_getPixelFunc(const ipc::Screen::Mode &mode);

uint8_t *vbet_getColor(tColor col);
void vbet_drawChar(const ipc::Screen::Mode &mode,uint8_t *frmbuf,gpos_t col,gpos_t row,
	uint8_t c,uint8_t color);

extern const uchar font8x16[];
