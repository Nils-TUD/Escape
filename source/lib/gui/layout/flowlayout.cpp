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

using namespace std;

namespace gui {
	void FlowLayout::add(Panel *p,shared_ptr<Control> c,A_UNUSED pos_type pos) {
		assert(_p == nullptr || p == _p);
		_p = p;
		_ctrls.push_back(c);
	}
	void FlowLayout::remove(Panel *p,shared_ptr<Control> c,A_UNUSED pos_type pos) {
		assert(_p == p && _ctrls.erase_first(c));
	}
	void FlowLayout::removeAll() {
		_ctrls.clear();
	}

	Size FlowLayout::getSizeWith(const Size&,size_func) const {
		if(_ctrls.size() == 0)
			return Size();
		Size size = _sameSize ? getMaxSize() : getTotalSize();
		if(_orientation == HORIZONTAL)
			return Size(size.width * _ctrls.size() + _gap * (_ctrls.size() - 1),size.height);
		return Size(size.width,size.height * _ctrls.size() + _gap * (_ctrls.size() - 1));
	}

	bool FlowLayout::rearrange() {
		if(!_p || _ctrls.size() == 0)
			return false;

		bool res = false;
		gsize_t pad = _p->getTheme().getPadding();
		Size size = _p->getSize() - Size(pad * 2,pad * 2);
		Size all = _sameSize ? getMaxSize() : getTotalSize();
		Pos pos(pad,pad);

		if(_orientation == VERTICAL) {
			swap(pos.x,pos.y);
			swap(size.width,size.height);
			swap(all.width,all.height);
		}

		gsize_t totalWidth = all.width * _ctrls.size() + _gap * (_ctrls.size() - 1);
		pos.y = size.height / 2 - all.height / 2;
		switch(_align) {
			case FRONT:
				pos.x = pad;
				break;
			case CENTER:
				pos.x = (size.width / 2) - (totalWidth / 2);
				break;
			case BACK:
				pos.x = pad + size.width - totalWidth;
				break;
		}

		if(_orientation == VERTICAL) {
			swap(pos.x,pos.y);
			swap(all.width,all.height);
		}

		for(auto it = _ctrls.begin(); it != _ctrls.end(); ++it) {
			Size csize = _sameSize ? all : (*it)->getPreferredSize();
			res |= configureControl(*it,pos,csize);
			if(_orientation == VERTICAL)
				pos.y += csize.height + _gap;
			else
				pos.x += csize.width + _gap;
		}
		return res;
	}

	Size FlowLayout::getTotalSize() const {
		Size total;
		for(auto it = _ctrls.begin(); it != _ctrls.end(); ++it) {
			Size size = (*it)->getPreferredSize();
			if(_orientation == VERTICAL) {
				total.height += size.height;
				total.width = max(total.width,size.width);
			}
			else {
				total.width += size.width;
				total.height = max(total.height,size.height);
			}
		}
		if(_orientation == VERTICAL)
			total.height /= _ctrls.size();
		else
			total.width /= _ctrls.size();
		return total;
	}

	Size FlowLayout::getMaxSize() const {
		Size max;
		for(auto it = _ctrls.begin(); it != _ctrls.end(); ++it)
			max = maxsize(max,(*it)->getPreferredSize());
		return max;
	}
}
