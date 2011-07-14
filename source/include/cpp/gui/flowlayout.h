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

#ifndef FLOWLAYOUT_H_
#define FLOWLAYOUT_H_

#include <esc/common.h>
#include <gui/layout.h>
#include <gui/control.h>

namespace gui {
	/**
	 * The borderlayout aligns all controls either on the left, centered or on the right. The
	 * width of all controls will be equal to the maximum preferred width of all controls and
	 * the height analogously.
	 */
	class FlowLayout : public Layout {
	public:
		/**
		 * The left-position: align all controls on the left
		 */
		static const int LEFT			= 0;
		/**
		 * The center-position: align all controls horizontally centered
		 */
		static const int CENTER			= 1;
		/**
		 * The right-position: align all controls on the right
		 */
		static const int RIGHT			= 2;

	private:
		static const gsize_t DEF_GAP	= 2;

	public:
		/**
		 * Constructor
		 *
		 * @param pos the alignment of the controls (LEFT, CENTER, RIGHT)
		 * @param gap the gap between the controls (default 2)
		 */
		FlowLayout(int pos,gsize_t gap = DEF_GAP)
			: _pos(pos), _gap(gap), _p(NULL), _ctrls(vector<Control*>()) {
		};
		/**
		 * Destructor
		 */
		virtual ~FlowLayout() {
		};

		virtual void add(Panel *p,Control *c,pos_type pos);
		virtual void remove(Panel *p,Control *c,pos_type pos);

		virtual gsize_t getMinWidth() const;
		virtual gsize_t getMinHeight() const;

		virtual void rearrange();

	private:
		gsize_t getMaxWidth() const;
		gsize_t getMaxHeight() const;

	private:
		int _pos;
		gsize_t _gap;
		Panel *_p;
		vector<Control*> _ctrls;
	};
}

#endif /* FLOWLAYOUT_H_ */
