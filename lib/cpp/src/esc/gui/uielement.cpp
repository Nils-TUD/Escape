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
			_klist = e._klist;
			_mlist = e._mlist;
			_g = NULL;
			_x = e._x;
			_y = e._y;
			_width = e._width;
			_height = e._height;
			return *this;
		}

		void UIElement::addMouseListener(MouseListener *l) {
			if(_mlist == NULL)
				_mlist = new vector<MouseListener*>();
			_mlist->push_back(l);
		}
		void UIElement::removeMouseListener(MouseListener *l) {
			if(_mlist == NULL)
				return;
			_mlist->erase_first(l);
		}

		void UIElement::addKeyListener(KeyListener *l) {
			if(_klist == NULL)
				_klist = new vector<KeyListener*>();
			_klist->push_back(l);
		}
		void UIElement::removeKeyListener(KeyListener *l) {
			if(_klist == NULL)
				return;
			_klist->erase_first(l);
		}

		void UIElement::onMouseMoved(const MouseEvent &e) {
			notifyListener(e);
		}
		void UIElement::onMouseReleased(const MouseEvent &e) {
			notifyListener(e);
		}
		void UIElement::onMousePressed(const MouseEvent &e) {
			notifyListener(e);
		}
		void UIElement::onKeyPressed(const KeyEvent &e) {
			notifyListener(e);
		}
		void UIElement::onKeyReleased(const KeyEvent &e) {
			notifyListener(e);
		}

		void UIElement::notifyListener(const MouseEvent &e) {
			if(_mlist == NULL)
				return;
			u8 type = e.getType();
			for(vector<MouseListener*>::iterator it = _mlist->begin(); it != _mlist->end(); ++it) {
				MouseListener *l = *it;
				switch(type) {
					case MouseEvent::MOUSE_MOVED:
						l->mouseMoved(e);
						break;
					case MouseEvent::MOUSE_PRESSED:
						l->mousePressed(e);
						break;
					case MouseEvent::MOUSE_RELEASED:
						l->mouseReleased(e);
						break;
				}
			}
		}
		void UIElement::notifyListener(const KeyEvent &e) {
			if(_klist == NULL)
				return;
			u8 type = e.getType();
			for(vector<KeyListener*>::iterator it = _klist->begin(); it != _klist->end(); ++it) {
				KeyListener *l = *it;
				switch(type) {
					case KeyEvent::KEY_PRESSED:
						l->keyPressed(e);
						break;
					case KeyEvent::KEY_RELEASED:
						l->keyReleased(e);
						break;
				}
			}
		}

		void UIElement::requestUpdate() {
			if(_g)
				_g->requestUpdate(getWindowId());
		}

		void UIElement::repaint() {
			if(_g) {
				paint(*_g);
				_g->requestUpdate(getWindowId());
			}
		}
	}
}
