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

	Size IconLayout::getPreferredSize() const {
		if(_ctrls.size() == 0)
			return Size();
		Size size;
		size_t cols = (size_t)sqrt(_ctrls.size());
		doLayout(cols,cols,size);
		return size;
	}

	Size IconLayout::getUsedSize(const Size &avail) const {
		gsize_t pad = _p->getTheme().getPadding();
		Size size = avail;
		doLayout(0,0,size);
		return size + Size(pad * 2,pad * 2);
	}

	void IconLayout::rearrange() {
		if(!_p || _ctrls.size() == 0)
			return;

		Size size = _p->getSize();
		doLayout(0,0,size,&IconLayout::configureControl);
	}

	void IconLayout::doLayout(size_t cols,size_t rows,Size &size,layout_func layout) const {
		Size usize,max;
		gsize_t pad = _p->getTheme().getPadding();
		switch(_pref) {
			case HORIZONTAL: {
				uint col = 0, row = 0;
				gpos_t x = pad, y = pad;
				for(vector<Control*>::const_iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
					Size csize = (*it)->getPreferredSize();
					if((cols && col == cols) || (!cols && col > 0 && x + csize.width + pad > size.width)) {
						usize.width = std::max<gsize_t>(usize.width,x - _gap - pad);
						y += max.height + _gap;
						x = pad;
						max.height = 0;
						col = 0;
						row++;
					}

					if(layout)
						(this->*layout)(*it,x,y,csize);
					x += csize.width + _gap;
					max.height = std::max(csize.height,max.height);
					col++;
				}
				usize.height = y + max.height - pad;
				break;
			}
			case VERTICAL: {
				uint col = 0, row = 0;
				gpos_t x = pad, y = pad;
				for(vector<Control*>::const_iterator it = _ctrls.begin(); it != _ctrls.end(); ++it) {
					Size csize = (*it)->getPreferredSize();
					if((rows && row == rows) || (!rows && row > 0 && y + csize.height + pad > size.height)) {
						usize.height = std::max<gsize_t>(usize.height,y - _gap - pad);
						x += max.width + _gap;
						y = pad;
						max.width = 0;
						col++;
						row = 0;
					}

					if(layout)
						(this->*layout)(*it,x,y,csize);
					y += csize.height + _gap;
					max.width = std::max(csize.width,max.width);
					row++;
				}
				usize.width = x + max.width - pad;
				break;
			}
		}
		size = usize;
	}
}
