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
#include <utility>
#include <map>

namespace gui {
	/**
	 * Should be used to specify positions in a gridlayout.
	 */
	class GridPos {
		static const unsigned MAX_COLS	= 1024;

	public:
		explicit GridPos(unsigned pos) : _col(pos % MAX_COLS), _row(pos / MAX_COLS) {
		}
		explicit GridPos(unsigned c,unsigned r) : _col(c), _row(r) {
		}

		operator int() const {
			return _row * MAX_COLS + _col;
		}
		unsigned col() const {
			return _col;
		}
		unsigned row() const {
			return _row;
		}

	private:
		unsigned _col;
		unsigned _row;
	};

	/**
	 * The gridlayout builds a table with specified number of columns and rows.
	 */
	class GridLayout : public Layout {
	public:
		static const gsize_t DEF_GAP	= 2;

		/**
		 * Constructor
		 *
		 * @param cols the number of columns
		 * @param rows the number of rows
		 */
		GridLayout(unsigned cols,unsigned rows,gsize_t gap = DEF_GAP)
			: Layout(), _cols(cols), _rows(rows), _gap(gap), _p(), _ctrls() {
		}

		virtual void add(Panel *p,Control *c,pos_type pos);
		virtual void remove(Panel *p,Control *c,pos_type pos);
		virtual void removeAll();

		virtual void rearrange();

	private:
		Size getMaxSize() const;
		virtual Size getSizeWith(const Size &avail,size_func func) const;

	private:
		unsigned _cols;
		unsigned _rows;
		gsize_t _gap;
		Control *_p;
		std::map<int,Control*> _ctrls;
	};
}
