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
#include <gui/uielement.h>
#include <gui/graphics.h>
#include <gui/window.h>

namespace gui {
	UIElement &UIElement::operator=(const UIElement &e) {
		// ignore self-assignments
		if(this == &e)
			return *this;
		_klist = e._klist;
		_mlist = e._mlist;
		_g = NULL;
		_parent = e._parent;
		_x = e._x;
		_y = e._y;
		_width = e._width;
		_height = e._height;
		return *this;
	}

	Window *UIElement::getWindow() {
		UIElement *p = _parent;
		// if its the window itself
		if(!p)
			return reinterpret_cast<Window*>(this);
		// otherwise go up until we've found the window
		while(p->_parent)
			p = p->_parent;
		return reinterpret_cast<Window*>(p);
	}

	gpos_t UIElement::getWindowX() const {
		if(_parent) {
			if(_parent->_parent)
				return _parent->getWindowX() + _x;
			return _x;
		}
		return 0;
	}
	gpos_t UIElement::getScreenX() const {
		if(_parent)
			return _parent->getScreenX() + _x;
		return _x;
	}
	gpos_t UIElement::getWindowY() const {
		if(_parent) {
			if(_parent->_parent)
				return _parent->getWindowY() + _y;
			return _y;
		}
		return 0;
	}
	gpos_t UIElement::getScreenY() const {
		if(_parent)
			return _parent->getScreenY() + _y;
		return _y;
	}

	void UIElement::addMouseListener(MouseListener *l) {
		if(_mlist == NULL)
			_mlist = new vector<MouseListener*>();
		_mlist->push_back(l);
	}
	void UIElement::removeMouseListener(MouseListener *l) {
		if(_mlist != NULL)
			_mlist->erase_first(l);
	}

	void UIElement::addKeyListener(KeyListener *l) {
		if(_klist == NULL)
			_klist = new vector<KeyListener*>();
		_klist->push_back(l);
	}
	void UIElement::removeKeyListener(KeyListener *l) {
		if(_klist != NULL)
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
		uchar type = e.getType();
		for(vector<MouseListener*>::iterator it = _mlist->begin(); it != _mlist->end(); ++it) {
			MouseListener *l = *it;
			switch(type) {
				case MouseEvent::MOUSE_MOVED:
					l->mouseMoved(*this,e);
					break;
				case MouseEvent::MOUSE_PRESSED:
					l->mousePressed(*this,e);
					break;
				case MouseEvent::MOUSE_RELEASED:
					l->mouseReleased(*this,e);
					break;
			}
		}
	}
	void UIElement::notifyListener(const KeyEvent &e) {
		if(_klist == NULL)
			return;
		uchar type = e.getType();
		for(vector<KeyListener*>::iterator it = _klist->begin(); it != _klist->end(); ++it) {
			KeyListener *l = *it;
			switch(type) {
				case KeyEvent::KEY_PRESSED:
					l->keyPressed(*this,e);
					break;
				case KeyEvent::KEY_RELEASED:
					l->keyReleased(*this,e);
					break;
			}
		}
	}

	void UIElement::requestUpdate() {
		if(_g)
			_g->requestUpdate();
	}

	void UIElement::repaint() {
		if(_g && _enableRepaint) {
			paint(*_g);
			_g->requestUpdate();
		}
	}
}
