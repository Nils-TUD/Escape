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
#include <gui/borderlayout.h>
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

	gsize_t BorderLayout::getPreferredWidth() const {
		gsize_t width = 0;
		if(_ctrls[WEST])
			width += _ctrls[WEST]->getPreferredWidth();
		if(_ctrls[CENTER]) {
			if(_ctrls[WEST])
				width += _gap;
			width += _ctrls[CENTER]->getPreferredWidth();
			if(_ctrls[EAST])
				width += _gap;
		}
		if(_ctrls[EAST])
			width += _ctrls[EAST]->getPreferredWidth();
		return width;
	}
	gsize_t BorderLayout::getPreferredHeight() const {
		gsize_t height = 0;
		if(_ctrls[NORTH])
			height += _ctrls[NORTH]->getPreferredWidth();
		if(_ctrls[CENTER]) {
			if(_ctrls[NORTH])
				height += _gap;
			height += _ctrls[CENTER]->getPreferredWidth();
			if(_ctrls[SOUTH])
				height += _gap;
		}
		if(_ctrls[SOUTH])
			height += _ctrls[SOUTH]->getPreferredWidth();
		return height;
	}

	void BorderLayout::rearrange() {
		if(!_p)
			return;

		Control *c;
		gpos_t x = 0;
		gpos_t y = 0;
		gsize_t width = _p->getWidth();
		gsize_t height = _p->getHeight();
		gsize_t rwidth = width;
		gsize_t rheight = height;
		if((c = _ctrls[NORTH])) {
			gsize_t pheight = c->getPreferredHeight();
			c->moveTo(x,y);
			c->resizeTo(width,pheight);
			y += pheight + _gap;
			rheight -= pheight + _gap;
		}
		if((c = _ctrls[SOUTH])) {
			gsize_t pheight = c->getPreferredHeight();
			c->moveTo(x,height - pheight);
			c->resizeTo(width,pheight);
			rheight -= pheight + _gap;
		}
		if((c = _ctrls[WEST])) {
			gsize_t pwidth = c->getPreferredWidth();
			c->moveTo(x,y);
			c->resizeTo(pwidth,rheight);
			x += pwidth + _gap;
			rwidth -= pwidth + _gap;
		}
		if((c = _ctrls[EAST])) {
			gsize_t pwidth = c->getPreferredWidth();
			c->moveTo(width - pwidth,y);
			c->resizeTo(pwidth,rheight);
			rwidth -= pwidth + _gap;
		}
		if((c = _ctrls[CENTER])) {
			c->moveTo(x,y);
			c->resizeTo(rwidth,rheight);
		}
	}
}
