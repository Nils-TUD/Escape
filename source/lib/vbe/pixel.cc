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
#include <sys/messages.h>
#include <ipc/proto/screen.h>
#include <vbe/vbe.h>
#include <assert.h>
#include <string.h>

static uint8_t *vbe_setPixel16(const ipc::Screen::Mode &mode,uint8_t *vidwork,uint8_t *color);
static uint8_t *vbe_setPixel24(const ipc::Screen::Mode &mode,uint8_t *vidwork,uint8_t *color);
static uint8_t *vbe_setPixel32(const ipc::Screen::Mode &mode,uint8_t *vidwork,uint8_t *color);

static fSetPixel setPixel[] = {
	/*  0 bpp */	NULL,
	/*  8 bpp */	NULL,
	/* 16 bpp */	vbe_setPixel16,
	/* 24 bpp */	vbe_setPixel24,
	/* 32 bpp */	vbe_setPixel32
};

void vbe_setPixel(const ipc::Screen::Mode &mode,uint8_t *frmbuf,gpos_t x,gpos_t y,uint8_t *color) {
	size_t bpp = mode.bitsPerPixel / 8;
	uint8_t *pos = frmbuf + (y * mode.width + x) * bpp;
	vbe_setPixelAt(mode,pos,color);
}

uint8_t *vbe_setPixelAt(const ipc::Screen::Mode &mode,uint8_t *pos,uint8_t *color) {
	assert(mode.bitsPerPixel / 8 >= 2 && mode.bitsPerPixel / 8 <= 4);
	return setPixel[mode.bitsPerPixel / 8](mode,pos,color);
}

fSetPixel vbe_getPixelFunc(const ipc::Screen::Mode &mode) {
	return setPixel[mode.bitsPerPixel / 8];
}

static uint8_t *vbe_setPixel16(const ipc::Screen::Mode &mode,uint8_t *vidwork,uint8_t *color) {
	uint8_t red = color[0] >> (8 - mode.redMaskSize);
	uint8_t green = color[1] >> (8 - mode.greenMaskSize);
	uint8_t blue = color[2] >> (8 - mode.blueMaskSize);
	uint16_t val = (red << mode.redFieldPosition) |
			(green << mode.greenFieldPosition) |
			(blue << mode.blueFieldPosition);
	*((uint16_t*)vidwork) = val;
	return vidwork + 2;
}

static uint8_t *vbe_setPixel24(const ipc::Screen::Mode &mode,uint8_t *vidwork,uint8_t *color) {
	uint8_t red = color[0] >> (8 - mode.redMaskSize);
	uint8_t green = color[1] >> (8 - mode.greenMaskSize);
	uint8_t blue = color[2] >> (8 - mode.blueMaskSize);
	uint32_t val = (red << mode.redFieldPosition) |
			(green << mode.greenFieldPosition) |
			(blue << mode.blueFieldPosition);
	vidwork[2] = val >> 16;
	vidwork[1] = val >> 8;
	vidwork[0] = val & 0xFF;
	return vidwork + 3;
}

static uint8_t *vbe_setPixel32(const ipc::Screen::Mode &mode,uint8_t *vidwork,uint8_t *color) {
	uint8_t red = color[0] >> (8 - mode.redMaskSize);
	uint8_t green = color[1] >> (8 - mode.greenMaskSize);
	uint8_t blue = color[2] >> (8 - mode.blueMaskSize);
	uint32_t val = (red << mode.redFieldPosition) |
			(green << mode.greenFieldPosition) |
			(blue << mode.blueFieldPosition);
	*((uint32_t*)vidwork) = val;
	return vidwork + 4;
}
