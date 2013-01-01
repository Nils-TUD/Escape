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

	gsize_t GridLayout::getPreferredWidth() const {
		if(_ctrls.size() == 0)
			return 0;
		return getMaxWidth() * _cols + _gap * (_cols - 1);
	}
	gsize_t GridLayout::getPreferredHeight() const {
		if(_ctrls.size() == 0)
			return 0;
		return getMaxHeight() * _rows + _gap * (_rows - 1);
	}

	void GridLayout::rearrange() {
		if(!_p || _ctrls.size() == 0)
			return;

		gsize_t pad = _p->getTheme().getPadding();
		// work with double here to distribute the inaccuracy over all gaps. otherwise it would
		// be at the end, which looks quite ugly.
		double cwidth = (_p->getWidth() - pad * 2 - (_gap * (_cols - 1))) / (double)_cols;
		double cheight = (_p->getHeight() - pad * 2 - (_gap * (_rows - 1))) / (double)_rows;

		for(map<int,Control*>::iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
			GridPos pos(it->first);
			gpos_t x = (gpos_t)(pad + pos.col() * (cwidth + _gap));
			gpos_t y = (gpos_t)(pad + pos.row() * (cheight + _gap));
			configureControl(it->second,x,y,(gsize_t)cwidth,(gsize_t)cheight);
		}
	}

	gsize_t GridLayout::getMaxWidth() const {
		gsize_t max = 0;
		for(map<int,Control*>::const_iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
			gsize_t width = it->second->getPreferredWidth();
			if(width > max)
				max = width;
		}
		return max;
	}
	gsize_t GridLayout::getMaxHeight() const {
		gsize_t max = 0;
		for(map<int,Control*>::const_iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
			gsize_t height = it->second->getPreferredHeight();
			if(height > max)
				max = height;
		}
		return max;
	}
}
