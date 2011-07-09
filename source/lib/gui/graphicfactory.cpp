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
#include <gui/graphics.h>
#include <gui/graphics16.h>
#include <gui/graphics24.h>
#include <gui/graphics32.h>
#include <gui/graphicfactory.h>

namespace gui {
	Graphics *GraphicFactory::get(GraphicsBuffer *buf,gpos_t x,gpos_t y) {
		switch(buf->getColorDepth()) {
			case 32:
				return new Graphics32(buf,x,y);
			case 24:
				return new Graphics24(buf,x,y);
			case 16:
				return new Graphics16(buf,x,y);
			default:
				return NULL;
		}
	}
}
