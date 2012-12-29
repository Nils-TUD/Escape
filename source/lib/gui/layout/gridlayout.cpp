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

	gsize_t GridLayout::getMinWidth() const {
		if(_ctrls.size() == 0)
			return 0;
		return getMaxWidth() * _cols;
	}
	gsize_t GridLayout::getMinHeight() const {
		if(_ctrls.size() == 0)
			return 0;
		return getMaxHeight() * _rows;
	}

	gsize_t GridLayout::getPreferredWidth() const {
		gsize_t pad = _p->getTheme().getPadding();
		return max<gsize_t>(_p->getParent()->getContentWidth() - pad * 2,getMinWidth());
	}
	gsize_t GridLayout::getPreferredHeight() const {
		gsize_t pad = _p->getTheme().getPadding();
		return max<gsize_t>(_p->getParent()->getContentHeight() - pad * 2,getMinHeight());
	}

	void GridLayout::rearrange() {
		if(!_p || _ctrls.size() == 0)
			return;

		gsize_t pad = _p->getTheme().getPadding();
		gsize_t cwidth = getPreferredWidth() / _cols;
		gsize_t cheight = getPreferredHeight() / _rows;

		for(map<int,Control*>::iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
			GridPos pos(it->first);
			gpos_t x = pad + pos.col() * cwidth;
			gpos_t y = pad + pos.row() * cheight;
			it->second->moveTo(x,y);
			it->second->resizeTo(cwidth - pad * 2,cheight - pad * 2);
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
