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

#ifndef GRAPHICFACTORY_H_
#define GRAPHICFACTORY_H_

#include <esc/common.h>

namespace gui {
	/**
	 * A factor for the different graphics-implementations, depending on the color-depth
	 */
	class GraphicFactory {
	public:
		/**
		 * Returns a graphics-instance that uses g as nested graphics-object, i.e. this method
		 * is intended for controls that use the graphics-object from the window.
		 *
		 * @param g the graphics-object of the window
		 * @param x the x-position of the control
		 * @param y the y-position of the control
		 * @return the graphics-object
		 */
		static Graphics *get(Graphics &g,gpos_t x,gpos_t y);

		/**
		 * Returns a graphics-instance for a window
		 *
		 * @param x the x-position of the window
		 * @param y the y-position of the window
		 * @param width the width of the window
		 * @param height the height of the window
		 * @param bpp the color-depth
		 * @return the graphics-object
		 */
		static Graphics *get(gpos_t x,gpos_t y,gsize_t width,gsize_t height,gcoldepth_t bpp);

	private:
		// no instantation
		GraphicFactory();
		~GraphicFactory();
		// no cloning
		GraphicFactory(const GraphicFactory &g);
		GraphicFactory &operator=(const GraphicFactory &g);
	};
}

#endif /* GRAPHICFACTORY_H_ */
