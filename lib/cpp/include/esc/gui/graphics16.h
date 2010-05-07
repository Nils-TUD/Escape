/**
 * $Id: graphics16.h 578 2010-03-29 15:54:22Z nasmussen $
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

#ifndef GRAPHICS16_H_
#define GRAPHICS16_H_

#include <esc/common.h>
#include <esc/gui/common.h>
#include <esc/gui/graphics.h>
#include <esc/gui/color.h>
#include <esc/gui/font.h>

namespace esc {
	namespace gui {
		/**
		 * The implementation of graphics for 16bit
		 */
		class Graphics16 : public Graphics {
		public:
			Graphics16(tCoord x,tCoord y,tSize width,tSize height,tColDepth bpp);
			Graphics16(Graphics &g,tCoord x,tCoord y);
			virtual ~Graphics16();

			void fillRect(tCoord x,tCoord y,tSize width,tSize height);

		protected:
			void doSetPixel(tCoord x,tCoord y);
		};
	}
}

#endif /* GRAPHICS16_H_ */
