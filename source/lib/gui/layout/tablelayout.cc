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

#include <gui/layout/tablelayout.h>
#include <gui/panel.h>
#include <sys/common.h>
#include <assert.h>

using namespace std;

namespace gui {
	Size TableLayout::getSizeWith(const Size &avail,size_func) const {
		if(_ctrls.size() == 0)
			return Size();

		Size pref = getPrefSize();
		if(_max & MAX_HORIZONTAL)
			pref.width = max(avail.width,pref.width);
		if(_max & MAX_VERTICAL)
			pref.height = max(avail.height,pref.height);
		return pref;
	}

	bool TableLayout::rearrange() {
		if(!_p || _ctrls.size() == 0)
			return false;

		bool res = false;
		gsize_t pad = _p->getTheme().getPadding();
		Size size = _p->getSize();
		gpos_t xend = size.width - pad;
		gpos_t yend = size.height - pad;

		size.width -= pad * 2;
		size.height -= pad * 2;
		size.width -= _gap * (_cols - 1);
		size.height -= _gap * (_rows - 1);

		// determine the relative sizes of all cells based on the preferred size
		std::vector<double> widths(_cols,0);
		std::vector<double> heights(_rows,0);

		Size pref = getPrefSize();
		pref.width -= _gap * (_cols - 1);
		pref.height -= _gap * (_rows - 1);

		for(uint col = 0; col < _cols; ++col) {
			gsize_t width = getColumnWidth(col);
			widths[col] = width / (double)pref.width;
		}
		for(uint row = 0; row < _rows; ++row) {
			gsize_t height = getRowHeight(row);
			heights[row] = height / (double)pref.height;
		}

		// now apply these sizes based on our real size
		gpos_t x = pad;
		for(uint col = 0; col < _cols; ++col) {
			gpos_t y = pad;
			// use round to spread the inaccuracy among all rows/columns
			// and let the last one simply use the remaining space
			gsize_t colWidth = col == _cols - 1 ? xend - x : round(widths[col] * size.width);
			for(uint row = 0; row < _rows; ++row) {
				gsize_t rowHeight = row == _rows - 1 ? yend - y : round(heights[row] * size.height);

				auto ctrl = _ctrls.at(GridPos(col,row));
				res |= configureControl(ctrl,Pos(x,y),Size(colWidth,rowHeight));

				y += rowHeight + _gap;
			}
			x += colWidth + _gap;
		}

		return res;
	}

	Size TableLayout::getPrefSize() const {
		Size size;

		for(uint col = 0; col < _cols; ++col)
			size.width += getColumnWidth(col);
		size.width += _gap * (_cols - 1);

		for(uint row = 0; row < _rows; ++row)
			size.height += getRowHeight(row);
		size.height += _gap * (_rows - 1);

		return size;
	}

	gsize_t TableLayout::getRowHeight(uint row) const {
		gsize_t height = 0;
		for(uint col = 0; col < _cols; ++col) {
			auto ctrl = _ctrls.at(GridPos(col,row));
			height = max(height,ctrl->getPreferredSize().height);
		}
		return height;
	}

	gsize_t TableLayout::getColumnWidth(uint col) const {
		gsize_t width = 0;
		for(uint row = 0; row < _rows; ++row) {
			auto ctrl = _ctrls.at(GridPos(col,row));
			width = max(width,ctrl->getPreferredSize().width);
		}
		return width;
	}
}
