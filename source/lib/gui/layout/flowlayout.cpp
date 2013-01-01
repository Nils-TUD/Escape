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
#include <gui/layout/flowlayout.h>
#include <gui/panel.h>
#include <assert.h>

namespace gui {
	void FlowLayout::add(Panel *p,Control *c,A_UNUSED pos_type pos) {
		assert(_p == NULL || p == _p);
		_p = p;
		_ctrls.push_back(c);
	}
	void FlowLayout::remove(Panel *p,Control *c,A_UNUSED pos_type pos) {
		assert(_p == p && _ctrls.erase_first(c));
	}

	Size FlowLayout::getPreferredSize() const {
		if(_ctrls.size() == 0)
			return Size();
		Size max = getMaxSize();
		return Size(max.width * _ctrls.size() + _gap * (_ctrls.size() - 1),max.height);
	}

	void FlowLayout::rearrange() {
		if(!_p || _ctrls.size() == 0)
			return;

		gsize_t pad = _p->getTheme().getPadding();
		Size size = _p->getSize() - Size(pad * 2,pad * 2);
		Size max = getMaxSize();
		Pos pos(pad,pad);
		gsize_t totalWidth = max.width * _ctrls.size() + _gap * (_ctrls.size() - 1);

		pos.y = size.height / 2 - max.height / 2;
		switch(_pos) {
			case LEFT:
				pos.x = pad;
				break;
			case CENTER:
				pos.x = (size.width / 2) - (totalWidth / 2);
				break;
			case RIGHT:
				pos.x = pad + size.width - totalWidth;
				break;
		}

		for(vector<Control*>::const_iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
			configureControl(*it,pos,max);
			pos.x += max.width + _gap;
		}
	}

	Size FlowLayout::getMaxSize() const {
		Size max;
		for(vector<Control*>::const_iterator it = _ctrls.begin(); it != _ctrls.end(); ++it)
			max = maxsize(max,(*it)->getPreferredSize());
		return max;
	}
}
