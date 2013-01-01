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
#include <gui/layout/iconlayout.h>
#include <gui/panel.h>
#include <assert.h>
#include <math.h>

namespace gui {
	void IconLayout::add(Panel *p,Control *c,A_UNUSED pos_type pos) {
		assert(_p == NULL || p == _p);
		_p = p;
		_ctrls.push_back(c);
	}
	void IconLayout::remove(Panel *p,Control *c,A_UNUSED pos_type pos) {
		assert(_p == p && _ctrls.erase_first(c));
	}

	gsize_t IconLayout::getPreferredWidth() const {
		if(_ctrls.size() == 0)
			return 0;
		gsize_t width,height;
		size_t cols = (size_t)sqrt(_ctrls.size());
		doLayout(cols,cols,width,height);
		return width;
	}
	gsize_t IconLayout::getPreferredHeight() const {
		if(_ctrls.size() == 0)
			return 0;
		gsize_t width,height;
		size_t cols = (size_t)sqrt(_ctrls.size());
		doLayout(cols,cols,width,height);
		return height;
	}

	void IconLayout::rearrange() {
		if(!_p || _ctrls.size() == 0)
			return;

		gsize_t width = _p->getWidth();
		gsize_t height = _p->getHeight();
		doLayout(0,0,width,height,layoutCtrl);
	}

	void IconLayout::layoutCtrl(Control *c,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		c->moveTo(x,y);
		c->resizeTo(width,height);
	}

	void IconLayout::doLayout(size_t cols,size_t rows,gsize_t &width,gsize_t &height,
	                          layout_func layout) const {
		gsize_t wmax = 0, hmax = 0;
		gsize_t pad = _p->getTheme().getPadding();
		if(cols && rows)
			width = height = 0;
		switch(_pref) {
			case HORIZONTAL: {
				uint col = 0, row = 0;
				gpos_t x = pad, y = pad;
				for(vector<Control*>::const_iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
					std::pair<gsize_t,gsize_t> size((*it)->getPreferredWidth(),(*it)->getPreferredHeight());
					if((cols && col == cols) || (!cols && col > 0 && x + size.first + pad > width)) {
						if(cols)
							width = max<gsize_t>(width,x - _gap - pad);
						y += hmax + _gap;
						x = pad;
						hmax = 0;
						col = 0;
						row++;
					}

					if(layout)
						layout(*it,x,y,size.first,size.second);
					x += size.first + _gap;
					hmax = std::max(size.second,hmax);
					col++;
				}
				if(cols)
					height = y + hmax - pad;
				break;
			}
			case VERTICAL: {
				uint col = 0, row = 0;
				gpos_t x = pad, y = pad;
				for(vector<Control*>::const_iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
					std::pair<gsize_t,gsize_t> size((*it)->getPreferredWidth(),(*it)->getPreferredHeight());
					if((rows && row == rows) || (!rows && row > 0 && y + size.second + pad > height)) {
						if(rows)
							height = max<gsize_t>(height,y - _gap - pad);
						x += wmax + _gap;
						y = pad;
						wmax = 0;
						col++;
						row = 0;
					}

					if(layout)
						layout(*it,x,y,size.first,size.second);
					y += size.second + _gap;
					wmax = std::max(size.first,wmax);
					row++;
				}
				if(rows)
					width = x + wmax - pad;
				break;
			}
		}
	}
}
