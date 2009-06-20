/**
 * $Id$
 * Copyright (C) 2008 - 2009 Nils Asmussen
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
#include <esc/gui/common.h>
#include <esc/gui/control.h>
#include <esc/gui/window.h>

namespace esc {
	namespace gui {
		Control &Control::operator=(const Control &c) {
			// ignore self-assignments
			if(this == &c)
				return *this;
			UIElement::operator=(c);
			// don't assign the window; the user has to do it manually
			_w = NULL;
			return *this;
		}

		void Control::setWindow(Window *w) {
			_w = w;
			// we share the memory with the window, so that a control simply paints to that memory
			// and just the window writes this memory to vesa
			_g = new Graphics(*_w->getGraphics(),getX(),getY() + _w->getTitleBarHeight());
		}

		tWinId Control::getWindowId() const {
			// TODO throw exception?
			if(_w == NULL)
				return -1;
			return _w->getId();
		}

		void Control::onFocusGained() {
			// do nothing by default
		}
		void Control::onFocusLost() {
			// do nothing by default
		}
	}
}
