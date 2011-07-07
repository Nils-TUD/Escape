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
#include <gui/graphics32.h>

namespace gui {
	Graphics32::Graphics32(Graphics &g,gpos_t x,gpos_t y)
		: Graphics(g,x,y) {

	}

	Graphics32::Graphics32(gpos_t x,gpos_t y,gsize_t width,gsize_t height,gcoldepth_t bpp)
		: Graphics(x,y,width,height,bpp) {

	}

	Graphics32::~Graphics32() {
		// nothing to do
	}

	void Graphics32::doSetPixel(gpos_t x,gpos_t y) {
		uint32_t *addr = (uint32_t*)(_pixels + ((_offy + y) * _width + (_offx + x)) * 4);
		*addr = _col;
	}

	void Graphics32::fillRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		validateParams(x,y,width,height);
		gpos_t yend = y + height;
		updateMinMax(x,y);
		updateMinMax(x + width - 1,yend - 1);
		gpos_t xcur;
		gpos_t xend = x + width;
		// optimized version for 16bit
		// This is necessary if we want to have reasonable speed because the simple version
		// performs too many function-calls (one to a virtual-function and one to memcpy
		// that the compiler doesn't inline). Additionally the offset into the
		// memory-region will be calculated many times.
		// This version is much quicker :)
		gsize_t widthadd = _width;
		uint32_t *addr;
		uint32_t *orgaddr = (uint32_t*)(_pixels + (((_offy + y) * _width + (_offx + x)) * 4));
		for(; y < yend; y++) {
			addr = orgaddr;
			for(xcur = x; xcur < xend; xcur++)
				*addr++ = _col;
			orgaddr += widthadd;
		}
	}
}
