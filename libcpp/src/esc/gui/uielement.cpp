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
#include <esc/gui/uielement.h>
#include <esc/gui/graphics.h>

namespace esc {
	namespace gui {
		UIElement &UIElement::operator=(const UIElement &e) {
			// ignore self-assignments
			if(this == &e)
				return *this;
			_g = NULL;
			_x = e._x;
			_y = e._y;
			_width = e._width;
			_height = e._height;
			return *this;
		}

		void UIElement::requestUpdate() {
			if(_g)
				_g->requestUpdate(getWindowId());
		}

		void UIElement::update(tCoord x,tCoord y,tSize width,tSize height) {
			if(_g)
				_g->update(x,y,width,height);
		}

		void UIElement::repaint() {
			if(_g) {
				paint(*_g);
				_g->requestUpdate(getWindowId());
			}
		}
	}
}
