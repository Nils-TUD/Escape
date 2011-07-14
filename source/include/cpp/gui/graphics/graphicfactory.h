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
#include <gui/graphics/graphics.h>
#include <gui/graphics/graphicsbuffer.h>

namespace gui {
	/**
	 * A factor for the different graphics-implementations, depending on the color-depth
	 */
	class GraphicFactory {
	public:
		/**
		 * Returns a graphics-instance that uses buf as buffer.
		 *
		 * @param buf the graphics-buffer to use
		 * @param width the width of the control
		 * @param height the height of the control
		 * @return the graphics-object
		 */
		static Graphics *get(GraphicsBuffer *buf,gsize_t width,gsize_t height);

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
