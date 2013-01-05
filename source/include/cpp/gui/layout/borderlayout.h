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

#pragma once

#include <esc/common.h>
#include <gui/layout/layout.h>
#include <gui/control.h>

namespace gui {
	/**
	 * The borderlayout provides 5 positions for controls: north, east, south, west and center.
	 * Each position may be empty. The controls at north and south will receive the complete
	 * width offered by the panel for which the layout is used and their preferred height.
	 * The west and east will receive the remaining height and the their preferred width. The
	 * control at center receives the remaining space.
	 */
	class BorderLayout : public Layout {
	public:
		enum Position {
			/**
			 * receive full width and preferred height
			 */
			NORTH,
			/**
			 * receive remaining height and preferred width
			 */
			EAST,
			/**
			 * receive full width and preferred height
			 */
			SOUTH,
			/**
			 * receive remaining height and preferred width
			 */
			WEST,
			/**
			 * receive remaining width and height
			 */
			CENTER
		};

		static const gsize_t DEF_GAP	= 2;

		/**
		 * Constructor
		 *
		 * @param gap the gap between the controls (default 2)
		 */
		BorderLayout(gsize_t gap = DEF_GAP) : Layout(), _gap(gap), _p(nullptr), _ctrls() {
		}

		virtual void add(Panel *p,std::shared_ptr<Control> c,pos_type pos);
		virtual void remove(Panel *p,std::shared_ptr<Control> c,pos_type pos);
		virtual void removeAll();

		virtual void rearrange();

	private:
		virtual Size getSizeWith(const Size &avail,size_func func) const;

		gsize_t _gap;
		Panel *_p;
		std::shared_ptr<Control> _ctrls[5];
	};
}
