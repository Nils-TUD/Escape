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

#ifndef GRAPHICS24_H_
#define GRAPHICS24_H_

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/gui/graphics.h>
#include <esc/gui/color.h>
#include <esc/gui/font.h>

namespace esc {
	namespace gui {
		/**
		 * The implementation of graphics for 24bit
		 */
		class Graphics24 : public Graphics {
		public:
			Graphics24(tCoord x,tCoord y,tSize width,tSize height,tColDepth bpp);
			Graphics24(Graphics &g,tCoord x,tCoord y);
			virtual ~Graphics24();

			void fillRect(tCoord x,tCoord y,tSize width,tSize height);

		protected:
			void doSetPixel(tCoord x,tCoord y);
		};
	}
}

#endif /* GRAPHICS24_H_ */
