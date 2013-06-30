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
#include <gui/control.h>
#include <algorithm>
#include <utility>

namespace gui {
	class Panel;

	template<typename T,typename... Args>
	inline std::shared_ptr<T> make_layout(Args&&... args) {
		return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
	}

	/**
	 * The interface for all layouts. Layouts are used to auto-position and -size the controls on
	 * a panel.
	 */
	class Layout {
	public:
		typedef int pos_type;
		typedef Size (*size_func)(std::shared_ptr<Control>,const Size&);

		Layout() {
		}
		virtual ~Layout() {
		}

		/**
		 * Adds the given control to the layout
		 *
		 * @param p the panel of the control
		 * @param c the control
		 * @param pos specifies the position (layout-implementation-dependend)
		 */
		virtual void add(Panel *p,std::shared_ptr<Control> c,pos_type pos) = 0;
		/**
		 * Removes the given control/position from the layout
		 *
		 * @param p the panel of the control
		 * @param c the control
		 * @param pos specifies the position (layout-implementation-dependend)
		 */
		virtual void remove(Panel *p,std::shared_ptr<Control> c,pos_type pos) = 0;

		/**
		 * Removes all controls from the layout
		 */
		virtual void removeAll() = 0;

		/**
		 * @return the total preferred size of this layout without padding
		 */
		Size getPreferredSize() const {
			return getSizeWith(Size(),getPrefSizeOf);
		}

		/**
		 * Determines what size would be used if <width>X<height> is available.
		 *
		 * @param avail the available size
		 * @return the size that would be used
		 */
		Size getUsedSize(const Size &avail) const {
			return getSizeWith(avail,getUsedSizeOf);
		}

		/**
		 * Rearranges the controls, i.e. determines the position and sizes again. Does not
		 * perform a repaint!
		 *
		 * @return true if something changed
		 */
		virtual bool rearrange() = 0;

	protected:
		/**
		 * Helper for getPreferredSize() and getUsedSize(). Should use func(ctrl,avail) to determine
		 * the size of one control and calculate the total size in the appropriate way for the layout.
		 *
		 * @param avail the available size
		 * @param func the function to call
		 * @return the size
		 */
		virtual Size getSizeWith(const Size &avail,size_func func) const = 0;

		static inline Size getPrefSizeOf(std::shared_ptr<Control> c,const Size &) {
			return c->getPreferredSize();
		}
		static inline Size getUsedSizeOf(std::shared_ptr<Control> c,const Size &avail) {
			return c->getUsedSize(avail);
		}

		bool configureControl(std::shared_ptr<Control> c,const Pos &pos,const Size &size) const {
			bool res = c->getPos() != pos || c->getSize() != size;
			c->moveTo(pos);
			c->resizeTo(size);
			return res;
		}

	private:
		// no cloning
		Layout(const Layout& bl);
		Layout& operator=(const Layout& bl);
	};
}
