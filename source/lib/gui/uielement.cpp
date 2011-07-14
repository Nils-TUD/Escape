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
#include <gui/control.h>
#include <gui/window.h>

namespace gui {
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
	void UIElement::onMouseWheel(const MouseEvent &e) {
		notifyListener(e);
	}
	void UIElement::onKeyPressed(const KeyEvent &e) {
		notifyListener(e);
	}
	void UIElement::onKeyReleased(const KeyEvent &e) {
		notifyListener(e);
	}

	void UIElement::setFocus(Control *c) {
		UNUSED(c);
		// by default its ignored
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
				case MouseEvent::MOUSE_WHEEL:
					l->mouseWheel(*this,e);
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

	void UIElement::paintRect(Graphics &g,gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		// save old rectangle
		gpos_t gx = g.getMinX(), gy = g.getMinY();
		gsize_t gw = g.getWidth(), gh = g.getHeight();

		// change g to cover only the rectangle to repaint
		g.setMinOff(g.getX() + x,g.getY() + y);
		g.setSize(_x,_y,x + width,y + height,_parent->getContentWidth(),_parent->getContentHeight());

		// now let the ui-element paint it; this does not make sense if the rectangle is empty
		if(g.getWidth() != 0 && g.getHeight() != 0)
			paint(g);

		// restore old rectangle
		g.setMinOff(gx,gy);
		g.setWidth(gw);
		g.setHeight(gh);
	}

	void UIElement::repaintRect(gpos_t x,gpos_t y,gsize_t width,gsize_t height) {
		if(_g) {
			paintRect(*_g,x,y,width,height);
			_g->requestUpdate();
		}
	}
}
