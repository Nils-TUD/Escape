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

#ifndef GRAPHICS32_H_
#define GRAPHICS32_H_

#include <esc/common.h>
#include <gui/graphics.h>
#include <gui/color.h>
#include <gui/font.h>

namespace gui {
	/**
	 * The implementation of graphics for 32bit
	 */
	class Graphics32 : public Graphics {
	public:
		Graphics32(gpos_t x,gpos_t y,gsize_t width,gsize_t height,gcoldepth_t bpp);
		Graphics32(Graphics &g,gpos_t x,gpos_t y);
		virtual ~Graphics32();

		void fillRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height);

	protected:
		void doSetPixel(gpos_t x,gpos_t y);
	};
}

#endif /* GRAPHICS32_H_ */
