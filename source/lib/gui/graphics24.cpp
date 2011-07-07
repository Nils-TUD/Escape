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
#include <esc/debug.h>
#include <gui/graphics.h>
#include <gui/graphics24.h>

namespace gui {
	Graphics24::Graphics24(Graphics &g,gpos_t x,gpos_t y)
		: Graphics(g,x,y) {

	}

	Graphics24::Graphics24(gpos_t x,gpos_t y,gsize_t width,gsize_t height,gcoldepth_t bpp)
		: Graphics(x,y,width,height,bpp) {

	}

	Graphics24::~Graphics24() {
		// nothing to do
	}

	void Graphics24::doSetPixel(gpos_t x,gpos_t y) {
		uint8_t *col = (uint8_t*)&_col;
		uint8_t *addr = _pixels + ((_offy + y) * _width + (_offx + x)) * 3;
		*addr++ = *col++;
		*addr++ = *col++;
		*addr = *col;
	}

	void Graphics24::fillRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		validateParams(x,y,width,height);
		gpos_t yend = y + height;
		updateMinMax(x,y);
		updateMinMax(x + width - 1,yend - 1);
		gpos_t xcur;
		gpos_t xend = x + width;
		// optimized version for 24bit
		// This is necessary if we want to have reasonable speed because the simple version
		// performs too many function-calls (one to a virtual-function and one to memcpy
		// that the compiler doesn't inline). Additionally the offset into the
		// memory-region will be calculated many times.
		// This version is much quicker :)
		uint8_t *col = (uint8_t*)&_col;
		gsize_t widthadd = _width * 3;
		uint8_t *addr;
		uint8_t *orgaddr = _pixels + (((_offy + y) * _width + (_offx + x)) * 3);
		for(; y < yend; y++) {
			addr = orgaddr;
			for(xcur = x; xcur < xend; xcur++) {
				*addr++ = *col;
				*addr++ = *(col + 1);
				*addr++ = *(col + 2);
			}
			orgaddr += widthadd;
		}
	}
}
