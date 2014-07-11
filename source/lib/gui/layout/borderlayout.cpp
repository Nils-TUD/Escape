/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
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
#include <gui/layout/borderlayout.h>
#include <gui/panel.h>
#include <assert.h>

using namespace std;

namespace gui {
	void BorderLayout::add(Panel *p,shared_ptr<Control> c,pos_type pos) {
		assert(pos < (pos_type)ARRAY_SIZE(_ctrls) && (_p == nullptr || p == _p));
		_p = p;
		_ctrls[pos] = c;
	}
	void BorderLayout::remove(A_UNUSED Panel *p,A_UNUSED shared_ptr<Control> c,pos_type pos) {
		assert(pos < (pos_type)ARRAY_SIZE(_ctrls) && _p == p && _ctrls[pos] == c);
		_ctrls[pos].reset();
	}
	void BorderLayout::removeAll() {
		_ctrls[WEST] = _ctrls[NORTH] = _ctrls[SOUTH] = shared_ptr<Control>();
		_ctrls[EAST] = _ctrls[CENTER] = shared_ptr<Control>();
	}

	Size BorderLayout::getSizeWith(const Size &avail,size_func func) const {
		Size size;
		if(_ctrls[WEST])
			size.width += _ctrls[WEST]->getPreferredSize().width;
		if(_ctrls[NORTH])
			size.height += _ctrls[NORTH]->getPreferredSize().height;
		if(_ctrls[EAST])
			size.width += _ctrls[EAST]->getPreferredSize().width;
		if(_ctrls[SOUTH])
			size.height += _ctrls[SOUTH]->getPreferredSize().height;

		if(_ctrls[CENTER]) {
			if(_ctrls[WEST])
				size.width += _gap;
			if(_ctrls[NORTH])
				size.height += _gap;
			if(_ctrls[EAST])
				size.width += _gap;
			if(_ctrls[SOUTH])
				size.height += _gap;
			size += func(_ctrls[CENTER],subsize(avail,size));
		}

		if(size.width == 0) {
			if(_ctrls[NORTH])
				size.width = _ctrls[NORTH]->getPreferredSize().width;
			if(_ctrls[SOUTH])
				size.width = max(size.width,_ctrls[SOUTH]->getPreferredSize().width);
		}
		if(size.height == 0) {
			if(_ctrls[WEST])
				size.height = _ctrls[WEST]->getPreferredSize().height;
			if(_ctrls[EAST])
				size.height = max(size.height,_ctrls[EAST]->getPreferredSize().height);
		}
		return size;
	}

	bool BorderLayout::rearrange() {
		if(!_p)
			return false;

		bool res = false;
		shared_ptr<Control> c;
		gsize_t pad = _p->getTheme().getPadding();
		Size orgsize = _p->getSize() - Size(pad * 2,pad * 2);
		Size rsize = getSizeWith(orgsize,getUsedSizeOf);
		Size size = maxsize(orgsize,rsize);
		Pos pos(pad,pad);

		Size ns = _ctrls[NORTH] ? _ctrls[NORTH]->getPreferredSize() + Size(_gap,_gap) : Size();
		Size ss = _ctrls[SOUTH] ? _ctrls[SOUTH]->getPreferredSize() + Size(_gap,_gap) : Size();
		Size ws = _ctrls[WEST] ? _ctrls[WEST]->getPreferredSize() + Size(_gap,_gap) : Size();
		Size es = _ctrls[EAST] ? _ctrls[EAST]->getPreferredSize() + Size(_gap,_gap) : Size();

		if((c = _ctrls[NORTH])) {
			res |= configureControl(c,pos,Size(size.width,ns.height - _gap));
			pos.y += ns.height;
			rsize.height -= ns.height;
		}
		if((c = _ctrls[SOUTH])) {
			res |= configureControl(c,Pos(pos.x,pad + size.height - (ss.height - _gap)),
							 	    Size(size.width,ss.height - _gap));
			rsize.height -= ss.height;
		}
		if((c = _ctrls[WEST])) {
			res |= configureControl(c,pos,Size(ws.width - _gap,rsize.height));
			pos.x += ws.width;
			rsize.width -= ws.width;
		}
		if((c = _ctrls[EAST])) {
			res |= configureControl(c,Pos(pad + size.width - (es.width - _gap),pos.y),
							 	 	Size(es.width - _gap,rsize.height));
			rsize.width -= es.width;
		}
		if((c = _ctrls[CENTER]))
			res |= configureControl(c,pos,rsize);
		return res;
	}
}
