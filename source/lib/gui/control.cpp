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
#include <gui/graphics/graphicfactory.h>
#include <gui/control.h>
#include <gui/window.h>

namespace gui {
	void Control::setParent(UIElement *e) {
		_parent = e;
		// we share the memory with the window, so that a control simply paints to that memory
		// and just the window writes this memory to vesa
		_g = GraphicFactory::get(e->getGraphics()->getBuffer(),0,0);
		setRegion();
	}

	void Control::resizeTo(gsize_t width,gsize_t height) {
		_width = width;
		_height = height;
		setRegion();
	}

	void Control::moveTo(gpos_t x,gpos_t y) {
		_x = x;
		_y = y;
		setRegion();
	}

	void Control::setRegion() {
		if(_g) {
			_g->setOff(getWindowX(),getWindowY());
			// don't change the min-offset for the header- and body-panel in the window; they're fixed
			if(_parent->_parent)
				_g->setMinOff(getParentOffX(_parent),getParentOffY(_parent));
			_g->setSize(_x,_y,_width,_height,_parent->getContentWidth(),_parent->getContentHeight());
		}
	}

	gpos_t Control::getParentOffX(UIElement *c) const {
		gpos_t x = c->getWindowX();
		while(c->_parent && c->_parent->_parent) {
			x = max(x,c->_parent->getWindowX());
			c = c->_parent;
		}
		return x;
	}
	gpos_t Control::getParentOffY(UIElement *c) const {
		gpos_t y = c->getWindowY();
		while(c->_parent && c->_parent->_parent) {
			y = max(y,c->_parent->getWindowY());
			c = c->_parent;
		}
		return y;
	}
}
