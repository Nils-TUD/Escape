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

#ifndef BORDERLAYOUT_H_
#define BORDERLAYOUT_H_

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
		/**
		 * The north-position: receive full width and preferred height
		 */
		static const pos_type NORTH		= 0;
		/**
		 * The east-position: receive remaining height and preferred width
		 */
		static const pos_type EAST		= 1;
		/**
		 * The south-position: receive full width and preferred height
		 */
		static const pos_type SOUTH		= 2;
		/**
		 * The west-position: receive remaining height and preferred width
		 */
		static const pos_type WEST		= 3;
		/**
		 * The center-position: receive remaining width and height
		 */
		static const pos_type CENTER	= 4;

	private:
		static const gsize_t DEF_GAP	= 2;

	public:
		/**
		 * Constructor
		 *
		 * @param gap the gap between the controls (default 2)
		 */
		BorderLayout(gsize_t gap = DEF_GAP) : Layout(), _gap(gap), _p(NULL), _ctrls() {
		};

		virtual void add(Panel *p,Control *c,pos_type pos);
		virtual void remove(Panel *p,Control *c,pos_type pos);

		virtual gsize_t getMinWidth() const;
		virtual gsize_t getMinHeight() const;

		virtual gsize_t getPreferredWidth() const;
		virtual gsize_t getPreferredHeight() const;

		virtual void rearrange();

	private:
		gsize_t _gap;
		Panel *_p;
		Control *_ctrls[5];
	};
}

#endif /* BORDERLAYOUT_H_ */
