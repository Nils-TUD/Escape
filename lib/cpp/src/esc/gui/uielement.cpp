/**
 * $Id: uielement.cpp 244 2009-06-20 16:35:38Z nasmussen $
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
			_keyListener = e._keyListener;
			_mouseListener = e._mouseListener;
			_g = NULL;
			_x = e._x;
			_y = e._y;
			_width = e._width;
			_height = e._height;
			return *this;
		}

		void UIElement::addMouseListener(MouseListener *l) {
			if(_mouseListener == NULL)
				_mouseListener = new Vector<MouseListener*>();
			_mouseListener->add(l);
		}
		void UIElement::removeMouseListener(MouseListener *l) {
			if(_mouseListener == NULL)
				return;
			_mouseListener->removeFirst(l);
		}

		void UIElement::addKeyListener(KeyListener *l) {
			if(_keyListener == NULL)
				_keyListener = new Vector<KeyListener*>();
			_keyListener->add(l);
		}
		void UIElement::removeKeyListener(KeyListener *l) {
			if(_keyListener == NULL)
				return;
			_keyListener->removeFirst(l);
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
			if(_mouseListener == NULL)
				return;
			u8 type = e.getType();
			for(u32 i = 0; i < _mouseListener->size(); i++) {
				MouseListener *l = (*_mouseListener)[i];
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
			if(_keyListener == NULL)
				return;
			u8 type = e.getType();
			for(u32 i = 0; i < _keyListener->size(); i++) {
				KeyListener *l = (*_keyListener)[i];
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
