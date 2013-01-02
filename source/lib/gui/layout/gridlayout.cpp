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

#include <esc/common.h>
#include <gui/layout/gridlayout.h>
#include <gui/panel.h>
#include <assert.h>

using namespace std;

namespace gui {
	void GridLayout::add(Panel *p,Control *c,pos_type pos) {
		assert(_p == NULL || p == _p);
		assert(GridPos(pos).col() < _cols && GridPos(pos).row() < _rows);
		_p = p;
		_ctrls[pos] = c;
	}
	void GridLayout::remove(Panel *p,Control *c,pos_type pos) {
		assert(_p == p && _ctrls[pos] == c);
		assert(GridPos(pos).col() < _cols && GridPos(pos).row() < _rows);
		_ctrls[pos] = NULL;
	}

	Size GridLayout::getSizeWith(const Size &avail,size_func) const {
		if(_ctrls.size() == 0)
			return Size();
		if(!avail.empty())
			return avail;
		Size max = getMaxSize();
		return Size(max.width * _cols + _gap * (_cols - 1),max.height * _rows + _gap * (_rows - 1));
	}

	void GridLayout::rearrange() {
		if(!_p || _ctrls.size() == 0)
			return;

		gsize_t pad = _p->getTheme().getPadding();
		Size size = _p->getSize();
		// work with double here to distribute the inaccuracy over all gaps. otherwise it would
		// be at the end, which looks quite ugly.
		double cwidth = (size.width - pad * 2 - (_gap * (_cols - 1))) / (double)_cols;
		double cheight = (size.height - pad * 2 - (_gap * (_rows - 1))) / (double)_rows;

		for(map<int,Control*>::iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
			GridPos pos(it->first);
			gpos_t x = (gpos_t)(pad + pos.col() * (cwidth + _gap));
			gpos_t y = (gpos_t)(pad + pos.row() * (cheight + _gap));
			configureControl(it->second,Pos(x,y),Size((gsize_t)cwidth,(gsize_t)cheight));
		}
	}

	Size GridLayout::getMaxSize() const {
		Size max;
		for(map<int,Control*>::const_iterator it = _ctrls.begin(); it != _ctrls.end(); ++it)
			max = maxsize(max,it->second->getPreferredSize());
		return max;
	}
}
