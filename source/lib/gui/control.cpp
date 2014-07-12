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

#include <sys/common.h>
#include <gui/control.h>
#include <gui/window.h>
#include <typeinfo>
#include <iostream>

namespace gui {
	void Control::setParent(UIElement *e) {
		_parent = e;
		// only do that if the parent has already been added to a container. if not, we'll try that
		// again later when the parent is added.
		if(e->getGraphics()) {
			// we share the memory with the window, so that a control simply paints to that memory
			// and just the window writes this memory to vesa
			_g = new Graphics(e->getGraphics()->getBuffer(),Size(0,0));
			setRegion();
		}
	}

	bool Control::resizeTo(const Size &size) {
		bool changed = _size != size;
		makeDirty(changed);
		getParent()->makeDirty(changed);
		_size = size;
		setRegion();
		return changed;
	}

	bool Control::moveTo(const Pos &pos) {
		bool changed = _pos != pos;
		makeDirty(changed);
		getParent()->makeDirty(changed);
		_pos = pos;
		setRegion();
		return changed;
	}

	void Control::setRegion() {
		if(_g) {
			_g->setOff(getWindowPos());
			Rectangle rect = _parent->getVisibleRect(Rectangle(_g->getPos(),_size));
			// don't change the min-offset for the header- and body-panel in the window; they're fixed
			if(_parent->_parent)
				_g->setMinOff(rect.getPos());
			_g->setSize(rect.getSize());
		}
	}

	Pos Control::getParentOff(UIElement *c) const {
		Pos pos = c->getWindowPos();
		while(c->_parent && c->_parent->_parent) {
			pos = maxpos(pos,c->_parent->getWindowPos());
			c = c->_parent;
		}
		return pos;
	}
}
