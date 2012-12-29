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

	gsize_t BorderLayout::getMinWidth() const {
		gsize_t width = 0;
		if(_ctrls[WEST])
			width += _ctrls[WEST]->getMinWidth();
		if(_ctrls[CENTER]) {
			if(_ctrls[WEST])
				width += _gap;
			width += _ctrls[CENTER]->getMinWidth();
			if(_ctrls[EAST])
				width += _gap;
		}
		if(_ctrls[EAST])
			width += _ctrls[EAST]->getMinWidth();
		if(width == 0) {
			if(_ctrls[NORTH])
				width = _ctrls[NORTH]->getMinWidth();
			if(_ctrls[SOUTH])
				width = max(width,_ctrls[SOUTH]->getMinWidth());
		}
		return width;
	}
	gsize_t BorderLayout::getMinHeight() const {
		gsize_t height = 0;
		if(_ctrls[NORTH])
			height += _ctrls[NORTH]->getMinHeight();
		if(_ctrls[CENTER]) {
			if(_ctrls[NORTH])
				height += _gap;
			height += _ctrls[CENTER]->getMinHeight();
			if(_ctrls[SOUTH])
				height += _gap;
		}
		if(_ctrls[SOUTH])
			height += _ctrls[SOUTH]->getMinHeight();
		if(height == 0) {
			if(_ctrls[WEST])
				height = _ctrls[WEST]->getMinHeight();
			if(_ctrls[EAST])
				height = max(height,_ctrls[EAST]->getMinHeight());
		}
		return height;
	}

	gsize_t BorderLayout::getPreferredWidth() const {
		gsize_t pad = _p->getTheme().getPadding();
		if(_ctrls[CENTER])
			return max<gsize_t>(_p->getParent()->getContentWidth() - pad * 2,getMinWidth());
		return getMinWidth();
	}
	gsize_t BorderLayout::getPreferredHeight() const {
		gsize_t pad = _p->getTheme().getPadding();
		if(_ctrls[CENTER])
			return max<gsize_t>(_p->getParent()->getContentHeight() - pad * 2,getMinHeight());
		return getMinHeight();
	}

	void BorderLayout::rearrange() {
		if(!_p)
			return;

		Control *c;
		gsize_t pad = _p->getTheme().getPadding();
		gsize_t width = _p->getWidth() - pad * 2;
		gsize_t height = _p->getHeight() - pad * 2;
		gpos_t x = pad;
		gpos_t y = pad;
		gsize_t rwidth = width;
		gsize_t rheight = height;

		gsize_t npheight = _ctrls[NORTH] ? _ctrls[NORTH]->getPreferredHeight() + _gap : 0;
		gsize_t spheight = _ctrls[SOUTH] ? _ctrls[SOUTH]->getPreferredHeight() + _gap : 0;
		gsize_t wpwidth = _ctrls[WEST] ? _ctrls[WEST]->getPreferredWidth() + _gap : 0;
		gsize_t epwidth = _ctrls[EAST] ? _ctrls[EAST]->getPreferredWidth() + _gap : 0;
		gsize_t cmheight = _ctrls[CENTER] ? _ctrls[CENTER]->getMinHeight() : 0;
		gsize_t cmwidth = _ctrls[CENTER] ? _ctrls[CENTER]->getMinWidth() : 0;

		// if we can't reach the minimum size for the controls, give each control the corresponding
		// amount of the space, depending on their share of the required size (this prevents
		// "size-bouncing")
		if(npheight + spheight + cmheight > rheight) {
			double nratio = npheight / (double)(npheight + spheight + cmheight);
			double sratio = spheight / (double)(npheight + spheight + cmheight);
			npheight = (gsize_t)(rheight * nratio);
			spheight = (gsize_t)(rheight * sratio);
		}
		if(wpwidth + epwidth + cmwidth > rwidth) {
			double wratio = wpwidth / (double)(wpwidth + epwidth + cmwidth);
			double eratio = epwidth / (double)(wpwidth + epwidth + cmwidth);
			wpwidth = (gsize_t)(rwidth * wratio);
			epwidth = (gsize_t)(rwidth * eratio);
		}

		if((c = _ctrls[NORTH])) {
			c->moveTo(x,y);
			c->resizeTo(width,npheight - _gap);
			y += npheight;
			rheight -= npheight;
		}
		if((c = _ctrls[SOUTH])) {
			c->moveTo(x,pad + height - (spheight - _gap));
			c->resizeTo(width,spheight - _gap);
			rheight -= spheight;
		}
		if((c = _ctrls[WEST])) {
			c->moveTo(x,y);
			c->resizeTo(wpwidth - _gap,rheight);
			x += wpwidth;
			rwidth -= wpwidth;
		}
		if((c = _ctrls[EAST])) {
			c->moveTo(pad + width - (epwidth - _gap),y);
			c->resizeTo(epwidth - _gap,rheight);
			rwidth -= epwidth;
		}
		if((c = _ctrls[CENTER])) {
			c->moveTo(x,y);
			c->resizeTo(rwidth,rheight);
		}
	}
}
