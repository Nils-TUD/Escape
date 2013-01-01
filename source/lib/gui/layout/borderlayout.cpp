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
#include <gui/layout/borderlayout.h>
#include <gui/panel.h>
#include <assert.h>

namespace gui {
	void BorderLayout::add(Panel *p,Control *c,pos_type pos) {
		assert(pos < (pos_type)ARRAY_SIZE(_ctrls) && (_p == NULL || p == _p));
		_p = p;
		_ctrls[pos] = c;
	}
	void BorderLayout::remove(Panel *p,Control *c,pos_type pos) {
		assert(pos < (pos_type)ARRAY_SIZE(_ctrls) && _p == p && _ctrls[pos] == c);
		_ctrls[pos] = NULL;
	}

	Size BorderLayout::getPreferredSize() const {
		Size size;
		if(_ctrls[WEST])
			size.width += _ctrls[WEST]->getPreferredSize().width;
		if(_ctrls[NORTH])
			size.height += _ctrls[NORTH]->getPreferredSize().height;
		if(_ctrls[CENTER]) {
			if(_ctrls[WEST])
				size += Size(_gap,_gap);
			size += _ctrls[CENTER]->getPreferredSize();
			if(_ctrls[EAST])
				size += Size(_gap,_gap);
		}
		if(_ctrls[EAST])
			size.width += _ctrls[EAST]->getPreferredSize().width;
		if(_ctrls[SOUTH])
			size.height += _ctrls[SOUTH]->getPreferredSize().height;

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

	void BorderLayout::rearrange() {
		if(!_p)
			return;

		Control *c;
		gsize_t pad = _p->getTheme().getPadding();
		Size size = _p->getSize() - Size(pad * 2,pad * 2);
		Size rsize = size;
		gpos_t x = pad;
		gpos_t y = pad;

		Size ns = _ctrls[NORTH] ? _ctrls[NORTH]->getPreferredSize() + Size(_gap,_gap) : Size();
		Size ss = _ctrls[SOUTH] ? _ctrls[SOUTH]->getPreferredSize() + Size(_gap,_gap) : Size();
		Size ws = _ctrls[WEST] ? _ctrls[WEST]->getPreferredSize() + Size(_gap,_gap) : Size();
		Size es = _ctrls[EAST] ? _ctrls[EAST]->getPreferredSize() + Size(_gap,_gap) : Size();
		Size cs = _ctrls[CENTER] ? _ctrls[CENTER]->getPreferredSize() : Size();

		// ensure that we use at least the minimum space
		rsize.height = max<gsize_t>(ns.height + ss.height + cs.height,rsize.height);
		rsize.width = max<gsize_t>(ws.width + es.width + cs.width,rsize.width);

		if((c = _ctrls[NORTH])) {
			configureControl(c,x,y,Size(size.width,ns.height - _gap));
			y += ns.height;
			rsize.height -= ns.height;
		}
		if((c = _ctrls[SOUTH])) {
			configureControl(c,x,pad + size.height - (ss.height - _gap),
							 Size(size.width,ss.height - _gap));
			rsize.height -= ss.height;
		}
		if((c = _ctrls[WEST])) {
			configureControl(c,x,y,Size(ws.width - _gap,rsize.height));
			x += ws.width;
			rsize.width -= ws.width;
		}
		if((c = _ctrls[EAST])) {
			configureControl(c,pad + size.width - (es.width - _gap),y,
							 Size(es.width - _gap,rsize.height));
			rsize.width -= es.width;
		}
		if((c = _ctrls[CENTER]))
			configureControl(c,x,y,rsize);
	}
}
