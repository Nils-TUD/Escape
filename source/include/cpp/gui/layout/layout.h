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

#ifndef LAYOUT_H_
#define LAYOUT_H_

#include <esc/common.h>

namespace gui {
	class Control;
	class Panel;

	/**
	 * The interface for all layouts. Layouts are used to auto-position and -size the controls on
	 * a panel.
	 */
	class Layout {
	public:
		typedef int pos_type;

	public:
		/**
		 * Constructor
		 */
		Layout() {
		};
		/**
		 * Destructor
		 */
		virtual ~Layout() {
		};

		/**
		 * Adds the given control to the layout
		 *
		 * @param p the panel of the control
		 * @param c the control
		 * @param pos specifies the position (layout-implementation-dependend)
		 */
		virtual void add(Panel *p,Control *c,pos_type pos) = 0;
		/**
		 * Removes the given control/position from the layout
		 *
		 * @param p the panel of the control
		 * @param c the control
		 * @param pos specifies the position (layout-implementation-dependend)
		 */
		virtual void remove(Panel *p,Control *c,pos_type pos) = 0;

		/**
		 * @return the total preferred width of this layout
		 */
		virtual gsize_t getMinWidth() const = 0;
		/**
		 * @return the total preferred height of this layout
		 */
		virtual gsize_t getMinHeight() const = 0;

		/**
		 * Rearranges the controls, i.e. determines the position and sizes again. Does not
		 * perform a repaint!
		 */
		virtual void rearrange() = 0;

	private:
		// no cloning
		Layout(const Layout& bl);
		Layout& operator=(const Layout& bl);
	};
}

#endif /* LAYOUT_H_ */
