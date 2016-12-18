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

#include <gui/layout/gridlayout.h>

namespace gui {
	/**
	 * The table layout builds a table with specified number of columns and rows and gives the
	 * columns a specified or the preferred width.
	 */
	class TableLayout : public GridLayout {
	public:
		enum {
			MAX_HORIZONTAL	= 1,
			MAX_VERTICAL	= 2,
		};

		/**
		 * Constructor
		 *
		 * @param cols the number of columns
		 * @param rows the number of rows
		 * @param max the dimensions to maximize
		 * @param gap the gap between controls
		 */
		TableLayout(unsigned cols,unsigned rows,unsigned max = MAX_HORIZONTAL,gsize_t gap = DEF_GAP)
			: GridLayout(cols,rows,gap), _max(max) {
		}

		virtual bool rearrange();

	private:
		Size getPrefSize() const;
		gsize_t getRowHeight(uint row) const;
		gsize_t getColumnWidth(uint col) const;

		virtual Size getSizeWith(const Size &avail,size_func func) const;

		unsigned _max;
	};
}
