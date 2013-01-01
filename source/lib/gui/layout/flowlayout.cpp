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

	gsize_t FlowLayout::getPreferredWidth() const {
		if(_ctrls.size() == 0)
			return 0;
		return getMaxWidth() * _ctrls.size() + _gap * (_ctrls.size() - 1);
	}
	gsize_t FlowLayout::getPreferredHeight() const {
		if(_ctrls.size() == 0)
			return 0;
		return getMaxHeight();
	}

	void FlowLayout::rearrange() {
		if(!_p || _ctrls.size() == 0)
			return;

		gsize_t pad = _p->getTheme().getPadding();
		gsize_t width = _p->getWidth() - pad * 2;
		gsize_t height = _p->getHeight() - pad * 2;
		gpos_t x = pad;
		gpos_t y = pad;
		gsize_t cwidth = getMaxWidth();
		gsize_t cheight = getMaxHeight();
		gsize_t totalWidth = cwidth * _ctrls.size() + _gap * (_ctrls.size() - 1);

		y = height / 2 - cheight / 2;
		switch(_pos) {
			case LEFT:
				x = pad;
				break;
			case CENTER:
				x = (width / 2) - (totalWidth / 2);
				break;
			case RIGHT:
				x = pad + width - totalWidth;
				break;
		}

		for(vector<Control*>::const_iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
			configureControl(*it,x,y,cwidth,cheight);
			x += cwidth + _gap;
		}
	}

	gsize_t FlowLayout::getMaxWidth() const {
		gsize_t max = 0;
		for(vector<Control*>::const_iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
			gsize_t width = (*it)->getPreferredWidth();
			if(width > max)
				max = width;
		}
		return max;
	}
	gsize_t FlowLayout::getMaxHeight() const {
		gsize_t max = 0;
		for(vector<Control*>::const_iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
			gsize_t height = (*it)->getPreferredHeight();
			if(height > max)
				max = height;
		}
		return max;
	}
}
