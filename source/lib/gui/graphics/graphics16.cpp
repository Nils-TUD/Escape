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
#include <gui/graphics/graphics16.h>

namespace gui {
	void Graphics16::doSetPixel(gpos_t x,gpos_t y) {
		gsize_t bwidth = _buf->getSize().width;
		uint16_t *addr = (uint16_t*)(_buf->getBuffer() + ((_off.y + y) * bwidth + (_off.x + x)) * 2);
		*addr = _col;
	}

	void Graphics16::fillRect(const Pos &pos,const Size &size) {
		Pos rpos = pos;
		Size rsize = size;
		if(!validateParams(rpos,rsize))
			return;

		gpos_t yend = rpos.y + rsize.height;
		updateMinMax(rpos);
		updateMinMax(Pos(rpos.x + rsize.width - 1,yend - 1));
		gpos_t xcur;
		gpos_t xend = rpos.x + rsize.width;
		gsize_t bwidth = _buf->getSize().width;
		uint8_t *pixels = _buf->getBuffer();
		// optimized version for 16bit
		// This is necessary if we want to have reasonable speed because the simple version
		// performs too many function-calls (one to a virtual-function and one to memcpy
		// that the compiler doesn't inline). Additionally the offset into the
		// memory-region will be calculated many times.
		// This version is much quicker :)
		gsize_t widthadd = bwidth;
		uint16_t *addr;
		uint16_t *orgaddr = (uint16_t*)(pixels + (((_off.y + rpos.y) * bwidth + (_off.x + rpos.x)) * 2));
		for(; rpos.y < yend; rpos.y++) {
			addr = orgaddr;
			for(xcur = rpos.x; xcur < xend; xcur++)
				*addr++ = _col;
			orgaddr += widthadd;
		}
	}
}
