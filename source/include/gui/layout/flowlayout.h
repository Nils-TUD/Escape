/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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

#include <sys/common.h>
#include <gui/layout/layout.h>
#include <gui/enums.h>
#include <gui/control.h>

namespace gui {
	/**
	 * The flowlayout puts all controls side by side with either the max of all widths/heights or
	 * the preferred width/height of each.
	 * The alignment of all controls can be left, center and right.
	 */
	class FlowLayout : public Layout {
	public:
		static const gsize_t DEF_GAP	= 2;

		/**
		 * Constructor
		 *
		 * @param align the alignment of the controls
		 * @param sameSize whether all controls should have the same width/height
		 * @param orientation the orientation
		 * @param gap the gap between the controls (default 2)
		 */
		FlowLayout(Align align,bool sameSize = true,Orientation orientation = HORIZONTAL,gsize_t gap = DEF_GAP)
			: Layout(), _sameSize(sameSize), _align(align), _orientation(orientation), _gap(gap),
			  _p(), _ctrls() {
		}

		virtual void add(Panel *p,std::shared_ptr<Control> c,pos_type pos);
		virtual void remove(Panel *p,std::shared_ptr<Control> c,pos_type pos);
		virtual void removeAll();

		virtual bool rearrange();

	protected:
		virtual Size getSizeWith(const Size &avail,size_func func) const;

	private:
		Size getTotalSize() const;
		Size getMaxSize() const;

		bool _sameSize;
		Align _align;
		Orientation _orientation;
		gsize_t _gap;
		Panel *_p;
		std::vector<std::shared_ptr<Control>> _ctrls;
	};
}
