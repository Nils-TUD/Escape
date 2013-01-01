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
#include <gui/graphics/graphics.h>
#include <gui/uielement.h>
#include <gui/control.h>
#include <gui/window.h>
#include <typeinfo>
#include <sstream>

using namespace std;

namespace gui {
	UIElement::id_type UIElement::_nextid = 1;

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

	void UIElement::onMouseMoved(const MouseEvent &e) {
		_mouseMoved.send(*this,e);
	}
	void UIElement::onMouseReleased(const MouseEvent &e) {
		_mouseReleased.send(*this,e);
	}
	void UIElement::onMousePressed(const MouseEvent &e) {
		_mousePressed.send(*this,e);
	}
	void UIElement::onMouseWheel(const MouseEvent &e) {
		_mouseWheel.send(*this,e);
	}
	void UIElement::onKeyPressed(const KeyEvent &e) {
		_keyPressed.send(*this,e);
	}
	void UIElement::onKeyReleased(const KeyEvent &e) {
		_keyReleased.send(*this,e);
	}

	void UIElement::setFocus(A_UNUSED Control *c) {
		// by default its ignored
	}

	void UIElement::debug() {
#ifndef DEBUG_GUI
		static int pos = 0;
		ostringstream ostr;
		ostr << _id;
		_g->setColor(Color(0xFF,0,0));

		gpos_t x = 0, y = 0;
		switch(pos++ % 3) {
			case 1:
				x = getSize().width - _g->getFont().getStringWidth(ostr.str());
				y = getSize().height - _g->getFont().getSize().height;
				break;
			case 2:
				x = getSize().width / 2 - _g->getFont().getStringWidth(ostr.str()) / 2;
				y = getSize().height / 2 - _g->getFont().getSize().height / 2;
				break;
		}
		_g->drawString(x,y,ostr.str());

		_g->drawRect(0,0,getSize());

		Window *win = _g->getBuffer()->getWindow();
		if(win->hasTitleBar())
			cout << "[" << win->getTitle();
		else
			cout << "[#" << win->getId();
		cout << ":" << getId() << ":" << typeid(*this).name() << "]";
		cout << " @" << getX() << "," << getY();
		cout << " size:" << getSize() << " pref:" << getPreferredSize() << endl;
#endif
	}

	void UIElement::requestUpdate() {
		if(_g)
			_g->requestUpdate();
	}

	void UIElement::repaint(bool update) {
		if(_g && _enableRepaint) {
			paint(*_g);
			debug();
			if(update)
				_g->requestUpdate();
		}
	}

	void UIElement::paintRect(Graphics &g,gpos_t x,gpos_t y,const Size &size) {
		// save old rectangle
		gpos_t gx = g.getMinX(), gy = g.getMinY();
		Size gs = g.getSize();

		// change g to cover only the rectangle to repaint
		g.setMinOff(g.getX() + x,g.getY() + y);
		g.setSize(_x,_y,Size(x + size.width,y + size.height),_parent->getContentSize());

		// now let the ui-element paint it; this does not make sense if the rectangle is empty
		if(!g.getSize().empty())
			paint(g);

		// restore old rectangle
		g.setMinOff(gx,gy);
		g.setSize(gs);
	}

	void UIElement::repaintRect(gpos_t x,gpos_t y,const Size &size) {
		if(_g) {
			paintRect(*_g,x,y,size);
			debug();
			_g->requestUpdate();
		}
	}
}
