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

using namespace std;

namespace gui {
	void IconLayout::add(Panel *p,shared_ptr<Control> c,A_UNUSED pos_type pos) {
		assert(_p == nullptr || p == _p);
		_p = p;
		_ctrls.push_back(c);
	}
	void IconLayout::remove(Panel *p,shared_ptr<Control> c,A_UNUSED pos_type pos) {
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
		Size req =  Size(dim.width * max.width + (dim.width - 1) * _gap,
				dim.height * max.height + (dim.height - 1) * _gap);
		return maxsize(avail,req);
	}

	bool IconLayout::rearrange() {
		if(!_p || _ctrls.size() == 0)
			return false;

		bool res = false;
		Size max;
		gsize_t pad = _p->getTheme().getPadding();
		Size dim = getDim(_p->getSize() - Size(pad * 2,pad * 2),max);

		Pos pos(pad,pad);
		gsize_t col = 0, row = 0;
		switch(_pref) {
			case HORIZONTAL: {
				for(auto it = _ctrls.begin(); it != _ctrls.end(); ++it) {
					res |= doConfigureControl(*it,pos,max);
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
					res |= doConfigureControl(*it,pos,max);
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
		return res;
	}

	bool IconLayout::doConfigureControl(shared_ptr<Control> c,const Pos &pos,const Size &max) {
		Size pref = c->getPreferredSize();
		Pos cpos(pos.x + (max.width - pref.width) / 2,
		         pos.y + (max.height - pref.height) / 2);
		return configureControl(c,cpos,pref);
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
}
