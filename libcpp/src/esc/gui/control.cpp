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
		void Control::setWindow(Window *w) {
			_w = w;
			// we share the memory with the window, so that a control simply paints to that memory
			// and just the window writes this memory to vesa
			_g = new Graphics(*_w->getGraphics(),getX(),getY() + _w->getTitleBarHeight());
		}

		void Control::onMouseMoved(const MouseEvent &e) {
			// do nothing by default
			UNUSED(e);
		}
		void Control::onMouseReleased(const MouseEvent &e) {
			// do nothing by default
			UNUSED(e);
		}
		void Control::onMousePressed(const MouseEvent &e) {
			// do nothing by default
			UNUSED(e);
		}
		void Control::onKeyPressed(const KeyEvent &e) {
			// do nothing by default
			UNUSED(e);
		}
		void Control::onKeyReleased(const KeyEvent &e) {
			// do nothing by default
			UNUSED(e);
		}

		void Control::paint() {
			// no window?
			// TODO throw exception!
			if(_w == NULL)
				return;
		}
	}
}
