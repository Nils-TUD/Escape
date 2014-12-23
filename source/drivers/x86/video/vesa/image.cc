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
#include <sys/io.h>
#include <gui/graphics/color.h>
#include <dirent.h>
#include <assert.h>
#include <stdlib.h>
#include "image.h"

void VESABMPainter::paintPixel(gpos_t x,gpos_t y,uint32_t col) {
	uint8_t bytes[4];
	gui::Color color(col);
	bytes[0] = color.getRed();
	bytes[1] = color.getGreen();
	bytes[2] = color.getBlue();
	bytes[3] = color.getAlpha();
	size_t bpp = _scr->mode->bitsPerPixel / 8;
	uint8_t *pos = _scr->frmbuf + ((_y + y) * _scr->mode->width + x + _x) * bpp;
	_setPixel(*_scr->mode,pos,bytes);
}

void VESAImage::paint(VESAScreen *scr,gpos_t x,gpos_t y) {
	gsize_t width,height;
	_img->getSize(&width,&height);

	/* ensure that we stay within bounds */
	if((gsize_t)x + width > scr->mode->width)
		width = scr->mode->width - x;
	if((gsize_t)y + height > scr->mode->height)
		height = scr->mode->height - y;

	_painter->reset(scr,x,y);
	_img->paint(0,0,width,height);
}
