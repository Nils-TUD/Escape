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
#include <gui/control.h>
#include <gui/window.h>
#include <gui/graphicfactory.h>

namespace gui {
	Control &Control::operator=(const Control &c) {
		// ignore self-assignments
		if(this == &c)
			return *this;
		UIElement::operator=(c);
		return *this;
	}

	void Control::setParent(UIElement *e) {
		_parent = e;
		// we share the memory with the window, so that a control simply paints to that memory
		// and just the window writes this memory to vesa
		_g = GraphicFactory::get(e->getGraphics()->getBuffer(),getWindowX(),getWindowY());
	}

	void Control::resizeTo(gsize_t width,gsize_t height) {
		_width = width;
		_height = height;
		getParent()->repaint();
	}

	void Control::moveTo(gpos_t x,gpos_t y) {
		_x = x;
		_y = y;
		_g->setXOff(getWindowX());
		_g->setYOff(getWindowY());
	}

	void Control::onFocusGained() {
		Panel *p = dynamic_cast<Panel*>(getParent());
		if(p)
			p->setFocus(this);
	}
	void Control::onFocusLost() {
		Panel *p = dynamic_cast<Panel*>(getParent());
		if(p)
			p->setFocus(NULL);
	}
}
