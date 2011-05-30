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

#ifndef GRAPHICFACTORY_H_
#define GRAPHICFACTORY_H_

#include <esc/common.h>
#include <gui/common.h>

namespace gui {
	class GraphicFactory {
	public:
		static Graphics *get(Graphics &g,tCoord x,tCoord y);
		static Graphics *get(tCoord x,tCoord y,tSize width,tSize height,tColDepth bpp);

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
