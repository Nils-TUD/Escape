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
#include <esc/debug.h>
#include <esc/gui/graphics.h>
#include <esc/gui/graphics32.h>

namespace esc {
	namespace gui {
		Graphics32::Graphics32(Graphics &g,tCoord x,tCoord y)
			: Graphics(g,x,y) {

		}

		Graphics32::Graphics32(tCoord x,tCoord y,tSize width,tSize height,tColDepth bpp)
			: Graphics(x,y,width,height,bpp) {

		}

		Graphics32::~Graphics32() {
			// nothing to do
		}

		void Graphics32::doSetPixel(tCoord x,tCoord y) {
			u32 *addr = (u32*)(_pixels + ((_offy + y) * _width + (_offx + x)) * 4);
			*addr = _col;
		}

		void Graphics32::fillRect(tCoord x,tCoord y,tSize width,tSize height) {
			validateParams(x,y,width,height);
			tCoord yend = y + height;
			updateMinMax(x,y);
			updateMinMax(x + width - 1,yend - 1);
			tCoord xcur;
			tCoord xend = x + width;
			// optimized version for 16bit
			// This is necessary if we want to have reasonable speed because the simple version
			// performs too many function-calls (one to a virtual-function and one to memcpy
			// that the compiler doesn't inline). Additionally the offset into the
			// memory-region will be calculated many times.
			// This version is much quicker :)
			u32 widthadd = _width;
			u32 *addr;
			u32 *orgaddr = (u32*)(_pixels + (((_offy + y) * _width + (_offx + x)) * 4));
			for(; y < yend; y++) {
				addr = orgaddr;
				for(xcur = x; xcur < xend; xcur++)
					*addr++ = _col;
				orgaddr += widthadd;
			}
		}
	}
}
