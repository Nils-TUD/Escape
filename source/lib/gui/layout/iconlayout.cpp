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
	void IconLayout::add(Panel *p,std::shared_ptr<Control> c,A_UNUSED pos_type pos) {
		assert(_p == nullptr || p == _p);
		_p = p;
		_ctrls.push_back(c);
	}
	void IconLayout::remove(Panel *p,std::shared_ptr<Control> c,A_UNUSED pos_type pos) {
		assert(_p == p && _ctrls.erase_first(c));
	}
	void IconLayout::removeAll() {
		_ctrls.clear();
	}

	Size IconLayout::getSizeWith(const Size &avail,size_func) const {
		if(_ctrls.size() == 0)
			return Size();
		Size max;
		Size dim = getDim(avail,max);
		return Size(dim.width * max.width + (dim.width - 1) * _gap,
					dim.height * max.height + (dim.height - 1) * _gap);
	}

	void IconLayout::rearrange() {
		if(!_p || _ctrls.size() == 0)
			return;

		Size max;
		gsize_t pad = _p->getTheme().getPadding();
		Size dim = getDim(_p->getSize() - Size(pad * 2,pad * 2),max);

		Pos pos(pad,pad);
		gsize_t col = 0, row = 0;
		switch(_pref) {
			case HORIZONTAL: {
				for(auto it = _ctrls.begin(); it != _ctrls.end(); ++it) {
					doConfigureControl(*it,pos,max);
					pos.x += max.width + _gap;
					if(++col == dim.width) {
						pos.x = pad;
						pos.y += max.height + _gap;
						col = 0;
						row++;
					}
				}
				break;
			}

			case VERTICAL: {
				for(auto it = _ctrls.begin(); it != _ctrls.end(); ++it) {
					doConfigureControl(*it,pos,max);
					pos.y += max.height + _gap;
					if(++row == dim.height) {
						pos.y = pad;
						pos.x += max.width + _gap;
						row = 0;
						col++;
					}
				}
				break;
			}
		}
	}

	void IconLayout::doConfigureControl(shared_ptr<Control> c,const Pos &pos,const Size &max) {
		Size pref = c->getPreferredSize();
		Pos cpos(pos.x + (max.width - pref.width) / 2,
		         pos.y + (max.height - pref.height) / 2);
		configureControl(c,cpos,pref);
	}

	Size IconLayout::getDim(const Size &avail,Size &max) const {
		gsize_t cols,rows;
		max = getMaxSize();
		if(avail.empty()) {
			cols = (gsize_t)sqrt(_ctrls.size());
			rows = (_ctrls.size() + cols - 1) / cols;
		}
		else if(_pref == HORIZONTAL) {
			cols = avail.width / (max.width + _gap);
			if(max.width * (cols + 1) + (_gap * cols) <= avail.width)
				cols++;
			cols = std::max<gsize_t>(1,cols);
			rows = (_ctrls.size() + cols - 1) / cols;
		}
		else {
			rows = avail.height / (max.height + _gap);
			if(max.height * (rows + 1) + (_gap * rows) <= avail.height)
				rows++;
			rows = std::max<gsize_t>(1,rows);
			cols = (_ctrls.size() + rows - 1) / rows;
		}
		return Size(cols,rows);
	}

	Size IconLayout::getMaxSize() const {
		Size max;
		for(auto it = _ctrls.begin(); it != _ctrls.end(); ++it)
			max = maxsize((*it)->getPreferredSize(),max);
		return max;
	}

#if 0
	void IconLayout::doLayout(size_t cols,size_t rows,gsize_t pad,Size &size,layout_func layout) const {
		Size usize,max;
		switch(_pref) {
			case HORIZONTAL: {
				uint col = 0, row = 0;
				Pos pos(pad,pad);
				for(auto it = _ctrls.begin(); it != _ctrls.end(); ++it) {
					Size csize = (*it)->getPreferredSize();
					if((cols && col == cols) ||
							(!cols && col > 0 && pos.x + csize.width + pad > size.width)) {
						usize.width = std::max<gsize_t>(usize.width,pos.x - _gap - pad);
						pos.y += max.height + _gap;
						pos.x = pad;
						max.height = 0;
						col = 0;
						row++;
					}

					if(layout)
						(this->*layout)(*it,pos,csize);
					pos.x += csize.width + _gap;
					max.height = std::max(csize.height,max.height);
					col++;
				}
				usize.height = pos.y + max.height - pad;
				break;
			}
			case VERTICAL: {
				uint col = 0, row = 0;
				Pos pos(pad,pad);
				for(auto it = _ctrls.begin(); it != _ctrls.end(); ++it) {
					Size csize = (*it)->getPreferredSize();
					if((rows && row == rows) ||
							(!rows && row > 0 && pos.y + csize.height + pad > size.height)) {
						usize.height = std::max<gsize_t>(usize.height,pos.y - _gap - pad);
						pos.x += max.width + _gap;
						pos.y = pad;
						max.width = 0;
						col++;
						row = 0;
					}

					if(layout)
						(this->*layout)(*it,pos,csize);
					pos.y += csize.height + _gap;
					max.width = std::max(csize.width,max.width);
					row++;
				}
				usize.width = pos.x + max.width - pad;
				break;
			}
		}
		size = usize;
	}
#endif
}
