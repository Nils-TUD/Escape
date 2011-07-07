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
#include <gui/graphics.h>
#include <gui/graphics16.h>
#include <gui/graphics24.h>
#include <gui/graphics32.h>
#include <gui/graphicfactory.h>

namespace gui {
	Graphics *GraphicFactory::get(Graphics &g,gpos_t x,gpos_t y) {
		switch(g.getColorDepth()) {
			case 32:
				return new Graphics32(g,x,y);
			case 24:
				return new Graphics24(g,x,y);
			case 16:
				return new Graphics16(g,x,y);
			default:
				return NULL;
		}
	}

	Graphics *GraphicFactory::get(gpos_t x,gpos_t y,gsize_t width,gsize_t height,gcoldepth_t bpp) {
		switch(bpp) {
			case 32:
				return new Graphics32(x,y,width,height,bpp);
			case 24:
				return new Graphics24(x,y,width,height,bpp);
			case 16:
				return new Graphics16(x,y,width,height,bpp);
			default:
				return NULL;
		}
	}
}
