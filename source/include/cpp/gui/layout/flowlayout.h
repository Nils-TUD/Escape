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
	 * The flowlayout puts all controls side by side with their minimum widths and heights.
	 * The alignment of all controls can be left, center and right.
	 */
	class FlowLayout : public Layout {
	public:
		enum Align {
			LEFT,
			CENTER,
			RIGHT,
		};

		static const gsize_t DEF_GAP	= 2;

		/**
		 * Constructor
		 *
		 * @param pos the alignment of the controls
		 * @param gap the gap between the controls (default 2)
		 */
		FlowLayout(Align pos,gsize_t gap = DEF_GAP)
			: Layout(), _pos(pos), _gap(gap), _p(), _ctrls() {
		};

		virtual void add(Panel *p,Control *c,pos_type pos);
		virtual void remove(Panel *p,Control *c,pos_type pos);

		virtual gsize_t getMinWidth() const;
		virtual gsize_t getMinHeight() const;

		virtual gsize_t getPreferredWidth() const;
		virtual gsize_t getPreferredHeight() const;

		virtual void rearrange();

	private:
		gsize_t getMaxWidth() const;
		gsize_t getMaxHeight() const;

	private:
		Align _pos;
		gsize_t _gap;
		Panel *_p;
		std::vector<Control*> _ctrls;
	};
}
