/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/gui/graphics.h>
#include <esc/gui/graphics24.h>

namespace esc {
	namespace gui {
		Graphics24::Graphics24(Graphics &g,tCoord x,tCoord y)
			: Graphics(g,x,y) {

		}

		Graphics24::Graphics24(tCoord x,tCoord y,tSize width,tSize height,tColDepth bpp)
			: Graphics(x,y,width,height,bpp) {

		}

		Graphics24::~Graphics24() {
			// nothing to do
		}

		void Graphics24::doSetPixel(tCoord x,tCoord y) {
			u8 *col = (u8*)&_col;
			u8 *addr = _pixels + ((_offy + y) * _width + (_offx + x)) * 3;
			*addr++ = *col++;
			*addr++ = *col++;
			*addr = *col;
		}

		void Graphics24::fillRect(tCoord x,tCoord y,tSize width,tSize height) {
			validateParams(x,y,width,height);
			tCoord yend = y + height;
			updateMinMax(x,y);
			updateMinMax(x + width - 1,yend - 1);
			tCoord xcur;
			tCoord xend = x + width;
			// optimized version for 24bit
			// This is necessary if we want to have reasonable speed because the simple version
			// performs too many function-calls (one to a virtual-function and one to memcpy
			// that the compiler doesn't inline). Additionally the offset into the
			// memory-region will be calculated many times.
			// This version is much quicker :)
			u8 *col = (u8*)&_col;
			u32 widthadd = _width * 3;
			u8 *addr;
			u8 *orgaddr = _pixels + (((_offy + y) * _width + (_offx + x)) * 3);
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
}
